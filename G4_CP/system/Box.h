#ifndef BOX_H
#define BOX_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         Box.h     
    
    Description:
    
    VERSION
    $Revision: 11 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "NVMgr.h"
#include "ClockMgr.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define BOX_NUM_OF_BOARDS 10
#define BOX_INFO_STR_LEN  16

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
#pragma pack(1)
typedef struct {
  INT8  Name[BOX_INFO_STR_LEN];               //Name of circuit card 
  INT8  SN[BOX_INFO_STR_LEN];                 //Serial number of card
  INT8  Rev[BOX_INFO_STR_LEN];                //Revision of card
}BOX_BOARD_INFO;

typedef struct {
  UINT32        MagicNumber;                  //Internal - set to BOX_MAGIC_NUMBER
                                              //           when file is valid
  INT8          SN[BOX_INFO_STR_LEN];         //Box Serial Number
  INT8          Rev[BOX_INFO_STR_LEN];        //Box Revision
  INT8          MfgDate[BOX_INFO_STR_LEN];    //Date of Manufacture
  INT8          RepairDate[BOX_INFO_STR_LEN]; //Last Box Repair Date
  BOX_BOARD_INFO  Board[BOX_NUM_OF_BOARDS];   //List of circuit cards in this 
}BOX_UNIT_INFO;                               //box


// Structure overlayed onto RTC RAM and EEPROM !
typedef struct {
  UINT32 PowerOnUsage;   // Box Power On Usage since Mfg / Re-Mfg (seconds) 
  UINT32 PowerOnCount;   // Box Power On Count since Mfg / Re-Mfg
}BOX_POWER_ON_INFO;   
#pragma pack()

typedef enum {
  POWERCOUNT_UPDATE_NOW,
  POWERCOUNT_UPDATE_BACKGROUND, 
  POWERCOUNT_UPDATE_MAX 
} POWERCOUNT_UPDATE_ACTION; 


#pragma pack(1)
typedef struct {
  SYS_APP_ID logReason;
}BOX_POWER_USAGE_RETRIEVE_LOG;
#pragma pack()



/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( BOXMGR_BODY )
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
EXPORT void Box_Init(void);
EXPORT void Box_GetSerno(INT8* SNStr);
EXPORT void Box_GetRev(INT8* RevStr);
EXPORT void Box_GetMfg(INT8* MfgStr);
EXPORT UINT32 Box_GetPowerOnTime(void);
EXPORT UINT32 Box_GetPowerOnCount(void);                                            

EXPORT void Box_PowerOn_SetStartupTime( TIMESTAMP ts );  
EXPORT void Box_PowerOn_InitializeUsage( void ); 
EXPORT void Box_PowerOn_StartTimeCounting( void ); 

EXPORT void Box_PowerOn_UpdateElapsedUsage( BOOLEAN bPadForTruncation, 
                                            POWERCOUNT_UPDATE_ACTION UpdateAction, 
                                            NV_FILE_ID nv_location ); 
                                            
#endif // BOX_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Box.h $
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed Unused functions
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR #613 Box Board array needs to hold at least 7 items
 * Increased max num of boards value to 10.
 * Changed magic number for file reset to new size.
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc Code review changes
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:50p
 * Updated in $/software/control processor/code/system
 * SCR #543 Fix spelling retreive -> retrieve
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 10/19/09   Time: 4:13p
 * Updated in $/software/control processor/code/system
 * Added externally callable functions for getting the power on count and
 * power on time.
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 10:27a
 * Updated in $/software/control processor/code/system
 * Add Revision Header Comment
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 275 Box Power On Usage Requirements
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:30a
 * Created in $/control processor/code/system
 * 
 * 
 *
 ***************************************************************************/
