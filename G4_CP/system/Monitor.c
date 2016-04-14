#define MONITOR_BODY
/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

  ECCN:        9D991

  File:        Monitor.c

  Description: GSE Communications port processing

  VERSION
      $Revision: 89 $  $Date: 2/05/15 10:26a $
******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "GSE.h"
#include "ClockMgr.h"
#include "Monitor.h"
#include "Version.h"
#include "alt_Time.h"
#include "utility.h"
#include "NVMgr.h"
#include "Assert.h"
#include "DIO.h"
#include "WatchDog.h"
#include "CfgManager.h"

#ifdef WIN32
#include "SPIManager.h"
extern void SPIMgr_UpdateEEProm   (SPIMGR_DEV_ID Dev); // Declared in SpiManager.c
extern void SPIMgr_UpdateRTCNVRam (SPIMGR_DEV_ID Dev); // Declared in SpiManager.c
#endif

#ifndef WIN32
#include "indsecinfo.h"
#endif

//extern NV_FILENAME* NV_GetFileName(NV_FILE_ID fileNum);
//extern INT32        NV_GetFileCRC (NV_FILE_ID fileNum);


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define TOK0             0
#define TOK1             1
#define TOK2             2
#define TOK3             3
#define TOK4             4
#define TOK5             5
#define TOK6             6
#define TOK7             7
#define TOK8             8
#define MAX_TOKENS      10      // Max of 10 tokens per Monitor command

#define ARRAY_32        32
#define ARRAY_64        64
#define ARRAY_80        80
#define ARRAY_128       128

#define BASE_16         16

#define RAM_SIZE    32000000
#define FLASH_SIZE  32000000

#define RTC_VIRTUAL_BASE_ADDR     0x20001000
#define EEPROM1_VIRTUAL_BASE_ADDR 0x20100000
#define EEPROM2_VIRTUAL_BASE_ADDR 0x20200000

#ifdef WIN32
    #define ADDR_WIN32( var, addr) var = (void*)&sysMemoryCmds
#else
    #define ADDR_WIN32( var, addr) var = addr;
#endif

#ifndef WIN32
// Addresses provided by linker
extern UINT32  __SP_INIT;       // stack top
extern UINT32  __SP_END;        // stack bottom

// addresses provided by linker
extern UINT32  __CRC_START;
extern UINT32  __CRC_END;
#else
UINT32  __SP_END[1024];
#define __SP_INIT __SP_END[1024]

extern UINT32  __CRC_START;  // declared in CBITManager.c
extern UINT32  __CRC_END;    // declared in CBITManager.c
#endif

#define ENABLE_SYS_DUMP_MEM_CMD

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

typedef struct
{
   void*      addr;   // memory section address
   UINT32     size;   // memory section size
} MEM_INFO;

typedef struct
{
   const INT8*  secName;    // memory section name
   MEM_INFO*    secInfo;    // memory section info
} MEM_SEC_INFO;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

//Pointer to an optional function that can handle commands not handled
//in Monitor.c
static MSG_HANDLER_CALLBACK MessageHandlerCallback;

//Previous command received for easy command repeat back
INT8  PrevCmd[GSE_GET_LINE_BUFFER_SIZE];
UINT32 PrevCmdLen;
RESULT PrevResult;

// Memory Map Variables for memory usage reporting
MEM_INFO  VectorTab;      // interrupt vectors
MEM_INFO  CodeSec;        // program flash
MEM_INFO  SecInfo;        // compile section info block
MEM_INFO  RomData;        // initialized ram init data
MEM_INFO  DataRam;        // uninitialized ram
MEM_INFO  IDataRam;       // initialized ram
MEM_INFO  Stack;          // stack

//      These strings represent enumerated values in other modules.  The string
//      definition should probably not be done here, in case the enumeration
//      order changes in the other module.
const INT8* YES_NO[] =
{
    "No",
    "Yes"
};

const INT8* TASK_TYPES[] =
{
    "DT",
    "RMT"
};

const INT8* TASK_STATES[] =
{
    "Idle",
    "Scheduled",
    "Active"
};

MEM_SEC_INFO memInfoTable[] =
{
    { ".bss",       &DataRam },
    { ".data",      &IDataRam },
    { ".stack",     &Stack },
    { ".vector",    &VectorTab },
    { ".text",      &CodeSec },
    { ".secinfo",   &SecInfo },
    { ".ROM.data",  &RomData }
};
INT16 memInfoTableLen = sizeof(memInfoTable)/sizeof(MEM_SEC_INFO);

// stack fill pattern for stack usage calculation
const UINT32 StackFill = 0xdeadbeef;

#ifdef WIN32
UINT32 sysMemoryCmds;
#endif

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void MonitorTask            (void* pPBlock);
static void MonitorSysCmds         (CHAR* Cmd);
static void MonitorSysGet          (CHAR* Token[]);
static void MonitorSysSet          (CHAR* Token[]);

static void MonitorGetMem          (CHAR* valueType, CHAR* address);
static void MonitorGetMemUsage     (void);
static void MonitorGetRtc          (void);
static void MonitorGetStackUsage   (void);
static void MonitorGetTask         (TASK_INDEX TaskId);
static void MonitorGetTaskList     (void);
static void MonitorGetTaskTiming   (void);
static void MonitorGetTm           (void);

#ifdef ENABLE_SYS_DUMP_MEM_CMD
static void MonitorDumpMem         (CHAR* Token[]);
#endif

static void MonitorSetMem          (CHAR* Token3, CHAR* Token4, CHAR* Token5, CHAR* Token6);
static void MonitorSetRtc          (CHAR* Tokens[]);

static void MonitorSetTaskMode     (CHAR* Token[]);
static void MonitorSetTaskEnable   (CHAR* Token[]);
static void MonitorSetTaskPulse    (CHAR* Token3, CHAR* Token4);

static void MonitorPrintCRCs       (void);
static void MonitorInvalidCommand  (char* msg);
static void* MonitorGetNvRAM  ( CHAR dataType, void* pAddress);
static BOOLEAN MonitorSetNvRAM( CHAR* address, UINT8 size, UINT32 value);
static BOOLEAN MonitorMemoryWrite( CHAR* address, UINT8 size, UINT32 value);

static void MonitorSysPerf(CHAR* Token[]);
static void MonitorSysPerfReset(CHAR* token3);
static void MonitorSysPerfStats(void);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    MonitorInitialize
 *
 * Description: Initialize task and data/control structures needed for
 *              operation of the Monitor.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void MonitorInitialize (void)
{
#ifndef WIN32
    secinfo_ptr pSec;       // pointer to linker supplied memory map list
#endif
    MEM_SEC_INFO* pMemInfo; // pointer to memory map decode table
    INT16 Index;

    TCB     TaskInfo;

    // Create the Monitor Task
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name,sizeof(TaskInfo.Name),"Monitor Task",_TRUNCATE);
    TaskInfo.TaskID         = Monitor_Task;
    TaskInfo.Function       = MonitorTask;
    TaskInfo.Priority       = taskInfo[Monitor_Task].priority;
    TaskInfo.Type           = taskInfo[Monitor_Task].taskType;
    TaskInfo.modes          = taskInfo[Monitor_Task].modes;
    TaskInfo.MIFrames       = taskInfo[Monitor_Task].MIFframes;
    TaskInfo.Rmt.InitialMif = taskInfo[Monitor_Task].InitialMif;
    TaskInfo.Rmt.MifRate    = taskInfo[Monitor_Task].MIFrate;
    TaskInfo.Enabled        = TRUE;
    TaskInfo.Locked         = FALSE;
    TaskInfo.pParamBlock    = NULL;

    TmTaskCreate (&TaskInfo);

    //Clear out previous command variables
    PrevCmd[0] = '\0';
    PrevCmdLen = 0;
    PrevResult = DRV_OK;

    //Init the non-sys. message handler callback to null.
    MessageHandlerCallback = NULL;

    // Initialize memory map section info table
    pMemInfo = memInfoTable;
    for (Index = 0; Index < memInfoTableLen; ++Index, ++pMemInfo)
    {
        memset(pMemInfo->secInfo, 0, sizeof(MEM_INFO));
    }
#ifndef WIN32
    // search table for matching section names
    for (pSec = &__secinfo; pSec != NULL; pSec = pSec->next)
    {
        pMemInfo = memInfoTable;
        for (Index = 0; Index < memInfoTableLen; ++Index, ++pMemInfo)
        {
            if (strcmp(pSec->name, pMemInfo->secName) == 0)
            {
                pMemInfo->secInfo->addr = pSec->addr;
                pMemInfo->secInfo->size = pSec->size;
                break;
            }
        }
    }
#endif
}
/*****************************************************************************
 * Function:    MonitorSetMsgCallback
 *
 * Description: Sets the message callback function.  Monitor only handles
 *              messages that start with sys.<something>.  If a message is
 *              received that starts with another string, it is passed to
 *              the callback routine.  If no call back is defined, the
 *              message is dropped and an error message is sent up to the
 *              GSE host.
 *
 * Parameters:  [in] MsgHandler: Pointer to a message handler function that
 *                   is of the form MSG_HANDLER_CALLBACK
 *
 * Returns:     none
 *
 * Notes:       None.
 *
 ****************************************************************************/
void MonitorSetMsgCallback(MSG_HANDLER_CALLBACK MsgHandler)
{
  MessageHandlerCallback = MsgHandler;
}


/*****************************************************************************
 * Function:    MonitorGetTaskId
 *
 * Description: Get the Task Id of the Monitor Task
 *
 * Parameters:  void
 *
 * Returns:     Task Id of Monitor Task
 *
 * Notes:       None.
 *
 ****************************************************************************/
TASK_INDEX MonitorGetTaskId (void)
{
    return (Monitor_Task);
}

/*****************************************************************************
 * Function:    MonitorPrintVersion
 *
 * Description: Prints the DTU Software ID message.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       SYS.VER
 *
 ****************************************************************************/
void MonitorPrintVersion (void)
{
    CHAR Str[ARRAY_64];

    GSE_PutLine ("\r\n");
    GSE_PutLine (PRODUCT_NAME);
    GSE_PutLine (" v");
    GSE_PutLine (PRODUCT_VERSION);
    GSE_PutLine (" ");
    GSE_PutLine (DATETIME);
    //GSE_PutLine ("\r\n");

    sprintf(Str," (Program CRC=0x%08X, %s CRC=0x%04X)",
                              __CRC_END,
                              NV_GetFileName(NV_CFG_MGR),
                              NV_GetFileCRC (NV_CFG_MGR) );
    GSE_PutLine (Str);
    GSE_PutLine ("\r\n");
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 * Function:    MonitorTask
 *
 * Description: RMT Task which process complete ASCII commands received from
 *              the GSE port.
 *
 * Parameters:  pPBlock - Pointer to task's parameter block
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static void MonitorTask (void* pPBlock)
{
  CHAR   Cmd[GSE_GET_LINE_BUFFER_SIZE];
  CHAR   CmdLower[GSE_GET_LINE_BUFFER_SIZE];
  UINT32 CmdLen;
  RESULT result;

  // Read the command string from the Rx Buffer
  memset (Cmd, 0, GSE_GET_LINE_BUFFER_SIZE);

  result = GSE_GetLine(Cmd);

  if (result == DRV_OK)
  {
    // If previous read was LINE_TOO_LONG, ignore this read and display error
    if  (SYS_GSE_GET_LINE_TOO_LONG == PrevResult)
    {
      GSE_PutLine ("\r\n*** Error: Invalid Command\r\n");
      GSE_PutLine(GSE_PROMPT);
    }
    else
    if( Cmd[0] != '\0')
    {
      CmdLen = strlen(Cmd);
      // If "repeat last command"
      if (Cmd[0] == '<')
      {
        strncpy_safe (Cmd, PrevCmdLen, PrevCmd, _TRUNCATE);
        CmdLen = PrevCmdLen;
      }
      else
      {
        strncpy_safe (PrevCmd, CmdLen, Cmd, _TRUNCATE);
        PrevCmdLen = CmdLen;
      }

      // Toggle the display of live data stream.
      if ( Cmd[0] == ESC_CHAR )
      {
        GSE_ToggleDisplayLiveStream();
      }
      else
      {
        // Trap empty strings consisting of only <LF> or <CR><LF>
        if ( Cmd[0] == '\n' || Cmd[0] == '\r' || CmdLen == 0)
        {
          GSE_PutLine(GSE_PROMPT);
        }
        else
        {
          strncpy_safe( CmdLower, GSE_GET_LINE_BUFFER_SIZE, Cmd, _TRUNCATE);
          Slower(CmdLower);
          if (strncmp( CmdLower, "sys", 3) == 0)
          {
            MonitorSysCmds( CmdLower);
          }
          else
          {
            ASSERT( MessageHandlerCallback != NULL);

            // Application command. Pass down to the Application
            MessageHandlerCallback(Cmd);  // Note: Func always ret TRUE
          }
        }
      }
    }
  }

  PrevResult = result;    // save current read status
}

/*****************************************************************************
* Function:    MonitorSysCmds
*
* Description: Handle all System Commands.
*
* Parameters:  CHAR* Cmd
*
* Returns:     void
*
* Notes:       None
*
****************************************************************************/
static void MonitorSysCmds( CHAR* Cmd)
{
    CHAR*  pChar;
    CHAR*  pChar1;
    CHAR*  Token[MAX_TOKENS];
    const CHAR Delimiters[] = " \t\r\n()[]{}<>,.=/:$";
    CHAR   NullString[] = "";
    UINT8  i;

    memset (Token, 0, sizeof(Token));

    // system cmds are all defined in lower case
    Slower( Cmd);
    StripString(Cmd);

    pChar = Cmd;
    pChar1 = Cmd;
    *(pChar + strlen(Cmd)) = '.';

    for (i = 0; i< MAX_TOKENS; ++i)
    {
        pChar = strpbrk (pChar, Delimiters);
        if (pChar == NULL)
        {
            break;
        }

        // ignore checksums exit the loop
        if ( *pChar == '$')
        {
            *pChar = '\0';
            Token[i] = pChar1;
            ++i;
            break;
        }
        else
        {
            *pChar = '\0';
            Token[i] = pChar1;
            ++pChar;
            pChar1 = pChar;
        }
    }

    // fill in the rest of the tokens with an empty string
    for ( ; i < MAX_TOKENS; ++i)
    {
        Token[i] = NullString;
    }

    // sys.set COMMAND FAMILY -------------------------------------------
    if (strcmp (Token[TOK1], "set") == 0)
    {
        MonitorSysSet( Token);
    }
    // sys.get COMMAND FAMILY -------------------------------------------
    else if (strcmp (Token[TOK1], "get") == 0)
    {
      MonitorSysGet( Token);
    }
    // sys.get COMMAND FAMILY -------------------------------------------
    else if (strcmp (Token[TOK1], "memory") == 0)
    {
      MonitorGetMemUsage();
    }
    else if (strcmp (Token[TOK1], "stack") == 0)
    {
      MonitorGetStackUsage();
    }
    // sys.ver ----------------------------------------------------------
    else if (strcmp (Token[TOK1], "ver") == 0)
    {
        MonitorPrintVersion();
    }
    // sys.crc ----------------------------------------------------------
    else if (strcmp (Token[TOK1], "crc") == 0)
    {
        MonitorPrintCRCs();
    }
    // sys.reboot -------------------------------------------------------
    else if (strcmp (Token[TOK1], "reboot") == 0)
    {
      WatchdogReboot(TRUE);
    }
    // sys.perf COMMAND FAMILY -------------------------------------------
    else if ( strcmp (Token[TOK1], "perf") == 0 )
    {
      MonitorSysPerf(Token);
    }

#ifdef ENABLE_SYS_DUMP_MEM_CMD
    /*vcast_dont_instrument_start*/
    else if (strcmp (Token[1], "dumpmem") == 0)
    {
        MonitorDumpMem(Token);
    }
    /*vcast_dont_instrument_end*/
#endif

    // UNKNOWN MONITOR COMMAND -------------------------------------------
    else
    {
        MonitorInvalidCommand("Unknown Monitor Command!");
    }

    GSE_PutLine( GSE_PROMPT);
}

/*****************************************************************************
* Function:    MonitorSysGet
*
* Description: Handle all System Get Commands.
*
* Parameters:  CHAR * Token[]
*
* Returns:     void
*
* Notes:       None
*
****************************************************************************/
static void MonitorSysGet(CHAR*  Token[])
{
    // sys.get.tm
    if (strcmp (Token[TOK2], "tm") == 0)
    {
        MonitorGetTm ();
    }

    // sys.get.task [blank | timing | #TASK_NUMBER]
    else if (strcmp (Token[TOK2], "task") == 0)
    {
        if (strcmp (Token[TOK3], "\0") == 0)
        {
            MonitorGetTaskList();
        }
        else if (strcmp (Token[TOK3], "timing") == 0)
        {
            MonitorGetTaskTiming();
        }
        else
        {
            // validate the parameter provided is a number
            INT16 x = atoi( Token[TOK3]);
            CHAR buf[ARRAY_32];
            sprintf(buf, "%d", x);
            if ( strcmp(buf, Token[TOK3])== 0)
            {
              MonitorGetTask( (TASK_INDEX)x );
            }
            else
            {
              MonitorInvalidCommand("Invalid Task Id");
            }
        }
    }

    // sys.get.rtc
    else if (strcmp (Token[TOK2], "rtc") == 0)
    {
        MonitorGetRtc();
    }
    // sys.get.mem.[b | w | l | f] #HEX_ADDRESS
    else if (strcmp (Token[TOK2], "mem") == 0)
    {
        MonitorGetMem (Token[TOK3], Token[TOK4]);
    }
    // UNKNOWN GET COMMAND
    else
    {
        MonitorInvalidCommand("Unknown Monitor Get Command");
    }
}

/*****************************************************************************
* Function:    MonitorSysSet
*
* Description: Handle all System Get Commands.
*
* Parameters:  CHAR* Token[]
*
* Returns:     void
*
* Notes:       None
*
****************************************************************************/
static void MonitorSysSet(CHAR* Token[])
{
    // sys.set.rtc #MM/DD/YY HH:MM:SS
    if (strcmp (Token[TOK2], "rtc") == 0)
    {
        MonitorSetRtc(&Token[TOK0]);
    }

    // sys.set.mem.[b | w | l | f] #HEX_ADDRESS
    else if (strcmp (Token[TOK2], "mem") == 0)
    {
        MonitorSetMem (Token[TOK3], Token[TOK4], Token[TOK5], Token[TOK6]);
    }

    // sys.set.pulse.#TASKID [-1,0,1,2,3] - pulse a specific task while active
    // sys.set.pulse on|off               - pulse MIF0 run on LSS0
    else if (strcmp (Token[TOK2], "pulse") == 0)
    {
        MonitorSetTaskPulse (Token[TOK3], Token[TOK4]);
    }

    // sys.set.taskmode modeId
    else if ( strcmp (Token[TOK2], "taskmode") == 0 )
    {
      MonitorSetTaskMode(&Token[TOK0]);
    }

    // sys.set.task.[#].enable
    else if ( strcmp (Token[TOK2], "task") == 0 && strcmp (Token[TOK4], "enable") == 0 )
    {
      MonitorSetTaskEnable (&Token[TOK0]);
    }
    // UNKNOWN SET COMMAND
    else
    {
        MonitorInvalidCommand("Unknown Monitor Set Command");
    }
}


/*****************************************************************************
* Function:    MonitorPrintCRCs
*
* Description: Prints the DTU Software ID message.
*
* Parameters:  void
*
* Returns:     void
*
* Notes:       SYS.CRC
*
****************************************************************************/
static void MonitorPrintCRCs(void)
{
  CHAR Str[ARRAY_64];
  UINT32 i;

  // Display the CRC for all files-in-use.
  // Files-in-use will have a non-zero CRC.
  GSE_PutLine ("\r\n");
  for( i = 0; i < NV_MAX_FILE; ++i )
  {
    if ( NV_GetFileCRC((NV_FILE_ID)i) != 0x0000 )
    {
      sprintf( Str, " File # %02d %-32s\tCRC=%04X\r\n", i,
                                NV_GetFileName((NV_FILE_ID)i),
                                NV_GetFileCRC ((NV_FILE_ID)i) );
      GSE_PutLine (Str);
    }
  }
}

/*****************************************************************************
* Function:    MonitorGetMem
*
* Description: Get the value at the specified address.
*
* Parameters:  CHAR* valueType
*              CHAR* address
*
* Returns:     void
*
* Notes:       SYS.GET.MEM.[b | W | L |F].address
*
****************************************************************************/
static void MonitorGetMem(CHAR* valueType, CHAR* address)
{
  INT8    Value8;
  UINT16  Value16;
  UINT32  Value32;
  FLOAT32 ValueFloat;
  void*   pAddress;
  void*   pShowAddr;
  CHAR    Str[ARRAY_64];

  pAddress = (void*)(strtoul(address, NULL, BASE_16));
  pShowAddr = pAddress;

  // If memory location is a 'virtual' address
  // representing data stored in EEPROM/RTC RAM,
  // call function to do SPI calls.
  if( (UINT32)pAddress >= RTC_VIRTUAL_BASE_ADDR &&
      (UINT32)pAddress <= EEPROM2_VIRTUAL_BASE_ADDR + EEPROM_SIZE )
  {
     pAddress = MonitorGetNvRAM(*valueType, pAddress);
  }
  else
  {
  #ifdef WIN32
/*vcast_dont_instrument_start*/
    pAddress = (void*)&sysMemoryCmds;
/*vcast_dont_instrument_end*/
  #endif
  }

  if (strcmp (valueType, "b") == 0)
  {
    Value8 = *(UINT8*)pAddress;
    GSE_PutLine("\r\n");
    sprintf(Str,"Address 0x%08X = 0x%02x\r\n", pShowAddr, Value8);
    GSE_PutLine(Str);
  }
  else if (strcmp (valueType, "w") == 0)
  {
    Value16 = *(UINT16*)pAddress;
    GSE_PutLine("\r\n");
    sprintf(Str,"Address 0x%08X = 0x%04x\r\n", pShowAddr, Value16);
    GSE_PutLine(Str);
  }
  else if (strcmp (valueType, "l") == 0)
  {
    Value32 = *(UINT32*)pAddress;
    GSE_PutLine ("\r\n");
    sprintf(Str,"Address 0x%08X = 0x%08x\r\n", pShowAddr, Value32);
    GSE_PutLine(Str);
  }
  else if (strcmp (valueType, "f") == 0)
  {
    ValueFloat = *(FLOAT32*)pAddress;
    GSE_PutLine ("\r\n");
    sprintf(Str,"Address 0x%08X = %f\r\n", pShowAddr, ValueFloat);
    GSE_PutLine(Str);
  }
  else
  {
    sprintf(Str,"Invalid type specifier. Use b,w,l,f.");
    GSE_PutLine(Str);
  }
}

/*****************************************************************************
 * Function:    MonitorSetMem
 *
 * Description: Set the value at the specified address.
 *
 * Parameters:  Tokens needed to convert all the various memory types:
 *
 *              CHAR* valueType,
 *              CHAR* address,
 *              CHAR* value,
 *              CHAR* fpFraction
 *
 * Returns:     void
 *
 * Notes:       SYS.SET.MEM.[b | w | l | f].#Address [HexValue|float]
 *
 ****************************************************************************/
static void MonitorSetMem (CHAR* valueType, CHAR* address, CHAR* value, CHAR* fpFraction)
{
    CHAR    Str[ARRAY_64];
    BOOLEAN setValue;
    UINT32  Value;
    void*   pAddress;
    FLOAT32* fPtr = (void*)&Value;

    // If memory location is a 'virtual' address representing
    // data stored in EEPROM/RTC RAM, set it.
    pAddress = (void*)(strtoul(address, NULL, BASE_16));

    // validate the memory access type
    if (strcmp (valueType, "b") == 0)
    {
      Value = strtoul(value, NULL, BASE_16);
      setValue = MonitorMemoryWrite( pAddress, sizeof(BYTE), Value);
    }
    else if (strcmp (valueType, "w") == 0)
    {
      Value = (UINT16)(strtoul(value, NULL, BASE_16));
      setValue = MonitorMemoryWrite( pAddress, sizeof(UINT16), Value);
    }
    else if (strcmp (valueType, "l") == 0)
    {
      Value = strtoul(value, NULL, BASE_16);
      setValue = MonitorMemoryWrite( pAddress, sizeof(UINT32), Value);
    }
    else if (strcmp (valueType, "f") == 0)
    {
      // see if we split up a FP number on the '.', which is a token delimiter
      if (fpFraction != NULL && *fpFraction != '\0')
      {
        *(--fpFraction) = '.';
      }

      // stuff the float into the UINT32
      *fPtr = (FLOAT32)atof(value);
      setValue = MonitorMemoryWrite( pAddress, sizeof(FLOAT32), Value);
    }
    else
    {
      sprintf(Str,"Invalid type specifier. Use b,w,l,f.");
      GSE_PutLine(Str);
      setValue = FALSE;
    }

    if (TRUE == setValue)
    {
      MonitorGetMem( valueType, address);
    }
}

/*****************************************************************************
 * Function:    MonitorMemoryWrite
 *
 * Description: Check the memory address is valid and perform the write.
 *
 * Parameters:  address (i): the address to write to
 *              size    (i): the number of bytes to write
 *              value   (i): the value to write to memory
 *
 * Returns:     TRUE when written, FALSE otherwise
 *
 * Notes:       None
 *
 ****************************************************************************/
static BOOLEAN MonitorMemoryWrite( CHAR* address, UINT8 size, UINT32 value)
{
  BOOLEAN setValue;

  if( (UINT32)address >= RTC_VIRTUAL_BASE_ADDR &&
      (UINT32)address <= (EEPROM2_VIRTUAL_BASE_ADDR + EEPROM_SIZE) )
  {
    setValue = MonitorSetNvRAM( address, size, value);
  }
  else
  {
    char* pAddress;

    ADDR_WIN32(pAddress, address);

    setValue = TRUE;

    ASSERT( ((size == sizeof(UINT8)) || (size == sizeof(UINT16)) || (size == sizeof(UINT32))) )

    if (size == sizeof(UINT8))
    {
      *(UINT8*)pAddress = (UINT8)value;
    }
    else if ( size == sizeof(UINT16))
    {
      *(UINT16*)pAddress = (UINT16)value;
    }
    else if ( size == sizeof(UINT32))
    {
      *(UINT32*)pAddress = value;
    }

  }

  return setValue;
}

/*****************************************************************************
 * Function:    MonitorSetRtc
 *
 * Description: Sets board RTC chip date and time
 *
 * Parameters:  CHAR* Tokens[]
 *
 * Returns:     void
 *
 * Notes:       SYS.SET.DATE MM/DD/YYYY HH:MM:SS
 *
 *              DD   = Day of Month
 *              YYYY = Year with 4 digits
 *              HH   = Hour in 24 Hr format
 *
 ****************************************************************************/
static void MonitorSetRtc (CHAR* Tokens[])
{
  BOOLEAN formatError;
  TIMESTRUCT DateTime;

  DateTime.Month  = (UINT16)atoi( Tokens[TOK3]);
  DateTime.Day    = (UINT16)atoi( Tokens[TOK4]);
  DateTime.Year   = (UINT16)atoi( Tokens[TOK5]);
  DateTime.Hour   = (UINT16)atoi( Tokens[TOK6]);
  DateTime.Minute = (UINT16)atoi( Tokens[TOK7]);
  DateTime.Second = (UINT16)atoi( Tokens[TOK8]);
  DateTime.MilliSecond = 0;

  if(((DateTime.Second   <= 59))  &&
     ((DateTime.Minute   <= 59))  &&
     ((DateTime.Hour     <= 23))  &&
     ((DateTime.Day      <= 31))  &&
     ((DateTime.Month    <= 12))  &&
     ((DateTime.Year     >= 2000) && (DateTime.Year <= MAX_YEAR) ) )
  {
    CM_SetRTCClock(&DateTime, TRUE);
    formatError = FALSE;
  }
  else
  {
    formatError = TRUE;
  }

  if (TRUE == formatError)
  {
    MonitorInvalidCommand( "Bad date/time or format.  Expected 'MM/DD/YYYY HH:MM:SS'");
  }
}

/*****************************************************************************
 * Function:    MonitorSetTaskPulse
 *
 * Description: Set the value at the specified address.
 *
 * Parameters:  Token3 - TaskId or command
 *              Token4 - DOUT to pulse when task is active
 *
 * Returns:     void
 *
 * Notes:       sys.set.pulse.<TaskId> [-1 .. 3] - pulse on a task active
 *              sys.set.pulse [on | off]         - pulse LSS0 on MIF0
 *              sys.set.pulse reset              - reset all pulse info
 *              sys.set.pulse test               - test overhead of DIO_SetPin
 *
 ****************************************************************************/
static void MonitorSetTaskPulse(CHAR* Token3, CHAR* Token4)
{
    INT32 i;
    INT32 dout;
    INT32 mif;
    TASK_INDEX TaskId;

    // Check command format for either:
    if ( isdigit( Token3[0]))
    {
        // convert task ID to task index
        TaskId = (TASK_INDEX)atoi( Token3);

        dout   = atoi( Token4);

        if ( (TaskId >= MAX_TASKS) || (Tm.pTaskList[TaskId] == NULL) )
        {
            MonitorInvalidCommand( "Invalid Task Id");
        }
        else if (dout < -1 || dout > 3)
        {
            MonitorInvalidCommand( "TaskPulse: Invalid DOUT Specified [-1..3]");
        }
        else
        {
            Tm.pTaskList[TaskId]->timingPulse = dout;
        }
    }
    else if (strcmp (Token3, "reset") == 0)
    {
        // clear all pulse flags
        mifPulse = -1;
        for (i = 0; i < MAX_TASKS; ++i)
        {
          if ( Tm.pTaskList[i] != NULL)
          {
            Tm.pTaskList[i]->timingPulse = -1;
          }
        }
    }
    else if (strcmp (Token3, "test") == 0)
    {
        DIO_SetPin(LSS0, DIO_SetHigh);
        DIO_SetPin(LSS0, DIO_SetLow);
        DIO_SetPin(LSS0, DIO_SetHigh);
        DIO_SetPin(LSS0, DIO_SetLow);
        DIO_SetPin(LSS0, DIO_SetHigh);
        DIO_SetPin(LSS0, DIO_SetLow);
        DIO_SetPin(LSS0, DIO_SetHigh);
        DIO_SetPin(LSS0, DIO_SetLow);
        DIO_SetPin(LSS0, DIO_SetHigh);
        DIO_SetPin(LSS0, DIO_SetLow);
    }
    else if (strcmp(Token3, "on") == 0 || strcmp(Token3, "off") == 0)
    {
        if ( strcmp(Token3, "on") == 0)
        {
            mif = atoi( Token4);
            if ( mif >= -1 && mif <= 31)
            {
                // One Frame pulse on MIF on LSS0, enable/disable
                mifPulse = mif;
            }
            else
            {
                MonitorInvalidCommand( "TaskPulse: MIF must be [-1..31]");
            }
        }
        else
        {
            mifPulse = -1;
        }
    }
    else
    {
      MonitorInvalidCommand("Invalid Pulse Cmd Syntax [on|off|reset|test]");
    }
}


/*****************************************************************************
 * Function:    MonitorGetRtc
 *
 * Description: Get the current RTC date and time.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       SYS.GET.RTC
 *
 ****************************************************************************/
static void MonitorGetRtc (void)
{
  TIMESTRUCT Time;
  INT8 Str[ARRAY_64];

  GSE_PutLine ("\r\n");

  CM_GetRTCClock( &Time );

  sprintf(Str,"\r\nCurrent RTC = %02d/%02d/%04d %02d:%02d:%02d\r\n",
                                                 Time.Month,
                                                 Time.Day,
                                                 Time.Year,
                                                 Time.Hour,
                                                 Time.Minute,
                                                 Time.Second);
  GSE_PutLine (Str);

}



/*****************************************************************************
 * Function:    MonitorGetTm
 *
 * Description: Get the current status information of the Task Manager.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       SYS.GET.TM
 *              Tm 'Performance Data' is not displayed
 *
 ****************************************************************************/
static void MonitorGetTm (void)
{
  INT32 i;
  INT8 Str[ARRAY_128];
  MIF_DUTY_CYCLE dc;
  FLOAT32 avgDc;

  GSE_PutLine ("\r\n");

  sprintf(Str,"Total Tasks         = %d\r\n", Tm.nTasks+1);
  GSE_PutLine(Str);
  sprintf(Str,"Number of DT Tasks  = %d\r\n", Tm.nDtTasks);
  GSE_PutLine(Str);
  sprintf(Str,"Number of RMT Tasks = %d\r\n", Tm.nRmtTasks);
  GSE_PutLine(Str);
  sprintf(Str,"DT Task overruns    = %d\r\n", Tm.nDtOverRuns);
  GSE_PutLine(Str);
  sprintf(Str,"RMT Task overruns   = %d\r\n", Tm.nRmtOverRuns);
  GSE_PutLine(Str);
  sprintf(Str,"Current System Mode = %d\r\n", Tm.systemMode);
  GSE_PutLine(Str);
  sprintf(Str,"Current MIF         = %d\r\n", Tm.currentMIF);
  GSE_PutLine(Str);
  sprintf(Str,"Total MIF           = %d\r\n", Tm.nMifCount);
                               GSE_PutLine(Str);
  sprintf(Str,"Max Duty Cycle %%    = %.2f%%\r\n",
         (FLOAT32)Tm.dutyCycle.max/TM_DUTY_CYCLE_TO_PCT);
  GSE_PutLine(Str);
  sprintf(Str,"Max Duty Cycle MIF  = %d\r\n", Tm.dutyCycle.mif);
  GSE_PutLine(Str);
  sprintf(Str,"Cur Duty Cycle %%    = %.2f%%\r\n",
         (FLOAT32)Tm.dutyCycle.cur/TM_DUTY_CYCLE_TO_PCT);
  GSE_PutLine(Str);
  sprintf(Str,"Cur MIF Time = %d\r\n",
                                                    Tm.dutyCycle.cumExecTime);
  GSE_PutLine(Str);

  GSE_PutLine ("\r\n");
  sprintf(Str,"MIF period (uSec)   = %d\r\n", Tm.nMifPeriod);
  GSE_PutLine(Str);
  sprintf(Str,"MIFs per cycle      = %d\r\n", MAX_MIF+1);
  GSE_PutLine(Str);

  GSE_PutLine("\r\n"
              "               DT MIF Loading (uS)\r\n"
              "           Min      Max     Cur       Avg\r\n"
              "        -------- -------- -------- --------\r\n");
  for ( i = 0; i < MAX_MIF+1; ++i)
  {
      memcpy( &dc, &Tm.mifDc[i], sizeof( dc));
      avgDc = (FLOAT32)((FLOAT32)(dc.cumExecTime/TTMR_HS_TICKS_PER_uS)/(FLOAT32)dc.exeCount);
      sprintf( Str, "MIF%02d:  %8d %8d %8d %8.1f\r\n",
          i,
          dc.min/TTMR_HS_TICKS_PER_uS,
          dc.max/TTMR_HS_TICKS_PER_uS,
          dc.cur/TTMR_HS_TICKS_PER_uS,
          avgDc);
      GSE_PutLine(Str);
  }
  // NOTE: Tm Perf Data is not displayed
}


/*****************************************************************************
 * Function:    MonitorGetTask
 *
 * Description: Get the TCB information of the specified Task
 *
 * Parameters:  TASK_INDEX TaskId
 *
 * Returns:     void
 *
 * Notes:       SYS.GET.TASK.#TaskId
 *
 ****************************************************************************/
static void MonitorGetTask (TASK_INDEX TaskId)
{
  TCB*    pTask;
  INT8    Str[ARRAY_128];
  UINT32  avgExecTime = 0;

  if ((TaskId < 0) || (TaskId >= MAX_TASKS) || (Tm.pTaskList[TaskId] == NULL))
  {
    MonitorInvalidCommand("Invalid Task Id");
  }
  else
  {
    pTask = Tm.pTaskList[TaskId];
    GSE_PutLine(NEW_LINE);
    //           0123456789_12345678
    sprintf(Str,"Name              = %s\r\n", pTask->Name);
    GSE_PutLine(Str);
    sprintf(Str,"Task ID           = %d\r\n", pTask->TaskID);
    GSE_PutLine(Str);
    sprintf(Str,"Type              = %s\r\n", TASK_TYPES[pTask->Type]);
    GSE_PutLine(Str);
    sprintf(Str,"Priority          = %d\r\n", pTask->Priority);
    GSE_PutLine(Str);
    sprintf(Str,"Modes             = 0x%08x\r\n", pTask->modes);
    GSE_PutLine(Str);
    sprintf(Str,"Function Address  = 0x%08X\r\n", pTask->Function);
    GSE_PutLine(Str);
    sprintf(Str,"State             = %s\r\n",TASK_STATES[pTask->state]);
    GSE_PutLine(Str);
    sprintf(Str,"Enabled           = %s\r\n",YES_NO[pTask->Enabled]);
    GSE_PutLine(Str);
    sprintf(Str,"Locked            = %s\r\n", YES_NO[pTask->Locked]);
    GSE_PutLine(Str);
    sprintf(Str,"PBlock Address    = 0x%08X\r\n", pTask->pParamBlock);
    GSE_PutLine(Str);

    if (pTask->Type == DT)
    {
      //           0123456789_12345678
      sprintf(Str,"MIF Frames        = %08X\r\n", pTask->MIFrames);
      GSE_PutLine(Str);
    }
    else if (pTask->Type == RMT)
    {
      //           0123456789_12345678
      sprintf(Str,"RMT Initial MIF   = %d\r\n",pTask->Rmt.InitialMif);
      GSE_PutLine(Str);
      sprintf(Str,"RMT MIF Rate      = %d\r\n", pTask->Rmt.MifRate);
      GSE_PutLine(Str);
      sprintf(Str,"RMT Overrun Cnt   = %d\r\n",pTask->Rmt.overrunCount);
      GSE_PutLine(Str);
      sprintf(Str,"RMT Interrupt Cnt = %d\r\n",pTask->Rmt.interruptedCount);
      GSE_PutLine(Str);
      sprintf(Str,"RMT Next Run      = %d @MIF: %d\r\n", pTask->Rmt.nextMif, Tm.nMifCount);
      GSE_PutLine(Str);
    }
    //           0123456789_12345678
    sprintf(Str,"Pulse On          = %d\r\n", pTask->timingPulse);
    GSE_PutLine(Str);
    sprintf(Str,"Last Exec Time    = %d\r\n", pTask->lastExecutionTime);
    GSE_PutLine(Str);
    sprintf(Str,"Min Exec Time     = %d\r\n", pTask->minExecutionTime);
    GSE_PutLine(Str);
    sprintf(Str,"Max Exec Time     = %d\r\n", pTask->maxExecutionTime);
    GSE_PutLine(Str);
    // has task been run once?
    if (pTask->totExecutionTimeCnt > 0)
    {
      avgExecTime = pTask->totExecutionTime / pTask->totExecutionTimeCnt;
    }
    sprintf(Str,"Avg Exec Time     = %d\r\n", avgExecTime);
    GSE_PutLine(Str);
    sprintf(Str,"Total Exec Time   = %d\r\n",pTask->totExecutionTime);
    GSE_PutLine(Str);
    sprintf(Str,"Max Exec Mif      = %d\r\n", pTask->maxExecutionMif);
    GSE_PutLine(Str);
    sprintf(Str,"Execution Count   = %d\r\n", pTask->executionCount);
    GSE_PutLine(Str);
  }
}

/*****************************************************************************
 * Function:    MonitorGetTaskList
 *
 * Description: Get a list of all tasks in the system.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       SYS.GET.TASK
 *
 ****************************************************************************/
static void MonitorGetTaskList (void)
{
  INT32 i;
  TCB* pTask;
  INT8 Str[ARRAY_64];

  GSE_PutLine ("\r\n");

  for (i = 0; i < MAX_TASKS; ++i)
  {
    pTask = Tm.pTaskList[i];
    if ( pTask != NULL )
    {
      sprintf(Str,"Task %d = %s\r\n", pTask->TaskID, pTask->Name);
      GSE_PutLine(Str);
    }
  }
}

/*****************************************************************************
 * Function:    MonitorGetTaskTiming
 *
 * Description: Get the current execution timing info for all tasks.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       SYS.GET.TASK.TIMING
 *
 ****************************************************************************/
static void MonitorGetTaskTiming (void)
{
  INT32 i;
  TCB* pTask;
  UINT32 OverrunCount;
  INT8 Str[ARRAY_128];

  sprintf(Str,"\r\nTotal Power On Time: %d ms\r\n", CM_GetTickCount());
  GSE_PutLine (Str);
  sprintf(Str,"                                Exec    Execution Time (uSec)      RMT\r\n");
  GSE_PutLine(Str);
  sprintf(Str,
          " #  Name                        Count   Min    Max    Avg    Last  Overruns\r\n");
  GSE_PutLine(Str);
  sprintf(Str,
          "--- -------------------------   ------ ------ ------ ------ ------ --------\r\n");
  GSE_PutLine(Str);

  for (i = 0; i < MAX_TASKS; ++i)
  {
    pTask = Tm.pTaskList[i];
    if ( pTask != NULL )
    {
      UINT32  avgExecTime = 0;

      // has task been run once?
      if (pTask->totExecutionTimeCnt > 0)
      {
          avgExecTime = pTask->totExecutionTime / pTask->totExecutionTimeCnt;
      }

      if (pTask->Type == RMT)
      {
          OverrunCount = pTask->Rmt.overrunCount;
          sprintf(Str,"%2i: %-25s : %6i %6i %6i %6i %6i %8i\r\n",
                       pTask->TaskID,
                       pTask->Name,
                       pTask->executionCount,
                       pTask->minExecutionTime,
                       pTask->maxExecutionTime,
                       avgExecTime,
                       pTask->lastExecutionTime,
                       OverrunCount);
      }
      else
      {
          sprintf(Str,"%2i: %-25s : %6i %6i %6i %6i %6i\r\n",
                       pTask->TaskID,
                       pTask->Name,
                       pTask->executionCount,
                       pTask->minExecutionTime,
                       pTask->maxExecutionTime,
                       avgExecTime,
                       pTask->lastExecutionTime);
      }
      GSE_PutLine(Str);
    }
  }
}

/*****************************************************************************
 * Function:    MonitorGetMemUsage
 *
 * Description: Get the memory usage stats.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       sys.memory
 *
 ****************************************************************************/
static void MonitorGetMemUsage (void)
{
  UINT32  PFlashSize;
  FLOAT32 PFlashPercent;

  UINT32  VRamSize;
  FLOAT32 VRamPercent;
  INT8   Str[ARRAY_128];

  // calculate use size of flash
  PFlashSize      = VectorTab.size + CodeSec.size + SecInfo.size + RomData.size;
  PFlashPercent   = ((FLOAT32)PFlashSize / (FLOAT32)FLASH_SIZE) * 100;

  // Calculate used size of Ram
  VRamSize        = DataRam.size + IDataRam.size + Stack.size;
  VRamPercent     =((FLOAT32)VRamSize / (FLOAT32)RAM_SIZE) * 100;

  GSE_PutLine ("\r\n");
  sprintf(Str,"Program FLASH    = %-6d bytes   %.2f%%\r\n",
            PFlashSize, PFlashPercent);
  GSE_PutLine(Str);

  sprintf(Str,"Variable RAM     = %-6d bytes   %.2f%%\r\n",
          VRamSize, VRamPercent);
  GSE_PutLine(Str);
}

/*****************************************************************************
 * Function:    MonitorGetStackUsage
 *
 * Description: Get the stack usage stats.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       sys.stack
 *
 ****************************************************************************/
static void MonitorGetStackUsage (void)
{
  INT8   Str[ARRAY_128];
  UINT32* pStack;
  UINT32* pStackTop;
  UINT32  stackSize;
  UINT32  stackUsed;
  FLOAT32 StackPercent;
  BOOLEAN bFound;

  UINT32 stackFill = StackFill;   // local copy

  pStack = &__SP_END;       // point to stack bottom
  pStackTop = &__SP_INIT;   // point to top of stack

  // Initialize the percentage to protect against the divide by zero
  // This condition is not possible if the linker directive file is
  // correct but we will protect against it anyway.
  StackPercent = 100.0;

  stackSize = (UINT32)((UINT8*)pStackTop - (UINT8*)pStack);   // size of stack

  // search stack for end of pre-filled pattern
  for (bFound = FALSE; (!bFound) && (pStack < pStackTop); ++pStack)
  {
      bFound = (BOOLEAN)(*pStack != stackFill);
  }

  // calculate use size of stack
  stackUsed = (UINT32)((UINT8*)pStackTop - (UINT8*)pStack);   // amount of stack used

  // Protect against divide by 0
  if (stackSize != 0)
  {
     StackPercent = (FLOAT32)((FLOAT32)stackUsed / (FLOAT32)stackSize) * 100.0;
  }

  sprintf(Str,"\r\nStack Usage    = %-6d bytes   %.2f%%\r\n",
            stackUsed, StackPercent);
  GSE_PutLine(Str);
}

/*****************************************************************************
 * Function:    MonitorInvalidCommand
 *
 * Description: Handle unknown commands.
 *
 * Parameters:  msg (i): a message to print out
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static void MonitorInvalidCommand( char* msg)
{
  if ( msg != NULL)
  {
    GSE_PutLine ("\r\n");
    GSE_PutLine ("\r\n");
    GSE_PutLine( "*** Error: ");
    // display the caller msg
    GSE_PutLine( msg);
    GSE_PutLine ("\r\n");
  }

  // ensure string is terminated
  PrevCmd[ PrevCmdLen ] = '\0';

  // display the invalid command
  GSE_PutLine( PrevCmd);
  GSE_PutLine ("\r\n");
}

/*****************************************************************************
* Function:    MonitorSetTaskMode
*
* Description: Set the task mode.
*
* Parameters:  Token - mode
*
* Returns:     void
*
* Notes:       sys.set.taskmode.X
*
****************************************************************************/
static void MonitorSetTaskMode(CHAR* Token[])
{
  // Set the mode
  if (*Token[TOK3] != NULL)
  {
    UINT32 x;
    x = (UINT32)(atoi( Token[TOK3]));
    if ((SYS_MODE_IDS)x >= SYS_NORMAL_ID && (SYS_MODE_IDS)x < SYS_MAX_MODE_ID)
    {
      TmSetMode( (SYS_MODE_IDS)x);
    }
    else
    {
      CHAR Str[ARRAY_80];
      sprintf(Str, "Invalid Task Mode");
      MonitorInvalidCommand(Str);
    }
  }
  else
  {
    MonitorInvalidCommand("Command requires a mode value.");
  }
}

/*****************************************************************************
 * Function:    MonitorSetTaskEnable
 *
 * Description: Enable / disable an unlocked task.
 *
 * Parameters:  Token - [ ON | OFF ]
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static void MonitorSetTaskEnable (CHAR* Token[])
{
  BOOLEAN State;
  TASK_INDEX TaskId;

  State = ON;
  if (strcmp (Token[TOK5], "off") == 0)
  {
    State = OFF;
  }

  TaskId = (TASK_INDEX)atoi( Token[TOK3]);
  if ( (TaskId >= 0) && (TaskId < MAX_TASK) && (Tm.pTaskList[TaskId] != NULL) )
  {
    TmTaskEnable ( TaskId, State );
  }
  else
  {
    MonitorInvalidCommand("Invalid Task Id");
  }
}


#ifdef ENABLE_SYS_DUMP_MEM_CMD
/*vcast_dont_instrument_start*/
/*****************************************************************************
* Function:    MonitorDumpMem
*
* Description: Dump the contents of a block of memory
*
* Parameters:  CHAR* Token[]
*
* Returns:     void
*
* Notes:       None.
*
****************************************************************************/

static void MonitorDumpMem(CHAR* Token[] )
{
  void*  pAddress;
  SINT32 size;
  BOOLEAN bWellKnownLocation = FALSE;

  // Handle well-known memory areas

  // sys.dumpmem.nvcfg
  if(strcmp(Token[TOK2], "nvcfg") == 0 )
  {
    bWellKnownLocation = TRUE;
    pAddress = (void*)CfgMgr_ConfigPtr();
    size     = sizeof(CFGMGR_NVRAM);
  }
  else
  {
    pAddress = (void*)strtoul(Token[TOK2], NULL, BASE_16);
    size     = strtoul(Token[TOK3], NULL, BASE_16);
  }

#ifdef WIN32
  if (!bWellKnownLocation)
  {
    pAddress = (void*)&sysMemoryCmds;
  }
#endif
  DumpMemory(pAddress, size, NULL);

}
/*vcast_dont_instrument_end*/
#endif



/*****************************************************************************
 * Function:    MonitorGetNvRAM
 *
 * Description: Read data from a location in EEPROM OR RTC NVRAM
 *
 * Parameters:  [in] dataType character representing the type of data.
 *              [in] pVirtualAddress pointer to a "virtual" address which is
 *                   mapped to a physical address in EEPROM or RTC RAM
 *
 * Returns:     Pointer to the data returned from EEPROM/RTC-RAM
 *
 * Notes:       None.
 *
 ****************************************************************************/
void* MonitorGetNvRAM(CHAR dataType, void* pVirtualAddress)
{
  static UINT8    Value8;
  static UINT16  Value16;
  static UINT32  Value32;
  static FLOAT32 ValueFloat;
  static void*   dataPtr;
  UINT16  size;
  UINT32 physAddr;
  NV_DEV_READ_FUNC pReadFunc = NULL;
  BOOLEAN success = TRUE;

  switch (dataType)
  {
    case 'b':
      size = sizeof(Value8);
      dataPtr = &Value8;
      break;

    case 'w':
      size = sizeof(Value16);
      dataPtr = &Value16;
      break;

    case 'l':
      size = sizeof(Value32);
      dataPtr = &Value32;
      break;

    case 'f':
      size = sizeof(ValueFloat);
      dataPtr = &ValueFloat;
      break;

    default:
      // Invalid, do nothing, return pointer "as-is"
      size = 0;
      dataPtr = pVirtualAddress;
      success = FALSE;
      break;
  }

  // Convert the virtual address to the corresponding
  // physical address for the NVRAM device.

  if((UINT32)pVirtualAddress >= EEPROM2_VIRTUAL_BASE_ADDR &&
     (UINT32)pVirtualAddress <= EEPROM2_VIRTUAL_BASE_ADDR + EEPROM_SIZE)
  {
    // EEPROM2
    physAddr =  0x10000000 + ( (UINT32)pVirtualAddress - EEPROM2_VIRTUAL_BASE_ADDR );
    pReadFunc = NV_EERead;
  }
  else if( (UINT32)pVirtualAddress >= EEPROM1_VIRTUAL_BASE_ADDR &&
           (UINT32)pVirtualAddress <= EEPROM1_VIRTUAL_BASE_ADDR + EEPROM_SIZE )
  {
    // EEPROM1
    physAddr =  0x00000000 + ( (UINT32)pVirtualAddress - EEPROM1_VIRTUAL_BASE_ADDR );
    pReadFunc = NV_EERead;
  }
  else if( (UINT32)pVirtualAddress >= RTC_VIRTUAL_BASE_ADDR &&
           (UINT32)pVirtualAddress <= RTC_VIRTUAL_BASE_ADDR + 0xFF )
  {
    // RTC NVRAM
    physAddr = ( (UINT32)pVirtualAddress - RTC_VIRTUAL_BASE_ADDR);
    pReadFunc = NV_RTCRead;
  }

  // Reade the data from the NV Memory device
  if (success && pReadFunc != NULL)
  {
    pReadFunc(dataPtr, physAddr,size);
  }

 return dataPtr;
}

/*****************************************************************************
* Function:    MonitorSetNvRAM
*
* Description: Set data by writing to EEPROM OR RTC NVRAM
*
* Parameters:  address (i)
*              CHAR* address
*              INT8  size (must be 1,2 or 4)
*              UINT32 value
*
* Returns:     BOOLEAN
*
* Notes:       None.
*
****************************************************************************/
BOOLEAN MonitorSetNvRAM( CHAR* address, UINT8 size, UINT32 value)
{
  INT16   i;
  UINT16  value16 = value;   // For width conversion.  This truncates the 32b
  UINT8   value8 = value;    // range, but that is okay b/c width is "size"
  void*   dataPtr;           // Pointer to one of the above values.
  IO_RESULT writeSignal[2];  // Worse-case scenario, a long-word straddles sectors.
  UINT32  physAddr;

  NV_DEV_WRITE_FUNC pWriteFunc = NULL;
  BOOLEAN success = TRUE;

  const  UINT32 IoTimeLimit = TICKS_PER_Sec;
  BOOLEAN timeoutFlag;
  UINT32 StartTime;

  dataPtr = &value;
  //Portable type width conversion.
  if(size == sizeof(value8))
  {
    dataPtr = &value8;
  }
  else if (size == sizeof(value16))
  {
    dataPtr = &value16;
  }

  //dataPtr = size == 1 ? value8 : size == 2 ? value16 : &value;
  //dataPtr = (INT8*)dataPtr + 4-size;

  // Convert the virtual address to the corresponding
  // physical offset on the appropriate NVRAM device.
  // Assign function pointer to the handler function.

  if ((UINT32)address >= EEPROM1_VIRTUAL_BASE_ADDR &&
      (UINT32)address <=  EEPROM1_VIRTUAL_BASE_ADDR + EEPROM_SIZE)
  {
    // EEPROM1
    physAddr =  0x00000000 +
                ( (UINT32)address - EEPROM1_VIRTUAL_BASE_ADDR);
    pWriteFunc = NV_EEWrite;
  }
  else if ((UINT32)address >= EEPROM2_VIRTUAL_BASE_ADDR &&
           (UINT32)address <=  EEPROM2_VIRTUAL_BASE_ADDR + EEPROM_SIZE)
  {
    // EEPROM2
    physAddr =  0x10000000 +
                ( (UINT32)address - EEPROM2_VIRTUAL_BASE_ADDR);
    pWriteFunc = NV_EEWrite;
  }
  else if((UINT32)address >= RTC_VIRTUAL_BASE_ADDR &&
          (UINT32)address <= RTC_VIRTUAL_BASE_ADDR + 0xFF )
  {
    // RTC NVRAM
    physAddr = ( (UINT32)address - RTC_VIRTUAL_BASE_ADDR);
    pWriteFunc = NV_RTCWrite;
  }

  //Init the semaphore flags.
  writeSignal[0] = IO_RESULT_READY;
  writeSignal[1] = IO_RESULT_READY;

  // Write the data to the NV Memory device
  // Handle writes which straddle sectors with multiple writes.
  if (success && pWriteFunc != NULL)
  {
    INT8 countWritten;
    i = 0;

    TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);
    timeoutFlag = FALSE;
    do
    {
      countWritten = pWriteFunc( dataPtr, physAddr, size, &writeSignal[i++]);
      if(countWritten == -1)
      {
        success = FALSE;
        break;
      }
      else if ( countWritten < size )
      {
        dataPtr  = (void*)( (UINT32)dataPtr + countWritten );
        physAddr += countWritten;
        size     -= countWritten;
      }
      else
      {
        size = 0;
      }

      if(TimeoutEx(TO_CHECK, IoTimeLimit,&StartTime,NULL))
      {
        timeoutFlag = TRUE;
        success     = FALSE;
      }
    }
    while( !timeoutFlag && size > 0);
  }

  // We need to ensure writing to NVRAM/EEPROM is complete before we exit so that
  // reads will show the correct data.
  // For WIN32 G4E emulation, force SpiManager to process queue entry(ies) holding the write.
  // For target build, wait until the semaphore indicates the SpiManager has completed.
  if (success)
  {
#ifdef WIN32
    /*vcast_dont_instrument_start*/
    SPIMgr_UpdateEEProm(NULL);
    SPIMgr_UpdateRTCNVRam(NULL);
    /*vcast_dont_instrument_end*/
#else
    // Set a timer to prevent infinite loop
    TimeoutEx(TO_START, IoTimeLimit, &StartTime, NULL);
    timeoutFlag = FALSE;

    while( !timeoutFlag &&
           (writeSignal[0] != IO_RESULT_READY ||
           writeSignal[1] != IO_RESULT_READY))
    {
      if(TimeoutEx(TO_CHECK, IoTimeLimit,&StartTime,NULL))
      {
        timeoutFlag = TRUE;
        success     = FALSE;
      }
    }
#endif
  }
  return success;
}

/*****************************************************************************
 * Function:    MonitorSysPerf
 *
 * Description: Base function for sys.perf.[reset|stats]
 *              calls. This function verifies the remainder of the string
 *              parses to 'reset.<task#>' or 'stats'. if not, an
 *              unknown monitor perf command msg is displayed
 *
 * Parameters:  Token - CHAR* should parse to 'reset.<task#>' or 'stats'
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
static void MonitorSysPerf(CHAR* Token[])
{
  // sys.perf.reset.<task#>
  if ( 0 == strcmp (Token[TOK2], "reset") )
  {
    MonitorSysPerfReset( Token[TOK3] );
  }
  else if ( 0 == strcmp (Token[TOK2], "stats") )
  {
    MonitorSysPerfStats();
  }
  else
  {
    MonitorInvalidCommand( "Unknown Monitor Perf command");
  }
}


/*****************************************************************************
 * Function:    MonitorSysPerfReset
 *
 * Description: Erase task times/counts for the indicated task
 *              sys.perf.reset.<task#>
 *
 * Parameters:  token3 the task id to be reset or -1 to disable buffering
 *
 * Returns:     void
 *
 * Notes:       If  0 <=taskId <= MAX_TASKS, then buffer is reset and current
 *              task cnts are reset. Data will be buffered until buffer is full
 *              If -1 == taskId, then current task(if any) is left unchanged, buffer is
 *              reset and buffering is disabled
 *
 ****************************************************************************/
static void MonitorSysPerfReset(CHAR* token3)
{
  TCB*   pTask;
  UINT32 intSave;
  INT32  taskId;
  CHAR*  pEnd;

  taskId = strtol(token3, &pEnd,  10);

  // Verify the task Id is numeric and in range, or '-1'
  if ( (pEnd != token3) && taskId >= -1 && taskId < MAX_TASKS)
  {
    if ( -1 == taskId )
    {
      // Disable collection, mark the taskId and reset the array size counter.
      // Don't clear counts for whomever may have been logging.
      Tm.perfTask = taskId;
      Tm.nPerfCount = 0;
    }
    else
    {
      // Reset the active count for the associated task and the perfdata area
      pTask = Tm.pTaskList[taskId];
      if ( pTask != NULL )
      {
        intSave = __DIR();
        pTask->lastExecutionTime    = 0;
        pTask->minExecutionTime     = UINT32MAX;
        pTask->maxExecutionTime     = 0;
        pTask->maxExecutionMif      = 0;
        pTask->executionCount       = 0;
        pTask->totExecutionTime     = 0;
        pTask->totExecutionTimeCnt  = 0;

        if (RMT == pTask->Type)
        {
          pTask->Rmt.overrunCount     = 0;
          pTask->Rmt.interruptedCount = 0;
        }

        // Clear out the accumulated totals
        // and set/disable collection
        Tm.perfTask = taskId;
        Tm.nPerfCount = 0;
        __RIR(intSave);
      }
    }
  }
  else
  {
    MonitorInvalidCommand( "Invalid Task Id");
  }
}


/*****************************************************************************
 * Function:    MonitorSysPerfStats
 *
 * Description: Displays formatted strings showing the exe stats from
 *              the first 1000 exe cnts since the counts were reset
 *              sys.perf.stats
 *
 * Parameters:  None
 *
 * Returns:     void
 *
 * Notes:       None
 *
 ****************************************************************************/
static void MonitorSysPerfStats(void)
{
  RESULT bResult = DRV_OK;
  INT8   str[ARRAY_128];
  UINT32 i;

  if (-1 != Tm.perfTask)
  {
    snprintf(str, ARRAY_128, "ExeCnt,Last,Min,Max,Tot,MaxExeMif\r\n");
    GSE_PutLineBlocked(str, TRUE);

    for ( i = 0; i < Tm.nPerfCount && (DRV_OK == bResult); ++i)
    {
      snprintf(str,ARRAY_128,"%d,%d,%d,%d,%d,%d\r\n",
                              Tm.perfData[i].executionCount,
                              Tm.perfData[i].lastExecutionTime,
                              Tm.perfData[i].minExecutionTime,
                              Tm.perfData[i].maxExecutionTime,
                              Tm.perfData[i].totExecutionTime,
                              Tm.perfData[i].maxExecutionMif);
      bResult = GSE_PutLineBlocked(str, TRUE);
    }
  }
  else
  {
    snprintf(str, ARRAY_128, "Perf Monitoring not active,\n (use sys.perf.reset.<id>)");
    GSE_PutLine(str);
  }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Monitor.c $
 *
 * *****************  Version 89  *****************
 * User: John Omalley Date: 2/05/15    Time: 10:26a
 * Updated in $/software/control processor/code/system
 * SCR 1192 - Code Review Updates
 *
 * *****************  Version 88  *****************
 * User: Contractor V&v Date: 2/03/15    Time: 7:15p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Code Review compliance update
 *
 * *****************  Version 87  *****************
 * User: Contractor V&v Date: 1/13/15    Time: 6:16p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Performance and timing added func sys.perf.reset and stats
 *
 * *****************  Version 86  *****************
 * User: Contractor V&v Date: 12/10/14   Time: 7:53p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Performance and timing added func sys.reset.counts.<taskid>
 *
 * *****************  Version 85  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 *
 * *****************  Version 84  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1099 - Code Review Updates
 *
 * *****************  Version 83  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 82  *****************
 * User: John Omalley Date: 10/11/11   Time: 4:38p
 * Updated in $/software/control processor/code/system
 * SCR 1078 - CR Update
 *
 * *****************  Version 81  *****************
 * User: Contractor2  Date: 4/29/11    Time: 11:14a
 * Updated in $/software/control processor/code/system
 * SCR #478 Error: handling cmds multiples of GSE Cmd buffer size 128
 *
 * *****************  Version 80  *****************
 * User: John Omalley Date: 10/18/10   Time: 10:21a
 * Updated in $/software/control processor/code/system
 * SCR 918 - Code Review Updates
 *
 * *****************  Version 79  *****************
 * User: Peter Lee    Date: 10/14/10   Time: 1:56p
 * Updated in $/software/control processor/code/system
 * SCR #918 Misc code coverage updates.
 *
 * *****************  Version 78  *****************
 * User: Jeff Vahue   Date: 9/21/10    Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR #848 - Code Cov
 *
 * *****************  Version 77  *****************
 * User: Jeff Vahue   Date: 9/10/10    Time: 6:29p
 * Updated in $/software/control processor/code/system
 * SCR# 866 - Monitor code needs to timeout on EEPROM write Q insert.
 *
 * *****************  Version 76  *****************
 * User: Jeff Vahue   Date: 9/07/10    Time: 7:31p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 *
 * *****************  Version 75  *****************
 * User: Jim Mood     Date: 9/07/10    Time: 5:50p
 * Updated in $/software/control processor/code/system
 * SCR 820 sys.set.mem .b and .w in NV devices did not work
 *
 * *****************  Version 74  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 5:59p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Coverage Mod to eliminate WIN32 code
 *
 * *****************  Version 73  *****************
 * User: Peter Lee    Date: 8/11/10    Time: 5:32p
 * Updated in $/software/control processor/code/system
 * SCR #777 Consolidate duplicate Program CRC.
 *
 * *****************  Version 72  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 71  *****************
 * User: Jeff Vahue   Date: 7/21/10    Time: 7:45p
 * Updated in $/software/control processor/code/system
 * SCR# 723 - remove duplicate code in memory write command
 *
 * *****************  Version 70  *****************
 * User: Jeff Vahue   Date: 7/16/10    Time: 7:24p
 * Updated in $/software/control processor/code/system
 * Stub out code for memory dump for VectorCast
 *
 * *****************  Version 69  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #636 add the ability to read/write EEPROM/RTC via the sys.g
 *
 * *****************  Version 68  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 *
 * *****************  Version 67  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * Misc- Cleanup build warnings
 *
 * *****************  Version 66  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:09p
 * Updated in $/software/control processor/code/system
 * SCR #636 add the ability to read/write EEPROM/RTC via the sys.g
 *
 * *****************  Version 65  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:06p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * Removed ID translation table. Coding standard fixes
 *
 * *****************  Version 64  *****************
 * User: Contractor V&v Date: 5/24/10    Time: 6:54p
 * Updated in $/software/control processor/code/system
 * SCR #585 Cfg File CRC changes from load to load
 *
 * *****************  Version 63  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 62  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:05p
 * Updated in $/software/control processor/code/system
 * SCR #536 Avg Task Execution Time Incorrect
 *
 * *****************  Version 61  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:38p
 * Updated in $/software/control processor/code/system
 * SCR 187: Degraded mode. Moved watchdog reset loop to WatchDog.c
 *
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy,SCR #70 Store/Restore interrupt
 *
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 4/07/10    Time: 12:09p
 * Updated in $/software/control processor/code/system
 * SCR #534 - Cleanup Program CRC messages
 *
 * *****************  Version 58  *****************
 * User: Contractor2  Date: 4/06/10    Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR 524 - Implement stack usage display in bytes and percentage
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 4/05/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR #481 - add Vectorcast cmds to eliminate code coverage on code to
 * support WIN32 execution.
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 3/29/10    Time: 2:30p
 * Updated in $/software/control processor/code/system
 * SCR# 514 - sys.get/set.mem commands conversion of address value
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 3/29/10    Time: 1:55p
 * Updated in $/software/control processor/code/system
 * SCR# 514 - The ACS Packet size max value changed from 10000 to 2500.
 * The sys.get.mem command used strtol to convert the address pointer, s/b
 * strtoul.
 *
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 52  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 5:56p
 * Updated in $/software/control processor/code/system
 * SCR# 484 - MIF pulse  on any MIF#, not just MIF0
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:42p
 * Updated in $/software/control processor/code/system
 * SCR #464 Move Version Info into own file and support it.
 *
 * *****************  Version 50  *****************
 * User: Contractor2  Date: 3/04/10    Time: 3:20p
 * Updated in $/software/control processor/code/system
 * SCR476 Memory Usage Display
 *
 * *****************  Version 49  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 1:08p
 * Updated in $/software/control processor/code/system
 * SCR# 350 - remove unused data items.
 *
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:42p
 * Updated in $/software/control processor/code/system
 * GH compiler warning fixes
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - misc fixes for sys cmd changes
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 1:00p
 * Updated in $/software/control processor/code/system
 * SCR# 449 - cast UINT32 to enums to keep the compiler happy
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 12:44p
 * Updated in $/software/control processor/code/system
 * SCR# 449 - add cmd error report to sys.set.taskmode
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 12:30p
 * Updated in $/software/control processor/code/system
 * SCR #449 - Add control of system mode via user command, display system
 * mode, clean up sys.get.task.x processing, provide GHS target debugging
 * hook to bypass DT overrun code.
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR# 441 - do not default sys.set/get.mem cmds to byte size when no
 * specifier is provided by the user.
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR# 412 - convert if/then/else to ASSERT
 *
 * *****************  Version 40  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 *
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 3:34p
 * Updated in $/software/control processor/code/system
 * Fix Lost Comment on revision 37
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 3:18p
 * Updated in $/software/control processor/code/system
 * SCR# 434 - one more issue, timing pulse changed from on/off to an
 * integer and it was indexing into a ON_OFF string table for its value in
 * GetTask
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 2:54p
 * Updated in $/software/control processor/code/system
 * SCR# 434 - fix a couple issues after some regression testing
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 1:40p
 * Updated in $/software/control processor/code/system
 * SCR# 434 - Pulse LSSx on task execution, clean up sys cmds and
 * processing
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 1/24/10    Time: 1:40p
 * Updated in $/software/control processor/code/system
 * SCR# 378 - clean up some Win32 memory read/write command processing
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:15p
 * Updated in $/software/control processor/code/system
 * SCR# 405
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:26p
 * Updated in $/software/control processor/code/system
 * SCR 145
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR #364 - INT8 -> CHAR, WIN32 updates fix sys.set.mem.f error
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR# 176 - remove redundant if from MonitorSetTaskEnable
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:16p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * Implement functions for accessing filenames and CRC
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 2:04p
 * Updated in $/software/control processor/code/system
 * Updates to support WIN32 version
 *
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:53p
 * Updated in $/software/control processor/code/system
 * Implement SCR 196 ESC' sequence when outputting continuous data to GSE.
 * Implement SCR 145 Configuration CRC read command
 *
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR #168 Update reference from time.h to alt_time.h
 *
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 4/09/09    Time: 1:57p
 * Updated in $/control processor/code/system
 * Added sys.set.rtc.  Removed sys.set/get.date.  This feature now must be
 * handled by a User Manager command
 *
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 4/07/09    Time: 10:13a
 * Updated in $/control processor/code/system
 * SCR# 161
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:23p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 8/26/08    Time: 11:28a
 * Updated in $/control processor/code/system
 * Removed DebugStr and moved implementation to Fault Mgr.  Updated file
 * header to the new "P&W ES" format.
 *
 ***************************************************************************/
