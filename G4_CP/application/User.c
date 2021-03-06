#define USER_BODY
/******************************************************************************
            Copyright (C) 2007-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:       9D991

    File:       User.c

    Description: User handled ASCII messages from the GSE or micro-server.
                 and provides a control and status interface into to other
                 applications running on the Coldfire.  Message are decoded
                 using tables that contain information about the type of data,
                 being accessed and a function to handle the message.


                 Expected message format is:
                                0.1.2.n[i]=Set$check
                 where:

                 0,1,2 and n:         Tokens, delimited by "." characters that
                                      describe a specific message. Any number
                                      of tokens may be implemented to make
                                      up a message.
                 [i]                  Is an optional index parameter in the
                                      message.  The string in between the [ and
                                      ] brackets shall be a number.
                 =                    Is an optional delimiter that indicates
                                      this message is writing or setting a value
                 Set                  Is the optional string following the "="
                                      This string is converted to the data type
                                      the particular message accesses.
                 $                    Is the checksum delimiter.  The incoming
                                      checksum is optional
                 check                16-bit checksum of the message

                 After begin processed by the message handler, a response
                 string is generated and sent back to the source that originated
                 the message

                 Each application can register a set of message tables by
                 calling User_AddRootCmd with a USER_MSG structure defining
                 the root command.
                 See User.H for information about the table format.


   VERSION
   $Revision: 125 $  $Date: 3/29/16 11:14p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <errno.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "TaskManager.h"
#include "Monitor.h"
#include "MSInterface.h"
#include "User.h"
#include "FIFO.h"
#include "gse.h"
#include "FaultMgr.h"
#include "Assert.h"
#include "LogManager.h"
#include "Assert.h"
#include "Utility.h"
#include "NVMgr.h"
#include "sensor.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define USER_CMD_Q_SIZE 8
// mask off the bits to index into the queue 2^x = USER_CMD_Q_SIZE, x = 3
#define Q_INDEX_MASK    MASK(0,3)

#define USER_ACTION_TOKEN "<USR_ACTION>"

#define BITS_PER_WORD       32
#define HEX_STRING_PREF_LEN 2  // skip over leading "0x"
#define HEX_CHARS_PER_WORD  8  // number of hex char per UINT32 ("00000000" -> "FFFFFFFF")

#define HEX_STR_BUFFER_SIZE (HEX_STRING_PREF_LEN + HEX_CHARS_PER_WORD)
#define MIN_HEX_STRING      (HEX_STRING_PREF_LEN + 1)
#define MAX_HEX_STRING      (HEX_STRING_PREF_LEN + (4*HEX_CHARS_PER_WORD))

#define BASE_10 10
#define BASE_16 16

#define MAX_GSE_SET_STR  256

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

//This message header is attached to the top of every message
//added to the message queue.
typedef struct{
  USER_MSG_SOURCES source; // Source of the message (enumeration)
  UINT32 tag;              // Allows the caller to "tag" a message with a
                           // 32-bit number that is returned to the PutRsp
                           // routine.
  CHAR cmd[GSE_GET_LINE_BUFFER_SIZE];
} USER_MSG_HEADER;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static INT32           userRequest;                       // Messages waiting for processing
static INT32           userProcessed;                     // Messages processed
static USER_MSG_HEADER userMsgQ[USER_CMD_Q_SIZE];         // queue to hold cmd requests

static INT8            userMsgBuf[USER_MESSAGE_BUF_SIZE]; //FIFO variables to create an
static FIFO            userMsgQueue;                      // incoming message queue
static USER_MSG_TBL    rootCmdTbl[USER_MAX_ROOT_CMDS];    //Starting table for matching user
                                                          //command messages.
static BOOLEAN         userPrivilegedMode;
static BOOLEAN         userFactoryMode;

static UINT16           checkSum;                 // Running checksum value.
static BOOLEAN          rspBuffOverflowDetected;  // The micro-server response msg is too long
static USER_MSG_SOURCES msgSource;                // source of the current command.
static UINT32           msgTag;                   // tag passed by caller of current command.
static CHAR             msBuffer[USER_SINGLE_MSG_MAX_SIZE]; // buffer for building rsp msgs
                                                            // to Micro-server


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void          User_ProcMsgTask             ( void *pBlock );
static BOOLEAN       User_PutMsg                  ( const INT8* Msg, USER_MSG_SOURCES Source,
                                                    UINT32 Tag);
static void          User_PutRsp                  ( const INT8* Msg, USER_MSG_SOURCES Source,
                                                    UINT32 Tag );
static void          User_ExecuteCmdMsg           ( INT8* msg, USER_MSG_SOURCES source,
                                                    UINT32 tag );
static INT32         User_ExtractIndex            ( INT8* msg );
static INT8*         User_ExtractAssign           ( INT8* msg );
static USER_MSG_TBL* User_TraverseCmdTables       ( INT8* MsgTokPtr, USER_MSG_TBL* CmdMsgTbl,
                                                    INT8* TokPtr );
static BOOLEAN       User_CvtSetStr               ( USER_DATA_TYPE Type, INT8* SetStr,
                                                    void **SetPtr, USER_ENUM_TBL* MsgEnumTbl,
                                                    USER_RANGE *Min, USER_RANGE *Max );
static BOOLEAN       User_ExecuteSingleMsg        ( USER_MSG_TBL* MsgTbl,
                                                    USER_MSG_SOURCES source,
                                                    INT32 Index, INT8* SetStr, INT8* RspStr,
                                                    UINT32 RspLen );
static void          User_ExecuteMultipleGetMsg   ( USER_MSG_TBL* MsgTblPtr,
                                                    USER_MSG_SOURCES source,
                                                    INT32 Index, INT8* rsp, UINT32 len );
static BOOLEAN       User_ValidateChecksum        ( INT8 *str );
static void          User_AppendChecksum          ( INT8 *str );
static BOOLEAN       User_ValidateMessage         ( USER_MSG_TBL* UserMsg,
                                                    USER_MSG_SOURCES source,
                                                    INT32 Index, INT8* RspStr, INT8* SetStr,
                                                    UINT32 Len );
static BOOLEAN       User_GSEMessageHandler       ( INT8* msg );
static BOOLEAN       User_MSMessageHandler        ( void* Data, UINT16 Size, UINT32 Sequence );
static void          User_SetMinMax               ( USER_RANGE *Min, USER_RANGE *Max,
                                                    USER_DATA_TYPE Type );
static void          User_ConversionErrorResponse ( INT8* RspStr,USER_RANGE Min,
                                                    USER_RANGE Max, USER_DATA_TYPE Type,
                                                    const USER_ENUM_TBL *Enum, UINT32 Len );

static BOOLEAN       User_CheckForLeafArray       ( CHAR* UserTableName, CHAR* MsgStr );
static BOOLEAN       User_ShowAllConfig           ( void );

static BOOLEAN       User_GetParamValue           ( USER_MSG_TBL* MsgTbl,
                                                    USER_MSG_SOURCES source,
                                                    INT32 Index, INT8* RspStr, UINT32 Len );
static void          User_LogUserActivity         ( CHAR* cmdString, CHAR* valuePrev,
                                                    CHAR* valueNew );
static BOOLEAN       User_AuthenticateModeRequest ( const INT8* msg, UINT8 mode );

static BOOLEAN       User_SetBitArrayFromHexString( USER_DATA_TYPE Type, INT8* SetStr,
                                                    void **SetPtr,
                                                    USER_RANGE *Min, USER_RANGE *Max );

static BOOLEAN       User_CvtGetBitListStr         ( USER_DATA_TYPE Type, INT8* GetStr,
                                                     UINT32 Len, void* GetPtr );

static BOOLEAN       User_SetBitArrayFromList      ( USER_DATA_TYPE Type, INT8* SetStr,
                                                     void **SetPtr,
                                                     USER_RANGE *Min, USER_RANGE *Max );

static BOOLEAN       User_SetBitArrayFromIntegerValue ( USER_DATA_TYPE Type,
                                                        INT8* SetStr, void **SetPtr,
                                                        USER_RANGE *Min, USER_RANGE *Max );

static BOOLEAN       User_BitSetIsValid           ( USER_DATA_TYPE Type, UINT32* destPtr,
                                                    USER_RANGE *Min, USER_RANGE *Max);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    User_Init
 * Description: Setup the User module.
 *              Setup to receive and queue messages from the GSE port, from the
 *              micro-server, as well as a task to process messages and return a
 *              response to the message source.
 * Parameters:
 * Returns:
 * Notes:
 *
 *****************************************************************************/
void User_Init(void)
{
  TCB tcbTaskInfo;

  userPrivilegedMode = FALSE;
  userFactoryMode    = FALSE;


  //Setup a FIFO queue to hold messages for processing
  FIFO_Init(&userMsgQueue,userMsgBuf,USER_MESSAGE_BUF_SIZE);

  //Hook into the system-level Monitor unit to receive data
  //from the GSE port.
  MonitorSetMsgCallback(User_GSEMessageHandler);

  //Hook into micro-server interface for the "User Mgr Cmd"
  MSI_AddCmdHandler(CMD_ID_USER_MGR_MS,User_MSMessageHandler);

  memset(rootCmdTbl, 0, sizeof(rootCmdTbl));

  userRequest = 0;                        //Messages waiting for processing
  userProcessed = 0;                      // Messages processed
  memset( userMsgQ, 0, sizeof(userMsgQ)); // queue to hold cmd requests

  //Setup the User task to process new messages
  memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
  strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name), "User Msg Proc", _TRUNCATE);
  tcbTaskInfo.TaskID         = User_Msg_Proc;
  tcbTaskInfo.Function       = User_ProcMsgTask;
  tcbTaskInfo.Priority       = taskInfo[User_Msg_Proc].priority;
  tcbTaskInfo.Type           = taskInfo[User_Msg_Proc].taskType;
  tcbTaskInfo.MIFrames       = taskInfo[User_Msg_Proc].MIFframes;
  tcbTaskInfo.modes          = taskInfo[User_Msg_Proc].modes;
  tcbTaskInfo.Rmt.InitialMif = taskInfo[User_Msg_Proc].InitialMif;
  tcbTaskInfo.Rmt.MifRate    = taskInfo[User_Msg_Proc].MIFrate;
  tcbTaskInfo.Enabled        = TRUE;
  tcbTaskInfo.Locked         = FALSE;
  tcbTaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&tcbTaskInfo);
}

/******************************************************************************
 * Function:    User_AddRootCmd
 *
 * Description: Add a command to the root command table.  The root is the first
 *              string in a command message before the "." delimiter.  The
 *              root command entry will typically point to other tables that
 *              define the sub commands. (ex root.xxx.yyy.zzz, where x y and z
 *              are sub command strings)
 *
 *              The sub command tables should be structured as an array of
 *              USER_MSG, with the last array element having it's MsgStr field
 *              set to NULL.
 *
 * Parameters:  [in] NewRootPtr: New command to add to the local root table
 *
 * Returns:     TRUE: Cmd added
 *              FALSE: No cmd added, no more room in the user table
 * Notes:
 *
 *****************************************************************************/
void User_AddRootCmd(USER_MSG_TBL* NewRootPtr)
{
  UINT32 i = 0;

  //Search for an empty slot in the root cmd table
  while(( i < USER_MAX_ROOT_CMDS ) && (rootCmdTbl[i].MsgStr != NULL))
  {
    i++;
  }

  ASSERT( i < USER_MAX_ROOT_CMDS);

  memcpy( &rootCmdTbl[i], NewRootPtr, sizeof( rootCmdTbl[0]));
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    User_GSEMessageHandler
 *
 * Description: Accepts incoming messages from the GSE port and inserts
 *              them into the User command queue
 *
 * Parameters:  [in] msg: A null terminated ASCII string message
 *
 * Returns:     TRUE: Message added successfully
 *              FALSE: Could not add message, queue is full
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN User_GSEMessageHandler(INT8* msg)
{
  return User_PutMsg(msg,USER_MSG_SOURCE_GSE,0);
}

/******************************************************************************
 * Function:    User_MSMessageHandler
 *
 * Description: Accepts incoming messages from the micro-server and inserts
 *              them into the User command queue.  This function matches the MS
 *              Interface command handler callback type.
 *
 * Parameters:  [in] Data
 *              [in] Size
 *              [in] Sequence
 *
 * Returns:     TRUE: Message added successfully
 *              FALSE: Could not add message, queue is full or message not
 *                     formatted correctly
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN User_MSMessageHandler(void* Data, UINT16 Size, UINT32 Sequence)
{
  MSCP_USER_MGR_CMD* msg = Data;
  BOOLEAN bOk = FALSE;

  if(Size < sizeof(*msg))
  {
    //Str functions used from this point forward so force null-termination.
    msg->cmd[Size] = '\0';
    GSE_DebugStr(NORMAL,FALSE,"User: Got MS Cmd -%s-",msg->cmd);
    bOk = User_PutMsg(msg->cmd,USER_MSG_SOURCE_MS,Sequence);
  }

  return (bOk);

}

//Define additional message handlers (for messages from other sources here)

/******************************************************************************
 * Function:    User_PutMsg
 * Description: Adds an ASCII null terminated message string to
 *              the user message queue
 *
 *
 * Parameters:  [in] Msg: A null terminated ASCII message from any source
 *              [in] MsgSource: An identifier indicating the message source.
 *                              This is used for directing the response.
 *              [in] Tag: This value is not used internally, but it is an
 *                        optional field that may be used to "tag" a message
 *                        with a unique identifier that will be returned by
 *                        PutRsp.
 *
 * Returns:     TRUE: Message added to queue
 *              FALSE: Error adding message to queue, driver returned an error
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN User_PutMsg(const INT8* Msg, USER_MSG_SOURCES Source, UINT32 Tag)
{
  UINT8 x;
  BOOLEAN result = TRUE;
  INT32 intrLevel;

  // if there is room in the Q, allocate it
  intrLevel = __DIR();
  ASSERT_MESSAGE((userRequest - userProcessed) <= USER_CMD_Q_SIZE,
                 "Rqst:%d Proc:%d", userRequest, userProcessed);
  x = userRequest & Q_INDEX_MASK;
  ++userRequest;
  __RIR(intrLevel);

  if (TRUE == result)
  {
      // fill in the queue entry info
      userMsgQ[x].source = Source;
      userMsgQ[x].tag = Tag;
      strncpy_safe( userMsgQ[x].cmd, GSE_GET_LINE_BUFFER_SIZE, Msg, _TRUNCATE );
  }

  return result;
}

/******************************************************************************
 * Function:    User_PutRsp
 *
 * Description: Dispatches a response string to the message source indicated
 *              by the "Source" parameter.
 *
 * Parameters:  [in/out] Msg: Response to send back to a message originator.
 *                            Must be writable because a checksum
 *                            string is appended to the end.
 *              [in] MsgSource: Source that sent the message this response
 *                              is for
 *              [in] Tag: Optional identifier that allows the message source
 *                        to assign a unique tag to each message.  This
 *                        tag is returned to the response handler with the same
 *                        value as was used in PutMsg.
 *
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void User_PutRsp(const INT8* Msg, USER_MSG_SOURCES Source, UINT32 Tag)
{
  CHAR chkstr[32];
  CHAR writeBuffer[USER_SINGLE_MSG_MAX_SIZE];

  INT32 msgSize = strlen(Msg) + sizeof(chkstr) + 1;

  // We should never have a single msg-part which is too big for the
  // USER_SINGLE_MSG_MAX_SIZE + checksum!
  ASSERT_MESSAGE( (msgSize < USER_SINGLE_MSG_MAX_SIZE),
       "Output msg exceeds buffer size: %d", msgSize);

  strncpy_safe(writeBuffer,USER_SINGLE_MSG_MAX_SIZE,Msg, strlen(Msg) );

  User_AppendChecksum(writeBuffer);

  switch(Source)
  {
    case USER_MSG_SOURCE_GSE:
      GSE_PutLine(writeBuffer);
      GSE_PutLine(GSE_PROMPT);
      break;

    case USER_MSG_SOURCE_MS:
      //Ignore errors, MS should timeout if the response fails.
      MSI_PutResponse(CMD_ID_USER_MGR_MS,writeBuffer,MSCP_RSP_STATUS_SUCCESS,
                               strlen(writeBuffer), Tag);
      break;


    //...Put additional response routers here

    default: FATAL("Unrecognized msg source: %d",Source);
      break;
  }
}


/******************************************************************************
 * Function:    User_ProcMsgTask
 *
 * Description: Process user command task.  This is setup as an RMT running
 *              every frame.  The message queue is checked to see if new
 *              messages are available, and if so they are processed by calling
 *              ParseCmd
 *
 * Parameters:  [in] pBlock: Pointer to the tasks data block (currently not
 *                           used)
 * Returns:
 *
 * Notes:
 *
 *****************************************************************************/
static
void User_ProcMsgTask(void *pBlock)
{
  CHAR msg[USER_SINGLE_MSG_MAX_SIZE];

  //If data is available in the message FIFO queue, read it out.
  if(userRequest > userProcessed)
  {
    UINT32 tag;
    USER_MSG_SOURCES source;
    UINT8 x = userProcessed++ & Q_INDEX_MASK;

    source = userMsgQ[x].source;
    tag = userMsgQ[x].tag;
    strncpy_safe( msg, GSE_GET_LINE_BUFFER_SIZE, userMsgQ[x].cmd, _TRUNCATE );
    User_ExecuteCmdMsg( msg, source, tag);
  }
}

/******************************************************************************
 * Function:     User_ExecuteCmdMsg
 *
 * Description:  Execute command message takes a command string and the source
 *               from which it originated.  The string is processed with the
 *               following general steps.
 *               1. The string is striped of any leading or trailing whitespace
 *               2. Any "index" parameter (a number between [ and ] brackets)
 *                  is converted to an integer if it is present
 *               3. Any "set" parameter (a string after the "=" assign
 *                  delimiter) is identified
 *               4. The tokens between the "." delimiters are identified and
 *                  are used to traverse the command message tables registered
 *                  with user.c (command tables may be registered by calling
 *                  User_AddRootCmd)
 *                  4a. If a command string leads to the last node table
 *                      the message handle is called
 *                  4b. If a command string does not get to the last node table
 *                      all the commands in the next table are executed.  This
 *                      allows a user to "get" multiple commands by issuing
 *                      only one command
 *               5. The data associated with the command(s) is/are
 *                  returned to the source in ASCII format.
 *
 * Parameters:    [in/out] msg: message string from the source.  The same buffer
 *                        is also reused to format the response.
 *                [in] source: Source of the message, indicates where to route
 *                    the message response
 *                [in] tag: Caller defined 32-bit value that is returned
 *                     to the PutRsp routine.  This allows the message source
 *                     to assign a unique identifier to each message.
 *
 * Returns:       No return, call status is returned to the "source" by the
 *                command message response
 *
 * Notes:
 *
 *****************************************************************************/
static
void User_ExecuteCmdMsg(INT8* msg,  USER_MSG_SOURCES source, UINT32 tag)
{
  INT8* setTokPtr = NULL;
  INT8* msgTokPtr = NULL;
  CHAR *tokPtr = NULL;
  USER_MSG_TBL* cmdTblPtr;
  INT32 nIndex;
  CHAR  oldValue[SYSTEM_LOG_DATA_SIZE];
  CHAR  newValue[SYSTEM_LOG_DATA_SIZE];
  CHAR  origCmdString[SYSTEM_LOG_DATA_SIZE];
  CHAR  rspString[USER_SINGLE_MSG_MAX_SIZE];
  BOOLEAN bLogThisChange   = FALSE;
  BOOLEAN bExecutionResult = FALSE;

  rspString[0] = '\0';

  // Initialize the output and checksum variables.
  msgSource = source;
  msgTag    = tag;
  checkSum  = 0;
  rspBuffOverflowDetected = FALSE;
  msBuffer[0] = '\0';

  // Strip leading and trailing unprintable characters
  // from command msg string.
  StripString( msg );

  // Check that the command msg is non-empty as result of strip.
  // or that the command is missing an l-value
  if( 0 == strlen(msg) || *msg == '=' )
  {
    //Empty/malformed Message found
    User_OutputMsgString( "\r\n"USER_MSG_INVALID_CMD, TRUE );
  }
  //Validate the message checksum (if present)
  else if(!User_ValidateChecksum(msg))
  {
    User_OutputMsgString(USER_MSG_BAD_CHECKSUM, TRUE);
  }
  // Validate Factory mode command
  else if(User_AuthenticateModeRequest(msg, USER_FACT) )
  {
    userFactoryMode    = TRUE;
    // Factory mode contains implicit priv mode.
    userPrivilegedMode = TRUE;
    User_OutputMsgString(USER_MSG_FACTORY_MODE, TRUE);
  }
  // Validate Privileged mode command
  else if(User_AuthenticateModeRequest(msg, USER_PRIV))
  {
    userPrivilegedMode = TRUE;
    User_OutputMsgString(USER_MSG_PRIVILEGED, TRUE);
  }
  else
  {
    //Search for assignment (=) to determine if this is a set or get cmd
    setTokPtr = User_ExtractAssign(msg);

    //The variable "msg" now contains the command string to the left of any "="
    //Before "msg" is parsed into pieces, store a copy of it
    //in case we need to log this command.
    strncpy_safe( origCmdString, sizeof(origCmdString), msg, _TRUNCATE);

    //Search for index ([]) delimiters in the message
    nIndex = User_ExtractIndex(msg);
    //Convert tokens to upper case for string comparison.
    Supper( msg );

    //Get ready to search the command message tables, setup the initial
    //table pointer and get the first message token
    msgTokPtr = strtok_r(msg,".",&tokPtr);

    // Check for global-scope commands. These are not associated with a specific command table.
    // ===============
    // SHOWCFG command
    // ===============
    if (strncmp(msgTokPtr, DISPLAY_CFG, strlen(DISPLAY_CFG) ) == 0)
    {
      // Make sure the user isn't trying to set the showcfg to a value
      if (setTokPtr != NULL)
      {
        User_OutputMsgString("\r\n", FALSE);
        User_OutputMsgString(USER_MSG_READ_ONLY, TRUE);
      }
      else
      {
        // Only GSE can issue this command
        if ( USER_MSG_SOURCE_GSE != source )
        {
          User_OutputMsgString(USER_MSG_GSE_ONLY, TRUE);
        }
        else if( User_ShowAllConfig() )
        {
          // global showcfg worked ok. All the output strings have been written along
          // the way. Pass an empty buffer to User_PutRsp and finalize the msg
          rspString[0] = '\0';
          User_OutputMsgString(rspString, TRUE);
        }
        else
        {
          // Something went wrong,
          FATAL("ShowCfg Failure", NULL);
        }
      }
    }
    else
    {
      // Search command tables
      cmdTblPtr = User_TraverseCmdTables(msgTokPtr,&rootCmdTbl[0],tokPtr);

      if(cmdTblPtr == NULL)
      {
        //Message not found
        User_OutputMsgString("\r\n"USER_MSG_INVALID_CMD, TRUE );
      }
      else if(cmdTblPtr->pNext != NULL)
      {
        //Message incomplete, if this is a "get" cmd, print multiple responses
        // =====================
        // EXECUTE MULTIPLE CMDS
        // =====================
        if(setTokPtr == NULL)
        {
          User_ExecuteMultipleGetMsg(cmdTblPtr,source,nIndex,msg, USER_SINGLE_MSG_MAX_SIZE);
          // finalize the output
          rspString[0] = '\0';
          User_OutputMsgString(rspString, TRUE);
        }
        else
        {
          //Cannot issue multiple write commands
          User_OutputMsgString(USER_MSG_WRITE_NOT_COMPLETE, TRUE);
        }
      }
      else if(cmdTblPtr->MsgHandler != NULL)
      {
        //Handler found, execute the command and return a response

        // Checks to see if this command should be logged:
        // 1. Logging must be enabled.
        // 2. Ignore-logging-bit is clear
        // 3. This is an assignment or user-action
        // 4. The input source is GSE.

        // If all test are met, get the current value so before-and-after values can be logged.
        // Note: Call to User_ValidateMessage() is used to check for read only target.
        // Function will be called again and response string used by User_ExecuteSingleMsg().
        if( !(cmdTblPtr->MsgAccess & USER_NO_LOG) && (USER_MSG_SOURCE_GSE == source) &&
             ( ((USER_TYPE_ACTION == cmdTblPtr->MsgType) || (setTokPtr != NULL)) &&
                 User_ValidateMessage(cmdTblPtr, source, nIndex, rspString,
                                      setTokPtr, USER_SINGLE_MSG_MAX_SIZE) ) )
        {
          // Get the current value of the param (if applicable) and whether this command
          // is log-able
          bLogThisChange =
            User_GetParamValue(cmdTblPtr, source, nIndex, oldValue, sizeof(oldValue));
        }
        // ==================
        // EXECUTE SINGLE CMD
        // ==================
        // The variable 'msg' which contained the command, will now be used to hold
        // the result string.
        User_OutputMsgString("\r\n", FALSE);
        rspString[0] = '\0';
        bExecutionResult = User_ExecuteSingleMsg(cmdTblPtr, source, nIndex,
                                                 setTokPtr, rspString,
                                                 USER_SINGLE_MSG_MAX_SIZE);
        // Output the results and finalize the display with CS and fast prompt
        User_OutputMsgString(rspString, TRUE);

        // if cmd is loggable and executed ok, log it.
        if(bLogThisChange && bExecutionResult )
        {
          // Bypass logging if "newvalue == oldvalue"
          if (0 != (strncmp(oldValue, USER_ACTION_TOKEN, sizeof(USER_ACTION_TOKEN)) ))
          {
            User_GetParamValue(cmdTblPtr, source, nIndex, newValue, sizeof(newValue));

            // Compare old and new for length of longest value.
            if( 0 == strncmp(oldValue, newValue, sizeof(oldValue)))
            {
              bLogThisChange = FALSE;
            }
          }

          if(bLogThisChange)
          {
            User_LogUserActivity(origCmdString, oldValue, newValue);
          }
        }
      }
      else
      {
        //Unexpected error, tables not programmed correctly
        FATAL("GSE Table format error. Cmd('%s')", msg);
      }
    }
  }
}

/******************************************************************************
 * Function:    User_TraverseCmdTables
 *
 * Description: Starting at the root message table pointed to by CmdMsgTbl, the
 *              routine attempts to match the first token with a string in
 *              the root table. If found, the next table is extracted from the
 *              root table, and the process repeats with the second token.
 *              Any extraneous characters in the tokens will cause a mismatch.
 *              The index [ ] and assign = delimiters need to be stripped off
 *              before calling this function.
 *
 *              The process terminates when:
 *              1. No more tokens are available
 *              2. A token is not found in a table
 *              3. A token matches a table entry that has an "action"
 *                 function
 *
 * Parameters:  [in/out] MsgTokPtr
 *              [in]     CmdMsgTbl: Pointer to the root command table
 *              [in/out] TokPtr: Message string containing "." delimited
 *                        commands (strtok function is used, which writes "."
 *                        to '\0'
 *
 * Returns:     USER_MSG_TBL* A pointer to the cmd that caused the search to
 *                        terminate
 *                        1. If Return_val.MsgHandler != NULL, the function
 *                           was found, the caller should call the MsgHandler
 *                        3. If Return_val.pNext != NULL, the command is
 *                           incomplete. The caller may want to list the tokens
 *                           in this table to the GSE user as a help function.
 *                        2. If Return_val == NULL, command message does not
 *                           exist in the user message tables
 *
 * Notes:
 *
 *****************************************************************************/
static
USER_MSG_TBL* User_TraverseCmdTables(INT8* MsgTokPtr, USER_MSG_TBL* CmdMsgTbl, INT8* TokPtr)
{
  //Traverse the command tables, token by token.
  while(MsgTokPtr != NULL)
  {
    //Search for token string in the current CMD table
    // while((strcasecmp(CmdMsgTbl->MsgStr,TokPtr) != 0) && CmdMsgTbl->MsgStr != NULL)
    // Change ordering of conditional operation so that when MsgStr == NULL we do not
    //    perform the strcasecmp() with NULL ptr.  This could result in unwanted
    //    behavior, although initial operation indicates there are no problem,
    //    in the future (with future builds), this might not be the case.
    while( (CmdMsgTbl->MsgStr != NULL) && (strcasecmp(CmdMsgTbl->MsgStr,MsgTokPtr) != 0) )
    {
      CmdMsgTbl++;
    }

    if(CmdMsgTbl->MsgStr != NULL)
    {
      //Token found, advance to next token
      MsgTokPtr = strtok_r(NULL,".",&TokPtr);

      //See if another table exists and if a token is available to search for
      if((CmdMsgTbl->pNext != NULL) && (MsgTokPtr != NULL))
      {
        //Advance to next command table
        CmdMsgTbl = CmdMsgTbl->pNext;
      }
    }
    else
    {
      //Encountered the end of this table without matching the token.
      //This cmd message was not valid, exit loop with CmdMsgPtr pointing
      //to the null table entry.
      MsgTokPtr = NULL;
      CmdMsgTbl = NULL;
    }
  }

  return CmdMsgTbl;
}

/******************************************************************************
 * Function:    User_ExecuteSingleMsg
 *
 * Description: Performs the task of setting, getting or performing the action
 *              associated with a single command message.
 *
 * Parameters:  [in] MsgTbl: Pointer to the message table entry that defines
 *                           this command message
 *              [in] source: Device this message came from.  This is needed
 *                           because certain commands can be issued from the
 *                           GSE port only.
 *              [in] Index:  Value of the index (if specified in the command)
 *              [in] SetStr: The string after the "=" delimiter (specified for
 *                           write command messages)
 *              [out] RspStr: Location to store the result of the message
 *                            handler for read command messages.
 *              [in]  Len:    Size of the location pointed to by RspStr.
 *                            If the response size exceeds "Len" the response
 *                            is truncated to Len.
 *
 * Returns:     TRUE if successful otherwise FALSE
 *
 * Notes:       tempInt size must be declared as USER_DATA_TYPE_MAX_WORDS
 *              to allow temp storage of the largest defined user data type.
 *              ref: < USER_TYPE_128_LIST >, < USER_TYPE_SNS_LIST >
 *****************************************************************************/
static
BOOLEAN User_ExecuteSingleMsg(USER_MSG_TBL* MsgTbl,USER_MSG_SOURCES source,
                              INT32 Index, INT8* SetStr, INT8* RspStr, UINT32 Len)
  {
    UINT32 tempInt[USER_DATA_TYPE_MAX_WORDS]; // Store up to USER_TYPE_128_LIST. See Notes.
    void* setPtr = tempInt;
    void* getPtr = tempInt;
    USER_RANGE min,max;
    BOOLEAN bSuccess = FALSE;

    if (User_ValidateMessage(MsgTbl,source,Index,RspStr,SetStr, Len))
    {
      //SetStr is the string after the "=" delimiter.  If it is present
      //this is a "set" command to write a value.
      if(SetStr != NULL)
      {
        memcpy(&min,&MsgTbl->MsgRangeMin,sizeof(min));
        memcpy(&max,&MsgTbl->MsgRangeMax,sizeof(max));
        if(User_CvtSetStr(MsgTbl->MsgType,SetStr,&setPtr,MsgTbl->MsgEnumTbl, &min,&max))
        {
          if( USER_RESULT_OK != MsgTbl->MsgHandler(MsgTbl->MsgType,
                                                   MsgTbl->MsgParam,
                                                   Index,
                                                   setPtr,
                                                   NULL))
          {
            strncpy_safe(RspStr,(INT32)Len, USER_MSG_INVALID_CMD_FUNC,_TRUNCATE);
          }
          else
          {
            bSuccess = TRUE;
          }
        }
        else
        {
          User_ConversionErrorResponse(RspStr,min,max,MsgTbl->MsgType,
                                       MsgTbl->MsgEnumTbl,Len);
        }
      }
      // No string or "=" delimiter found, then:
      // this is a "get" command to read a value
      // or...
      // an "action" command (has no 'set' or 'get').
      // Either way, we handle the same.
      else
      {
        if( USER_RESULT_OK != MsgTbl->MsgHandler(MsgTbl->MsgType,
                                                 MsgTbl->MsgParam,
                                                 Index,
                                                 NULL,
                                                 &getPtr) )
        {
          strncpy_safe(RspStr, (INT32)Len, USER_MSG_INVALID_CMD_FUNC, _TRUNCATE);
        }
        else
        {
          if(!User_CvtGetStr(MsgTbl->MsgType,RspStr,Len,getPtr,
                             MsgTbl->MsgEnumTbl))
          {
            //Conversion error
            strncpy_safe(RspStr,(INT32)Len, USER_MSG_RSP_CONVERSION_ERR, _TRUNCATE);
          }
          else
          {
            bSuccess = TRUE;
          }
        }
      }
    }
    return bSuccess;
  }

/******************************************************************************
 * Function:     User_ExecuteMultipleGetMsg
 *
 * Description:  This function executes a command message that is not completed
 *               to the leaf token.  It executes the one or more command
 *               messages that is one level beyond the last token typed in the
 *               command message string.  This method is only available to read
 *               multiple values, it is not possible to write multiple values
 *
 * Parameters:  [in] MsgTblPtr: Pointer to a non-leaf user message table
 *                              entry for this command
 *              [in] source:    Device this message came from.  This is needed
 *                              because certain commands can be issued from the
 *                              GSE port only.
 *              [in] Index:     Value of the index(if specified in the command)
 *              [out] rsp:      Location to store the result of the message
 *                              handler for read command messages.
 *              [in] Len:       size of "rsp" buffer.
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
static
void User_ExecuteMultipleGetMsg(USER_MSG_TBL* MsgTblPtr,
                                USER_MSG_SOURCES source, INT32 Index,
                                INT8* rsp, UINT32 Len)
{
  INT8 tempStr[256];
  if(MsgTblPtr->pNext)
  {
    *rsp = '\0';
    MsgTblPtr = MsgTblPtr->pNext;

    //execute each message in the command table
    while(MsgTblPtr->MsgStr != NULL)
    {
      snprintf(tempStr,sizeof(tempStr),"\r\n %s =",MsgTblPtr->MsgStr);
      PadString(tempStr,USER_MAX_MSG_STR_LEN+6);

      //Append MsgStr to message, exit loop if rsp buffer is full
      if(!User_OutputMsgString(tempStr, FALSE))
      {
        break;
      }

      //Execute the message as a get function if it is not an "action" type
      if( MsgTblPtr->MsgHandler != NULL &&
          MsgTblPtr->MsgType    != USER_TYPE_ACTION)
      {
        User_ExecuteSingleMsg(MsgTblPtr,source,Index,NULL, tempStr, sizeof(tempStr));
      }
      else if(MsgTblPtr->MsgType == USER_TYPE_ACTION)
      {
        strncpy_safe(tempStr,sizeof(tempStr),"Action function",_TRUNCATE);
      }
      else
      {
        strncpy_safe(tempStr, sizeof(tempStr), "More...",_TRUNCATE);
      }

      //Append response to message, exit loop if rsp buffer is full
      if(!User_OutputMsgString(tempStr, FALSE))
      {
        break;
      }
      //Print out the message and the result of the handler function
      MsgTblPtr++;
    }
  }
}

/******************************************************************************
 * Function:    User_ValidateMessage
 *
 * Description: Checks the message parameters for an index and compares it to
 *              the valid index range.
 *
 *              Checks the read/write only status and
 *              returns false if a read-only message is written, or a
 *              message write-only is read.
 *
 *              Check the message source to make sure the source can execute
 *              the command
 *
 *              Checks the access level (Privileged or Factory)
 *
 * Parameters:  [in] MsgTbl:  Pointer to the location of the message table
 *                            entry for this command message.
 *
 *              [in] source:  Used for validating that the message source
 *                            (GSE, Micro-server...)has permission to write
 *                            or execute this command message.
 *              [in] Index:   Index in the command message, it is validated
 *                            against the range in the UserMsg table entry.
 *              [out]RspStr:  Location of the response string to write
 *                            a descriptive error string if validation fails
 *              [in] SetStr:  Pointer to the set string for write command
 *                            messages.
 *              [in] Len
 * Returns:     TRUE - when all checks are ok, FALSE otherwise
 *
 * Notes:
 *
 *****************************************************************************/
static
BOOLEAN User_ValidateMessage(USER_MSG_TBL* MsgTbl,USER_MSG_SOURCES source,
                             INT32 Index, INT8* RspStr, INT8* SetStr, UINT32 Len)
{
  BOOLEAN isWr;  // current cmd is a write/set cmd
  BOOLEAN roOk;  // Read only flag ok
  BOOLEAN woOk;  // write only ok
  BOOLEAN pmOk;  // priv mode ok
  BOOLEAN fmOk;  // factory mode ok
  BOOLEAN gseOk; // cmd src GSE ok

  BOOLEAN result = TRUE;

  /*Check for an invalid or out of range index if index != -1*/
  if( (MsgTbl->MsgIndexMin > Index) || (MsgTbl->MsgIndexMax < Index))
  {
    //Report the index is not applicable to this command
    if(MsgTbl->MsgIndexMin == USER_INDEX_NOT_DEFINED &&
       MsgTbl->MsgIndexMax == USER_INDEX_NOT_DEFINED)
    {
      strncpy_safe(RspStr,(INT32)Len, USER_MSG_NOT_INDEXABLE, _TRUNCATE);
      result = FALSE;
    }
    //Report the valid index range if needed
    else
    {
      snprintf(RspStr,(INT32)Len,"%s (%d-%d)", USER_MSG_INDEX_OUT_OF_RANGE,
              MsgTbl->MsgIndexMin,
              MsgTbl->MsgIndexMax);
      result = FALSE;
    }
  }

  if (TRUE == result)
  {
    // Validate all of the Access Flags and their required conditions
    isWr  = SetStr != NULL;

    if ( (MsgTbl->MsgAccess & USER_RW) == USER_RW)
    {
      roOk = TRUE;
      woOk = TRUE;
    }
    else
    {
      // RO is ok if the command is a read/get cmd
      roOk  = (MsgTbl->MsgAccess & USER_RO) ? !isWr : TRUE;

      // WO is ok if the command is a Write/set cmd
      woOk  = (MsgTbl->MsgAccess & USER_WO) ? isWr  : TRUE;
    }

    // Priv Mode is ok if we are in priv mode or doing a read
    pmOk  = (MsgTbl->MsgAccess & USER_PRIV) ? (userPrivilegedMode || !isWr) : TRUE;

    // Fact Mode is ok if we are in fact mode or doing a read
    fmOk  = (MsgTbl->MsgAccess & USER_FACT) ? (userFactoryMode || !isWr)    : TRUE;

    // GSE is ok if the source was the GSE, or it is a read from the Ms
    gseOk = (MsgTbl->MsgAccess & USER_GSE) ? ((source == USER_MSG_SOURCE_GSE) || !isWr) : TRUE;

    // action types must be RO - this is a coding standard so we assert
    if (MsgTbl->MsgType == USER_TYPE_ACTION)
    {
      ASSERT_MESSAGE( MsgTbl->MsgAccess & USER_RO,
        "USER TABLE ERROR: %s is an action cmd but not RO", MsgTbl->MsgStr);
    }

    // All constraints must be true
    if (!gseOk)
    {
      strncpy_safe(RspStr,(INT32)Len, USER_MSG_GSE_ONLY, _TRUNCATE);
      result = FALSE;
    }
    else if ( !pmOk)
    {
      strncpy_safe(RspStr,(INT32)Len, USER_MSG_ACCESS_LEVEL, _TRUNCATE);
      result = FALSE;
    }
    else if ( !fmOk)
    {
      strncpy_safe(RspStr, (INT32)Len, USER_MSG_ACCESS_LEVEL, _TRUNCATE);
      result = FALSE;
    }
    else if ( !roOk)
    {
      strncpy_safe(RspStr,(INT32)Len, USER_MSG_READ_ONLY, _TRUNCATE);
      result = FALSE;
    }
    else if ( !woOk)
    {
      strncpy_safe(RspStr,(INT32)Len, USER_MSG_WRITE_ONLY, _TRUNCATE);
      result = FALSE;
    }
  }

  return result;
}

/******************************************************************************
 * Function:    User_ExtractAssign
 *
 * Description: Searches the command message string for the "=" delimiter.
 *              If found, this means the command string is writing a value.
 *              The string preceding the "=" char is null terminated and this
 *              routine returns a pointer to the string after the "=" char.
 *
 *              If "=" is not found, this means the command is reading a value.
 *              The msg string is unchanged, and the return value is NULL
 *
 *              This routine should be called before ExtractCmdIndex
 *
 * Parameters:  [in/out] msg: Full command string.  This routine will write
 *                            '\0' to the first "=" char found. White space
 *                            between the command and "=" is stripped.
 *
 * Returns:     INT8* to the string after the "=" char, minus any leading or
 *              trailing whitespace and non-printalbe chars. If no "=" char
 *              present the routine returns NULL.
 *
 * Notes:
 *
 *****************************************************************************/
static
INT8* User_ExtractAssign(INT8* msg)
{
  INT8* ptr = NULL;

  if((ptr = strchr(msg,'=')) != NULL)
  {
    *ptr++ = '\0';
    StripString(ptr); // Strip whitespace around the r-value.
    StripString(msg); // Strip whitespace around the l-value
  }

  return ptr;
}

/******************************************************************************
 * Function:    User_ExtractIndex
 *
 * Description: Searches the user manager command for the "[" and "]"
 *              delimiters that precede and terminate the index string.
 *              This routine attempts to convert the string in between the
 *              brackets to an integer.  If the conversion is successful, a
 *              null terminator replaces the '[' bracket so that the message
 *              only is in the msg string.  If the conversation fails or the
 *              brackets do not exist in the string, -1 is returned.
 *
 *              This routine should be called after ExtractAssign
 *
 *
 * Parameters:  [in/out] msg: The string of the entire command. If an
 *                            index is found, the string remaining after the
 *                            ] character is copied to the location of the
 *                            [ character.
 *
 * Returns:     The array index, if present and convertible to an integer.
 *
 * Notes:       Max index allowed is 4 digits (9999)
 *
 *****************************************************************************/
static
INT32 User_ExtractIndex(INT8* msg)
{
  INT32 i,nIndex = -1;
  INT8* openBktPtr;
  INT8* closeBktPtr;

  //search for index delimiters
  openBktPtr = strchr(msg,'[');
  closeBktPtr = strchr(msg,']');

  //Sanity check on bracket index length.  Space between the open and
  //close bracket should be between 1 and 4 characters
  if((closeBktPtr-openBktPtr > 1) && (closeBktPtr-openBktPtr < 6))
  {
    //verify every character between the [] are numbers
    for(i = 0;i < ((closeBktPtr-openBktPtr)-1); i++)
    {
      if(!isdigit(openBktPtr[i+1]))
      {
        openBktPtr = NULL;
        closeBktPtr = NULL;
      }
    }
  }
  else
  {
    openBktPtr = NULL;
    closeBktPtr = NULL;
  }

  //If both pointers are found and the string valid
  //convert the index string to integer. Null terminate
  //the string before the index
  if(openBktPtr != NULL && closeBktPtr != NULL)
  {
    nIndex = atoi(openBktPtr+1);
    strncpy_safe(openBktPtr,USER_SINGLE_MSG_MAX_SIZE, closeBktPtr+1,_TRUNCATE);
  }

  return nIndex;
}

/******************************************************************************
 * Function:     User_CvtSetStr
 *
 * Description:  Converts the string pointed to by SetStr to the data
 *               type indicated by the DataType member of HandlerParams
 *
 * Parameters:   [in] Type: Class of data to be set as indicated by this
 *                          commands' table entry
 *               [in] SetStr: ASCII terminated string the user entered after
 *                            the "=" delimiter
 *               [in/out] SetPtr: Pointer to the location to contain the
 *                                converted result.  Needs to point to a
 *                                32-bit location on entry for number
 *                                conversions
 *               [in] MsgEnumTbl: For ENUM classes of data only, pointer to
 *                                the string to enum table
 *               [in/out] Min,Max: Minimum and Maximum range the value can
 *                                 be set to.  See User_SetMinMax.
 *
 * Returns:      TRUE: If conversion was successful
 *               FALSE: If unable to convert the set string
 * Notes:
******************************************************************************/
static
BOOLEAN User_CvtSetStr(USER_DATA_TYPE Type,INT8* SetStr,void **SetPtr,
    USER_ENUM_TBL* MsgEnumTbl,USER_RANGE *Min,USER_RANGE *Max)
{
  CHAR* end;
  UINT32 i;
  UINT32 uint_temp;
  UINT32 byteCnt = 0;

  INT32  int_temp;
  FLOAT32 float_temp;
  FLOAT64 float64_temp;
  INT32 base = 10;
  BOOLEAN result = FALSE;

  if ( Type != USER_TYPE_STR)
  {
    Supper(SetStr);
  }

  //For numerical types, test the sign and base
  switch(Type)
  {
    case USER_TYPE_HEX8:
    case USER_TYPE_UINT8:
    case USER_TYPE_HEX16:
    case USER_TYPE_UINT16:
    case USER_TYPE_HEX32:
    case USER_TYPE_UINT32:
    //case USER_TYPE_INT8:
    //case USER_TYPE_INT16:
    //case USER_TYPE_INT32:
    if ( strncmp("0X",SetStr,2) == 0)
    {
      base = 16;
    }
    else if ( SetStr[0] == '0' && strlen(SetStr) > 1)
    {
      base = 8;
    }

    // Unhandled USER types for this operation
    //lint -fallthrough
    case USER_TYPE_128_LIST:
    case USER_TYPE_ACTION:
    case USER_TYPE_ACT_LIST:
    case USER_TYPE_BOOLEAN:
    case USER_TYPE_END:
    case USER_TYPE_ENUM:
    case USER_TYPE_FLOAT:
    case USER_TYPE_FLOAT64:
    case USER_TYPE_INT32:
    case USER_TYPE_NONE:
    case USER_TYPE_ONOFF:
    case USER_TYPE_SNS_LIST:
    case USER_TYPE_STR:
    case USER_TYPE_YESNO:
    default:
      // Do nothing, these types are ignored!
      break;
  }

  /*Check and set Min/Max to data type limits if no limits are defined*/
  User_SetMinMax(Min,Max,Type);

  /*Convert string and cast into the data type as indicated by the message
    table*/
  switch(Type)
  {
    case USER_TYPE_HEX8:
    case USER_TYPE_UINT8:
      /*UINT8 type, check min/max bounds incl. the max value a uint8 can
        represent.  Value also must be positive*/
      uint_temp = strtoul(SetStr,&end,base);
      if( *end == '\0' && uint_temp <= Max->Uint && uint_temp >= Min->Uint)
      {
        **(UINT8**)SetPtr = (UINT8)uint_temp;
        result = TRUE;
      }
      break;

    case USER_TYPE_HEX16:
    case USER_TYPE_UINT16:
      /* UINT16 type, check min/max bounds incl. the max value a uint16 can
         represent.  Value also must be positive */
      uint_temp = strtoul(SetStr,&end,base);
      if( *end == '\0' && uint_temp <= Max->Uint && uint_temp >= Min->Uint)
      {
        **(UINT16**)SetPtr = (UINT16)uint_temp;
        result = TRUE;
      }
      break;

    case USER_TYPE_HEX32:
    case USER_TYPE_UINT32:
      // special cases we need to address for very large integers
      // .. as strtoul quietly limits the value to UINT32_MAX
      {
        CHAR reverseBuffer[20];
        uint_temp = strtoul(SetStr,&end,base);
        if (base == 16)
        {
          // need to deal with len of input string in hex case as "0X00FF1234" != "0XFF1234"
          snprintf( reverseBuffer, 20, "0X%0*X", strlen(SetStr)-2, uint_temp);
        }
        else if (base == 8)
        {
          // need to deal with len of input string in octal case as "001234" != "01234"
          snprintf( reverseBuffer, 20, "0%0*o", strlen(SetStr)-1, uint_temp);
        }
        else
        {
          snprintf( reverseBuffer, 20, "%u", uint_temp);
        }

        // did we get back what the user put in ?
        if (strcmp( SetStr, reverseBuffer) == 0)
        {
          /* UINT32 type, check min/max bounds.  Value also must be positive */
          if( *end == '\0' && uint_temp <= Max->Uint && uint_temp >= Min->Uint)
          {
            **(UINT32**)SetPtr = uint_temp;
            result = TRUE;
          }
        }
      }
      break;

    case USER_TYPE_ACT_LIST:
    case USER_TYPE_SNS_LIST:
    case USER_TYPE_128_LIST:
      //*********************************************************************************
      // NOTE: if adding new USER_TYPE_xxxx_LIST, update how byteCnt is assigned below.
      //*********************************************************************************

      // Clear the receiving bit field based on the byteCnt of specific USER_TYPE
      byteCnt = (USER_TYPE_ACT_LIST == Type) ? sizeof(UINT32) : sizeof(BITARRAY128);
      memset((UINT32*)*SetPtr, 0, byteCnt );

      // Check if value is a list of 1..128 CSV decimal-values enclosed by square brackets
      // e.g.: [ 2,6,23, 56,127 ]
      if (0 != strstr(SetStr, "[") && 0 != strstr(SetStr, "]") )
      {
        result = User_SetBitArrayFromList(Type, SetStr, SetPtr, Min, Max);
      }
      // Bit list defined as a Hex string?
      else if (0 != strstr(SetStr, "0X"))
      {
        result = User_SetBitArrayFromHexString(Type, SetStr, SetPtr, Min, Max);
      }
      // Bit list defined as a decimal integer
      else if ( isdigit(*SetStr) )
      {
        result = User_SetBitArrayFromIntegerValue(Type, SetStr, SetPtr, Min, Max);
      }
      // ... if nothing in the set string, the user just wants to clear all  bits.
      else if (0 == strlen(SetStr))
      {
        result = TRUE;
      }
      else
      // Invalid assignment, display a error msg.
      {
        result = FALSE;
      }
    break;


    //case USER_TYPE_INT8:
    //  /* INT8 type, check min/max bounds incl. the min/max value a int8 can
    //     represent.*/
    //  int_temp = strtol(SetStr,&end,base);
    //  if(*end == '\0' && int_temp <= Max->Sint && int_temp >= Min->Sint)
    //  {
    //    **(UINT8**)SetPtr = (UINT8)int_temp;
    //    result = TRUE;
    //  }
    //  break;

    //case USER_TYPE_INT16:
    //  /* INT16 type, check min/max bounds incl. the min/max value a int16 can
    //     represent. */
    //  int_temp = strtol(SetStr,&end,base);
    //  if(*end == '\0' && int_temp <= Max->Sint && int_temp >= Min->Sint)
    //  {
    //    **(UINT16**)SetPtr = (UINT16)int_temp;
    //    result = TRUE;
    //  }
    //  break;

    case USER_TYPE_INT32:
      /* INT32 type, check min/max bounds
         special cases we need to address for very large integers
         .. as strtol quietly limits the value to INT32_MAX */
      {
        CHAR reverseBuffer[20];
        int_temp = strtol(SetStr, &end, base);
        snprintf( reverseBuffer, 20, "%d", int_temp);

        if (strcmp( SetStr, reverseBuffer) == 0)
        {
          /* INT32 type, check min/max bounds. */
          if( *end == '\0' && int_temp <= Max->Sint && int_temp >= Min->Sint)
          {
            **(INT32**)SetPtr = int_temp;
            result = TRUE;
          }
        }
      }
      break;

    case USER_TYPE_STR:
      if(strlen(SetStr) >= Min->Uint && strlen(SetStr)+1 <= Max->Uint)
      {
        *(INT8**)SetPtr = SetStr;
        result = TRUE;
      }
      break;

    case USER_TYPE_FLOAT:
      /* FLOAT type
         - check min/max bounds incl. the min/max value a float32 can represent.
         - check the entire string is consumed by the conversion */
      float_temp= (float)strtod(SetStr, &end);
      if( *end == '\0' && float_temp <= Max->Float && float_temp >= Min->Float)
      {
        **(FLOAT32**)SetPtr = float_temp;
        result = TRUE;
      }
      break;

    case USER_TYPE_FLOAT64:
      /* FLOAT64 type
         - check min/max bounds incl. the min/max value a float64 can represent.
         - check the entire string is consumed by the conversion
         - Note: strtod() returns double  */
      float64_temp= (FLOAT64)strtod(SetStr, &end);
      if( *end == '\0' && float64_temp <= Max->Float64 && float64_temp >= Min->Float64)
      {
        **(FLOAT64**)SetPtr = float64_temp;
        result = TRUE;
      }
      break;

    case USER_TYPE_BOOLEAN:
      // Do one extra character to ensure there is nothing else following these like "TRUEX"
      if(strncmp("TRUE",SetStr,5) == 0)
      {
        **(BOOLEAN**)SetPtr = TRUE;
        result = TRUE;
      }
      else if(strncmp("FALSE",SetStr,6) == 0)
      {
         **(BOOLEAN**)SetPtr = FALSE;
        result = TRUE;
      }
      else
      {
        result = FALSE;
      }
      break;

    case USER_TYPE_YESNO:
      // Do one extra character to ensure there is nothing else following these like "YESX"
      if(strncmp("YES",SetStr,4) == 0)
      {
        **(BOOLEAN**)SetPtr = TRUE;
        result = TRUE;
      }
      else if(strncmp("NO",SetStr,3) == 0)
      {
         **(BOOLEAN**)SetPtr = FALSE;
        result = TRUE;
      }
      else
      {
        result = FALSE;
      }
      break;

    case USER_TYPE_ONOFF:
      // Do one extra character to ensure there is nothing else following these like "ONX"
      if(strncmp("ON",SetStr,3) == 0)
      {
        **(BOOLEAN**)SetPtr = TRUE;
        result = TRUE;
      }
      else if(strncmp("OFF",SetStr,4) == 0)
      {
         **(BOOLEAN**)SetPtr = FALSE;
        result = TRUE;
      }
      else
      {
        result = FALSE;
      }
      break;

    case USER_TYPE_ENUM: //lint -fallthrough
    //case USER_TYPE_ENUM16:
    //case USER_TYPE_ENUM8:
      result = TRUE;

      //find the string the the table and return its corresponding integer value
      ASSERT(MsgEnumTbl != NULL);
      for(i = 0; MsgEnumTbl[i].Str != NULL; i++)
      {
        if(strcmp(MsgEnumTbl[i].Str,SetStr) == 0)
        {
          if(USER_TYPE_ENUM == Type)
          {
            ASSERT_MESSAGE( (MsgEnumTbl[i].Num <= UINT32_MAX),
                           "Enum Table value: %d exceeds USER_TYPE_ENUM: %d",
                           MsgEnumTbl[i].Num, UINT32_MAX);
            **(UINT32**)SetPtr = MsgEnumTbl[i].Num;
          }
          /*else if(USER_TYPE_ENUM8 == Type)
          {
            ASSERT_MESSAGE( (MsgEnumTbl[i].Num <= UINT8_MAX),
              "Enum Table value: %d exceeds USER_TYPE_ENUM8: %d",
              MsgEnumTbl[i].Num, UINT8_MAX);
            **(UINT8**)SetPtr = (UINT8)MsgEnumTbl[i].Num;
          }
          else if(USER_TYPE_ENUM16 == Type)
          {
            ASSERT_MESSAGE( (MsgEnumTbl[i].Num <= UINT16_MAX),
                           "Enum Table value: %d exceeds USER_TYPE_ENUM16: %d",
                           MsgEnumTbl[i].Num, UINT16_MAX);
                           **(UINT16**)SetPtr = (UINT16)MsgEnumTbl[i].Num;
          }*/
          result = TRUE;
          break;
        }
      }

      //If the string could not be found, return an error
      if(MsgEnumTbl[i].Str == NULL)
      {
        result = FALSE;
      }
      break;

    case USER_TYPE_NONE:    // Deliberate Fall-through
    case USER_TYPE_END:     // Deliberate Fall-through
    case USER_TYPE_ACTION:  // Deliberate Fall-through
    default:
      FATAL("Unsupported USER_DATA_TYPE = %d", Type );
      break;
  }

  return result;
}

/******************************************************************************
 * Function:     User_CvtGetStr
 *
 * Description:  Used for "get" type commands, to convert the data type
 *               returned by the message handler function to an ASCII string.
 *
 * Parameters:   [in] Type: Class of data to be set as indicated by this
 *                          commands' table entry
 *               [out] GetStr: Pointer to a location to store the string
 *                             conversion result
 *               [in]  Len:    Size of the GetStr location.  Gets longer than
 *                             Len will be truncated.
 *               [in]  GetPtr: Pointer to the location of the data to convert
 *                             to a string
 *               [in] MsgEnumTbl: For ENUM classes of data only, pointer to
 *                                the string to enum table
 *
 * Returns:      TRUE if result of operation was successful, otherwise FALSE
 *
 * Notes:
 ******************************************************************************/
BOOLEAN User_CvtGetStr(USER_DATA_TYPE Type, INT8* GetStr, UINT32 Len,
                       void* GetPtr, USER_ENUM_TBL* MsgEnumTbl)
{
  BOOLEAN result = TRUE;
  UINT32 i;

  ASSERT(GetStr != NULL);
  ASSERT(GetPtr != NULL);

  switch(Type)
  {
    case USER_TYPE_UINT32:
      snprintf(GetStr, (INT32)Len, "%u",*(UINT32*)GetPtr);
      break;

    case USER_TYPE_UINT16:
      snprintf(GetStr, (INT32)Len, "%u",*(UINT16*)GetPtr);
      break;

    case USER_TYPE_UINT8:
      snprintf(GetStr,(INT32)Len, "%u",*(UINT8*)GetPtr);
      break;

    case USER_TYPE_INT32:
      snprintf(GetStr,(INT32)Len, "%d",*(UINT32*)GetPtr);
      break;

    //case USER_TYPE_INT16:
    //  sprintf(GetStr,"%d",*(UINT16*)GetPtr);
    //  break;

    //case USER_TYPE_INT8:
    //  snprintf(GetStr,(INT32)Len, "%d",*(UINT8*)GetPtr);
    //  break;

    case USER_TYPE_HEX8:
      snprintf(GetStr, (INT32)Len, "0x%02X",*(UINT8*)GetPtr);
      break;

    case USER_TYPE_HEX16:
      snprintf(GetStr, (INT32)Len, "0x%04X",*(UINT16*)GetPtr);
      break;

    case USER_TYPE_HEX32:
      snprintf(GetStr, (INT32)Len, "0x%08X",*(UINT32*)GetPtr);
      break;

    case USER_TYPE_ACT_LIST:
    case USER_TYPE_SNS_LIST:
    case USER_TYPE_128_LIST:
      result = User_CvtGetBitListStr(Type, GetStr, Len, GetPtr);
      break;

    case USER_TYPE_STR:
      //GetStr[Len-1] = '\0';
      strncpy_safe(GetStr,(INT32)Len,(INT8*)GetPtr, _TRUNCATE);
      break;

    case USER_TYPE_FLOAT:
      snprintf(GetStr, (INT32)Len, "%f",*(FLOAT32*)GetPtr);
      break;

    case USER_TYPE_FLOAT64:
      snprintf(GetStr, (INT32)Len, "%.15f",*(FLOAT64*)GetPtr);
      break;

    case USER_TYPE_BOOLEAN:
      strncpy_safe(GetStr,(INT32)Len, *(BOOLEAN*)GetPtr ? "TRUE" : "FALSE", _TRUNCATE);
      break;

    case USER_TYPE_YESNO:
      strncpy_safe(GetStr,(INT32)Len,*(BOOLEAN*)GetPtr ? "YES" : "NO", _TRUNCATE);
      break;

    case USER_TYPE_ONOFF:
      strncpy_safe(GetStr,(INT32)Len,*(BOOLEAN*)GetPtr ? "ON" : "OFF", _TRUNCATE);
      break;


    //case USER_TYPE_ENUM8:// Fallthrough
    //case USER_TYPE_ENUM16:
    case USER_TYPE_ENUM:

      ASSERT(MsgEnumTbl != NULL);

      //find the number in the the table and return its corresponding string
      for(i = 0; MsgEnumTbl[i].Str != NULL; i++)
      {
        if( MsgEnumTbl[i].Num == *(UINT32*)GetPtr)
        {
          strncpy_safe(GetStr,Len, MsgEnumTbl[i].Str, _TRUNCATE);
          break;
        }
        ////Cast based on enum width for pseudo-enums.
        //if( MsgEnumTbl[i].Num == (Type == USER_TYPE_ENUM ? *(UINT32*)GetPtr
        //                                                 : *(UINT8*)GetPtr) )
        // //    Type == USER_TYPE_ENUM16 ? *(UINT16*)GetPtr : *(UINT8*)GetPtr) )
        //{
        //  ASSERT_MESSAGE( MsgEnumTbl[i].Num <= (Type == USER_TYPE_ENUM ? UINT32_MAX
        //                                                               : UINT8_MAX),
        //                 "Enum Table value exceeds range for type: %d", MsgEnumTbl[i].Num);
        //  strncpy_safe(GetStr,Len, MsgEnumTbl[i].Str, _TRUNCATE);
        //  break;
        //}

      }

      //If the string could not be found, return an error
      if(MsgEnumTbl[i].Str == NULL)
      {
        result = FALSE;
      }
      break;

    case USER_TYPE_ACTION:
      break;

    case USER_TYPE_END: // Fallthrough
    case USER_TYPE_NONE:// Fallthrough
    default:

      FATAL("User_CvtGetStr() Unrecognized Data Type: %d", Type);
      break;
  }

  return result;
}

/******************************************************************************
 * Function:     User_SetMinMax
 *
 * Description:  Checks the min max ranges based on data type.
 *               The input min/max are modified if they do not fall within
 *               the range for the given data type (ex. -127/+128 for int8).
 *               Also, the no limit condition is tested, where MIN and MAX are
 *               set to zero, and if so, the upper and lower limits of the
 *               Type are used.
 *
 *
 *
 *
 * Parameters:   [in/out] Min: Minimum limit for the value
 *               [in/out] Max: Maximum limit for the value
 *               [in]     Type
 * Returns:
 *
 * Notes:
******************************************************************************/
static
void User_SetMinMax(USER_RANGE *Min,USER_RANGE *Max,USER_DATA_TYPE Type)
{
  BOOLEAN noLimit = (Min->Sint == 0 && Max->Sint == 0) ? TRUE : FALSE;

  /*Switch differentiates and bounds 8 different types:
    Signed 8,16, and 32 bit
    Unsigned 8,16,32 bit
    Float
    String lengths
  */

  switch(Type)
  {
    case USER_TYPE_UINT8:
    case USER_TYPE_HEX8:
      Min->Uint = noLimit ? 0          : Min->Uint;
      Max->Uint = noLimit ? UINT8_MAX  : MIN(UINT8_MAX,Max->Uint);
      break;

    case USER_TYPE_UINT16:
    case USER_TYPE_HEX16:
      Min->Uint = noLimit ? 0          : Min->Uint;
      Max->Uint = noLimit ? UINT16_MAX : MIN(UINT16_MAX,Max->Uint);
      break;

    case USER_TYPE_UINT32:
    case USER_TYPE_HEX32:
      Min->Uint = noLimit ? 0          : Min->Uint;
      Max->Uint = noLimit ? UINT32_MAX : MIN(UINT32_MAX,Max->Uint);
      break;

    //case USER_TYPE_INT8:
    //  Min->Sint = noLimit ? INT8_MIN   : MAX(INT8_MIN,Min->Sint);
    //  Max->Sint = noLimit ? INT8_MAX   : MIN(INT8_MAX,Max->Sint);
    //  break;

    //case USER_TYPE_INT16:
    //  Min->Sint = NoLimit ? INT16_MIN  : MAX(INT16_MIN,Min->Sint);
    //  Max->Sint = NoLimit ? INT16_MAX  : MIN(INT16_MAX,Max->Sint);
    //  break;

    case USER_TYPE_INT32:
      Min->Sint = noLimit ? INT32_MIN  : MAX(INT32_MIN,Min->Sint);
      Max->Sint = noLimit ? INT32_MAX  : MIN(INT32_MAX,Max->Sint);
      break;

    case USER_TYPE_FLOAT:
      Min->Float = noLimit ? -FLT_MAX  : MAX(-FLT_MAX,Min->Float);
      Max->Float = noLimit ? FLT_MAX  : MIN(FLT_MAX,Max->Float);
      break;

    case USER_TYPE_FLOAT64:
      Min->Float64 = noLimit ? -DBL_MAX  : MAX(-DBL_MAX,(FLOAT64)Min->Float);
      Max->Float64 = noLimit ?  DBL_MAX  : MIN(DBL_MAX,(FLOAT64)Max->Float);
      break;

      //String length limit to half of the command string.
      //This allows ample room for the command, and should be sufficient
      //for any string value that needs to be set.
    case USER_TYPE_STR:
      Min->Uint = noLimit ? 0          : Min->Uint;
      Max->Uint = noLimit ? USER_SINGLE_MSG_MAX_SIZE/2 :
                                    MIN(USER_SINGLE_MSG_MAX_SIZE/2, Max->Uint);
      break;

    case USER_TYPE_128_LIST:
        Min->Uint = noLimit ? 0 : Min->Uint;

        Max->Uint = noLimit ? 127 :
                    MIN(127, Max->Uint);
        break;
    //lint -fallthrough
    case USER_TYPE_NONE:
    case USER_TYPE_ENUM:
    case USER_TYPE_ACT_LIST:
    case USER_TYPE_SNS_LIST:
    case USER_TYPE_BOOLEAN:
    case USER_TYPE_YESNO:
    case USER_TYPE_ONOFF:
    case USER_TYPE_ACTION:
    case USER_TYPE_END:
    default:
      /* Other types are not limit checked, they will default to here
         and no action is taken,  DO NOT PUT AN ASSERT HERE! */
      break;
  }
}

/******************************************************************************
 * Function:     User_ConversionErrorResponse
 *
 * Description:  Generates a helpful string to assist the user when there is
 *               and error setting a value.  It creates a string that identifies
 *               the data type of the value and the range that the value
 *               can be set to.
 *
 * Parameters:   [out] RspStr: Pointer to a location that will receive the
 *                             help string
 *               [in]  Min     Min range limit for the value
 *               [in]  Max     Max range limit for the value
 *               [in]  Type    Data type for the value
 *               [in]  Enum    Pointer to the list of discrete values for
 *                             Enum types.  Should be NULL if the Type is not
 *                             USER_TYPE_ENUM
 *               [in]  Len     Length
 *
 * Returns:    none
 *
 * Notes:
 ******************************************************************************/
static
void User_ConversionErrorResponse(INT8* RspStr,USER_RANGE Min,USER_RANGE Max,
                                  USER_DATA_TYPE Type,
                                  const USER_ENUM_TBL *Enum, UINT32 Len)
{
  INT32 i;
  // Calculate available dest-length(size of the buffer - strlen already in use.)
  UINT32 destLength = Len - strlen(RspStr);

  switch(Type)
  {
    case USER_TYPE_UINT8:
    case USER_TYPE_UINT16:
    case USER_TYPE_UINT32:
      snprintf(RspStr,destLength,USER_MSG_CMD_CONVERSION_ERR"valid range is %u to %u.%s",
              Min.Uint,Max.Uint, USER_MSG_VFY_FORMAT);
      break;

    case USER_TYPE_HEX8:
    case USER_TYPE_HEX16:
    case USER_TYPE_HEX32:
      snprintf(RspStr,destLength,
               USER_MSG_CMD_CONVERSION_ERR"valid range is 0x%x to 0x%x.%s",
               Min.Uint,Max.Uint, USER_MSG_VFY_FORMAT);
      break;

    //case USER_TYPE_INT8:
    //case USER_TYPE_INT16:
    case USER_TYPE_INT32:
      snprintf(RspStr,destLength,USER_MSG_CMD_CONVERSION_ERR"valid range is %d to %d.%s",
               Min.Sint,Max.Sint, USER_MSG_VFY_FORMAT);
      break;

    case USER_TYPE_FLOAT:
      snprintf(RspStr,destLength,
               USER_MSG_CMD_CONVERSION_ERR"valid range is %.7g to %.7g.%s%s",
               Min.Float,Max.Float, NEWLINE, "Floating point number formats only.");
      break;

    case USER_TYPE_FLOAT64:
      snprintf(RspStr,destLength,
               USER_MSG_CMD_CONVERSION_ERR"valid range is %.7g to %.7g.%s%s",
               Min.Float64,Max.Float64, NEWLINE, "Floating64 point number formats only.");
      break;

    case USER_TYPE_STR:
      snprintf(RspStr,destLength,
               USER_MSG_CMD_CONVERSION_ERR"length must be %u to %u characters",
               Min.Uint,Max.Uint-1);
      break;

    case USER_TYPE_BOOLEAN:
      strncpy_safe(RspStr,(INT32)destLength,
             USER_MSG_CMD_CONVERSION_ERR"valid set is [TRUE,FALSE]",_TRUNCATE);
      break;

    case USER_TYPE_YESNO:
      strncpy_safe(RspStr,(INT32)destLength,
             USER_MSG_CMD_CONVERSION_ERR"valid set is [YES,NO]",_TRUNCATE);
      break;

    case USER_TYPE_ONOFF:
      strncpy_safe(RspStr,(INT32)destLength,
             USER_MSG_CMD_CONVERSION_ERR"valid set is [ON,OFF]",_TRUNCATE);
      break;

    // lint - fallthrough
    case USER_TYPE_ENUM:
    //case USER_TYPE_ENUM8:
      if(Enum != NULL)
      {
        strncpy_safe(RspStr,(INT32)destLength,
               USER_MSG_CMD_CONVERSION_ERR"valid set is [",_TRUNCATE);
        for(i = 0; Enum[i].Str != NULL; i++)
        {
          SuperStrcat( RspStr, Enum[i].Str, destLength);
          if(Enum[i+1].Str != NULL)
          {
            SuperStrcat(RspStr, ",", destLength);
          }
        }
      }
      SuperStrcat(RspStr, "]", destLength);
      break;

    case USER_TYPE_ACT_LIST:
      snprintf(RspStr,destLength, USER_MSG_CMD_CONVERSION_ERR
              "Valid Action bits are 0-7(When met), 12-19(On duration), 27(Latch) "
              "and 31(Acknowledge)."NEW_LINE
              "Set via number list or hex value.");
      break;

    case USER_TYPE_SNS_LIST:
      snprintf(RspStr,destLength, USER_MSG_CMD_CONVERSION_ERR
              "Accepts sensor numbers 0-%d or a hex value of up to %d bits."NEW_LINE
              "Allows for %u..%u sensors to be selected.",
              MAX_SENSORS-1, MAX_SENSORS, Min.Uint, Max.Uint);
      break;

    case USER_TYPE_128_LIST:
      snprintf(RspStr,destLength, USER_MSG_CMD_CONVERSION_ERR\
              "Accepts a number list consisting of values in the range %u-%u, "NEW_LINE
              "a hex string where only bits %u-%u are allowed on."NEW_LINE
              "or an integer where only bits %u-%u are allowed on.",
              Min.Uint, Max.Uint, Min.Uint, Max.Uint, Min.Uint, Max.Uint );
      break;
    case USER_TYPE_ACTION:
      // lint -fallthrough
    case USER_TYPE_END:
      // lint -fallthrough
    case USER_TYPE_NONE:
       //lint -fallthrough
    default:
      FATAL("Unsupported USER_DATA_TYPE = %d", Type );
      break;
  }
}

/******************************************************************************
 * Function:     User_ValidateChecksum
 *
 * Description:  If the checksum delimiter "$" is present, the checksum
 *               of all the characters up to the $ is computed and compared
 *               against the value after the "$"
 *
 *
 * Parameters:   [in/out] str: string to verify the checksum of.  If present,
 *                             the checksum delimiter is replaced with a
 *                             null terminator.
 *
 * Returns:      True, when no user checksum is supplied
 *               True, when user supplied checksum matches computed checksum
 *               False, otherwise
 *
 * Notes:
******************************************************************************/
static
BOOLEAN User_ValidateChecksum( INT8 *str )
{
  UINT16 chksum = 0;
  INT8* chkstr;
  BOOLEAN result;
  CHAR* tokPtr = NULL;

  strtok_r( str, "$", &tokPtr);
  chkstr = strtok_r( NULL, "$", &tokPtr);

  // If "$" found, compute and verify the checksum
  if(chkstr != NULL)
  {
    while(*str != '\0')
    {
      chksum += *str++;
    }
    if(chksum == strtoul(chkstr,NULL,16))
    {
      result = TRUE;
    }
    else
    {
      result = FALSE;
    }
  }
  //No "$" found, message always verifies
  else
  {
    result = TRUE;
  }
  return result;
}

/******************************************************************************
 * Function:     User_AppendChecksum
 *
 * Description:  Appends the running CheckSum value of string to the current
*                msg buffer.
 *
 *               "\r\n$nnnn\r\n"
 *
 *               where "$" is the checksum delimiter and nnnn is the 4 char
 *               ASCII-hex representation of the 16-bit checksum
 *
 *
 * Parameters:   [in/out] str: null-terminated string to compute a checksum on
 *                             Upon return, the checksum is appended to the end
 *                             of the input string in the format described above
 *
 * Returns:
 *
 * Notes: The CheckSum value represents the check sum
 *        of an entire stream since the last call to ResetCheckSum. The string
 *        pointed to by str may represent only the FINAL string segment being
 *        outputted.
 *
******************************************************************************/
static
void User_AppendChecksum( INT8 *str )
{
  INT8   chkstr[16];
  snprintf(chkstr, sizeof(chkstr),"\r\n$%04x\r\n", checkSum);
  strcat(str,chkstr);
}

/******************************************************************************
* Function:     User_ShowAllConfig
*
* Description:  Checks the all command tables for the existence of a show config
*               command and executes it.
*
* Parameters:   [in] none
*
* Returns:     none
*
* Notes:
*****************************************************************************/
static
BOOLEAN User_ShowAllConfig(void)
{
  UINT32 tempInt;
  void* getPtr = &tempInt;
  UINT32 i = 0;
  USER_MSG_TBL* pCmdMsgTbl;
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  // Loop thru the the Root cmd Table
  while( ( result == USER_RESULT_OK ) &&
         ( i < USER_MAX_ROOT_CMDS )   &&
         ( rootCmdTbl[i].MsgStr != NULL ) )
  {
    // By design, each show config command is implemented in the
    // respective root table;(i.e. <table>.<show-cmd> )
    // There is no need to walk the branches if not found at root.

    // Point into the command table
    pCmdMsgTbl = rootCmdTbl[i].pNext;

    while(pCmdMsgTbl != NULL && pCmdMsgTbl->MsgStr != NULL && result == USER_RESULT_OK )
    {
      if( strcasecmp(pCmdMsgTbl->MsgStr, DISPLAY_CFG) == 0 &&
                     pCmdMsgTbl->MsgHandler != NULL  &&
                     pCmdMsgTbl->MsgType == USER_TYPE_ACTION )
      {
         result = pCmdMsgTbl->MsgHandler(pCmdMsgTbl->MsgType, pCmdMsgTbl->MsgParam,
                                         0, NULL, &getPtr);
         break;
      }
      else
      {
        ++pCmdMsgTbl;  // Next entry in this command table
      }
    }
    ++i; // Next command table
  }
  return (BOOLEAN)(result == USER_RESULT_OK);
}

/******************************************************************************
* Function:     User_DisplayConfigTree
*
* Description:  Recursively called function to display the contents of a cfg
*               tree
*
* Parameters:   BranchName
*               MsgTbl
*               SensorIdx
*               RecursionCount
*               UserTableName
*
* Returns:     USER_HANDLER_RESULT
*
* Notes:
*****************************************************************************/

USER_HANDLER_RESULT User_DisplayConfigTree(CHAR* BranchName, USER_MSG_TBL* MsgTbl,
                                          INT16 SensorIdx, UINT16 RecursionCount,
                                          CHAR* UserTableName)
{
  UINT32 tempInt;
  void* getPtr = &tempInt;

  USER_HANDLER_RESULT result;
  CHAR nextBranchName[USER_MAX_MSG_STR_LEN * 4 ];
  CHAR key[USER_MAX_MSG_STR_LEN * 3];
  CHAR value[32];
  USER_MSG_TBL*  pMsgTbl;
  INT32 nIndex;
  INT16 leafElementIndex = -1; // Init as disabled

  result = USER_RESULT_OK;

  // Sanity check that we don't keep recursing indefinitely!
  ASSERT_MESSAGE( RecursionCount < MAX_RECURSIVE_CALLS,
                  "MAX_RECURSIVE_CALLS(%d) EXCEEDED (%d)",
                  MAX_RECURSIVE_CALLS, RecursionCount);

  // Store a local copy of the pointer.
  pMsgTbl = MsgTbl;

  while (pMsgTbl->MsgStr  != NULL  &&
         pMsgTbl->MsgType != USER_TYPE_ACTION &&
         result == USER_RESULT_OK)
  {
    // If this field is a leaf, display the value.
    if(pMsgTbl->pNext == NULL)
    {
      // Check if caller has indicated that this table contains an
      // array whose size is asymmetric versus it's same-level leaves.
      // UserTableName contains the table name, MsgStr is the currently
      // processed leaf name.

      if (UserTableName != NULL && User_CheckForLeafArray(UserTableName, pMsgTbl->MsgStr) )
      {
        // Activate/increment the LeafArrayElementIndex
        ++leafElementIndex;
      }

      // If leaf element is active ( > -1 ) display the entry as an
      // array element of the leaf
      if (leafElementIndex > -1)
      {
        snprintf(key,sizeof(key),"\r\n%s%s[%d] =",BranchName,pMsgTbl->MsgStr,leafElementIndex);
        nIndex = leafElementIndex;
      }
      else // Just a single element
      {
        snprintf(key, sizeof(key), "\r\n%s%s =", BranchName, pMsgTbl->MsgStr);
        nIndex = SensorIdx;
      }

      PadString(key, USER_MAX_MSG_STR_LEN + 15);

      if (!User_OutputMsgString(key, FALSE) )
      {
        result = USER_RESULT_ERROR;
        break;
      }

      if( pMsgTbl->MsgHandler != NULL && USER_RESULT_OK == result )
      {
        if( USER_RESULT_OK != pMsgTbl->MsgHandler(pMsgTbl->MsgType, pMsgTbl->MsgParam,
                                                  nIndex, NULL, &getPtr))
        {
          User_OutputMsgString( USER_MSG_INVALID_CMD_FUNC, FALSE);
        }
        else
        {
          if(User_CvtGetStr(pMsgTbl->MsgType, value, sizeof(value),
                            getPtr, pMsgTbl->MsgEnumTbl))
          {
            if (!User_OutputMsgString( value, FALSE) )
            {
              result = USER_RESULT_ERROR;
              break;
            }
          }
          else
          {
            //Conversion error
            User_OutputMsgString( USER_MSG_RSP_CONVERSION_ERR, FALSE );
          }
        }
      }
    }
    else // this is a branch...
    {
      // Setup branch label info, then // Descend into the subtree by
      // calling myself with the next recursion level.
      snprintf(nextBranchName, sizeof(nextBranchName), "%s%s.", BranchName, pMsgTbl->MsgStr);

      result = User_DisplayConfigTree(nextBranchName, pMsgTbl->pNext,
                                    SensorIdx, RecursionCount + 1, UserTableName);
    }

    // If LeafElementIndex is being used and we have processed the last entry
    // of an array-leaf, clear the flag.
    if (leafElementIndex != -1 && leafElementIndex == pMsgTbl->MsgIndexMax)
    {
       leafElementIndex = -1;
    }

    // If not iterating through the subscripts of a leaf's array,
    // move on to the next leaf/node.
    if (leafElementIndex == -1)
    {
       pMsgTbl++;
    }

  } // while

  return result;
}


/******************************************************************************
* Function:     User_CheckForLeafArray
*
* Description:  Helper function used to test if a leaf should be handled as an array
*               of items
*
* Parameters:   [in] UserTableName:  Pointer to a string containing the name
*                                    of the table
*
*               [in] MsgStr:    Pointer to a string containing the name of
*                               leaf item being processed
*
* Returns:     BOOLEAN          True if the current leaf is identified by
*                               UserTableName.MsgStr should be treated as a array.
*
* Notes:
*****************************************************************************/
static
BOOLEAN User_CheckForLeafArray(CHAR* UserTableName, CHAR* MsgStr)
{
   typedef struct{
      CHAR* tableName;    //User Cmd Table name
      CHAR* leafNames;
   }ASYMMETRIC_LEAF_TABLE;

   ASYMMETRIC_LEAF_TABLE asymmetricTable[] =
   {
      {"QAR",  "|BARKER|"},
      // Add other tables with entries here as indicated...
      //{"TABLE","|LEAFNAME_1|LEAFNAME_2|...|"}

      {NULL,NULL}
   };
   INT16 i;
   BOOLEAN result = FALSE;

   // Check the ResultBuffer to see if the table currently being processed
   // is in the list as containing asymmetric entries.
   i = 0;
   while (asymmetricTable[i].tableName != NULL)
   {
      if (NULL != strstr(UserTableName, asymmetricTable[i].tableName))
      {
         // This table has has asymmetric entries.
         // Check if the current MsgStr is in the list.
         if (NULL != strstr(asymmetricTable[i].leafNames, MsgStr))
         {
            result = TRUE;
            break;
         }
      }
      ++i;
   }
   return result;
}

/******************************************************************************
* Function:     User_GenericAccessor
*
* Description:  Universal Accessor function for all data user types
*
* Parameters:   [in] DataType:  C type of the data to be read or changed, used
*                               for casting the data pointers
*               [in/out] Param: Pointer to the configuration item to be read
*                               or changed
*               [in] Index:     Index parameter is used to reference the
*                               specific sensor to change.  Range is validated
*                               by the user manager
*               [in] SetPtr:    For write commands, a pointer to the data to
*                               write to the configuration.
*               [out] GetPtr:   For read commands, UserCfg function will set
*                               this to the location of the data requested.
*
* Returns:     USER_RESULT_OK
*
* Notes:
*****************************************************************************/
USER_HANDLER_RESULT User_GenericAccessor(USER_DATA_TYPE DataType,
                                                USER_MSG_PARAM Param,
                                                UINT32 Index,
                                                const void *SetPtr,
                                                void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  if(SetPtr == NULL)
  {
    //Set the GetPtr to the Cfg member referenced in the table
    *GetPtr = Param.Ptr;
  }
  else
  {
    // Set the param data with the data at the SetPtr
    switch(DataType)
    {
      //lint -fallthrough
      case USER_TYPE_UINT8:
      //case USER_TYPE_ENUM8:
        *(UINT8*)Param.Ptr = *(UINT8*)SetPtr;
        break;

      case USER_TYPE_UINT16:
        *(UINT16*)Param.Ptr = *(UINT16*)SetPtr;
        break;

      case USER_TYPE_STR:
        strcpy ( Param.Ptr, SetPtr);
        break;

      case USER_TYPE_FLOAT:
        *(FLOAT32*)Param.Ptr = *(FLOAT32*)SetPtr;
        break;

      case USER_TYPE_FLOAT64:
        *(FLOAT64*)Param.Ptr = *(FLOAT64*)SetPtr;

      //lint -fallthrough
      case USER_TYPE_ENUM:
      case USER_TYPE_UINT32:
      case USER_TYPE_HEX32:
        *(UINT32*)Param.Ptr = *(UINT32*)SetPtr;
        break;

      case USER_TYPE_ACT_LIST:
        memcpy(Param.Ptr, SetPtr, sizeof(UINT32));
        break;

      // lint -fallthrough
      case USER_TYPE_SNS_LIST:
      case USER_TYPE_128_LIST:
        memcpy(Param.Ptr, SetPtr, sizeof(BITARRAY128));
        break;

      // lint -fallthrough
      case USER_TYPE_BOOLEAN:
      case USER_TYPE_YESNO:
      case USER_TYPE_ONOFF:
        *(BOOLEAN *) Param.Ptr = *(BOOLEAN*)SetPtr;
        break;

     /*
      case USER_TYPE_INT8:
      case USER_TYPE_INT16:
      case USER_TYPE_ENUM8:
      case USER_TYPE_ENUM16:
     */

      case USER_TYPE_HEX8:
        *(UINT8*)Param.Ptr = *(UINT8*)SetPtr;
        break;

      case USER_TYPE_INT32:
        *(INT32*)Param.Ptr = *(INT32*)SetPtr;
        break;

      case USER_TYPE_HEX16:
        *(UINT16*)Param.Ptr = *(UINT16*)SetPtr;
        break;

      case USER_TYPE_ACTION: // Do nothing
        break;

      case USER_TYPE_NONE:
        // lint -fallthrough
      case USER_TYPE_END:
        // lint -fallthrough
      default:
        ASSERT_MESSAGE(  DataType < USER_TYPE_END,
                         "Unrecognized USER_DATA_TYPE: %d", DataType);
        break;
    } // End of switch (DataType)
  }
  return result;
}



/******************************************************************************
* Function:     User_GetParamValue
*
* Description:  Accessor function to retrieve the current value of a param if possible
*
* Parameters:   MsgTbl - Ptr to specific user message table
*               source - source of user command
*               Index - index of array (if any defined)
*               RspStr - ptr to response for the command
*               Len - size of reponse data
*
* Returns:      TRUE - If update was made
*               FALSE - No update was made
*
* Notes:        Detects if the entry is associated with an action
*****************************************************************************/
static
BOOLEAN User_GetParamValue(USER_MSG_TBL* MsgTbl,USER_MSG_SOURCES source, INT32 Index,
                                INT8* RspStr, UINT32 Len)
{
  USER_MSG_TBL tblEntry;
  BOOLEAN logThisChange = FALSE;


  // If the entry is a action command,
  // return the dummy token-value.
  if ( (USER_TYPE_ACTION == MsgTbl->MsgType ) ||
       ( NULL != MsgTbl->MsgHandler  && MsgTbl->MsgParam.Ptr == NULL) )
  {
    strncpy_safe(RspStr,(INT32)Len, USER_ACTION_TOKEN, _TRUNCATE);
    logThisChange = TRUE;
  }
  else
  {
    // Make a temp copy of the cmd table entry so we can force the,
    // access to allow reading.
    memcpy(&tblEntry, MsgTbl, sizeof(USER_MSG_TBL));
    tblEntry.MsgAccess = USER_RO;
    if (!User_ExecuteSingleMsg(&tblEntry,source,Index,NULL,RspStr,Len ))
    {
       strncpy_safe(RspStr,(INT32)Len, "VALUE-NOT-READ",_TRUNCATE);
    }
    else
    {
      logThisChange = TRUE;
    }
  }
  return logThisChange;
}

/******************************************************************************
* Function:     User_LogUserActivity
*
* Description:  Log a command or param change event.
*
* Parameters:   cmdString entered by user
*               valuePrev of the param if applicable,
*               valueNew of the param if applicable
* Returns:      None
*
* Notes:        Detects if the entry is associated with an action
*****************************************************************************/
static
void User_LogUserActivity(CHAR* cmdString, CHAR* valuePrev, CHAR* valueNew)
{
  CHAR logBuffer[SYSTEM_LOG_DATA_SIZE];
  SYS_APP_ID loggingId;

  Supper(cmdString);
  // log an action command string. (e.g. fast.reset, log.erase)
  if ( strncmp(valuePrev, USER_ACTION_TOKEN,sizeof(USER_ACTION_TOKEN))== 0 )
  {
    loggingId = APP_UI_USER_ACTION_COMMAND_LOG;
    snprintf(logBuffer, sizeof(logBuffer),"%s",cmdString);
  }
  else // log a param change (e.g. sensor[0].cfg.name = ...")
  {
    loggingId = APP_UI_USER_PARAM_CHANGE_LOG;
    snprintf(logBuffer, sizeof(logBuffer),"%s from: [%s] to [%s]",
             cmdString,valuePrev,valueNew );
  }

  LogWriteSystem(loggingId, LOG_PRIORITY_LOW, logBuffer, (UINT16)strlen(logBuffer), NULL);

  //GSE_DebugStr(NORMAL,TRUE, "Logged: %s",logBuffer);
}

/******************************************************************************
* Function:     User_AuthenticateModeRequest
*
* Description:  Checks if the submitted msg is the correct password for entering
*               Factory or privilege mode.
*               The entered user string is XOR'ed using the same key and offset
*               that was used for encrypting the valid password for the expected
*               user mode The resulting cryptogram is then tested against the
*               encrypted valid password; the clear-value of the valid password
*               is not present anywhere in the program. (SRS-1251)
*
*               The key table and offsets for each password are found in
*               "../application/validate.h" which is included into this function.
*               The validate.h file is generated by the Encryptor tool.
*
* Parameters:   [in] msg entered by user
*               [in] User mode to test. USER_FACT, or USER_PRIV
*
* Returns:      TRUE is the user string matches the password for the expected mode
*               otherwise, FALSE.
*
* Notes:        Detects if the entry is associated with an action
*****************************************************************************/
static
BOOLEAN User_AuthenticateModeRequest(const INT8* msg, UINT8 mode)
{

/* Include validation info here */
#include "validate.h"
/* */

  UINT8   cryptogram[32];
  BOOLEAN bFlag = TRUE;
  INT8    offset;
  const UINT8*  pWord = NULL;
  INT8    i;
  UINT8   length;

  // Set key-offset, compare phrase and length based on mode being tested.
  switch (mode)
  {
    case USER_FACT:
      offset = factoryKeyOffset;
      length = sizeof(fact);
      pWord = fact;
      break;

    case USER_PRIV:
      offset = privilegedKeyOffset;
      length = sizeof(priv);
      pWord = priv;
      break;
    default:
      FATAL("Invalid USER_DATA_ACCESS: %d", mode );
  }

  // Encrypt the user-entered phrase
  for ( i = 0; i < length; ++i )
  {
    cryptogram[i] = msg[i] ^ key[offset++];

    // NOTE:
    //  The following if statement will only be true if a password is very
    //  long and/or the initial offset is near the end of the key table.
    //  The current passwords (as of 10/2010) do not enter the if statement and
    //  cause the 'offset = 0' statement to appear as dead code.
    //  Since the intial offset is generated, this may change in the future.
    if ( key[offset] == NULL )
    {
      offset = 0; /* restart at the beginning of the key */
    }
  }

   // Set the result flag based on whether the entered password matches the expected.
  bFlag = (strncmp((const CHAR*)pWord, (const CHAR*)cryptogram, length) == 0) ? TRUE : FALSE;

  return bFlag;
}


/******************************************************************************
* Function:     User_OutputMsgString
*
* Description:  Return the Message source of the current command
*
* Parameters:   [in] source   pointer the string to be appended to the end of dest.
*               [in] len      size in bytes of string.
*               [in] finalize flag to append/write the checksum after source.
*
* Returns:      True if successful otherwise false.
*
* Notes:
******************************************************************************/
BOOLEAN User_OutputMsgString( const CHAR* string, BOOLEAN finalize)
{
  BOOLEAN statusFlag = TRUE;

  // Update checksum
  checkSum += (UINT16)ChecksumBuffer(string,strlen(string),UINT16MAX);

  switch(msgSource)
  {
    case USER_MSG_SOURCE_GSE:

      if( !finalize )
      {
        statusFlag = (DRV_OK == GSE_PutLineBlocked(string, TRUE));
      }
      else
      {
        // write out the final string with appended checksum and 'FAST>' prompt
        User_PutRsp(string, msgSource, msgTag );
      }
      break;

    case USER_MSG_SOURCE_MS:

      // If the buffer hasn't been flagged as overflowed, concatenate
      // the string to the end of the buffer.
      if (!rspBuffOverflowDetected)
      {
        rspBuffOverflowDetected = !SuperStrcat(msBuffer, string, sizeof(msBuffer));

        if ( rspBuffOverflowDetected )
        {
          GSE_DebugStr(NORMAL,TRUE,"Micro Server buffer overflow. Size: %d", strlen(msBuffer));

          MSI_PutResponse(CMD_ID_USER_MGR_MS, USER_MSG_RESP_OVERFLOW, MSCP_RSP_STATUS_FAIL,
            strlen(USER_MSG_RESP_OVERFLOW), msgTag);
          statusFlag = FALSE;
        }

        // If the stream is finalized, call to write the entire msg buffer back to MS
        if (finalize)
        {
          User_PutRsp(msBuffer, msgSource, msgTag );
        }
      }
      break;

    default:
      FATAL("Unrecognized msg source: %d", msgSource);
      break;
  }

  if (finalize)
  {
    checkSum = 0;
    msBuffer[0] = '\0';
  }

  return statusFlag;
}


/******************************************************************************
 * Function:     User_CvtGetBitListStr
 *
 * Description:  Used for "get" type commands, to convert Bit-List data type
 *               returned by the message handler function to an ASCII string.
 *
 * Parameters:   [in] Type: Class of data to be set as indicated by this
 *                          commands' table entry
 *               [out] GetStr: Pointer to a location to store the string
 *                             conversion result
 *               [in]  Len:    Size of the GetStr location.  Gets longer than
 *                             Len will be truncated.
 *               [in]  GetPtr: Pointer to the location of the data to convert
 *                             to a string
 *
 * Returns:      TRUE if result of operation was successful, otherwise FALSE
 *
 * Notes:
 ******************************************************************************/
static BOOLEAN User_CvtGetBitListStr(USER_DATA_TYPE Type, INT8* GetStr,UINT32 Len,void* GetPtr)
{
  BOOLEAN result = TRUE;
  BOOLEAN bEmptyArray;
  UINT32  tempWord;
  UINT32 i;
  UINT32 arraySizeWords;
  UINT32 displayHexStart;
  CHAR   numStr[5];  // max of 5 digits for an integer numbers
  CHAR   tempOutput[GSE_GET_LINE_BUFFER_SIZE];
  CHAR   bufHex128[16 * 2 + 3 ]; // 16 bytes x 2 chars per byte + "0x"  + null

  CHAR*   destPtr = tempOutput;
  UINT32* word32Ptr = (UINT32*)GetPtr;

  ASSERT(GetStr != NULL);
  ASSERT(GetPtr != NULL);

  if (Type == USER_TYPE_ACT_LIST)
  {
    arraySizeWords  = 1;  // size of array for USER_TYPE_ACT_LIST
    displayHexStart = 0;  // start of string offset
  }
  else
  {
    arraySizeWords = sizeof(BITARRAY128) / sizeof(UINT32);
    displayHexStart = HEX_STRING_PREF_LEN + 1; // hex string digits begins after leading "0x"
  }

  // Display a string containing both the hex and enumeration string:
  // e.g. 0x0000000000000000000000000000001F [0,1,2,3]

  // CHECK FOR EMPTY
  tempWord = 0;
  for (i = 0; i < arraySizeWords; ++i )
  {
    tempWord |= word32Ptr[i];
  }
  bEmptyArray = (0 == tempWord) ? TRUE : FALSE;

  // DISPLAY THE HEX STRING
  destPtr = bufHex128;
  strncpy_safe(destPtr, HEX_STRING_PREF_LEN + 1, "0x", _TRUNCATE);
  destPtr += HEX_STRING_PREF_LEN;

  // Display order: 127...0
  // Read the array from back to front and convert each word
  // to hex string.

  for( i = 0; i < arraySizeWords; ++i )
  {
    tempWord = word32Ptr[displayHexStart];
    snprintf( destPtr, sizeof(bufHex128), "%08X", tempWord );
    destPtr += HEX_CHARS_PER_WORD;
    displayHexStart -= 1;
  }
  strncpy_safe(tempOutput, sizeof(tempOutput), bufHex128, _TRUNCATE);

  // APPEND THE ENUMERATED LIST

  // Check each bit in the BITARRAY128.
  // For each 'on' bit, convert the index to a string
  // and concatenate to the output buffer.

  destPtr = tempOutput + strlen(tempOutput);

  SuperStrcat(destPtr, " [", sizeof(tempOutput));

  // Move ptr after " [" to fill with the converted string
  destPtr += 2;

  if ( bEmptyArray )
  {
    // Display "NONE SELECTED" String
    snprintf(destPtr,GSE_GET_LINE_BUFFER_SIZE, "NONE SELECTED,");
    destPtr = tempOutput + strlen(tempOutput);
  }
  else
  {
    // Display a list of integer values, one for each "on" bit.
    for( i = 0; i < (arraySizeWords * BITS_PER_WORD); ++i )
    {
      if (GetBit((INT32)i, (UINT32*)GetPtr, sizeof(BITARRAY128) ))
      {
        snprintf(numStr, sizeof(numStr), "%d,", i);
        SuperStrcat(destPtr, numStr, sizeof(tempOutput));
        destPtr += strlen(numStr);
      }
    }
  }

  // replace the final ',' with ']' and 'null'
  *(--destPtr) = ']';
  *(++destPtr) = '\0';

  strncpy_safe(GetStr, (INT32)Len, tempOutput, _TRUNCATE);

  return result;


}
/******************************************************************************
* Function:     User_SetBitArrayFromHexString
*
* Description:  Sets a BitArray128 storage using a hex string as input
*
* Parameters:   [in] Type: Class of data to be set as indicated by this
*                          commands' table entry
*               [in] SetStr: ASCII terminated string the user entered after
*                            the "=" delimiter
*               [in/out] SetPtr: Pointer to the location to contain the
*                                converted result.  Needs to point to a
*                                32-bit location on entry for number
*                                conversions
*               [in/out] Min,Max: Minimum and Maximum range the value can
*                                 be set to.  See User_SetMinMax.
*
* Returns:      True if successful otherwise false.
*
* Notes:
******************************************************************************/
static BOOLEAN User_SetBitArrayFromHexString(USER_DATA_TYPE Type,INT8* SetStr,void **SetPtr,
                                             USER_RANGE *Min,USER_RANGE *Max)
{
  BOOLEAN bResult = TRUE;
  UINT32  uint_temp;
  CHAR*   ptr;
  CHAR*   end;
  INT32   copyLen;
  CHAR    hexStrBuffer[HEX_STR_BUFFER_SIZE+1];
  CHAR    reverseBuffer[HEX_STR_BUFFER_SIZE+1];
  UINT32* destPtr = (UINT32*)*SetPtr;   // convenience ptr to output buffer
  INT16   offset = 0;                   // word-offset index into output buffer
  UINT32  inputLen  = strlen( SetStr ); // length of the input string.

  // Input can be empty otherwise
  // s/b "0x0" -> "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"

  if ( inputLen >= MIN_HEX_STRING && inputLen <= MAX_HEX_STRING)
  {
    // Set a pointer to the end of the input 'SetStr',
    // Skip leading '0x' prefix.
    inputLen -= HEX_STRING_PREF_LEN;
    ptr = &SetStr[HEX_STRING_PREF_LEN] + inputLen;

    // Process the hex string in R->L order, using up to 8 characters at a time to fill
    // a single 32 bit word of the HEX128 array.
    // (i.e. Big-Endian, the right-most char in string represents bits 0-3 in array entry [0])
    do
    {
      copyLen = MIN( (ptr - &SetStr[HEX_STRING_PREF_LEN]), HEX_CHARS_PER_WORD);
      ptr -= copyLen;

      // Prepend '0x' into buffer for strtoul, then copy up to 8 chars of hex digits
      // and convert to UINT32.
      strncpy_safe(hexStrBuffer,sizeof(hexStrBuffer),"0X",HEX_STRING_PREF_LEN);
      strncpy_safe(&hexStrBuffer[HEX_STRING_PREF_LEN],
                   sizeof(hexStrBuffer) - HEX_STRING_PREF_LEN,
                   ptr, copyLen);

      uint_temp = (UINT32)strtoul(hexStrBuffer, &end, BASE_16);

      // Reverse calculated value to verify conversion.
      // need to deal with len of input string in hex case as "0X00FF1234" != "0XFF1234"
      snprintf( reverseBuffer, sizeof(reverseBuffer),
                "0X%0*X", strlen(hexStrBuffer)-HEX_STRING_PREF_LEN, uint_temp);
      if (strcmp( hexStrBuffer, reverseBuffer) == 0)
      {
        destPtr[offset++] = uint_temp;
      }
      else
      {
        bResult = FALSE;
      }
    }
    while( ptr > &SetStr[2] && offset >= 0 && bResult );
  }

  // Validate the conversion based on the Min/Max and Type values passed in
  bResult = bResult && User_BitSetIsValid(Type, destPtr, Min, Max);

  return bResult;
}
/******************************************************************************
* Function:     User_SetBitArrayFromIntegerValue
*
* Description:  Sets a BitArray128 storage using an UINT32 value.
*
*
* Parameters:   [in] Type: Class of data to be set as indicated by this
*                          commands' table entry
*               [in] SetStr: ASCII terminated string the user entered after
*                            the "=" delimiter
*               [in/out] SetPtr: Pointer to the location to contain the
*                                converted result.  Needs to point to a
*                                32-bit location on entry for number
*                                conversions
*               [in] MsgEnumTbl: For ENUM classes of data only, pointer to
*                                the string to enum table
*               [in/out] Min,Max: Minimum and Maximum range the value can
*                                 be set to.  See User_SetMinMax.
*
* Returns:      True if successful otherwise false.
*
* Notes:        This conversion is ONLY INTENDED TO SUPPORT BACKWARD-COMPATIBILITY
*               FOR SENSOR/'TRIGGER SETTINGS which were previously handled in a single UINT32
*               word. Attempting to set bits using a value greater than 4294967295 will result
*               in an error.
*
******************************************************************************/
static BOOLEAN User_SetBitArrayFromIntegerValue(USER_DATA_TYPE Type,INT8* SetStr,void **SetPtr,
                                                USER_RANGE *Min,USER_RANGE *Max)
{
  BOOLEAN bResult;
  UINT32  decValue;
  CHAR    hexString[HEX_STR_BUFFER_SIZE+1]; // "0X" + 8 hex-chars + 1 null
  CHAR*   leftOver;
  CHAR    strDecValue[MAX_GSE_SET_STR];
  INT32   setStrLen = strlen(SetStr);

  // Convert the input string to decimal integer (if possible).
  decValue = strtoul(SetStr, &leftOver, BASE_10);

  // Convert the number back to integer string and compare to input string.
  // In the event of a range-error the stroul above will return a value of UINT_MAX which
  // IS NOT helpful since UINT_MAX is also a valid value for a bitarray. Therefore
  // a comparison to the original input string for it's length is needed to be sure
  // conversion was successful. If conversion was halted by invalid char, leftOver will be
  // non-null.
  snprintf(strDecValue, sizeof(strDecValue), "%0*u", setStrLen, decValue);

  if ((0 == strncmp(strDecValue, SetStr, setStrLen)) && *leftOver == '\0')
  {
    snprintf(hexString, sizeof(hexString), "0x%08X", decValue);
    bResult = User_SetBitArrayFromHexString(Type, hexString, SetPtr, Min, Max);
  }
  else
  {
    bResult = FALSE;
  }
  return bResult;
}

/******************************************************************************
* Function:     User_SetBitArrayFromList
*
* Description:  Sets a BitArray128 storage using a string of bracket-enclosed
*               CSV decimal values as input
*
* Parameters:   [in] Type: Class of data to be set as indicated by this
*                          commands' table entry
*               [in] SetStr: ASCII terminated string the user entered after
*                            the "=" delimiter
*               [in/out] SetPtr: Pointer to the location to contain the
*                                converted result.  Needs to point to a
*                                32-bit location on entry for number
*                                conversions
*               [in/out] Min,Max: Minimum and Maximum range the value can
*                                 be set to.  See User_SetMinMax.
*
* Returns:      True if successful otherwise false.
*
* Notes:
******************************************************************************/
static BOOLEAN User_SetBitArrayFromList(USER_DATA_TYPE Type,INT8* SetStr,void **SetPtr,
                                        USER_RANGE *Min,USER_RANGE *Max)
{
  BOOLEAN bResult = TRUE;
  CHAR*   ptr;
  CHAR*   end;
  UINT32* destPtr = (UINT32*)*SetPtr;   // convenience ptr to output buffer
  INT32   nIndex;
  UINT32  inputLen = strlen( SetStr ); // length of the input string.

  // Verify the string starts with '[' and ends with ']' and the CSV list is not empty.
  if (SetStr[0] == '[' && SetStr[inputLen - 1] == ']' && inputLen >= 3 )
  {
    // Skip the '[' and Loop until null-terminator is found.
    ptr = SetStr + 1;

    while((*ptr != '\0' && bResult) )
    {
      //Ignore spaces and delimiters
      if(*ptr == ' ' || *ptr == ',' || *ptr == ']' )
      {
        ptr++;
      }
      else if ( !isdigit(*ptr) )
      {
        bResult = FALSE;
      }
      else
      {
        // Attempt to convert to a base 10 integer and
        // verify within range for this entry.
        nIndex = strtol(ptr, &end, BASE_10 );
        if (nIndex >= 0 && nIndex < MAX_SENSORS)
        {
          SetBit(nIndex, destPtr, sizeof(BITARRAY128));
          ptr = end;
        }
        else
        {
          bResult = FALSE;
        }
      }
    } // while more tokens
  }

  // Validate the conversion based on the Min/Max and Type values passed in
  bResult = bResult && User_BitSetIsValid(Type, destPtr, Min, Max);

  return bResult;
}

/******************************************************************************
* Function:     User_BitSetIsValid
*
* Description:  Verifies that only valid bits are set in the 128 bit structure
*
* Parameters:   type (i): the type of object being operated on
*               destPtr (i): pointer to a 128 bit array
*               usrMin (i): min acceptable value (interpretation depends on type)
*               usrMax (i): max acceptable value (interpretation depends on type)
*
* Returns:      True if successful otherwise false.
*
* Notes:
******************************************************************************/
static
BOOLEAN User_BitSetIsValid(USER_DATA_TYPE type, UINT32* destPtr,
                           USER_RANGE *usrMin, USER_RANGE *usrMax)
{
#define MAX_BIT 128
  UINT32 mask;
  UINT8 iWord;
  UINT8 iBit;
  UINT8 minBit = MAX_BIT;
  UINT8 maxBit = 0;
  UINT8 bitIndex = 0;
  UINT8 bitCount = 0;
  UINT32* word = destPtr;
  BOOLEAN status = TRUE;



  if (type == USER_TYPE_ACT_LIST)
  {
    // verify only bit 0-7, 12-19, 27 and 31 are on
    status = ((word[0] & ~0x880ff0ff) != 0) ? FALSE : TRUE;
  }
  else
  {
    // scan the bits set and collect stats, min Bit, maxBit, count of bits set
    bitIndex = 0;
    for (iWord = 0; iWord < 4; ++iWord)
    {
      mask = 1;

      // see which bits are turned on and keep stats
      for ( iBit=0; iBit < 32; iBit++)
      {
        // is the bit on?
        if (word[iWord] & mask)
        {
          // only get to set min once as we walk through the bit array
          if (minBit == MAX_BIT)
          {
            minBit = bitIndex;
          }

          // usrMax always gets set as we walk through the bit array
          maxBit = bitIndex;

          // count the total number of bits set
          bitCount += 1;
        }

        // move to the next bit
        bitIndex += 1;
        mask <<= 1;
      }
    }

    // based on the type determine is we are good
    if (type == USER_TYPE_SNS_LIST)
    {
      // range for a SNS_LIST is the number of bits allowed
      if ( bitCount < usrMin->Uint || bitCount > usrMax->Uint )
      {
        status = FALSE;
      }
    }
    else
    {
      // range for other types (USER_TYPE_128_LIST) is min/max bit set
      // - count does not matter
      if (minBit < usrMin->Uint || maxBit > usrMax->Uint)
      {
        status = FALSE;
      }
    }
  } // process bit lists,



  return status;
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: User.c $
 * 
 * *****************  Version 125  *****************
 * User: Peter Lee    Date: 3/29/16    Time: 11:14p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7
 * 
 * *****************  Version 124  *****************
 * User: John Omalley Date: 2/23/16    Time: 11:51a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Code Review Updates
 * 
 * *****************  Version 123  *****************
 * User: John Omalley Date: 11/19/15   Time: 4:10p
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Updates for Display Processing Application
 * 
 * *****************  Version 122  *****************
 * User: Contractor V&v Date: 8/26/15    Time: 7:18p
 * Updated in $/software/control processor/code/application
 * SCR #1299 - P100 Implement the Deferred Virtual Sensor Requirements
 *
 * *****************  Version 121  *****************
 * User: John Omalley Date: 1/19/15    Time: 2:37p
 * Updated in $/software/control processor/code/application
 * SCR 1167 - Updates per Code Review
 *
 * *****************  Version 120  *****************
 * User: John Omalley Date: 12/11/14   Time: 1:13p
 * Updated in $/software/control processor/code/application
 * SCR 1167 - User Min length for strings incorrect
 *
 * *****************  Version 119  *****************
 * User: Contractor V&v Date: 11/17/14   Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR #1262 - LiveData CP to MS found existing bug, fixed
 *
 * *****************  Version 118  *****************
 * User: John Omalley Date: 11/14/14   Time: 9:45a
 * Updated in $/software/control processor/code/application
 * SCR 1267 - Made "showcfg" command RO
 *
 * *****************  Version 117  *****************
 * User: Contractor V&v Date: 10/22/14   Time: 6:26p
 * Updated in $/software/control processor/code/application
 * SCR #1271 - Initialization of ACTION list exceeds field size
 *
 * *****************  Version 116  *****************
 * User: John Omalley Date: 4/07/14    Time: 11:54a
 * Updated in $/software/control processor/code/application
 * SCR 1212 - Fixed Range check in User_ValidateMessage() function
 *
 * *****************  Version 115  *****************
 * User: John Omalley Date: 13-01-21   Time: 3:46p
 * Updated in $/software/control processor/code/application
 * SCR 1219 - Misc Creep User Table Updates
 *
 * *****************  Version 114  *****************
 * User: Contractor V&v Date: 1/04/13    Time: 3:13p
 * Updated in $/software/control processor/code/application
 * SCR #197 change User_SetBitArrayFromInteger to avoid using errno to
 * detect conversion could not be performed.
 *
 * *****************  Version 113  *****************
 * User: John Omalley Date: 12-12-26   Time: 10:37a
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 112  *****************
 * User: Contractor V&v Date: 12/20/12   Time: 5:36p
 * Updated in $/software/control processor/code/application
 * SCR #1107 CR  remove USER_TYPE_HEX8
 *
 * *****************  Version 111  *****************
 * User: John Omalley Date: 12-12-20   Time: 4:57p
 * Updated in $/software/control processor/code/application
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 110  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 4:31p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 109  *****************
 * User: Contractor V&v Date: 12/10/12   Time: 7:08p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Code Review
 *
 * *****************  Version 108  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:35p
 * Updated in $/software/control processor/code/application
 * SCR #1186 Followup: Reset errno prior to strtoul
 *
 * *****************  Version 107  *****************
 * User: Jeff Vahue   Date: 11/10/12   Time: 11:53a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - 128 Bit List parsing fix BB# 66
 *
 * *****************  Version 106  *****************
 * User: Jeff Vahue   Date: 11/10/12   Time: 11:17a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - 128 Bit parsing fix.
 *
 * *****************  Version 105  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:27p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 104  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:08p
 * Updated in $/software/control processor/code/application
 * SCR #1190 Creep Requirements
 *
 * *****************  Version 103  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 12:26p
 * Updated in $/software/control processor/code/application
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 102  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 5:50p
 * Updated in $/software/control processor/code/application
 * SCR# 1107: ACT_LIST conversion fix
 *
 * *****************  Version 101  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 3:45p
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - Fix Action List validation
 *
 * *****************  Version 100  *****************
 * User: Jeff Vahue   Date: 9/17/12    Time: 10:53a
 * Updated in $/software/control processor/code/application
 * SCR# 1107 - Add ACT_LIST, clean up msgs
 *
 * *****************  Version 99  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:45p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 User SensorArray GSE handling
 *
 * *****************  Version 98  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:21p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added HEX8 logic
 *
 * *****************  Version 97  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 6:41p
 * Updated in $/software/control processor/code/application
 * SCR#1107 BIT128 LIST Read Display show None Selected
 *
 * *****************  Version 96  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 *
 * *****************  Version 95  *****************
 * User: Contractor V&v Date: 8/22/12    Time: 5:27p
 * Updated in $/software/control processor/code/application
 * FAST 2 Issue #15 legacy conversion
 *
 * *****************  Version 94  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:20p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 *
 * *****************  Version 93  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Trigger processing
 *
 * *****************  Version 92  *****************
 * User: Contractor2  Date: 5/12/11    Time: 10:53a
 * Updated in $/software/control processor/code/application
 * SCR #914 Error: User - Param Change issue when reading FPGA register
 *
 *
 * *****************  Version 91  *****************
 * User: Contractor2  Date: 5/05/11    Time: 3:48p
 * Updated in $/software/control processor/code/application
 * SCR #1005 Enhancement: - User_CvtSetStr extra code
 *
 * *****************  Version 90  *****************
 * User: Contractor2  Date: 10/18/10   Time: 3:53p
 * Updated in $/software/control processor/code/application
 * SCR #952 Misc - User_AuthenticateModeRequest add comment to offset
 * wraparound
 *
 * *****************  Version 89  *****************
 * User: Contractor2  Date: 10/18/10   Time: 3:18p
 * Updated in $/software/control processor/code/application
 * SCR #951 Misc - User type Hex8 is not used and should be
 * removed/commented out
 *
 * *****************  Version 88  *****************
 * User: Contractor2  Date: 10/18/10   Time: 2:48p
 * Updated in $/software/control processor/code/application
 * SCR #951 Misc - User type Hex8 is not used and should be
 * removed/commented out
 *
 * *****************  Version 87  *****************
 * User: John Omalley Date: 10/15/10   Time: 8:56a
 * Updated in $/software/control processor/code/application
 * SCR 937 - Removed UserTbl.h
 *
 * *****************  Version 86  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 5:02p
 * Updated in $/software/control processor/code/application
 * SCR# 925 - oh and don't forget the NULL
 *
 * *****************  Version 85  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 4:15p
 * Updated in $/software/control processor/code/application
 * SCR# 925 - FATAL on showcfg error
 *
 * *****************  Version 84  *****************
 * User: Jim Mood     Date: 10/01/10   Time: 6:35p
 * Updated in $/software/control processor/code/application
 * SCR 818 Code Review Changes, dead code removal
 *
 * *****************  Version 83  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 4:10p
 * Updated in $/software/control processor/code/application
 * SCR 901 Code Coverage Issues
 *
 * *****************  Version 82  *****************
 * User: John Omalley Date: 9/10/10    Time: 9:59a
 * Updated in $/software/control processor/code/application
 * SCR 858 - Added FATAL to defaults
 *
 * *****************  Version 81  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 11:06a
 * Updated in $/software/control processor/code/application
 * SCR #806 Code Review Updates
 *
 * *****************  Version 80  *****************
 * User: Contractor V&v Date: 7/22/10    Time: 6:48p
 * Updated in $/software/control processor/code/application
 * SCR #643  Add showcfg to UartMgrUserTable and F7XUserTable
 *
 * *****************  Version 79  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:29p
 * Updated in $/software/control processor/code/application
 * SCR# 716 - fix INT32 access and unindent code in FastMgr.c
 *
 * *****************  Version 78  *****************
 * User: Jeff Vahue   Date: 7/08/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 685 - Make GSE cmds read-only access form the MS
 *
 * *****************  Version 77  *****************
 * User: Jeff Vahue   Date: 6/23/10    Time: 1:47p
 * Updated in $/software/control processor/code/application
 * SCR# 657 - fix HEX16 type conversion
 *
 * *****************  Version 76  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 6:27p
 * Updated in $/software/control processor/code/application
 * SCR 646 Micro-Server commands not getting responses fix
 *
 * *****************  Version 75  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:51p
 * Updated in $/software/control processor/code/application
 * SCR #615 Showcfg/Long msg enhancement
 *
 * *****************  Version 74  *****************
 * User: Contractor V&v Date: 6/02/10    Time: 7:09p
 * Updated in $/software/control processor/code/application
 * SCR #622 Misc - parameter change log error
 *
 * *****************  Version 73  *****************
 * User: Contractor V&v Date: 5/19/10    Time: 6:10p
 * Updated in $/software/control processor/code/application
 * SCR #560 ShowCfg does not return FAST> prompt
 *
 * *****************  Version 72  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:53p
 * Updated in $/software/control processor/code/application
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 71  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:34p
 * Updated in $/software/control processor/code/application
 * SCR #560 ShowCfg does not return prompt:Add CRC at end
 *
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:37p
 * Updated in $/software/control processor/code/application
 * SCR #560 ShowCfg does not return FAST> prompt SCR #453 Misc fix
 *
 * *****************  Version 69  *****************
 * User: Jeff Vahue   Date: 4/16/10    Time: 11:15a
 * Updated in $/software/control processor/code/application
 * SCR #550 - reimplement ValidateMsg to check each flags required
 * conditions exist.
 *
 * *****************  Version 68  *****************
 * User: Jeff Vahue   Date: 4/14/10    Time: 6:33p
 * Updated in $/software/control processor/code/application
 * SCR #547 - remove unused User variable types
 *
 * *****************  Version 67  *****************
 * User: Jeff Vahue   Date: 4/14/10    Time: 6:15p
 * Updated in $/software/control processor/code/application
 * SCR #547 - remove unused User variable types
 *
 * *****************  Version 66  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:07p
 * Updated in $/software/control processor/code/application
 * SCR# 314, SCR# 317, SCR# 70
 *
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:15p
 * Updated in $/software/control processor/code/application
 * SCR #512 Password cannot be visible/reverse-eng SCR #317 implement
 * strncpy
 *
 * *****************  Version 64  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 63  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/application
 * SCR #483 - Function Naming
 * SCR# 452 - Code Coverage Improvements
 *
 * *****************  Version 61  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 7:32p
 * Updated in $/software/control processor/code/application
 * SCR 248 bug fix for access
 *
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:39p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 59  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:35p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 58  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:56a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:56p
 * Updated in $/software/control processor/code/application
 * SCR# 455 remove LINT issues
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:52p
 * Updated in $/software/control processor/code/application
 * SCR# 163 - User_PutMsg is now reentrant
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 1:09p
 * Updated in $/software/control processor/code/application
 * SCR# 447 - octal numbers must be more than just 0.
 *
 * *****************  Version 54  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 12:54p
 * Updated in $/software/control processor/code/application
 * SCR# 447 - remove reporting number base on number conversion.
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 6:44p
 * Updated in $/software/control processor/code/application
 * SCR# 447 - if setting a value = 0 don't do it as base 8
 *
 * *****************  Version 52  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 6:16p
 * Updated in $/software/control processor/code/application
 * SCR# 350 - strtok_r cleanup
 *
 * *****************  Version 51  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 2:49p
 * Updated in $/software/control processor/code/application
 * SCR# 447 - allow for octal value conversion, and alerting the user to
 * the interpreted number base.
 *
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 1:38p
 * Updated in $/software/control processor/code/application
 * SCR# 350 - use reentrant strtok (strtok_r).  Fix typos in user.h
 *
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:03a
 * Updated in $/software/control processor/code/application
 * SCR# 412 - Convert if/then/else to ASSERT
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/application
 * SCR 314
 *
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:00p
 * Updated in $/software/control processor/code/application
 * SCR# 397
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:57p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 45  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:20p
 * Updated in $/software/control processor/code/application
 * SCR 371
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR #326
 *
 * *****************  Version 43  *****************
 * User: Contractor V&v Date: 12/18/09   Time: 6:53p
 * Updated in $/software/control processor/code/application
 * SCR 31 Test for NULL function pointers.
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 11:58a
 * Updated in $/software/control processor/code/application
 * SCR# 42 - update check on ShowCfg command
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:39p
 * Updated in $/software/control processor/code/application
 * SCR# 363 - typos in USER_TYPE_INT32 handling
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:40p
 * Updated in $/software/control processor/code/application
 * SCR 42
 *
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:21p
 * Updated in $/software/control processor/code/application
 * SCR# 364 - User_AddRootCmd and redundant checks on number conversion
 * range.
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 2:17p
 * Updated in $/software/control processor/code/application
 * SCR# 363
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/application
 * SCR# 360
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:14p
 * Updated in $/software/control processor/code/application
 * Fix for SCR 37
 * Correction for typos
 *
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 11/05/09   Time: 3:04p
 * Updated in $/software/control processor/code/application
 * SCR #320
 *
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 10/28/09   Time: 11:21a
 * Updated in $/software/control processor/code/application
 * Workaround for SCR #317 Pre-null terminated the GetStr to prevent
 * overrunning the string buffer
 *
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 10/14/09   Time: 5:39p
 * Updated in $/software/control processor/code/application
 * SCR# 302.  Fixed some errors with the new limit checking code in
 * SetMinMax()
 *
 * *****************  Version 32  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/application
 * SCR #283
 *
 * *****************  Version 31  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 9:22a
 * Updated in $/software/control processor/code/application
 * SCR #84, #124, #162, #178
 *
 * *****************  Version 30  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 4:24p
 * Updated in $/software/control processor/code/application
 *
 * *****************  Version 29  *****************
 * User: Jim Mood     Date: 8/18/09    Time: 1:35p
 * Updated in $/software/control processor/code/application
 * SCR #223  "Aggregate" writes
 *
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 11:17a
 * Updated in $/software/control processor/code/application
 * SCR #168 Updated ordering of conditional in User_TraverseCmdTables() to
 * avoid NULL being passed into strcasecmp().
 *
 * *****************  Version 27  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 8:57a
 * Updated in $/control processor/code/application
 * Added processing of User commands from the Micro Server MSSIM
 * application.  Echo commands to the GSE port in NORMAL output mode
 *
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 3/20/09    Time: 9:06a
 * Updated in $/control processor/code/application
 * Added the function to handle User commands from the micro-server port
 *
 * *****************  Version 25  *****************
 * User: Jim Mood     Date: 1/06/09    Time: 4:56p
 * Updated in $/control processor/code/application
 * SCR# 127 Increased the maximum index value from 99 to 9999
 *
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 10:47a
 * Updated in $/control processor/code/application
 * SCR #87 Function Prototype
 *
 *
 ***************************************************************************/
