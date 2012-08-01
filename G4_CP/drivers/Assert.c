#define ASSERT_BODY
/******************************************************************************
            Copyright (C) 2009-2011 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:        Assert.c

    Description: Handles, displays, and logs Assert and Unhandled Exceptions
                 to EE.  Works with Exception.s, that contains routines that
                 are better suited to assembly coded functions such as decoding
                 the stack and the exception stack frame.

    VERSION
    $Revision: 27 $  $Date: 6/27/11 1:29p $   
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "assert.h"
//#include "faultmgr.h"
#include "NVMgr.h"
#include "GSE.h"
#include "RTC.h"
#include "Exception.h"
#include "WatchDog.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
/*Assert buffer header.  The assert buffer has the following layout:

  [Header][Data (0..1024 bytes long)]

*/
typedef struct {
  UINT32  MagicNumber;
  BOOLEAN Enabled;
  BOOLEAN Read;
  UINT32  LogSize;
}ASSERT_BUF_HDR;

  
//Assert data magic number value.  This is used to detect an
//uninitialized file.
#define ASSERT_FILE_MAGIC_NUMBER        0x600DF00D

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static BOOLEAN IsEERdy = FALSE; //Indicates the EE init routine ran successfully

#ifdef WIN32
/*vcast_dont_instrument_start*/

CHAR ExOutputBuf[ASSERT_MAX_LOG_SIZE];
CHAR ReOutputBuf[ASSERT_MAX_LOG_SIZE];
void AssertDump(void) 
{
  GSE_PutLine(ExOutputBuf);
}

/*vcast_dont_instrument_end*/
#endif

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    Assert_Init
 *
 * Description: Power on initialization for the Assert processing.  Opens and
 *              checks the assert EE buffer.  If the buffer is uninitialized, it
 *              is then initialized.  
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:
 *
 *****************************************************************************/
void Assert_Init( void )
{
  ASSERT_BUF_HDR AssertHdr;

  IsEERdy = TRUE;

  //Try to open the EE Buffer
  if(SYS_OK != NV_Open(NV_ASSERT_LOG))
  { //return value purposely ignored, boolean return is only to fit the call
    //signature of NVMgr
    Assert_InitEE();
  }
  else
  {
    //Check if the EE buffer is initialized, and if not, initialize it.
    NV_Read(NV_ASSERT_LOG,0,&AssertHdr,sizeof(AssertHdr));
    if(AssertHdr.MagicNumber != STPU( ASSERT_FILE_MAGIC_NUMBER, eTpAcCfg3802))
    { //return value purposely ignored.
      Assert_InitEE();
    }
  }

  // clear out the exception buffer
  memset( ExOutputBuf, 0, ASSERT_MAX_LOG_SIZE);
  memset( ReOutputBuf, 0, ASSERT_MAX_LOG_SIZE);
}

/******************************************************************************
 * Function:    Assert_GetLog
 *
 * Description: If an assert log is recorded in EEPROM and it has not been read
 *              previously, this routine copies the log into the passed in
 *              buffer.  
 *
 * Parameters:  [out] buf: pointer to a location to store the data.  Must be no
 *                         less than ASSERT_MAX_LOG_SIZE bytes
 *
 * Returns:     0 : No data returned
 *              >0: Number of bytes returned into "buf"  
 *
 * Notes:       
 *
 *****************************************************************************/
size_t Assert_GetLog( void* buf )
{
  ASSERT_BUF_HDR Hdr;
  size_t size;
 
  size = 0;
  
  NV_Read( NV_ASSERT_LOG, 0, &Hdr, sizeof(Hdr));
  
  //Check for data in the EE buffer and read it out if it exists
  //Do not return data if it has already been read
  if( Hdr.LogSize != 0 && !Hdr.Read)
  {
    NV_Read(NV_ASSERT_LOG, sizeof(Hdr), buf, Hdr.LogSize);   
    size = Hdr.LogSize;      
  }

  return size;
}


/******************************************************************************
 * Function:    Assert_MarkLogRead
 *
 * Description: Mark the EE log as read.  This stops GetLog from returning
 *              data on future calls.  To clear the assert log from EE, and
 *              allow it to record more Assert logs, call ClearLog
 *
 * Parameters:  none
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
void Assert_MarkLogRead( void )
{
  ASSERT_BUF_HDR Hdr;

  //Check if data exists in the buffer
  NV_Read( NV_ASSERT_LOG, 0, &Hdr, sizeof(Hdr));
  
  //Check for data in the EE buffer and read it out if it exists
  if(Hdr.LogSize != 0)
  {
    Hdr.Read = TRUE;
    NV_Write(NV_ASSERT_LOG, offsetof(ASSERT_BUF_HDR,Read),
             &Hdr.Read, sizeof(Hdr.Read));
  }  
}

/******************************************************************************
 * Function:    Assert_ClearLog
 *
 * Description: Clear the assert EEPROM buffer.  This is to be used after the
 *              assert log has been logged to FLASH and uploaded to the micro-
 *              server.  The assert buffer will be ready to record another
 *              assert log after this call
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
void Assert_ClearLog(void)
{
  ASSERT_BUF_HDR Hdr;
 
  Hdr.LogSize     = 0;

  NV_Write(NV_ASSERT_LOG,
           offsetof(ASSERT_BUF_HDR,LogSize),
           &Hdr.LogSize, sizeof(Hdr.LogSize));   
}

/******************************************************************************
 * Function:    Assert
 *
 * Description: Records an assert from the "ASSERT" macro defined in the header
 *              The stack trace is dumped to the screen first, then the ASSERT
 *              information.  Finally, the data is dumped to EEPROM to be
 *              written to a FLASH log on the next reboot.  The routine prevent
 *              asserts on assert by setting a "reentered" flag on entry.  If
 *              the routine is re-entered, it immediately reboots without any
 *              processing.
 *
 * Parameters:  AssertStr - ptr to Assert str
 *              UserMsg -   the message to be used for the assert
 *              File -      file where the assert took place
 *              Line -      line where assert took place
 *              "..." -     arguments to be used for the assert
 *
 * Returns:     none
 *
 * Notes:       DOES NOT RETURN, SYSTEM IS REBOOTED!
 *              WD MUST BE ENABLED
 *
 *****************************************************************************/
void Assert( CHAR *AssertStr, INT8* UserMsg, INT8 *File, INT32 Line, ...)
{
  va_list args;
  // Create a variable argument list with all params after param "Line"
  va_start(args, Line);
  AssertHandler(AssertStr, UserMsg, File, Line, args );  
}



/******************************************************************************
 * Function:    AssertMessage
 *
 * Description: Records an assert from the "ASSERT" macro defined in the header
 *              The stack trace is dumped to the screen first, then the ASSERT
 *              information.  Finally, the data is dumped to EEPROM to be
 *              written to a FLASH log on the next reboot.  The routine prevent
 *              asserts on assert by setting a "reentered" flag on entry.  If
 *              the routine is re-entered, it immediately reboots without any
 *              processing.
 *
 * Parameters:  AssertStr - ptr to Assert str
 *              Cond -      Not used?
 *              UserMsg -   the message to be used for the assert
 *              File -      file where the assert took place
 *              Line -      line where assert took place
 *              "..." -     arguments to be used for the assert
 *
 * Returns:     none
 *
 * Notes:       DOES NOT RETURN, SYSTEM IS REBOOTED!
 *              WD MUST BE ENABLED
 *
 *****************************************************************************/
void AssertMessage( CHAR *AssertStr, CHAR* Cond, CHAR* UserMsg, CHAR *File, INT32 Line, ...)
{
  CHAR    userString[ASSERT_MAX_LOG_SIZE];
  va_list args;

  // Concatenate the condition and user message strings and hand off to Assert_Handler  
  snprintf( userString, ASSERT_MAX_LOG_SIZE, "%s\r\n%s", Cond, UserMsg);  
  va_start(args, Line);
  AssertHandler(AssertStr, userString, File, Line, args );
}

/******************************************************************************
 * Function:    AssertHandler
 *
 * Description: Records an assert from the "ASSERT"/"ASSERT_MESSAGE" macros
 *              defined in the header.
 *              The stack trace is dumped to the screen first, then the ASSERT
 *              information.  Finally, the data is dumped to EEPROM to be
 *              written to a FLASH log on the next reboot.  The routine prevent
 *              asserts on assert by setting a "reentered" flag on entry.  If
 *              the routine is re-entered, it immediately reboots without any
 *              processing.
 *
 * Parameters:  AssertStr - ptr to Assert str
 *              UserMsg -   the message to be used for the assert
 *              File -      file where the assert took place
 *              Line -      line where assert took place
 *              args -      list of variadic "..." arguments to be used for the assert.
 *
 * Returns:     none
 *
 * Notes:       THIS FUNCTION SHOULD NOT BE CALLED DIRECTLY.
 *              IT SHOULD BE CALLED BY USING MACRO'S ASSERT_MESSAGE OR ASSERT.
 *
 *              DOES NOT RETURN, SYSTEM IS REBOOTED!
 *
 *              WD MUST BE ENABLED
 *
 *****************************************************************************/
void AssertHandler( CHAR *AssertStr, CHAR *UserMsg, CHAR *File, INT32 Line, va_list args)
{
  CHAR       userString[ASSERT_MAX_LOG_SIZE];
  TIMESTRUCT Ts;
  static BOOLEAN reentered = FALSE;  

  __DIR(); // Interrupt level not needed, task will be disabled.  

  // Create a variable argument list with all params after param "Line"
  vsprintf(userString, UserMsg, args);
  va_end(args);
 
  if(!reentered)
  {
    reentered = TRUE;

    //Print out assert msg
    RTC_GetTime(&Ts);

    //Print stack and assert message to EE buffer
    snprintf( ExOutputBuf, ASSERT_MAX_LOG_SIZE, 
              "\r\n%04d/%02d/%02d %02d:%02d:%02d %s\r\n%s\r\n%s:%d\r\n",
              Ts.Year, Ts.Month, Ts.Day, Ts.Hour, Ts.Minute, Ts.Second,
              AssertStr, userString, File, Line);

    //Dump stack (also outputs to GSE inside AssertDump)
    AssertDump();

    Assert_WriteToEE( ExOutputBuf, ASSERT_MAX_LOG_SIZE);
  }

  WatchdogReboot(FALSE);
}

/******************************************************************************
 * Function:    Assert_UnhandledException
 *
 * Description: Grabs the Exception data buffer after Exception.s traps an
 *              unhandled interrupt and writes it to EEPROM.  This routine
 *              is expected to be called from Exception.s and will not return
 *              The reenterd flag prevents an exception-on-exception infinite
 *              loop.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       DOES NOT RETURN, SYSTEM IS REBOOTED!
 *              WD MUST BE ENABLED
 *
 *****************************************************************************/
void Assert_UnhandledException(void)
{
  TIMESTRUCT Ts;
  static BOOLEAN reentered = FALSE;
 
  __DIR(); // Interrupt level not needed, task will be disabled.
  if(!reentered)
  {
    reentered = TRUE;
    //Print out assert msg
    RTC_GetTime(&Ts);

    snprintf( ExOutputBuf, ASSERT_MAX_LOG_SIZE, 
      "%04d/%02d/%02d %02d:%02d:%02d \r\n%s\r\n",
      Ts.Year, Ts.Month, Ts.Day, Ts.Hour, Ts.Minute, Ts.Second,
      ReOutputBuf);

    Assert_WriteToEE( ExOutputBuf, ASSERT_MAX_LOG_SIZE);
  }

  WatchdogReboot(FALSE);
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    Assert_InitEE
 *
 * Description: Set the Assert EE buffer to the initial known state.  The
 *              routine sets the magic number and number of bytes in the
 *              buffer to zero
 *
 * Parameters:  none
 *
 * Returns:     TRUE:  Initialized the EEPROM successfully
 *              FALSE: Could not initialize the EEPROM
 *
 * Notes:       BOOLEAN return required for File Init in NvMgr
 *
 *****************************************************************************/
BOOLEAN Assert_InitEE(void)
{
  ASSERT_BUF_HDR AssertHdr; 
 
  //Set the assert buffer header to a valid magic number
  //the empty initial state and 
  AssertHdr.MagicNumber = ASSERT_FILE_MAGIC_NUMBER;
  AssertHdr.Enabled     = TRUE;
  AssertHdr.LogSize     = 0;
  AssertHdr.Read        = FALSE;
  
  NV_Write(NV_ASSERT_LOG,0,&AssertHdr,sizeof(AssertHdr));  
  return TRUE;
}



/******************************************************************************
 * Function:    Assert_WriteToEE
 *
 * Description: Write a block of data to the EEPROM assert log buffer.
 *              WriteNow is used b/c asserts are typically followed by
 *              a system reset.
 *              
 *
 * Parameters:  [in] data: pointer to the block of data to write to the EE
 *              [in] size: size of the data to write to the EE
 *
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void Assert_WriteToEE(void *data, size_t size)
{
 ASSERT_BUF_HDR AssertHdr;

  size = size < ASSERT_MAX_LOG_SIZE ? size : ASSERT_MAX_LOG_SIZE;
  if(IsEERdy)
  {
    NV_Read(NV_ASSERT_LOG,0,&AssertHdr,sizeof(AssertHdr));
    
    //Verify there is no data in the buffer waiting to be written to flash.
    if(AssertHdr.LogSize == 0)
    {
      //Write data first, then size.  This ensures the data is valid before
      //the size indicates there is data in the buffer.  Reset the "read"
      //flag to indicate this is a new assert log.
      if(SYS_OK == NV_WriteNow(NV_ASSERT_LOG, sizeof(AssertHdr),data, size))
      {
        AssertHdr.LogSize = size;
        AssertHdr.Read    = FALSE;
        if( SYS_OK != NV_WriteNow(NV_ASSERT_LOG,0,&AssertHdr,
                                  sizeof(AssertHdr)) )
        {
          GSE_DebugStr(NORMAL,TRUE,"ASSERT: CANNOT WRITE EE!");
        }
      }
      else
      {
        GSE_DebugStr(NORMAL,TRUE,"ASSERT: CANNOT WRITE EE!");
      }
    }
  }
  else
  {
    GSE_DebugStr(NORMAL,TRUE,"ASSERT: CANNOT WRITE EE!");
  }
}

/******************************************************************************
 *  MODIFICATIONS
 *    $History: Assert.c $
 * 
 * *****************  Version 27  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 * 
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 11/08/10   Time: 4:06p
 * Updated in $/software/control processor/code/drivers
 * SCR 979.  Unhandeled exception log missing data
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 10/02/10   Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 9/30/10    Time: 6:14p
 * Updated in $/software/control processor/code/drivers
 * SCR 907 Code Coverage Changes
 * 
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 6:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 707 - Coverage Mod WIN32 code exclusion
 * 
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 19  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 18  *****************
 * User: Contractor2  Date: 4/27/10    Time: 1:39p
 * Updated in $/software/control processor/code/drivers
 * SCR 187: Degraded mode. Moved watchdog reset loop to WatchDog.c
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:08p
 * Updated in $/software/control processor/code/drivers
 * SCR #70 Store/Restore interrupt level
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 4/05/10    Time: 10:01a
 * Updated in $/software/control processor/code/drivers
 * SCR #481 - add Vectorcast cmds to eliminate code coverage on code to
 * support WIN32 execution.
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:29p
 * Updated in $/software/control processor/code/drivers
 * SCR #279 Making NV filenames uppercase
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/drivers
 * SCR# 483 - Function Names
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 3/03/10    Time: 6:52p
 * Updated in $/software/control processor/code/drivers
 * Put a CR/LF in front of all assert messages so they start on their own
 * line.
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:07p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 6:13p
 * Updated in $/software/control processor/code/drivers
 * SCR# 456 - Assert cleanup
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 5:00p
 * Updated in $/software/control processor/code/drivers
 * SCR# 456 - Assert Processing cleanup
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:56p
 * Updated in $/software/control processor/code/drivers
 * SCR 279
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 1/13/10    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:22p
 * Updated in $/software/control processor/code/drivers
 * SCR 371
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:22p
 * Updated in $/software/control processor/code/drivers
 * Implementing SCR 172
 * Fix typos
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 9/23/09    Time: 10:49a
 * Updated in $/software/control processor/code/drivers
 * Added detail to some comment blocks
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 9/15/09    Time: 4:56p
 * Updated in $/software/control processor/code/drivers
 * Completed Assert and Exception requrement implimentation
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 9/08/09    Time: 6:32p
 * Created in $/software/control processor/code/drivers
 * First pass, writes assert message to EE and GSE.  
 * 
 *
 *****************************************************************************/
 
