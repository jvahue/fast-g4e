#ifndef UTILITY_H
#define UTILITY_H

/******************************************************************************
            Copyright (C) 2007-2011 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:         Utility.h

    Description:  Utility functions to perform simple tasks common to many
                  modules.

   VERSION
    $Revision: 28 $  $Date: 7/19/12 11:07a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "Assert.h"
#include "TTMR.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum {
  CRC_FUNC_SINGLE,
  CRC_FUNC_START,
  CRC_FUNC_NEXT,
  CRC_FUNC_END
}CRC_FUNC;

typedef enum {
  TO_START,
  TO_CHECK
}TIMEOUT_OP;

typedef enum
{
  CM_CRC16,
  CM_CSUM16
}CHECK_METHOD;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( UTILITY_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

#ifndef WIN32
#define _TRUNCATE -1
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void    Supper(INT8* str);
EXPORT void    Slower(INT8* str);
EXPORT void    StripString(INT8* Str);
EXPORT void    PadString(INT8* Str, UINT32 Len);
EXPORT BOOLEAN SuperStrcat(CHAR* dest, const CHAR* source, UINT32 len);
EXPORT BOOLEAN strncpy_safe(CHAR* dest, INT32 sizeDest, const CHAR* source, INT32 count );

EXPORT UINT32 ChecksumBuffer(const void* Buf, UINT32 Size, UINT32 ChkMask);
EXPORT UINT16 CRC16(const void* Buf, UINT32 Size);
EXPORT void CRC32(const void *data, UINT32 Size, UINT32* CRC, CRC_FUNC Func);
EXPORT UINT16 CRC_CCITT(const void* Buf, UINT32 Size);
EXPORT UINT16 CalculateCheckSum(CHECK_METHOD method, void* Addr, UINT32 Size );
EXPORT UINT32 BitToBitMask(UINT32 BitPos);
//EXPORT BOOLEAN CmpBytes ( UINT8 *pDest, UINT8 *pSrc, UINT32 nSize );

EXPORT BOOLEAN Timeout(TIMEOUT_OP Op, UINT32 Timeout, UINT32* StartTime);
EXPORT BOOLEAN TimeoutEx(TIMEOUT_OP Op, UINT32 Timeout, UINT32* StartTime, UINT32* lastRead);
EXPORT UINT32  UTIL_MinuteTimerDisplay    ( BOOLEAN bStartTimer );

EXPORT BOOLEAN GetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes);
EXPORT void    SetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes);
EXPORT void    SetBits(UINT32 mask[], INT32 maskSizeBytes,
                       UINT32 array[], INT32 arraySizeBytes);
EXPORT void    ResetBit(INT32 bitOffset, UINT32 array[], INT32 arraySizeBytes);
EXPORT void    ResetBits(UINT32 mask[], INT32 maskSizeBytes,
                         UINT32 array[], INT32 arraySizeBytes);
EXPORT BOOLEAN TestBits(UINT32 mask[], INT32 maskSizeBytes,
                        UINT32 data[], INT32 dataSizeBytes,
                        BOOLEAN exact );
EXPORT INT32 CompareVersions(const char* v1,const char* v2);

// Define to activate the sys.dumpmem command
// #define ENABLE_SYS_DUMP_MEM_CMD

// define to activate DumpMemory function only ( useful for programmer debugging.)
//#define ENABLE_DUMP_MEMORY

#ifdef ENABLE_SYS_DUMP_MEM_CMD
  #ifndef ENABLE_DUMP_MEMORY_FUNC
    #define ENABLE_DUMP_MEMORY_FUNC
  #endif
#endif


#ifdef ENABLE_DUMP_MEMORY_FUNC
EXPORT void DumpMemory(UINT8* addr, UINT32 size, CHAR* string);
#endif

#endif // UTILITY_H


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Utility.h $
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR 1107 Added BIT processing for 128 bit array
 * 
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Support trigger/event
 * 
 * *****************  Version 25  *****************
 * User: Contractor2  Date: 4/13/11    Time: 10:56a
 * Updated in $/software/control processor/code/system
 * SCR 1013: Code Coverage: Removed unused function BCDToBinaryConvert 
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 10/01/10   Time: 12:59p
 * Updated in $/software/control processor/code/system
 * SCR #915 Loop termination error in SuperStrcat
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 8/04/10    Time: 1:49p
 * Updated in $/software/control processor/code/system
 * SCR #765 Remove unused func UpdateCheckSum(). 
 * 
 * *****************  Version 22  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #636 add the ability to read/write EEPROM/RTC via the sys.g
 * 
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 6/08/10    Time: 5:55p
 * Updated in $/software/control processor/code/system
 * SCR #615 Showcfg/Long msg enhancement
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 5/24/10    Time: 6:57p
 * Updated in $/software/control processor/code/system
 * SCR #585 Cfg File CRC changes from load to load
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 3/29/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR 67 Changed type ret on CalculateCheckSum , added DumpMemory
 *
 * *****************  Version 17  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:59p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 11:24a
 * Updated in $/software/control processor/code/system
 * SCR# 467 - move minute timer display to Utitlities and rename TTMR to
 * UTIL in function name
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:58p
 * Updated in $/software/control processor/code/system
 * SCR 330
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 1/28/10    Time: 10:32a
 * Updated in $/software/control processor/code/system
 * SCR# 142: Error: Faulted EEPROM "Write In Progress" bit causes infinite
 * loop.
 *
 * Extended the Timeout function by creating TimeoutEx that returns the
 * last time read when the timeout was decalred so the caller may know how
 * much over the limit they were.
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 1/27/10    Time: 5:25p
 * Updated in $/software/control processor/code/system
 * SCR 142
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:44p
 * Updated in $/software/control processor/code/system
 * Added a Timeout() utility function
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 11/30/09   Time: 9:31a
 * Updated in $/software/control processor/code/system
 * Added XMODEM/CCITT compatable CRC routine
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 11/05/09   Time: 3:04p
 * Updated in $/software/control processor/code/system
 * Added SuperStrcat as part of scr # 320
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/23/09   Time: 9:35a
 * Updated in $/software/control processor/code/system
 * SCR #315 Add CmpBytes() routine to support CBIT / SEU Reg Processing
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 11/24/08   Time: 2:32p
 * Updated in $/control processor/code/system
 * Added CRC32 function and prototype
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 9/23/08    Time: 10:15a
 * Updated in $/control processor/code/system
 *
 *
 *************************************************************************/
