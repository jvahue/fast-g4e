#ifndef DISPLAY_PROCESSING_H
#define DISPLAY_PROCESSING_H

/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        90991

    File:        DispProcessingApp.h

    Description: Contains data structures related to the Display Processing 
                 Application 
    
    VERSION
      $Revision: 3 $  $Date: 12/18/15 11:09a $     

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
#define PWCDISP_DISPLAY_HEALTH_WAIT_MAX 3   // The maximum number of 
                                            // consecutive D_HLTH packets 
                                            // containing nonzero values after
                                            // PBIT                   = 3  sec.
#define DISPLAY_DEFAULT_DCRATE        0 //Double Click Rate Default

#define MAX_SCREEN_SIZE              24 // Total sum of characters on lines 
                                        // one and two
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
#define DISPLAY_VALID_BUTTONS         6 // The current total number of valid
                                        // push buttons.

#define DISPLAY_CFG_DEFAULT           \
        DISPLAY_DEFAULT_DCRATE,       \
        DISPLAY_INVALID_BUTTON_TIME,  \
        DISPLAY_AUTO_ABORT_TIME,      \
        PWCDISP_DISPLAY_HEALTH_WAIT_MAX  

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
/********************/
/*   Enumerators    */
/********************/

typedef enum
{
  M1,  M2,  M3,  M4,  M5, 
  M6,  M7,  M8,  M12, M13, 
  M14, M17, M19, M20, M21, 
  M23, M24, M25, M26, M27, 
  M28, M29, M30,
  DISPLAY_SCREEN_COUNT,
  A0 = 100,
  A1, A2, A3, A4, A5,
  A6, A7, A8, A9, A10,
  A11, A12, A13, A14, A15,
  A16, A17, A18, A19, A20,
  A21, A22, A23, A24, A25,
  A26, A27, A28, NO_ACTION
}DISPLAY_SCREEN_ENUM;

typedef enum
{
  INCOMING,
  OUTGOING
} DISPLAY_ACTION_DIRECTION;

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
  DISC_ZERO,
  DISC_ONE,
  DISC_TWO,
  DISC_THREE,
  DISC_FOUR,
  DISC_FIVE,
  DISC_SIX,
  DISC_SEVEN,
  DISPLAY_DISCRETES_COUNT
}DISPLAY_DISCRETES;

typedef enum
{
  DISPLAY_LOG_TIMEOUT,
  DISPLAY_LOG_TRANSITION,
  DISPLAY_LOG_MANUAL_ABORT,
  DISPLAY_LOG_PBIT_ACTIVE,
  DISPLAY_LOG_PBIT_PASS,
  DISPLAY_LOG_INOP_MON_FAULT,
  DISPLAY_LOG_INOP_SIGNAL_FAULT,
  DISPLAY_LOG_RX_COM_FAULT,
  DISPLAY_LOG_DISPLAY_FAULT,
  DISPLAY_LOG_DIAGNOSTIC_MODE,
  DISPLAY_LOG_D_HLTH_UNKNOWN
}DISPLAY_LOG_RESULT_ENUM;

/********************************/
/*  Enumerator and Char Tables  */
/********************************/

typedef struct
{
  UINT16   ID;
  CHAR     Name[24];
  UINT16   Size;
}DISPLAY_ENUM_TABLE, *DISPLAY_ENUM_TABLE_PTR;

typedef struct
{
  UINT8*   Name;
}DISPLAY_CHAR_TABLE, *DISPLAY_CHAR_TABLE_PTR;

/********************************/
/*   Screen State Information   */
/********************************/
typedef struct
{
  CHAR  strInsert[24];
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
  char                      menuString[24];
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
  DISPLAY_SCREEN_ENUM       NoButtonInput;
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
  INT8   defaultDCRATE;  // record of the current dbl click rate.
  UINT32 invalidButtonTime_ms; // Amount of time spent on screen M28
  UINT32 autoAbortTime_s; // auto abort time for display screens
  UINT32 no_HS_Timeout_s; // max invalid D_HLTH wait in MIF ticks
}DISPLAY_SCREEN_CONFIG, *DISPLAY_SCREEN_CONFIG_PTR;

/*********************
*       Debug        *
*********************/
typedef struct
{
  BOOLEAN     bDebug;      //Enable debug frame display
  BOOLEAN     bNewFrame; //New frame = TX data checked 
  DISPLAY_BUTTON_STATES buttonInput; //Saved button state for debug
}DISPLAY_DEBUG, *DISPLAY_DEBUG_PTR;

/*************************************
  Display Processing App File Header
*************************************/
#pragma pack(1)
typedef struct
{
  UINT16      size;
  INT8        defaultDCRATE;
  UINT32      invalidButtonTime_ms;
  UINT32      autoAbortTime_s;
  UINT32      no_HS_Timeout_s;
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
  BOOLEAN   bD_HLTHReset;   // Is true when D_HLTH byte is 0x00.
  BOOLEAN   bInvalidButton; // If an invalid key is pressed this boolean will 
                            // allow the Screen to update despite the lack
                            // of button presses.
  BOOLEAN   bNewDebugData;
  UINT32    invalidKeyTimer;// The Invalid key timer counter //TODO: Make this a static variable
  UINT32    dispHealthTimer;// The Invalid Display Health Timer
  UINT8     partNumber;     // Display Part Number
  UINT8     versionNumber;  // Display Software version
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
  UINT16                   currentScreen;
  UINT16                   buttonPressed;
  BOOLEAN                  bReset;
}DISPLAY_TRANSITION_LOG;

typedef struct
{
  DISPLAY_LOG_RESULT_ENUM    result;
  UINT32                     no_HS_Timeout_s;
  UINT8                      D_HLTHcode;    //The D_HLTH Hex code
  TIMESTAMP                  lastFrameTime;
}DISPLAY_DHLTH_LOG;
#pragma pack()
/*********************
*   Miscellaneous    *
*********************/
typedef struct
{
  TASK_INDEX      MgrTaskID;
} DISPLAY_DEBUG_TASK_PARAMS;

typedef struct
{
    TASK_INDEX    MgrTaskID;
} DISPLAY_APP_TASK_PARAMS;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#if defined (DISPLAY_PROCESSING_BODY)
// Screen index enumeration for modules which ref Display Processing App
USER_ENUM_TBL screenIndexType[] =
{ 
  {   "M1", M1  }, {"M2",  M2  }, {"M3",  M3  },
  {   "M4", M4  }, {"M5",  M5  }, {"M6",  M6  },
  {   "M7", M7  }, {"M8",  M8  }, {"M12", M12 },
  {   "M13",M13 }, {"M14", M14 }, {"M17", M17 }, 
  {   "M19",M19 }, {"M20", M20 }, {"M21",M21 }, 
  {   "M23",M23 }, {"M24", M24 }, {"M25",M25 }, 
  {   "M26",M26 }, {"M27", M27 }, {"M28",M28 }, 
  {   "M29",M29 }, {"M30", M30 },
  { "UNUSED", DISPLAY_SCREEN_COUNT }, 
  {   "A0", A0  }, {"A1",  A1  }, {"A2",  A2  },
  {   "A3", A3  }, {"A4",  A4  }, {"A5",  A5  },
  {   "A6", A6  }, {"A7",  A7  }, {"A8",  A8  },
  {   "A9", A9  }, {"A10", A10 }, {"A11", A11 },
  {   "A12",A12 }, {"A13", A13 }, {"A14", A14 },
  {   "A15",A15 }, {"A16", A16 }, {"A17", A17 },
  {   "A18",A18 }, {"A19", A19 }, {"A20", A20 },
  {   "A21",A21 }, {"A22", A22 }, {"A23", A23 },
  {   "A24",A24 }, {"A25", A25 }, {"A26", A26 },
  {   "A27",A27 }, {"A28", A28 },
  {   "NO_ACTION", NO_ACTION },   { NULL, 0 }
};
#else
// Export the triggerIndex enum table
EXPORT USER_ENUM_TBL screenIndexType[];
#endif

#if defined (DISPLAY_PROCESSING_BODY)
// Screen index enumeration for modules which ref Display Processing App
USER_ENUM_TBL buttonIndexType[] =
{ 
  { "RIGHT_BUTTON",        RIGHT_BUTTON          },
  { "LEFT_BUTTON",         LEFT_BUTTON           },
  { "ENTER_BUTTON",        ENTER_BUTTON          }, 
  { "UP_BUTTON",           UP_BUTTON             },
  { "DOWN_BUTTON",         DOWN_BUTTON           },
  { "UNUSED1_BUTTON",      UNUSED1_BUTTON        },
  { "UNUSED2_BUTTON",      UNUSED2_BUTTON        }, 
  { "EVENT_BUTTON",        EVENT_BUTTON          },
  { "RIGHT_DBLCLICK",      RIGHT_DBLCLICK        },
  { "LEFT_DBLCLICK",       LEFT_DBLCLICK         }, 
  { "ENTER_DBLCLICK",      ENTER_DBLCLICK        }, 
  { "UP_DBLCLICK",         UP_DBLCLICK           },
  { "DOWN_DBLCLICK",       DOWN_DBLCLICK         }, 
  { "UNUSED1_DBLCLICK",    UNUSED1_DBLCLICK      }, 
  { "UNUSED2_DBLCLICK",    UNUSED2_DBLCLICK      }, 
  { "EVENT_DBLCLICK",      EVENT_DBLCLICK        }, 
  { "UNUSED",              DISPLAY_BUTTONS_COUNT }, 
  { "NO_PUSH_BUTTON_DATA", NO_PUSH_BUTTON_DATA   },
  { NULL, 0 }
};
#else
// Export the triggerIndex enum table
EXPORT USER_ENUM_TBL buttonIndexType[];
#endif


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

EXPORT void                      DispProcessingApp_Initialize(void);
EXPORT void                      DispProcessingApp_Handler(void *pParam);
EXPORT BOOLEAN                   DispProcessingApp_FileInit(void);
EXPORT UINT16                    DispProcessingApp_ReturnFileHdr(UINT8 *dest, 
                                                         const UINT16 max_size,
                                                         UINT16 ch);
EXPORT void                      DispProcAppDebug_Task(void *param);
EXPORT DISPLAY_SCREEN_STATUS_PTR DispProcessingApp_GetStatus(void);
EXPORT DISPLAY_DEBUG_PTR         DispProcessingApp_GetDebug(void);
EXPORT DISPLAY_SCREEN_CONFIG_PTR DispProcessingApp_GetCfg(void);
EXPORT void                      DispProcApp_DisableLiveStream(void);
#endif // DISPLAY_PROCESSING_H

/******************************************************************************
 *  MODIFICATIONS
 *    $History: DispProcessingApp.h $
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

