#define GSE_BODY

/******************************************************************************
            Copyright (C) 2008 - 2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

 File:        GSE.c

 Description: Provides a GSE specific interface to UART port 0.  This module
              inits UART 0 to to the configuration specified in GSE.h  Functions
              for reading and writing lines to and from the terminal are
              provided.
              
 VERSION
     $Revision: 21 $  $Date: 10/07/10 7:00p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "alt_basic.h"
#include "GSE.h"
#include "ClockMgr.h"
#include "CfgManager.h"
#include "QarManager.h"

#include "TestPoints.h"

#ifdef GHS_SIMULATOR_RUN
#include <stdio.h>
#endif //GHS_SIMULATOR_RUN

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
INT8   Buf[GSE_GET_LINE_BUFFER_SIZE];
UINT32 Cnt;
} GSE_GET_LINE_BUFFER;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FIFO TxFIFO;
GSE_GET_LINE_BUFFER GSE_GetLineBuf;

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void GSE_vDebugOutput( const BOOLEAN newLine, const BOOLEAN showTime, 
                              const CHAR* str, va_list args);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/*****************************************************************************
 * Function:    GSE_Init    
 *
 * Description: Setup UART channel 0 as the GSE port.  The default configuration
 *              for this port is
 *              115,200 bits per second, 8 data bits, 1 stop bit and no parity.
 *
 * Parameters:  SysLogId - Ptr to return System Log ID into 
 *              pdata -    Ptr to buffer to return system log data 
 *              psize -    Ptr to return size of system log data
 *
 * Returns:     RESULT
 ****************************************************************************/
RESULT GSE_Init (SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize)
{
  UART_CONFIG config;
  static INT8 TxBuf[GSE_TX_BUFFER_SIZE]; 
 
  GSE_DRV_PBIT_LOG  *pdest; 

  pdest = (GSE_DRV_PBIT_LOG *) pdata;
  memset ( pdest, 0, sizeof(GSE_DRV_PBIT_LOG) ); 
  pdest->result = DRV_OK; 
  
  *psize = sizeof(GSE_DRV_PBIT_LOG); 
  
 
  FIFO_Init(&TxFIFO,TxBuf,GSE_TX_BUFFER_SIZE);

  // Init the Live Data Stream display state to enabled.
  
  //Configure and open UART channel 0
  memset(&config,0,sizeof(config));

  config.Port             = GSE_UART_PORT;
  config.Duplex           = GSE_UART_DUPLEX;
  config.BPS              = GSE_UART_BPS;
  config.DataBits         = GSE_UART_DATA_BITS;
  config.StopBits         = GSE_UART_STOP_BITS;
  config.Parity           = GSE_UART_PARITY;
  config.LocalLoopback    = FALSE;
  config.TxFIFO           = &TxFIFO;
  config.RxFIFO           = NULL;

  //Init the get line buffer to zero chars
  GSE_GetLineBuf.Cnt = 0;
  
  pdest->result = UART_OpenPort(&config);
  
  // Note: If ->result == DRV_OK, then this value is ignored. 
  *SysLogId = SYS_ID_GSE_PBIT_REG_INIT_FAIL;  
  
  return pdest->result;
}


/*****************************************************************************
 * Function:    GSE_PutLineBlocked
 *
 * Description: Prints a null-terminated string out the GSE port. Strings can
 *              be no longer than GSE_TX_BUFFER_SIZE chars. If specified by the 
 *              bBlocked param, the function will wait for sufficient room in FIFO queue
 *              before writing to the UART.
 *
 *              For longer data, use the UART functions directly
 *
 * Parameters:  [in] Str:      String to print out the GSE port
 *              [in] bBlock:   Block/wait for room in FIFO Queue before 
 *                             issuing transmit. 
 *
 * Returns:     DRV_OK: String written successfully
 *              DRV_GSE_PUT_LINE_TOO_LONG: No null terminator found within 8192
 *                                       bytes of the string start.  No data
 *                                       was transmitted
 ****************************************************************************/
RESULT GSE_PutLineBlocked(const INT8* Str, BOOLEAN bBlock)
{
  UINT16 i = 0;
  UINT32 WaitStart = 0;
  RESULT result = DRV_OK;

#ifdef GHS_SIMULATOR_RUN
/*vcast_dont_instrument_start*/
  printf(Str);
  return DRV_OK;  
/*vcast_dont_instrument_end*/
#else
  //Find the null terminator
  while( (Str[i] != '\0') && (i < GSE_TX_BUFFER_SIZE) )
  {
    i++;
  }

  //Write the string to the UART
  //If block flag is set, wait until there is sufficient room in FIFO
  //queue for our msg size.
  WaitStart = CM_GetTickCount();
  while( bBlock && FIFO_FreeBytes(&TxFIFO) < i )
  {     
    // Waiting for room in text buffer FIFO...
    // Timeout after GSE_TX_BUFFER_AVAIL_WAIT_MS period.
    if ( (CM_GetTickCount() - WaitStart) > GSE_TX_BUFFER_AVAIL_WAIT_MS )
    {
      // TODO DaveB - Define a wait-timeout alert notification.
      break;
    }
  }
  result = UART_Transmit(GSE_UART_PORT,(INT8*)Str,i,NULL);

#endif //GHS_SIMULATOR_RUN
  return result;
}



/*****************************************************************************
* Function:    GSE_PutLine
*
* Description: Prints a null-terminated string out the GSE port. Strings can
*              be no longer than GSE_TX_BUFFER_SIZE chars
*
*              For longer data, use the UART functions directly
*
* Parameters:  [in] Str: String to print out the GSE port*              
*
* Returns:     none
* 
****************************************************************************/
void GSE_PutLine(const INT8* Str)
{
  // Make standard-call to PutLineBlocked. 
  // Don't request transmit blocking pending fifo space. 
  GSE_PutLineBlocked(Str, FALSE);
}

/*****************************************************************************
 * Function:    GSE_GetLine
 *
 * Description: Reads data from the GSE UART channel. Data is buffered locally
 *              until a carriage return character is detected in the read bytes.
 *              Once the CR is found, all data buffered up to that character
 *              is copied into the caller's Str buffer as a null-terminated
 *              string.
 *              If the Str parameter returns as an empty null-terminated string,
 *              no CR was detected.
 *              Backspace characters ("\b") are handled by removing the character
 *              preceding the backspace and copying the string after the \b
 *              to the position of the removed character.  If there are no
 *              characters in the buffer preceding the \b, the string after \b
 *              is moved to the zeroth position in the buffer.
 *              
 *              Caller's Str buffer needs to be at least
 *              GSE_GET_LINE_BUFFER_SIZE
 *             
 *
 * Parameters:  [out] Str: A string buffer to copy the received line into
 *              
 * Returns:     DRV_OK = Read successfully, does not imply CR was found though
 *              DRV_GSE_GET_LINE_TOO_LONG = Line was too long before CR was
 *                                          detected
 *              Other = See ResultCodes.h
 ****************************************************************************/
RESULT GSE_GetLine(INT8* Str)
{
  INT32 i;
  RESULT result = DRV_OK;
  UINT16 BytesReturned = 0;
 
  Str[0] = '\0'; 

#ifdef GHS_SIMULATOR_RUN
/*vcast_dont_instrument_start*/
  //Get chars from stdio one at a time, this
  //is fine for simulation purposes.
  if( (i = getc(stdin)) != EOF)
  {
    GSE_GetLineBuf.Buf[GSE_GetLineBuf.Cnt] = i;
    BytesReturned = 1;
  }
/*vcast_dont_instrument_end*/
#else
  //Get data from the GSE port
  result = UART_Receive( GSE_UART_PORT,
                          &GSE_GetLineBuf.Buf[GSE_GetLineBuf.Cnt],
                          (UINT16)(GSE_GET_LINE_BUFFER_SIZE-GSE_GetLineBuf.Cnt),
                          &BytesReturned);
#endif //GHS_SIMULATOR_RUN

  if (result == DRV_OK && BytesReturned != 0)
  {
    i = GSE_GetLineBuf.Cnt;
    GSE_GetLineBuf.Cnt += BytesReturned;
    GSE_GetLineBuf.Buf[GSE_GetLineBuf.Cnt] = '\0';
    
    while(GSE_GetLineBuf.Buf[i] != '\0')
    {
      //Remove one char for every backspace char found    
      if(GSE_GetLineBuf.Buf[i] == '\b')
      {
        i = MAX(i-1,0);
        strncpy_safe(&GSE_GetLineBuf.Buf[i],GSE_GET_LINE_BUFFER_SIZE,
               &GSE_GetLineBuf.Buf[i+2],_TRUNCATE);
        //Ensure this unsigned quantity never underflows
        GSE_GetLineBuf.Cnt = GSE_GetLineBuf.Cnt > 1 ? GSE_GetLineBuf.Cnt-2 : 0;
      }
      else if((GSE_GetLineBuf.Buf[i] == '\r')||(GSE_GetLineBuf.Buf[i] == '\n') 
               || (GSE_GetLineBuf.Buf[i] == ESC_CHAR ))
      {
        GSE_GetLineBuf.Buf[i+1] = '\0';
        strncpy_safe(Str,GSE_GET_LINE_BUFFER_SIZE,GSE_GetLineBuf.Buf,_TRUNCATE);
        GSE_GetLineBuf.Cnt = 0;
        break;
      }
      else
      {
        i++;
      }    
    }

    if( GSE_GetLineBuf.Cnt == GSE_GET_LINE_BUFFER_SIZE)
    {
      result = SYS_GSE_GET_LINE_TOO_LONG;
      GSE_GetLineBuf.Cnt = 0;
    }
  }
  return result;
}


/*****************************************************************************
 * Function:    GSE_getc
 *
 * Description: Read a single character from the GSE receive buffer.  Return
 *              value is the char, or error code
 *
 * Parameters:  none
 *              
 * Returns:     0 to 255: Character read from GSE
 *              -1:       No char to read
 *              -2:       Receive error (parity or framing)
 *              
 ****************************************************************************/
INT16 GSE_getc(void)
{
  INT16 result = -2;
  INT8  buf;
  UINT16 BytesReturned = 0;
 
  //Get data from the GSE port
  if( DRV_OK == UART_Receive( GSE_UART_PORT,
                              &buf,
                              1,
                              &BytesReturned))
  {
    if(BytesReturned == 1)
    {
      result = buf;
    }
    else
    {
      result = -1;
    }
  }
  return result;
}



/*****************************************************************************
 * Function:    GSE_write
 *
 * Description: Write a buffer to the GSE port.
 *
 * Parameters:  [in] buf: Pointer to buffer location
 *              [in] size: Size, number of bytes to write from "buf"
 *              
 * Returns:     TRUE :       Success
 *              FALSE:       size != size sent by UART
 *              
 ****************************************************************************/
BOOLEAN GSE_write(const void *buf, UINT16 size)
{
  BOOLEAN result = FALSE;
  UINT16 size_written = 0;

  if(DRV_OK == UART_Transmit(GSE_UART_PORT, buf, size, &size_written))
  {
    if(size == size_written)
    {
      result = TRUE;
    }
  }
  
  return TPU( result, eTpGseWrite);
}



/*****************************************************************************
 * Function:    GSE_read
 *
 * Description: Read data from the GSE port into a buffer
 *
 * Parameters:  [in] buf: Pointer to buffer location
 *              [in] size: Size, number of bytes to write from "buf"
 *              
 * Returns:     > 0:       Success, number of bytes read
 *              -1 :       Error returned by UART_Receive
 *              
 ****************************************************************************/
INT16 GSE_read(void *buf, UINT16 size)
{
  INT16 result = -1;
  UINT16 size_read = 0;

  if(DRV_OK == UART_Receive(GSE_UART_PORT, buf, size, &size_read))
  {
    result = size_read;
  }
  
  return (INT16)TPU( result, eTpGseRead);
}



/*****************************************************************************
* Function:    GSE_ToggleDisplayLiveStream
*
* Description: Toggles(disables) the state flag controlling enabling/disabling display
*              of live data streams.
*
* Parameters:  void
*
* Returns:     void
*
* Notes:       None.
*
****************************************************************************/
void GSE_ToggleDisplayLiveStream(void)
{ 
  // Disable the volatile-state flags for all controlled types
  
  // Disable livedata output stream
  SensorDisableLiveStream();

  // Disable Arinc429 output stream
  Arinc429MgrDisableLiveStream();

  // Disable QAR output stream
  QARMgr_DisableLiveStream();

  // Disable F7X output stream
  F7XProtocol_DisableLiveStream();

  // Disable Debug Output
  Flt_SetDebugVerbosity(DBGOFF);

  // Let the user know output is disabled.
  GSE_PutLine("\r\n\r\n ESC Mode Entered, Output is suppressed.\r\n");
}


/*****************************************************************************
* Function:    GSE_DebugStr
*
* Description: Print a debug string, up to 80 characters to the GSE console.
*              Function provides printf-like functionality, with a variable 
*              length argument list.  The str format is the same as the printf 
*              functions.
*
*              The caller is responsible for ensuring the resulting
*              formatted string does not exceed 80 characters.  Longer
*              strings will be truncated.
*
*              This function will always start the output on a new line.
*
* Parameters:  [in] DbgLevel: Debug level to print the string out at.  The
*                             string will only print to the console if the
*                             current debug level is set equal to or higher
*                             than the DbgLevel parameter
*              [in] Timestamp: TRUE: Print a timestamp before the message
*                              FALSE: Omit the timestamp
*              [in] str:      Formatted string to print to the console.  
*              [in] ...:      Variable number of arguments for any "%" tokens
*                             in the string.
*
* Returns:     none, errors are handled silently by setting an error flag.
*              If the flag is set, the routine will attempt to print an
*              error message on subsequent calls to the GSE_DebugStr routine.
*
* Notes:       
*
*****************************************************************************/
void GSE_DebugStr( const FLT_DBG_LEVEL DbgLevel, const BOOLEAN Timestamp, 
                   const CHAR* str, ...)
{
  //Check the current debug level, print the debug message if the message level
  //is within the current debug level
  if( DbgLevel <= Flt_GetDebugVerbosity())
  {
    va_list args;
    va_start(args,str);
    GSE_vDebugOutput( TRUE, Timestamp, str, args);
  }
}

/*****************************************************************************
* Function:    GSE_StatusStr
*
* Description: Simplified version of GSE_DebugStr.  Prints out a string to the 
*              GSE port as it is passed in without appending newlines or date 
*              stamps.  GSE driver failures are ignored.
*
* Parameters:  [in] DbgLevel: Debug level to print the string out at.  The
*                             string will only print to the console if the
*                             current debug level is set equal to or higher
*                             than the DbgLevel parameter
*              [in] str:      Formatted string to print to the console.  
*              [in] ...:      Variable number of arguments for any "%" tokens
*                             in the string.
*
* Returns:     void
*
* Notes:       This output can be turned off if the MonitorDebugLevel is set
*              to be less than NORMAL.
*
*****************************************************************************/
void GSE_StatusStr( const FLT_DBG_LEVEL DbgLevel, const CHAR* str, ...)
{
  if( DbgLevel <= Flt_GetDebugVerbosity())
  {
    va_list args;
    va_start(args,str);
    GSE_vDebugOutput( FALSE, FALSE, str, args);
  }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
* Function:    GSE_vDebugOutput
*
* Description: Actual Debug Output function.
*
* Parameters:  [in] newLine : does the caller want a new line
*              [in] showTime: does the caller want to display time
*              [in] str:      Formatted string to print to the console.  
*              [in] args:     Variable number of arguments for any "%" tokens
*                             in the string.
*
* Returns:     void
*
* Notes:       This output can be turned off if the MonitorDebugLevel is set
*              to be less than NORMAL.
*
*****************************************************************************/
static void GSE_vDebugOutput( const BOOLEAN newLine, const BOOLEAN showTime, 
                              const CHAR* str, va_list args)
{
    CHAR buf[1024];
    TIMESTRUCT DateTime;

    if ( newLine)
    {
      GSE_PutLine(NEW_LINE);
    }

    if( showTime)
    {
      CM_GetSystemClock( &DateTime );
      snprintf(buf,sizeof(buf),"[%02d:%02d:%02d.%03d] ", 
        DateTime.Hour, DateTime.Minute, DateTime.Second, DateTime.MilliSecond);
      GSE_PutLine(buf);
    }

    vsnprintf( buf, sizeof(buf), str, args);
    va_end(args);

    GSE_PutLine(buf);
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: GSE.c $
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 7:00p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - need a INT not an UINT
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 6:28p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 4:19p
 * Updated in $/software/control processor/code/system
 * SCR# 925 - forgot the include file
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 10/07/10   Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 9/30/10    Time: 10:53p
 * Updated in $/software/control processor/code/system
 * SCR# 910 - WTF 
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 9/30/10    Time: 10:20p
 * Updated in $/software/control processor/code/system
 * SCR# 910 - could not find strnlen replaced it with the old code
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 9/30/10    Time: 10:03p
 * Updated in $/software/control processor/code/system
 * SCR# 910 - Cleanup - remove else oops
 * 
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 7:54p
 * Updated in $/software/control processor/code/system
 * SCR 910 Code Coverage Changes
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:22p
 * Updated in $/software/control processor/code/system
 * SCR# 853 - UART Int Monitor
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 8:06p
 * Updated in $/software/control processor/code/system
 * SCR #698 Code Review Updates
 * 
 * *****************  Version 10  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 7/13/10    Time: 2:30p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence & Box Config
 * Turn off debug strings on ESC
 * 
 * *****************  Version 8  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR #486 Livedata enhancement
 * 
 * *****************  Version 5  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 4  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy SCR #451 Live Data terminated 
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 4/06/10    Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #526 - increase max GSE Tx buffer size
 * 
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 3/24/10    Time: 4:13p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - reversed parameter call in GSE_DebugStr to GSE_vDebugOutput
 * 
 * *****************  Version 1  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:35p
 * Created in $/software/control processor/code/system
 * SCR# 496 - Move GSE files from driver to system
 * 
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #493 Code Review compliance
 * 
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 2/24/10    Time: 10:49a
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:34p
 * Updated in $/software/control processor/code/drivers
 * SCR# 378
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:42p
 * Updated in $/software/control processor/code/drivers
 * SCR 42
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - typos ifdef
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:25p
 * Updated in $/software/control processor/code/drivers
 * Implementing SCR 196
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 11/17/09   Time: 4:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #283-item 16 GSE Max Length magic number.  Also, added get char and
 * write binary fuctions for the binary log file transfer process
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:03p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 12:03p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/
