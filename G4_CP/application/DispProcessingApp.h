#ifndef DISPLAY_PROCESSING_H
#define DISPLAY_PROCESSING_H

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        DispProcessingApp.h

    Description: Contains data structures related to the Display Processing 
                 Application 
    
    VERSION
      $Revision: 14 $  $Date: 5/23/16 3:34p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "alt_stdtypes.h"
#include "TaskManager.h"
#include "UartMgr.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define DISPLAY_INVALID_BUTTON_TIME 1000//Default Invalid button time = 1  sec.
#define DISPLAY_AUTO_ABORT_TIME     600 //Auto Abort Screen Time      = 10 min.
#define DISPLAY_FAIL_MS_DEFAULT     150 //Default Display Failure TO  = 150 ms.
#define PWCDISP_DISPLAY_HEALTH_WAIT_MAX 3   // The maximum number of 
                                            // consecutive D_HLTH packets 
                                            // containing nonzero values after
                                            // PBIT                   = 3  sec.
#define VERSION_NUMBER_INIT        "?.?"//The initialized value of the version
                                        // number status string.
#define PART_NUMBER_INIT              0 //The initialized value of the part
                                        // number status string.
#define DISPLAY_DEFAULT_DCRATE        0 //Double Click Rate Default
#define DBLCLICK_TIMEOUT             50 // default dbl click time in ticks. To
                                        // be combined with saved dblclk time.
#define MAX_SCREEN_SIZE              24 // Total sum of characters on lines 
                                        // one and two.
#define MAX_LINE_LENGTH              12 // Total sum of characters on one line.
#define MAX_CHAR_LENGTH              24 // Maximum string length of structure 
                                        // related strings
#define MAX_ACTIONS_COUNT            29 // Maximum number of actions
#define MAX_SCREEN_VARIABLE_COUNT    16 // Maximum number of screen variables
#define MAX_VARIABLES_PER_SCREEN      6 // Max number of variables per screen
#define MAX_SUBSTRING_SIZE           16 // The maximum debug temporary string 
                                        // size.
#define HOME_SCREEN                   1 // The designated Home Screen
#define DCRATE_CONVERSION            25 // Conversion from clicks to byte value
                                        // of DCRATE. (clicks to ms/2)
#define INVBUTTON_TIME_CONVERSION    10 // Conversion from ms to 10 * ms
#define AUTO_ABORT_CONVERSION       100 // Conversion from s to 10 * ms
#define NO_HS_TIMEOUT_CONVERSION    100 // Conversion from s to 10 *ms
#define VERSION_MAJOR_NUMBER          0 // The major number of the SW version
#define VERSION_MINOR_NUMBER          1 // The minor number of the SW version
#define BUTTON_STATE_COUNT            8 // Total number of possible button bits
#define DISCRETE_STATE_COUNT          8 // Total number of possible discretes
#define DISPLAY_DCRATE_MAX            5 // The maximum value (from 1 to 5) of
                                        // DCRATE on screen M21.
#define DISPLAY_VALID_BUTTONS        16 // The current total number of valid
                                        // push buttons.
#define D_HLTH_ACTIVE              0x00 // Display is transmitting packets with 
                                        // valid data.
#define D_HLTH_PBIT_ACTIVE         0x80 // Unit is performing the lamp test
#define D_HLTH_INOP_SIGNAL_FAULT   0xC3 // Unit not receiving discrete monitor
                                        // indication but is receiving packets
#define D_HLTH_COM_RX_FAULT        0xCC // Unit receiving discrete monitor 
                                        // indication, but no packets
#define D_HLTH_MON_INOP_FAULT      0xCF // Unit not receiving discrete monitor
                                        // indication and no packets
#define D_HLTH_DISPLAY_FAULT       0xFF // Unit has failed readback CBIT and 
                                        // the screen is blanked
#define DEFAULT_PART_NUMBER        0x01 // The default Part Number
#define DEFAULT_VERSION_NUMBER_MJR 0x01 // The default Major Version Number
#define DEFAULT_VERSION_NUMBER_MNR 0x00 // The default Minor Version Number
#define CHANGED_CHARACTER          0x2A
#define ACTION_ENUM_OFFSET          100 // The offset between actions and 
                                        // menu screen enumerator values

#define DISPLAY_CFG_DEFAULT               \
        DISPLAY_INVALID_BUTTON_TIME,      \
        DISPLAY_AUTO_ABORT_TIME,          \
        PWCDISP_DISPLAY_HEALTH_WAIT_MAX , \
        DISPLAY_FAIL_MS_DEFAULT
/******************************************************************************
                                 Package Typedefs
******************************************************************************/
/********************/
/*   Enumerators    */
/********************/

typedef enum
{
  M00, M01, M02, M03, M04,  
  M05, M06, M07, M08, M09, 
  M10, M11, M12, M13, M14, 
  M15, M16, M17, M18, M19, 
  M20, M21, M22, M23, M24, 
  M25, M26, M27, M28, M29, 
  M30,
  DISPLAY_SCREEN_COUNT,
  A00 = 100,
  A01, A02, A03, A04, A05,
  A06, A07, A08, A09, A10,
  A11, A12, A13, A14, A15,
  A16, A17, A18, A19, A20,
  A21, A22, A23, A24, A25,
  A26, A27, A28, NO_ACTION
}DISPLAY_SCREEN_ENUM;

typedef BOOLEAN (*SCREEN_ACTION)();

typedef enum
{
  RIGHT_BUTTON,
  LEFT_BUTTON,
  ENTER_BUTTON,
  UP_BUTTON,
  DOWN_BUTTON,
  UNUSED1_BUTTON,
  UNUSED2_BUTTON,
  EVENT_BUTTON,
  RIGHT_DBLCLICK,
  LEFT_DBLCLICK,
  ENTER_DBLCLICK,
  UP_DBLCLICK,
  DOWN_DBLCLICK,
  UNUSED1_DBLCLICK,
  UNUSED2_DBLCLICK,
  EVENT_DBLCLICK,
  DISPLAY_BUTTONS_COUNT,
  NO_PUSH_BUTTON_DATA
}DISPLAY_BUTTON_STATES;

typedef enum
{
  DISPLAY_LOG_TIMEOUT,
  DISPLAY_LOG_TRANSITION
}DISPLAY_LOG_RESULT_ENUM;

/********************************/
/*  Enumerator and Char Tables  */
/********************************/

typedef struct
{
  DISPLAY_BUTTON_STATES   id;
  CHAR                    name[MAX_CHAR_LENGTH];
  UINT16                  size;
}DISPLAY_BUTTON_TABLE, *DISPLAY_BUTTON_TABLE_PTR;

typedef struct
{
  CHAR* name;
}DISPLAY_CHAR_TABLE, *DISPLAY_CHAR_TABLE_PTR;

/********************************/
/*   Screen State Information   */
/********************************/
typedef struct
{
  CHAR  strInsert[MAX_CHAR_LENGTH];
  UINT8 variablePosition;
  UINT8 variableLength;
}DISPLAY_VARIABLE_TABLE, *DISPLAY_VARIABLE_TABLE_PTR;

typedef struct
{
  SCREEN_ACTION       action;
  DISPLAY_SCREEN_ENUM retTrue;
  DISPLAY_SCREEN_ENUM retFalse;
}DISPLAY_ACTION_TABLE, *DISPLAY_ACTION_TABLE_PTR;

typedef struct
{
  CHAR                      menuString[MAX_CHAR_LENGTH];
  DISPLAY_SCREEN_ENUM       dblUpScreen;
  DISPLAY_SCREEN_ENUM       dblDownScreen;
  DISPLAY_SCREEN_ENUM       dblLeftScreen;
  DISPLAY_SCREEN_ENUM       dblRightScreen;
  DISPLAY_SCREEN_ENUM       dblEnterScreen;
  DISPLAY_SCREEN_ENUM       dblEventScreen;
  DISPLAY_SCREEN_ENUM       upScreen;
  DISPLAY_SCREEN_ENUM       downScreen;
  DISPLAY_SCREEN_ENUM       leftScreen;
  DISPLAY_SCREEN_ENUM       rightScreen;
  DISPLAY_SCREEN_ENUM       enterScreen;
  DISPLAY_SCREEN_ENUM       eventScreen;
  DISPLAY_SCREEN_ENUM       noButtonInput;
  INT8                      var1;
  INT8                      var2;
  INT8                      var3;
  INT8                      var4;
  INT8                      var5;
  INT8                      var6;
}DISPLAY_STATE, *DISPLAY_STATE_PTR;
/********************/
/*      Config      */
/********************/
typedef struct
{
  UINT32 invalidButtonTime_ms; // Amount of time spent on screen M28
  UINT32 autoAbortTime_s;      // auto abort time for display screens
  UINT32 no_HS_Timeout_s;      // max invalid D_HLTH wait in MIF ticks
  UINT32 disc_Fail_ms;         // max data loss timeout for discreet validation
}DISPLAY_SCREEN_CONFIG, *DISPLAY_SCREEN_CONFIG_PTR;

/*********************
*       Debug        *
*********************/
typedef struct
{
  BOOLEAN               bDebug;      //Enable debug frame display
  BOOLEAN               bNewFrame;   //New frame = TX data checked 
  DISPLAY_BUTTON_STATES buttonInput; //Saved button state for debug
}DISPLAY_DEBUG, *DISPLAY_DEBUG_PTR;

/*************************************
  Display Processing App File Header
*************************************/
#pragma pack(1)
typedef struct
{
  UINT16      size;
  UINT32      invalidButtonTime_ms;
  UINT32      autoAbortTime_s;
  UINT32      no_HS_Timeout_s;
  UINT32      disc_Fail_ms;
}DISPLAY_FILE_HDR, *DISPLAY_FILE_HDR_PTR;
#pragma pack()

/*********************
*       Status       *
*********************/
typedef struct
{
  DISPLAY_SCREEN_ENUM    currentScreen;  // The current Screen on the Display
  DISPLAY_SCREEN_ENUM    previousScreen; // The Previous Screen on the Display
  DISPLAY_SCREEN_ENUM    nextScreen;     // The Next Screen on the Display
  UINT8     charString[MAX_SCREEN_SIZE]; // Current Character String on Display
  UINT8     displayHealth;  // Value of D_HLTH
  BOOLEAN   bNewData;       // New RX data available
  BOOLEAN   bPBITComplete;  // When true the UART can transmit packets and the 
                            // screen is permitted to transition based on the 
                            // button presses.
  BOOLEAN   bButtonEnable;  // Indicates that D_HLTH is 0x00 and the protocol 
                            // may continue to transmit.
  BOOLEAN   bAPACProcessing;// Used during frames M8-M11 to allow updates to 
                            // take place without the use of a button click
  DISPLAY_BUTTON_STATES buttonInput;// The interpreted command from the display
  BOOLEAN   bButtonReset;   // Is true when all button states are false.
  BOOLEAN   bInvalidButton; // If an invalid key is pressed this boolean will 
                            // allow the Screen to update despite the lack
                            // of button presses.
  BOOLEAN   bNewDebugData;
  UINT32    dispHealthTimer;// The Invalid Display Health Timer
  TIMESTAMP dispHealthTOStart; // The start of the D_HLTH Timeout
  UINT8     partNumber;     // Display Part Number
  UINT8     versionNumber;  // Display Software version
  CHAR      versionNumStr[MAX_SUBSTRING_SIZE];
  UINT32    lastFrameTime;
  TIMESTAMP lastFrameTS;
}DISPLAY_SCREEN_STATUS, *DISPLAY_SCREEN_STATUS_PTR;

/*******************************************
  Display Processing App Non-Volatile Data
*******************************************/
typedef struct
{
  INT8        lastDCRate;
}DISPLAY_APP_DATA, *DISPLAY_APP_DATA_PTR;

/*********************
*        Logs        *
*********************/
#pragma pack(1)
typedef struct
{
  DISPLAY_LOG_RESULT_ENUM  result;
  UINT16                   previousScreen;
  UINT16                   currentScreen;
  DISPLAY_BUTTON_STATES    buttonPressed;
}DISPLAY_APP_TRANSITION_LOG;

typedef struct
{
  UINT8                      d_HLTHcode; //The D_HLTH Hex code
  TIMESTAMP                  start_of_Timeout;
}DISPLAY_APP_DHLTH_TO_LOG;

typedef struct
{
  BOOLEAN buttonState[BUTTON_STATE_COUNT];
  BOOLEAN dblClkState[BUTTON_STATE_COUNT];
}DISPLAY_INVALID_BTN_LOG;
#pragma pack()
/*********************
*   Miscellaneous    *
*********************/
typedef struct
{
  TASK_INDEX      mgrTaskID;
} DISPLAY_DEBUG_TASK_PARAMS;

typedef struct
{
    TASK_INDEX    mgrTaskID;
} DISPLAY_APP_TASK_PARAMS;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT
#if defined ( DISPLAY_PROCESSING_BODY )
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

EXPORT void                      DispProcessingApp_Initialize(BOOLEAN bEnable);
EXPORT BOOLEAN                   DispProcessingApp_FileInit(void);
EXPORT void                      DispProcApp_DisableLiveStream(void);
#endif // DISPLAY_PROCESSING_H

/******************************************************************************
 *  MODIFICATIONS
 *    $History: DispProcessingApp.h $
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 5/23/16    Time: 3:34p
 * Updated in $/software/control processor/code/application
 * SCR 1335 - Fixed compiler warning
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 3/30/16    Time: 11:13a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Code Review Updates
 * 
 * *****************  Version 12  *****************
 * User: John Omalley Date: 3/23/16    Time: 8:50a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 2/25/16    Time: 5:03p
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Design Review Updates
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 2/10/16    Time: 5:36p
 * Updated in $/software/control processor/code/application
 * SCR 1303 Removed GSE Commands
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:11a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 2/01/16    Time: 9:34a
 * Updated in $/software/control processor/code/application
 * Fix compiler warning from NVMgr "a value of type "void (*) (void)"
 * cannot be used to initialize an entity of type "NV_INFO_INIT_FUNC". 
 * 
 * Update "void DispProcessingApp_FileInit(void)" to 
 * "BOOLEAN DispProcessingApp_FileInit(void)"
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 1/29/16    Time: 12:02p
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #18 Call DispProcessing Init thru APAC Mgr
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 1/29/16    Time: 9:42a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Display Processing Updates for DIO
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:40p
 * Updated in $/software/control processor/code/application
 * SCR 1312 - Updates from user feedback on navigation
 *
  * *****************  Version 4  *****************
 * User: John Omalley Date: 1/04/16    Time: 6:19p
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Updates from Performance Software
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 12/18/15   Time: 11:09a
 * Updated in $/software/control processor/code/application
 * SCR 1303 - Updates from PSW Contractor
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-12-02   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1303 Display App Updates from Jeremy.  
 *
 * *****************  Version 1  *****************
 * User: Jeremy Hester    Date: 9/22/2015    Time: 8:17a
 * Created in $software/control processor/code/system
 * Initial Check In.  Implementation of the Display Processing Application 
 *                    Requirements
 * 
 *
 *****************************************************************************/

