#ifndef USER_H
#define USER_H

/******************************************************************************
            Copyright (C) 2007-2010 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         USER.h     
    
    Description:
    
    VERSION
    $Revision: 41 $  $Date: 12-09-11 2:21p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "Utility.h"
//#include "GSE.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#define USER_MESSAGE_BUF_SIZE 16384
#define USER_SINGLE_MSG_MAX_SIZE 8192

//Number of different commands at the root level (i.e. the string before the
//first "." delimiter)
#define USER_MAX_ROOT_CMDS 40

#define USER_MSG_READ_ONLY          "Error: Read Only Command"
#define USER_MSG_WRITE_ONLY         "Error: Write Only Command"
#define USER_MSG_INVALID_CMD        "Error: Invalid Command"
#define USER_MSG_INVALID_CMD_FUNC   "Error: Command Failed"
#define USER_MSG_CMD_CONVERSION_ERR "Error:"
#define USER_MSG_BAD_CHECKSUM       "Error: Bad checksum"
#define USER_MSG_INDEX_OUT_OF_RANGE "Index range"
#define USER_MSG_NOT_INDEXABLE      "Command not indexable"
#define USER_MSG_RSP_CONVERSION_ERR "Error: Response conversion"
#define USER_MSG_WRITE_NOT_COMPLETE "Error: Cannot set incomplete command"
#define USER_MSG_GSE_ONLY           "Error: Command can only be executed from GSE port"
#define USER_MSG_ACCESS_LEVEL       "Error: You do not have permission to modify this value"
#define USER_MSG_FACTORY_MODE       NEW_LINE "Factory Mode Entered"
#define USER_MSG_PRIVILEGED         NEW_LINE "Privileged Mode Entered"
#define USER_MSG_VFY_FORMAT         NEW_LINE "Verify number format."
#define USER_MSG_RESP_OVERFLOW      "Error: Response msg too long."


#define USER_INDEX_NOT_DEFINED  -1
#define USER_MAX_MSG_STR_LEN 20 //Max length of each message part, i.e. the
                                //length of the strings between the "."
                                //delimiters
#define NO_NEXT_TABLE NULL
#define NO_LIMIT 0,0
#define NO_HANDLER_DATA USER_TYPE_NONE,USER_RW,NULL,-1,-1,0,0,NULL

// Define for performing dumping cfg info to log
#define DISPLAY_CFG "SHOWCFG"  

#define MAX_RECURSIVE_CALLS 6

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum{
  USER_MSG_SOURCE_GSE = 0,
  USER_MSG_SOURCE_MS
}USER_MSG_SOURCES;

/* The message handler function is passed a parameter from the command table
   This command may either be a void pointer or a 32-bit integer value.  A
   union type is used to ensure enough space is reserved for both the pointer
   and 32-bit integer.
*/
typedef union {
  void* Ptr;  
  UINT32 Int;
}USER_MSG_PARAM;
  

/*USER_DATA_TYPE
  Defines what type of data a message is expected to use for "set" and "get"
  commands, and this defines how the binary value is converted into a string
  for response back to the message originator.
  Some messages do not have data to set or get, but instead execute
  some "action."  These message types usually have a verb in their name, such
  as the message "qar.reconfigure."

  NOTE: The four commented out types are provided, but currently not used.  If  
  future code makes use of nay of these types grep the code to reenable the  
  functionality associated with these types.
*/
typedef enum{
  USER_TYPE_NONE = 0,
  USER_TYPE_UINT8,
  USER_TYPE_UINT16,
  USER_TYPE_UINT32,  
  //USER_TYPE_INT8,
  //USER_TYPE_INT16,
  USER_TYPE_INT32,  
  USER_TYPE_HEX8,
  USER_TYPE_HEX16,
  USER_TYPE_HEX32,
  //USER_TYPE_ENUM8,
  //USER_TYPE_ENUM16,
  USER_TYPE_ENUM,
  USER_TYPE_STR,
  USER_TYPE_SNS_LIST,  // a list of sensor ids
  USER_TYPE_128_LIST,  // defines a string containing a BITARRAY128 entry list  
  USER_TYPE_FLOAT,
  USER_TYPE_BOOLEAN,
  USER_TYPE_YESNO,
  USER_TYPE_ONOFF,
  USER_TYPE_ACTION,
  USER_TYPE_END,
}USER_DATA_TYPE;

/*USER_DATA_ACCESS
  Defines read/write attributes of the parameter this messages accesses
  NOTE: Changed to #define from enum.  This allows bit masking the
        flags together without getting a type warning from the compiler
  Bits 0:1 Access
  [READ][WRITE]
  Read: Allow/Disallow reading of a value
  Write: Allow/Disallow writing of a value

  
  Bits 2:4 Write Restrictions
  [WRITE ONLY FACTORY][WRITE ONLY PRIVILEGED][WRITE ONLY GSE]
  Write Only Factory:  Allow write to this value only when
                       "Factory Access Mode" is entered.
  Write Only Privileged:Allow write to this value only when "Privileged Access
                       Mode" or "Factory Access Mode" is entered.
  Write Only GSE:      Allow write access only from the physical GSE port (not
                       from the Micro Server)
  Bit 5 ChangeLogging  Don't log changes to this parameter.

*/
#define  USER_RO      0x1
#define  USER_WO      0x2
#define  USER_RW      0x3
#define  USER_PRIV    0x4
#define  USER_FACT    0x8
#define  USER_GSE     0x10
#define  USER_NO_LOG  0x20


/*USER_DATA_ACCESS
  Defines result codes for the message handler
*/
typedef enum {
  USER_RESULT_OK = 0,
  USER_RESULT_ERROR
}USER_HANDLER_RESULT;


/*USER_ENUM_TBL
 This type is used for messages that need convert strings into an enumerated
 type.  To implement an enumeration table, define an array of type
 USER_ENUM_TBL, with string/enum value pairs.  When a get message is received,
 string will be displayed instead of the enumeration number, and when setting
 the parameter, the string is used instead of the number.  To implement an enum
 table, include a pointer to the table in the message's
 USER_MSG_HANDLER_TABLE_DATA struct and set the MsgType to USER_TYPE_ENUM.
*/
typedef struct {
  INT8* Str;
  UINT32 Num;
}USER_ENUM_TBL;


/*USER_RANGE
  This is used to hold the upper and lower range for values written to by
  user manager.  The union allows signed, unsigned, string lengths and float
  limits to be defined
*/
typedef union {
  INT32   Sint;
  UINT32  Uint;
  FLOAT32 Float;
}USER_RANGE;


/*USER_MSG_TBL
  This structure is used to make up tables for command message definitions.
  Each entry in a table represents a token that makes up the entire command
  message part of a message.  Tokens of the command messages are the strings
  between the "." delimiters.  The last or leaf token defines what handler
  function is called to handle setting or getting the value referenced by the
  command message

  MsgStr:     The token string for this section of the command message.
  MsgTbl:     Points to the next USER_MSG table to decode the next part
              of the message, OR is defined as null if this is a leaf message
  MsgHandler: Defined on the leaf message, this is the function that is called
              to set or get the value that is represented by this command
              message
  MsgType:    Data type of the value
  MsgAccess:  Read/Write definition and access restrictions
  MsgParam:   A value from the table that is passed to the MsgHandler function
              This can store a pointer to a value or an integer
  MsgIndex:   Range for indexable command messages.  The range of the index
              contained in the [] brackets
  MsgRange:   The Range allowed for the value, this supports signed/unsigned
              integers, string length and floating point.
  MsgEnumTbl: Pointer to a table to decode ENUM type values.  
*/
typedef struct MsgTbl {
  INT8*                   MsgStr; 
  struct                  MsgTbl *pNext;  
  USER_HANDLER_RESULT     (*MsgHandler)(USER_DATA_TYPE DataType,
                                        USER_MSG_PARAM Param,
                                        UINT32 Index,
                                        const void *SetPtr,
                                        void **GetPtr);    
  USER_DATA_TYPE          MsgType;
  UINT32                  MsgAccess;
  USER_MSG_PARAM          MsgParam;
  INT32                   MsgIndexMin;
  INT32                   MsgIndexMax;
  USER_RANGE              MsgRangeMin;
  USER_RANGE              MsgRangeMax;
  USER_ENUM_TBL*          MsgEnumTbl;
} USER_MSG_TBL;

 
/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( USER_BODY )
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
EXPORT void User_Init(void);
EXPORT void User_AddRootCmd(USER_MSG_TBL* NewRootPtr);
EXPORT USER_HANDLER_RESULT User_DisplayConfigTree(CHAR* BranchName, USER_MSG_TBL* MsgTbl,
                                          INT16 SensorIdx, UINT16 RecursionCount,
                                          CHAR* UserTableName);

EXPORT USER_HANDLER_RESULT User_GenericAccessor(USER_DATA_TYPE DataType, USER_MSG_PARAM Param,
                                           UINT32 Index, const void *SetPtr,
                                           void **GetPtr);

EXPORT BOOLEAN User_OutputMsgString( const CHAR* string, BOOLEAN finalize);
EXPORT BOOLEAN User_CvtGetStr(USER_DATA_TYPE Type, INT8* GetStr, UINT32 Len,
                                void* GetPtr, USER_ENUM_TBL* MsgEnumTbl);

#endif // USER_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: User.h $
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:21p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added HEX8 logic
 * 
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
 * 
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:20p
 * Updated in $/software/control processor/code/application
 * SCR #1107 FAST 2 BITARRAY128 input as integer list
 * 
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:42p
 * Updated in $/software/control processor/code/application
 * Increased USER_MAX_ROOT_CMDS
 * 
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Trigger processing
 * 
 * *****************  Version 36  *****************
 * User: Contractor2  Date: 10/18/10   Time: 2:48p
 * Updated in $/software/control processor/code/application
 * SCR #951 Misc - User type Hex8 is not used and should be
 * removed/commented out
 * 
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 10/01/10   Time: 6:35p
 * Updated in $/software/control processor/code/application
 * SCR 818 Code Review Changes, dead code removal
 * 
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 7/22/10    Time: 6:49p
 * Updated in $/software/control processor/code/application
 * SCR #643  Add showcfg to UartMgrUserTable and F7XUserTable
 * 
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:52p
 * Updated in $/software/control processor/code/application
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:37p
 * Updated in $/software/control processor/code/application
 * SCR #559 Check clock diff CP-MS on startup
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 4/14/10    Time: 6:15p
 * Updated in $/software/control processor/code/application
 * SCR #547 - remove unused User variable types
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:37p
 * Updated in $/software/control processor/code/application
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 * 
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:39p
 * Updated in $/software/control processor/code/application
 * SCR #248 Parameter Log Change
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:35p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 26  *****************
 * User: Contractor2  Date: 3/02/10    Time: 11:56a
 * Updated in $/software/control processor/code/application
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 1:38p
 * Updated in $/software/control processor/code/application
 * SCR# 350 - use reentrant strtok (strtok_r).  Fix typos in user.h
 * 
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:57p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/application
 * SCR# 378
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:40p
 * Updated in $/software/control processor/code/application
 * SCR 42
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:21p
 * Updated in $/software/control processor/code/application
 * SCR# 364 - User_AddRootCmd returns void.
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 2:17p
 * Updated in $/software/control processor/code/application
 * SCR# 363
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 10/28/09   Time: 11:28a
 * Updated in $/software/control processor/code/application
 * Increased buffer size to 8k, accomidates messages that occupy an entire
 * MS<->CP message size.
 * 
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 9:22a
 * Updated in $/software/control processor/code/application
 * SCR #84, #124, #162, #178
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 4:24p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 1/06/09    Time: 4:56p
 * Updated in $/control processor/code/application
 * SCR# 127 Increased the maximum index value from 99 to 9999
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:25a
 * Updated in $/control processor/code/application
 * Added prototype for User_Init (SCR #87)
 * 
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 11:48a
 * Updated in $/control processor/code/application
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 10:53a
 * Updated in $/control processor/code/application
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
