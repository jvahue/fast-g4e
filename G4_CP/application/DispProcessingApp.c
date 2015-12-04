#define DISPLAY_PROCESSING_BODY

/******************************************************************************
            Copyright (C) 2008-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        90991

    File:        DispProcessingApp.c

    Description: Contains all functions and data related to the Display  
                 Processing Application 
    
    VERSION
      $Revision: 2 $  $Date: 15-12-02 6:38p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "DispProcessingApp.h"
#include "PWCDispProtocol.h"
#include "UartMgr.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"
#include "APACMgr_Interface.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

typedef struct
{
  BOOLEAN buttonState[BUTTON_STATE_COUNT];
  BOOLEAN dblClickState[BUTTON_STATE_COUNT];
  BOOLEAN discreteState[DISCRETE_STATE_COUNT];
  UINT8   displayHealth;
  UINT8   partNumber;
  UINT8   versionNumber[2]; // 0 Index - Major Number
                            // 1 Index - Minor Number
}DISPLAY_RX_INFORMATION, *DISPLAY_RX_INFORMATION_PTR;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

static UARTMGR_PROTOCOL_DATA m_PWCDisp_TXData[PWCDISP_TX_ELEMENT_CNT_COMPLETE]=
{
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_UINT8, NULL, -1, -1 },
  { USER_TYPE_INT8,  NULL, -1, -1 }
};
static DISPLAY_VARIABLE_TABLE m_VariableTable[22] = 
{//                        strInsert  Pos Len
/* 0  CFG             */ { "",         7,  5 },
/* 1  CHART           */ { "",        23,  1 },
/* 2  PAC_STATUS      */ { "",        12, 12 },
/* 3  ENGINE          */ { "",         1,  1 },
/* 4  CATEGORY        */ { "",         6,  1 },
/* 5  NG_MARGIN       */ { "",         7,  4 },
/* 6  ENGINE          */ { "",        13,  1 },
/* 7  CATEGORY        */ { "",        18,  1 },
/* 8  ITT_MARGIN      */ { "",        20,  3 },
/* 9  ENGINE          */ { "",         6,  1 },
/*10  CATEGORY        */ { "",        12,  1 },
/*11  NG_MARGIN       */ { "",        14,  4 },
/*12  ERROR_1         */ { "",         0, 12 },
/*13  ERROR_2         */ { "",        12,  5 },
/*14  PAC_DATE        */ { "",         0, 11 },
/*15  PAC_TIME        */ { "",        12,  5 },
/*16  PAC_TIME_STATUS */ { "",         0, 12 },
/*17  PAC_VALUES      */ { "",        12, 12 },
/*18  DBL_CLICK_RATE  */ { "",        21,  3 },
/*19  ENGINE          */ { "",         7,  1 },
/*20  ENGINE          */ { "",        10,  1 },
/*21  DBL_CLICK_CHANGE*/ { "",        23,  1 }
};

static UARTMGR_PROTOCOL_DATA  m_PWCDisp_RXData[PWCDISP_RX_ELEMENT_COUNT];
static UARTMGR_PROTOCOL_DATA  m_DispProcApp_Request[DISPLAY_APP_REQUESTS_COUNT];
static DISPLAY_SCREEN_STATUS  m_DispProcApp_Status;
static DISPLAY_SCREEN_CONFIG  m_DispProcApp_Cfg;
static DISPLAY_RX_INFORMATION m_DispProcRX_Info;
static DISPLAY_APP_DATA       m_DispProcApp_AppData;
static DISPLAY_DEBUG          m_DispProcApp_Debug;
static UINT16                 auto_Abort_Timer     = 0;
static UINT16                 invalid_Button_Timer = 0;
static UINT16                 dblClickTimer        = 0;
static BOOLEAN                dblClickFlag         = FALSE;

static DISPLAY_DEBUG_TASK_PARAMS DispProcAppDisplayDebugBlock;
static DISPLAY_APP_TASK_PARAMS   DispProcAppBlock;
static INT8 scaledDCRATE = 0; // The scaled value of DCRATE for M21
static INT8 savedDCRATE;      // The saved DCRATE setting on the display
static DISPLAY_BUTTON_STATES previousButtonState = NO_PUSH_BUTTON_DATA;
static UINT16 invalidButtonTime_Converted; //converted to ticks
static UINT16 AutoAbortTime_Converted;     //converted to ticks
static UINT16 no_HS_Timeout_Converted;     //converted to ticks
static UINT16 dblClickTimeout = 50;// default double click time in ticks.
                                   // to be combined with saved dblclk time.
static DISPLAY_ENUM_TABLE ValidButtonTable[DISPLAY_VALID_BUTTONS] =
{
  {DOWN_BUTTON,   "", 1},
  {UP_BUTTON,     "", 1},
  {ENTER_BUTTON,  "", 1},
  {LEFT_BUTTON,   "", 1},
  {RIGHT_BUTTON,  "", 1},
  {LEFT_DBLCLICK, "", 2}
};

static UINT8 m_Str[40];

/********************************/
/*      Enumerator Tables       */
/********************************/

DISPLAY_CHAR_TABLE DisplayScreenTbl[DISPLAY_SCREEN_COUNT] =
{
  { "M1 " }, { "M2 " }, { "M3 " }, { "M4 " }, { "M5 " },
  { "M6 " }, { "M7 " }, { "M8 " }, { "M12" }, { "M13" },
  { "M14" }, { "M17" }, { "M19" }, { "M20" }, { "M21" }, 
  { "M23" }, { "M24" }, { "M25" }, { "M26" }, { "M27" }, 
  { "M28" }, { "M29" }, { "M30" }
};

DISPLAY_CHAR_TABLE DisplayButtonTbl[DISPLAY_BUTTONS_COUNT + 2] =
{
  { "RIGHT_BUTTON    " }, { "LEFT_BUTTON     " }, { "ENTER_BUTTON    " },
  { "UP_BUTTON       " }, { "DOWN_BUTTON     " }, { "UNUSED1_BUTTON  " },
  { "UNUSED2_BUTTON  " }, { "EVENT_BUTTON    " }, { "RIGHT_DBLCLICK  " },
  { "LEFT_DBLCLICK   " }, { "ENTER_DBLCLICK  " }, { "UP_DBLCLICK     " },
  { "DOWN_DBLCLICK   " }, { "UNUSED1_DBLCLICK" }, { "UNUSED2_DBLCLICK" },
  { "EVENT_DBLCLICK  " }, { "BUTTON_COUNT    " }, { "NO_PUSH_BUTTON  " },
};

DISPLAY_CHAR_TABLE DisplayDiscreteTbl[DISPLAY_DISCRETES_COUNT] =
{
  { "DISC_ZERO " }, { "DISC_ONE  " }, { "DISC_TWO  " }, { "DISC_THREE" },
  { "DISC_FOUR " }, { "DISC_FIVE " }, { "DISC_SIX  " }, { "DISC_SEVEN" },
};
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

static void                  Init_SpecialCharacterStrings(void);
static BOOLEAN               DispProcessingApp_GetNewDisplayData( void );
static DISPLAY_STATE_PTR     DispProcessingApp_AutoAbortValid(DISPLAY_STATE_PTR pScreen,
	                                                          UINT16 buttonInput);
static DISPLAY_SCREEN_ENUM   PerformAction(DISPLAY_SCREEN_ENUM nextAction);
static void                  DispProcessingApp_D_HLTHcheck(UINT8 dispHealth, 
                                                           BOOLEAN bNewData);
static void                  DispProcessingApp_D_HLTHResult(UINT8 dispHealth, 
                                                            UINT32 no_HS_Timeout_s);
static void                  DispProcessingApp_UpdateProtocol(void);
static BOOLEAN               InvalidButtonInput();
static BOOLEAN               GetConfig();
static BOOLEAN               RecallDate();
static BOOLEAN               RecallNextDate();
static BOOLEAN               RecallPreviousDate();
static BOOLEAN               RecallPAC();
static BOOLEAN               RecallNextPAC();
static BOOLEAN               RecallPreviousPAC();
static BOOLEAN               SavedDoubleClick();
static BOOLEAN               IncDoubleClick();
static BOOLEAN               DecDoubleClick();
static BOOLEAN               SaveDoubleClick();
static BOOLEAN               ResetPAC();
static BOOLEAN               CorrectSetup();
static BOOLEAN               ValidateMenu();
static BOOLEAN               RunningPAC();
static BOOLEAN               PACResult();
static BOOLEAN               PACSuccess();
static BOOLEAN               ErrorMessage();
static BOOLEAN               AquirePACData();
static BOOLEAN               PACDecision();
static BOOLEAN               PowerSelect();
static void                  DispProcessingApp_SendNewMonitorData( char characterString[] );
static DISPLAY_BUTTON_STATES DispProcessingApp_GetDisplayButtonInput(BOOLEAN bNewData);
static void                  FormatCharacterString(DISPLAY_STATE_PTR pScreen);
static DISPLAY_SCREEN_ENUM   GetNextState(DISPLAY_BUTTON_STATES button, 
                                          BOOLEAN thisScreen,
                                          DISPLAY_STATE_PTR pParamScreen);
static void                  NoButtonData(void);
static void                  DispProcessingApp_CreateTransLog(DISPLAY_LOG_RESULT_ENUM result,
                                                              BOOLEAN reset);
static void                  DispProcessingApp_RestoreAppData(void);
#include "DispProcessingUserTables.c"

static DISPLAY_STATE MenuStateTbl[DISPLAY_SCREEN_COUNT] =
{
//Menu  Menu String                 DblClkUP DblClkDOWN DblClkLEFT DblClkRIGHT DblClkENTER DblClkEVENT UP   DOWN LEFT  RIGHT ENTER EVENT None   Var: 1   2   3   4   5   6
/*M1 */{"TO BE INITIALIZED       ", A0,      A0,         A0,       A0,         A0,         A0,         A0,   A0,  M4,   M2,   A0,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M2 */{"TO BE INITIALIZED       ", A0,      A0,         A0,       A0,         A0,         A0,         A0,   A0,  M1,   M3,   A1,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M3 */{"TO BE INITIALIZED       ", A0,      A0,         A0,       A0,         A0,         A0,         A0,   A0,  M2,   M4,   A2,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M4 */{"TO BE INITIALIZED       ", A0,      A0,         A0,       A0,         A0,         A0,         A0,   A0,  M3,   M1,   A8,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M5 */{"CONFRM 11111CHRT ISSUE 2", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A12,   A0,   M6,  A0,   NO_ACTION,  0,  1, -1, -1, -1, -1},
/*M6 */{"HTR/COND SOVOFF CONFIRM?", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A14,   A0,  M29,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1}, 
/*M7 */{"SEL ENGINE  START PAC?  ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  M6,   A0,  A15,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M8 */{"RUNNING PAC 111111111111", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,   A0,  A0,   A17,        2, -1, -1, -1, -1, -1},
/*M12*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,  A22,  A0,   NO_ACTION,  3,  4,  5,  8,  6,  7},
/*M13*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A23,  A23,  A27,  A0,   NO_ACTION, 20, 10, 11,  8, -1, -1},
/*M14*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A22,  A22,  A27,  A0,   NO_ACTION, 20, 10, 11,  8, -1, -1},
/*M17*/{"11111111111122222 RETRY?", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,  M29,  A0,   A20,       12, 13, -1, -1, -1, -1},
/*M19*/{"11111111111 22222 UTC   ", A0,      A0,        A13,       A0,         A0,         A0,         A3,   A4,  M3,   A0,   A5,  A0,   NO_ACTION, 14, 15, -1, -1, -1, -1},
/*M20*/{"11111 222 334 55555 6666", A0,      A0,        A13,       A0,         A0,         A0,         A6,   A7,  A2,   A0,   A0,  A0,   NO_ACTION, 16, 17, -1, -1, -1, -1},
/*M21*/{"DBL CLICK   Rate     112", A0,      A0,        A13,       A0,         A0,         A0,         A9,  A10,  M4,   A0,  A11,  A0,   NO_ACTION, 18, 21, -1, -1, -1, -1},
/*M23*/{"PRFRM MANUALPAC   READY?", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,  A17,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M24*/{"VALIDATE PACTO COMPUTED?", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,  A25,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M25*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0,  A0,   A0,  A26,  A0,   NO_ACTION,  3,  4,  5,  8,  6,  7},
/*M26*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A26,  A26,  A27,  A0,   NO_ACTION,  9, 10, 11,  8, -1, -1},
/*M27*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, A25,  A25,  A27,  A0,   NO_ACTION, 19, 10, 11,  8, -1, -1},
/*M28*/{"VALID KEYS:             ", A0,      A0,         A0,       A0,         A0,         A0,         A0,   A0,  A0,   A0,   A0,  A0,   A0,        -1, -1, -1, -1, -1, -1},
/*M29*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, M30,  M30,  A28,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M30*/{"TO BE INITIALIZED       ", A0,      A0,        A13,       A0,         A0,         A0,         A0,   A0, M29,  M29,  A28,  A0,   NO_ACTION, -1, -1, -1, -1, -1, -1},
};

static DISPLAY_ACTION_TABLE m_ActionTable[29] =
{
//A#    Function,           Return True, Return False
/*A0 */{InvalidButtonInput, NO_ACTION,   NO_ACTION},
/*A1 */{GetConfig,           M5,          M5      },
/*A2 */{RecallDate,         M19,         M19      },
/*A3 */{RecallNextDate,      A2,          A2      },
/*A4 */{RecallPreviousDate,  A2,          A2      },
/*A5 */{RecallPAC,          M20,         M20      },
/*A6 */{RecallNextPAC,       A5,          A5      },
/*A7 */{RecallPreviousPAC,   A5,          A5      },
/*A8 */{SavedDoubleClick,   M21,         M21      },
/*A9 */{IncDoubleClick,      A8,          A8      },
/*A10*/{DecDoubleClick,      A8,          A8      },
/*A11*/{SaveDoubleClick,     A8,          A8      },
/*A12*/{ResetPAC,            M2,          M2      },
/*A13*/{ResetPAC,            M1,          M1      },
/*A14*/{ResetPAC,            A1,          A1      },
/*A15*/{CorrectSetup,       A16,         A20      },
/*A16*/{ValidateMenu,       M23,         A17      },
/*A17*/{RunningPAC,         A18,          M8      },
/*A18*/{PACResult,          A19,         A20      },
/*A19*/{PACSuccess,         M24,         A21      },
/*A20*/{ErrorMessage,       M17,         M17      },
/*A21*/{AquirePACData,      M12,         M12      },
/*A22*/{AquirePACData,      M13,         M13      },
/*A23*/{AquirePACData,      M14,         M14      },
/*A24*/{AquirePACData,      M25,         M25      },
/*A25*/{AquirePACData,      M26,         M26      },
/*A26*/{AquirePACData,      M27,         M27      },
/*A27*/{PACDecision,        M29,         M29      },
/*A28*/{PowerSelect,         M7,          M7      }  
};
/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    DispProcessingApp_Initialize
 *
 * Description: Initializes the Display Processing Application functionality 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void DispProcessingApp_Initialize(void)
{  
  DISPLAY_SCREEN_STATUS_PTR pStatus = 
      (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;
  
  // Local Data
  TCB TaskInfo, DebugTaskInfo;
  
  // Sets both Current and previous screen to M1 and 
  // both bNewData and bPBITComplete to FALSE
  memset(&m_DispProcApp_Status, 0, sizeof(m_DispProcApp_Status));
  memset(&m_DispProcRX_Info,    0, sizeof(m_DispProcRX_Info)   );
  memset(&m_DispProcApp_Cfg,    0, sizeof(m_DispProcApp_Cfg)   );
  m_DispProcApp_Status.buttonInput = NO_PUSH_BUTTON_DATA;

  User_AddRootCmd(&DispProcAppRootTblPtr);

  memcpy(&m_DispProcApp_Cfg, &CfgMgr_RuntimeConfigPtr()->DispProcAppConfig,
      sizeof(m_DispProcApp_Cfg));

  // Restore App Data
  DispProcessingApp_RestoreAppData();
  savedDCRATE = m_DispProcApp_AppData.lastDCRate;
  // Assign the Invalid button time to the APAC Manager
  APACMgr_IF_InvldDelayTimeOutVal(&m_DispProcApp_Cfg.invalidButtonTime_ms);
  // Convert configurable times to ticks.
  invalidButtonTime_Converted    = m_DispProcApp_Cfg.invalidButtonTime_ms 
                                    / INVBUTTON_TIME_CONVERSION;
  AutoAbortTime_Converted        = m_DispProcApp_Cfg.autoAbortTime_s 
                                   * AUTO_ABORT_CONVERSION;
  no_HS_Timeout_Converted        = m_DispProcApp_Cfg.no_HS_Timeout_s 
                                   * NO_HS_TIMEOUT_CONVERSION;

  m_DispProcApp_Debug.bDebug     = FALSE;
  m_DispProcApp_Debug.bNewFrame  = FALSE;
  
  // Initialize Character Strings that require specific formatting.
  Init_SpecialCharacterStrings();
  
  m_DispProcRX_Info.partNumber       = 0x01;
  m_DispProcRX_Info.versionNumber[0] = 0x01; 
  m_DispProcRX_Info.versionNumber[1] = 0x00;
  
  // Update Status
  CM_GetTimeAsTimestamp((TIMESTAMP *) &pStatus->lastFrameTS);
  pStatus->lastFrameTime = CM_GetTickCount();

  // Create Display Processing App Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),
               "Display Processing Application", _TRUNCATE);
  TaskInfo.TaskID         = DispProcApp_Handler;
  TaskInfo.Function       = DispProcessingApp_Handler;
  TaskInfo.Priority       = taskInfo[DispProcApp_Handler].priority;
  TaskInfo.Type           = taskInfo[DispProcApp_Handler].taskType;
  TaskInfo.modes          = taskInfo[DispProcApp_Handler].modes;
  TaskInfo.MIFrames       = taskInfo[DispProcApp_Handler].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[DispProcApp_Handler].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[DispProcApp_Handler].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &DispProcAppBlock;
  TmTaskCreate(&TaskInfo);
  
  // Create Display Processing App Display Task
  memset(&DebugTaskInfo, 0, sizeof(DebugTaskInfo));
  strncpy_safe(DebugTaskInfo.Name, sizeof(DebugTaskInfo.Name),
               "Display Processing Debug",_TRUNCATE);
  DebugTaskInfo.TaskID         = DispProcApp_Debug;
  DebugTaskInfo.Function       = DispProcAppDebug_Task;
  DebugTaskInfo.Priority       = taskInfo[DispProcApp_Debug].priority;
  DebugTaskInfo.Type           = taskInfo[DispProcApp_Debug].taskType;
  DebugTaskInfo.modes          = taskInfo[DispProcApp_Debug].modes;
  DebugTaskInfo.MIFrames       = taskInfo[DispProcApp_Debug].MIFframes;
  DebugTaskInfo.Rmt.InitialMif = taskInfo[DispProcApp_Debug].InitialMif;
  DebugTaskInfo.Rmt.MifRate    = taskInfo[DispProcApp_Debug].MIFrate;
  DebugTaskInfo.Enabled        = TRUE;
  DebugTaskInfo.Locked         = FALSE;
  DebugTaskInfo.pParamBlock    = &DispProcAppDisplayDebugBlock;
  TmTaskCreate (&DebugTaskInfo);
}


/******************************************************************************
 * Function:    DispProcessingApp_Handler
 *
 * Description: Processes the decoded data from the PWC Display Protocol and 
 *              provides messages to be encoded for transmission. 
 *
 * Parameters:  None
 *
 * Returns:     None
 *              
 * Notes:       None
 *
 *****************************************************************************/
void DispProcessingApp_Handler(void *pParam)
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  BOOLEAN                   *bNewData;
  DISPLAY_STATE_PTR          pScreen;
  
  pStatus  = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  bNewData = (BOOLEAN*)&pStatus->bNewData;
  pScreen  = (DISPLAY_STATE_PTR)(&(MenuStateTbl[0]) + pStatus->currentScreen);
  
  // Look for new RX data
  *bNewData = DispProcessingApp_GetNewDisplayData();
  
  // Get available button input
  pStatus->buttonInput = DispProcessingApp_GetDisplayButtonInput(*bNewData);

  // Handle the Auto Abort timer and the Manual return to M1.
  pScreen = DispProcessingApp_AutoAbortValid(pScreen, pStatus->buttonInput);

  //Process MenuScreen based on button input received by the protocol
  pStatus->nextScreen = GetNextState(pStatus->buttonInput,TRUE, NULL);
  if (pStatus->buttonInput != NO_PUSH_BUTTON_DATA)
  {
    DispProcessingApp_CreateTransLog(DISPLAY_LOG_TRANSITION, FALSE);
  }
  if (pStatus->nextScreen != NO_ACTION)
  {// Update Previous screen
    pStatus->previousScreen = (pStatus->currentScreen != M28) ?
        pStatus->currentScreen : pStatus->previousScreen;
    while(pStatus->nextScreen >= 100)
    { // If the next screen requires an action process the action.
      pStatus->nextScreen = PerformAction(pStatus->nextScreen);
    }
    // Update status and format the new screen.
    pStatus->currentScreen = pStatus->nextScreen;
    pScreen = (DISPLAY_STATE_PTR)(&(MenuStateTbl[0]) + pStatus->currentScreen);
    FormatCharacterString(pScreen);
  }

  pStatus->bInvalidButton = (pStatus->currentScreen == M28) ? TRUE : FALSE;
  memcpy(pStatus->charString,pScreen->menuString,MAX_SCREEN_SIZE);
  // Update the Debug display if there is any new significant RX data.
  if (m_DispProcApp_Debug.bNewFrame != TRUE)
  {
    m_DispProcApp_Debug.bNewFrame = pStatus->bNewDebugData;
    m_DispProcApp_Debug.buttonInput = pStatus->buttonInput;
  }
  pStatus->bNewDebugData = FALSE;

  // Deliver special behavioural requests to the protocol from the app
  DispProcessingApp_UpdateProtocol();
  
  // Update the Protocol with new character string and DCRATE
  DispProcessingApp_SendNewMonitorData(pScreen->menuString);

  // Update Status
  CM_GetTimeAsTimestamp((TIMESTAMP *)&pStatus->lastFrameTS);
  pStatus->lastFrameTime = CM_GetTickCount();
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/******************************************************************************
 * Function:    Init_SpecialCharacterStrings
 *
 * Description: Function initially formats all character strings containing 
 *              special symbols.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
void Init_SpecialCharacterStrings()
{
  DISPLAY_STATE_PTR      pScreen;
  DISPLAY_ENUM_TABLE_PTR pButtons;
  UINT16                 i;
  
  pScreen  = (DISPLAY_STATE_PTR) &MenuStateTbl[0];
  pButtons = (DISPLAY_ENUM_TABLE_PTR) &ValidButtonTable[0];
  
  snprintf((pScreen + M1)->menuString,  sizeof((pScreen + M1)->menuString), 
           "ETM ACTIVE  %c%c CMD?     ",  0x04, 0x02);
  snprintf((pScreen + M2)->menuString,  sizeof((pScreen + M2)->menuString), 
           "ETM ACTIVE  %c%c RUN PAC? ",  0x04, 0x02);
  snprintf((pScreen + M3)->menuString,  sizeof((pScreen + M3)->menuString), 
           "ETM ACTIVE  %c%c RCL PAC? ",  0x04, 0x02);
  snprintf((pScreen + M4)->menuString,  sizeof((pScreen + M4)->menuString), 
           "ETM ACTIVE  %c%c SETTINGS?",  0x04, 0x02);
  snprintf((pScreen + M12)->menuString, sizeof((pScreen + M12)->menuString) +1,
	   "E1NG  23333%cE4ITT 5 666C",   0x25);
  snprintf((pScreen + M13)->menuString, sizeof((pScreen + M13)->menuString) +1,
	   "ACPT PAC E1?2 3444%c 555C",   0x25);
  snprintf((pScreen + M14)->menuString, sizeof((pScreen + M14)->menuString) +1,
	   "RJCT PAC E1?2 3444%c 555C",   0x25);
  snprintf((pScreen + M25)->menuString, sizeof((pScreen + M25)->menuString) +1,
	   "E1NG  23333%cE4ITT 5 666C",   0x25);
  snprintf((pScreen + M26)->menuString, sizeof((pScreen + M26)->menuString) +1,
	   "VLID E1APAC?2 3444%c 555C",   0x25);
  snprintf((pScreen + M27)->menuString, sizeof((pScreen + M27)->menuString) +1,
	   "INVLD E1PAC?2 3444%c 555C",   0x25);
  snprintf((pScreen + M29)->menuString,(sizeof((pScreen + M29)->menuString)+1),
       "100%cNr NORMLCONFIRM?  %c%c", 0x25, 0x04, 0x02);
  snprintf((pScreen + M30)->menuString,(sizeof((pScreen + M30)->menuString)+1),
	   "102%cNr CAT-ACONFIRM?  %c%c", 0x25, 0x04, 0x02);

  // Format stings for Invalid Push Button Screen
  for (i = 0; i < DISPLAY_VALID_BUTTONS; i++)
  {
    switch (pButtons->ID)
    {
      case RIGHT_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "%c", 0x02);
        break;
      case LEFT_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "%c", 0x04);
        break;
      case ENTER_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "%c", 0x05);
        break;
      case UP_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "%c", 0x01);
        break;
      case DOWN_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "%c", 0x03);
        break;
      case EVENT_BUTTON:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "E");
        break;
      case RIGHT_DBLCLICK:
        snprintf((char *)pButtons->Name, sizeof(pButtons->Name), "%c%c", 0x02,
                 0x02);
        break;
      case LEFT_DBLCLICK:
        snprintf((char *)pButtons->Name, sizeof(pButtons->Name), "%c%c", 0x04,
                 0x04);
        break;
      case ENTER_DBLCLICK:
        snprintf((char *)pButtons->Name, sizeof(pButtons->Name), "%c%c", 0x05,
                 0x05);
        break;
      case UP_DBLCLICK:
        snprintf((char *)pButtons->Name, sizeof(pButtons->Name), "%c%c", 0x01,
                 0x01);
        break;
      case DOWN_DBLCLICK:
        snprintf((char *)pButtons->Name, sizeof(pButtons->Name), "%c%c", 0x03,
                 0x03);
        break;
      case EVENT_DBLCLICK:
        snprintf((char *)pButtons->Name, pButtons->Size + 1, "EE");
        break;
    }
    pButtons++;
  }
}

/******************************************************************************
 * Function:    DispProcessingApp_GetNewDisplayData
 *
 * Description: Utility function that acquires state data from the PWC Display 
 *              protocol and saves it to a manageable structure. Data remains
 *              the same if no new information is received.
 *
 * Parameters:  None
 *
 * Returns:     BOOLEAN  bNewData - Returns TRUE if there is new RX Data from 
 *                                  the PWC Display Protocol 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN DispProcessingApp_GetNewDisplayData( void )
{
  BOOLEAN bNewData, bButtonReset;
  UINT16 i;
  UINT8 tempChar;
  UARTMGR_PROTOCOL_DATA_PTR  pData;
  DISPLAY_RX_INFORMATION_PTR pDest;
  DISPLAY_SCREEN_STATUS_PTR  pStatus;

  pData    = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_RXData[0];
  pDest    = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  pStatus  = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  bNewData = PWCDispProtocol_Read_Handler(pData, 
                                          UARTMGR_PROTOCOL_PWC_DISPLAY, 
                                          RX_PROTOCOL, 
                                          PWCDISP_RX_ELEMENT_COUNT);
  bButtonReset = TRUE;
  
  if(bNewData == TRUE)
  {
    // Check to see if all new button presses are FALSE and update bButtonReset
    for(i = 1; i <= 12; i++)
    {
      if((BOOLEAN)(pData + i)->data.Int != FALSE)
      {
        bButtonReset = FALSE;
      }
    }
    
    // If all previous button states were FALSE then accept new presses
    if(pStatus->bButtonReset == TRUE && bButtonReset == FALSE)
    {
      for (i = 0; i < BUTTON_STATE_COUNT; i++)
      {
        if( i > 4)
        {
          if(i == 7)
          {
            pDest->buttonState[i]   = 
              (BOOLEAN)(pData + BUTTONSTATE_EVENT)->data.Int;
            pDest->dblClickState[i] = 
              (BOOLEAN)(pData + DOUBLECLICK_EVENT)->data.Int;
          }
        }
        else
        {
          pDest->buttonState[i]   = 
            (BOOLEAN)(pData + BUTTONSTATE_RIGHT + i)->data.Int;
          pDest->dblClickState[i] = 
            (BOOLEAN)(pData + DOUBLECLICK_RIGHT + i)->data.Int;
        }
      }
    }
    else
    {                                                                          
      // There were button states set to true for the last set of data received
      // Ignore all button presses.                                            
      NoButtonData();                                                      
    }                                                                          
                                                                               
    for(i = 0; i < DISPLAY_DISCRETES_COUNT; i++)                               
    {
      if (pDest->discreteState[i] != 
          (BOOLEAN)(pData + DISCRETE_ZERO + i)->data.Int)
      {
        // Update Debug Display if 
        pStatus->bNewDebugData = TRUE;
      }
      pDest->discreteState[i] = (BOOLEAN)(pData + DISCRETE_ZERO + i)->data.Int;
    }
    
    pDest->displayHealth = (UINT8)(pData + DISPLAY_HEALTH)->data.Int;
    
    // Update button reset status
    pStatus->bButtonReset = bButtonReset;
    
    pDest->partNumber = (UINT8)((pData + PART_NUMBER)->data.Int);
    pStatus->partNumber = pDest->partNumber;
    
    pDest->versionNumber[VERSION_MAJOR_NUMBER] = 
     (UINT8) ((pData + VERSION_NUMBER)->data.Int >> 4);

    tempChar = (UINT8)((pData + VERSION_NUMBER)->data.Int << 4);
    
    pDest->versionNumber[VERSION_MINOR_NUMBER] = (UINT8)tempChar >> 4;
    pStatus->versionNumber = (pData + VERSION_NUMBER)->data.Int;
  }
  // As soon as D_HLTH is received as 0x00 for the first time, Display Pbit is
  // considered to be completed and the protocol is allowed to transmit.
  // When this is complete an invalid D_HLTH timer will increment for 
  // nonzero D_HLTH bytes
  DispProcessingApp_D_HLTHcheck(pDest->displayHealth, bNewData);

  return (bNewData);
}

/******************************************************************************
* Function:    DispProcessingApp_AutoAbortValid
*
* Description: Function controls the auto abort timer, incrementing it every 
*              10 msec that no buttons are pressed and resetting the screen
*              to M1 when expired. 
*
* Parameters:  pScreen - Pointer to the screen to be updated if necessary.
*              buttonInput - Value of the current input.
*
* Returns:     None
*
* Notes:       Needs more cowbell.
*
*****************************************************************************/
static
DISPLAY_STATE_PTR DispProcessingApp_AutoAbortValid(DISPLAY_STATE_PTR pScreen,
	                                           UINT16 buttonInput)
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;

  pStatus = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;

  if (buttonInput == NO_PUSH_BUTTON_DATA)
  {
    // Increment Auto Abort Timer
    auto_Abort_Timer++;
    
    if (auto_Abort_Timer == AutoAbortTime_Converted)
    {
      // Update Transition Log, Reset APAC.
      DispProcessingApp_CreateTransLog(DISPLAY_LOG_TIMEOUT, TRUE);
      APACMgr_IF_Reset(NULL, NULL, NULL, NULL, APAC_IF_TIMEOUT);

      // Auto Abort Timer has expired. Return to Screen M1. 
      pStatus->previousScreen = pStatus->currentScreen;
      pStatus->currentScreen  = M1;
      auto_Abort_Timer        = 0;
      pScreen = (DISPLAY_STATE_PTR)&(MenuStateTbl[0]) +
	             pStatus->currentScreen;
    }
  }
  else
  {
    // reset the auto abort timer
    auto_Abort_Timer = 0;
  }
  return(pScreen);
}

/******************************************************************************
* Function:    PerformAction
*
* Description: Perform the required action for this screen.
*
* Parameters:  nextAction - The next action to be performed.
*
* Returns:     Returns the next screen or action to be performed.
*
* Notes:       None
*
*****************************************************************************/
static
DISPLAY_SCREEN_ENUM PerformAction(DISPLAY_SCREEN_ENUM nextAction)
{
  DISPLAY_ACTION_TABLE_PTR pAction;

  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (nextAction - 100);
  return(pAction->action() == TRUE ? pAction->retTrue : pAction->retFalse);
}

/******************************************************************************
 * Function:    DispProcessingApp_D_HLTHcheck
 *
 * Description: Function simply checks the D_HLTH byte and logs an error if 
 *              it has been nonzero too long.
 *
 * Parameters:  dispHealth - the D_HLTH byte from the encoded rx packet.
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/ 
static 
void DispProcessingApp_D_HLTHcheck(UINT8 dispHealth, BOOLEAN bNewData)
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_SCREEN_CONFIG_PTR pCfg;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pCfg    = (DISPLAY_SCREEN_CONFIG_PTR)&m_DispProcApp_Cfg;

  if(dispHealth == 0x00) 
  {
    pStatus->dispHealthTimer = 0;
    if (bNewData == TRUE)
    { 
      // Ensure that these elements are not set at startup to TRUE
      pStatus->bPBITComplete   = TRUE;
      pStatus->bButtonEnable   = TRUE;
      pStatus->bD_HLTHReset    = TRUE;
    }
    if (pStatus->bPBITComplete == FALSE)
    {
      pStatus->displayHealth = 0x80;
    }
    else
    { 
      if(pStatus->displayHealth != dispHealth)
      {
        pStatus->displayHealth = dispHealth;
        pStatus->bNewDebugData = TRUE;
      }
    }
  }
  // For critical D_HLTH codes, call D_HLTH inspector 
  else if (dispHealth == 0xcc || dispHealth == 0xc3 ||
           dispHealth == 0xcf || dispHealth == 0xff)
  {
    if(pStatus->displayHealth != dispHealth) //Fresh critical D_HLTH code.
    {
      DispProcessingApp_D_HLTHResult(dispHealth, 0);

      // Update the Debug Display when D_HLTH is nonzero too long.
      pStatus->displayHealth = dispHealth;
      pStatus->bButtonEnable = FALSE;
      pStatus->bNewDebugData = TRUE;
    }
  }
  // Set timer for PBIT and diagnostic D_HLTH codes
  else
  {
    pStatus->displayHealth = dispHealth;
  	// Report error if the Max D_HLTH nonzero wait time is reached
  	if (pStatus->dispHealthTimer >= no_HS_Timeout_Converted)
  	{
      DispProcessingApp_D_HLTHResult(dispHealth, pCfg->no_HS_Timeout_s);

      // Update the Debug Display when D_HLTH is nonzero too long.
      pStatus->bNewDebugData = TRUE;
  	  //Reset log timer
  	  pStatus->dispHealthTimer = 0;
      pStatus->bD_HLTHReset = FALSE;
  	}
    else if (pStatus->bD_HLTHReset == TRUE)
  	{
  	  pStatus->dispHealthTimer++;
  	}
    pStatus->bButtonEnable = FALSE;
  }
}

/******************************************************************************
* Function:    DispProcessingApp_D_HLTHResult
*
* Description: Function simply checks the D_HLTH byte and returns the proper 
               RESULT.
*
* Parameters:  dispHealth - the D_HLTH byte from the encoded rx packet.
*
* Returns:     RESULT - returns D_HLTH log result.
*
* Notes:       None
*
*****************************************************************************/
static
void DispProcessingApp_D_HLTHResult(UINT8 dispHealth, UINT32 no_HS_Timeout_s)
{
  DISPLAY_LOG_RESULT_ENUM result;
  DISPLAY_DHLTH_LOG       displayHealthTOLog;
  switch (dispHealth)
  {
    case 0x80:
      result = DISPLAY_LOG_PBIT_ACTIVE;
      break;
    case 0x88:
      result = DISPLAY_LOG_PBIT_PASS;
      break;
    case 0xCF:
      result = DISPLAY_LOG_INOP_MON_FAULT;
      break;
    case 0xC3:
      result = DISPLAY_LOG_INOP_SIGNAL_FAULT;
      break;
    case 0xCC:
      result = DISPLAY_LOG_RX_COM_FAULT;
      break;
    case 0xFF:
      result = DISPLAY_LOG_DISPLAY_FAULT;
      break;
    case 0x40:
      result = DISPLAY_LOG_DIAGNOSTIC_MODE;
      break;
    default:
      result = DISPLAY_LOG_D_HLTH_UNKNOWN;
      break;
  }
   
  // Process Log
  displayHealthTOLog.result        = result;
  displayHealthTOLog.D_HLTHcode    = dispHealth;
  displayHealthTOLog.no_HS_Timeout_s = no_HS_Timeout_s;
  CM_GetTimeAsTimestamp((TIMESTAMP *)&displayHealthTOLog.lastFrameTime);

  LogWriteETM(SYS_ID_DISPLAY_APP_DHLTH_TO, LOG_PRIORITY_LOW,
      &displayHealthTOLog, sizeof(displayHealthTOLog), NULL);
}
/******************************************************************************
 * Function:    DispProcessingApp_UpdateProtocol
 *
 * Description: Utility function that sends out special requests that modify
 *              the PWC Display Protocols behaviour
 *
 * Parameters:  None                                         
 *
 * Returns:     None
 *              
 * Notes:       None
 *
 *****************************************************************************/
static 
void DispProcessingApp_UpdateProtocol(void)
{
  UARTMGR_PROTOCOL_DATA_PTR pData;
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  
  pData     = (UARTMGR_PROTOCOL_DATA_PTR )&m_DispProcApp_Request[0];
  pStatus   = (DISPLAY_SCREEN_STATUS_PTR )&m_DispProcApp_Status;
  
  (pData + TX_START_FLAG)->data.Int        = (UINT32)TRUE; //Allow protocol to
                                                           //begin transmitting
  (pData + INVALID_D_HLTH_TIMER)->data.Int = pStatus->dispHealthTimer;
  
  PWCDispProtocol_Write_Handler(pData, UARTMGR_PROTOCOL_PWC_DISPLAY, 
                                DISPLAY_APP_REQUEST, 
                                DISPLAY_APP_REQUESTS_COUNT);
}

/******************************************************************************
 * Function:    DispProcessingApp_SendNewMonitorData
 *
 * Description: Utility function that sends out updated data to be encoded by
 *              the PWC Display Protocol.
 *
 * Parameters:  UINT8*  characterString - contains the unencoded character 
 *                                        string for the current display  
 *                                        screen                                          
 *
 * Returns:     None
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
void DispProcessingApp_SendNewMonitorData( char characterString[] )
{
  UARTMGR_PROTOCOL_DATA_PTR pData, pCharData;
  DISPLAY_APP_DATA_PTR      pCfg;
  UINT8                    *pString;
  UINT16                    i;
  
  pData     = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_TXData[0];
  pCfg      = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pString   = (UINT8*) &characterString[0];
  pCharData = pData + CHARACTER_0;
  
  // Copy character string into the outgoing TX packet
  for(i = 0; i < MAX_SCREEN_SIZE; i++)
  {
    pCharData->data.Int = *pString;
    pCharData++; pString++;
  }
  
  (pData + DC_RATE)->data.Int = pCfg->lastDCRate * DCRATE_CONVERSION;
   
  PWCDispProtocol_Write_Handler(pData, UARTMGR_PROTOCOL_PWC_DISPLAY, 
                                TX_PROTOCOL, PWCDISP_TX_ELEMENT_CNT_COMPLETE);
}

/******************************************************************************
* Function:    InvalidButtonInput
*
* Description: Processes the Invalid button Input Action for screen M28
*
* Parameters:  None
*
* Returns:     TRUE
*
* Notes:       None
*
*****************************************************************************/
static BOOLEAN InvalidButtonInput()
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_ACTION_TABLE_PTR  pAction;
  DISPLAY_STATE_PTR         pScreen, pScreenPrevious;
  DISPLAY_ENUM_TABLE_PTR    pButtons;
  UINT16                    i, size, charCount, min, max;
  CHAR                      strInsert[24] = "";

  pStatus         = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction         = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A0 - 100);
  pScreen         = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + M28;
  pScreenPrevious = (DISPLAY_STATE_PTR)&MenuStateTbl[0] 
                    + pStatus->previousScreen;
  pButtons        = (DISPLAY_ENUM_TABLE_PTR)&ValidButtonTable[0];
  size            = MAX_SCREEN_SIZE / 2;
  charCount       = 0;
  min             = 12;

  if (invalid_Button_Timer >= invalidButtonTime_Converted)
  {
    // timer expired. return to the previous screen. Update previous screen
    pAction->retTrue        = pStatus->previousScreen;
    pAction->retFalse       = pStatus->previousScreen;
    pStatus->previousScreen = pStatus->currentScreen;
    invalid_Button_Timer    = 0;
  }
  else
  {
    // return to M28 regardless of button press
    pAction->retTrue  = M28;
    pAction->retFalse = M28;
    // Increment the invalid button timer
    invalid_Button_Timer++;
  }

  // Perform check to see what buttons are valid and add them to display.
  for (i = 0; i < DISPLAY_VALID_BUTTONS; i++)
  {
    if (GetNextState((DISPLAY_BUTTON_STATES)pButtons->ID, FALSE,
        pScreenPrevious) != A0)
    {
      if (charCount == 0)
      {
        // Add symbol minus the comma
        strcat(strInsert, (const char *)pButtons->Name);
        charCount += pButtons->Size;
      }
      else if (charCount != 0 &&
               (charCount + pButtons->Size + 1) <= size)
      {
        // Add symbol plus the comma
        strcat(strInsert, (const char *)(","));
        strcat(strInsert, (const char *)pButtons->Name);
        charCount += (pButtons->Size + 1);
      }
    }
    // Check next possible button
    pButtons++;
  }
  // Update Max
  max = min + (charCount - 1);

  for (i = min; i < MAX_SCREEN_SIZE; i++)
  {
    if (i <= max)
    {
      pScreen->menuString[i] = strInsert[i - min];
    }
    else
    {
      pScreen->menuString[i] = ' ';
    }
  }
  return(TRUE);
}

/******************************************************************************
 * Function:    GetConfig
 *
 * Description: Acquires the config information for screen M5
 *
 * Parameters:  NONE
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN GetConfig()
{
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_VARIABLE_TABLE_PTR pVar, ac_Cfg, chart_Rev;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pVar = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A1 - 100);
  pScreen = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  ac_Cfg    = pVar + pScreen->var1;
  chart_Rev = pVar + pScreen->var2;
  
  APACMgr_IF_Config(ac_Cfg->strInsert, chart_Rev->strInsert,
                    NULL, NULL, APAC_IF_MAX);
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallDate
 *
 * Description: Acquires the current recalled Date.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallDate()
{
  DISPLAY_VARIABLE_TABLE_PTR pVar, date, time;
  DISPLAY_STATE_PTR pScreen;
  DISPLAY_ACTION_TABLE_PTR pAction;
  
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A2 - 100);
  pScreen = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  date    = pVar + pScreen->var1;
  time    = pVar + pScreen->var2;
  
  APACMgr_IF_HistoryData(date->strInsert, time->strInsert, NULL, NULL,
                         APAC_IF_DATE);
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallNextDate
 *
 * Description: Selects the next saved Date in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallNextDate()
{
  APACMgr_IF_HistoryAdv(NULL, NULL, NULL, NULL, APAC_IF_DATE);
  
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallPreviousDate
 *
 * Description: Selects the previous saved Date in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPreviousDate()
{
  APACMgr_IF_HistoryDec(NULL, NULL, NULL, NULL, APAC_IF_DATE);
  
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallPAC
 *
 * Description: Acquires the current recalled PAC.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPAC()
{
  DISPLAY_VARIABLE_TABLE_PTR pVar, line1, line2;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A5 - 100);
  pScreen = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  line1   = pVar + pScreen->var1;
  line2   = pVar + pScreen->var2;
  
  APACMgr_IF_HistoryData(line1->strInsert, line2->strInsert, NULL, NULL,
                         APAC_IF_DAY);
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallNextPAC
 *
 * Description: Selects the next saved PAC in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallNextPAC()
{
  APACMgr_IF_HistoryAdv(NULL, NULL, NULL, NULL, APAC_IF_DAY);
  
  return(TRUE);
}

/******************************************************************************
 * Function:    RecallPreviousPAC
 *
 * Description: Selects the Previous saved PAC in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPreviousPAC()
{
  APACMgr_IF_HistoryDec(NULL, NULL, NULL, NULL, APAC_IF_DAY);
  
  return(TRUE);
}

/******************************************************************************
 * Function:    SavedDoubleClick
 *
 * Description: Acquires the double click data for screen M21.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN SavedDoubleClick()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_APP_DATA_PTR       pCfg;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_VARIABLE_TABLE_PTR pVar, dcRate, changed;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pCfg    = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A8 - 100);
  pScreen = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  dcRate  = pVar + pScreen->var1;
  changed = pVar + pScreen->var2;
  
  if (pStatus->previousScreen != M21)
  {
    scaledDCRATE = savedDCRATE;
  }
  if(scaledDCRATE >= 0)
  {
    sprintf(dcRate->strInsert, "+%d", scaledDCRATE);
  }
  else
  {
    sprintf(dcRate->strInsert, "%d", scaledDCRATE);
  }
  if (pCfg->lastDCRate == scaledDCRATE)
  {
      changed->strInsert[0] = ' '; 
  }
  else
  {
    snprintf((char *)(changed)->strInsert,
             sizeof((changed)->strInsert), "%c", 0x2A);
  }
  return(TRUE);
}

/******************************************************************************
 * Function:    IncDoubleClick
 *
 * Description: Increment the double click rate by 1 click.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN IncDoubleClick()
{
  if(scaledDCRATE < DISPLAY_DCRATE_MAX)
  {
    scaledDCRATE++;
    m_DispProcApp_Status.previousScreen = m_DispProcApp_Status.currentScreen;
  }
  return(TRUE);
}

/******************************************************************************
 * Function:    DecDoubleClick
 *
 * Description: Decrement the double click rate by one click.
 *
 * Parameters:  [in] direction: Tells the action function whether to process
 *              transition actions going into or out of a screen
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN DecDoubleClick()
{
  if(scaledDCRATE > (DISPLAY_DCRATE_MAX * -1))
  {
    scaledDCRATE--;
    m_DispProcApp_Status.previousScreen = m_DispProcApp_Status.currentScreen;
  }
  return(TRUE);
}

/******************************************************************************
 * Function:    SaveDoubleClick
 *
 * Description: Writes the currently displayed DCRATE to NVM.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN SaveDoubleClick()
{
  DISPLAY_APP_DATA_PTR pCfg;
  
  pCfg             = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pCfg->lastDCRate = (INT8)(scaledDCRATE);
  savedDCRATE      = scaledDCRATE;
  
  // Update App Data
  NV_Write(NV_DISPPROC_CFG, 0, (void *)&m_DispProcApp_AppData,
           sizeof(DISPLAY_APP_DATA));
      
  return(TRUE);
}

/******************************************************************************
 * Function:    ResetPAC
 *
 * Description: Resets the APAC Processing
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN ResetPAC()
{
  APACMgr_IF_Reset(NULL, NULL, NULL, NULL, APAC_IF_DCLK);
  
  return(TRUE);
}

/******************************************************************************
 * Function:    CorrectSetup
 *
 * Description: Verifies that the APAC setup is correct. 
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - APACMgr setup is correct
 *              FALSE - APACMgr setup is incorrect. 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN CorrectSetup()
{
  return(APACMgr_IF_Setup(NULL,NULL,NULL,NULL,APAC_IF_MAX));
}

/******************************************************************************
 * Function:    ValidateMenu
 *
 * Description: Verifies that APAC_E#_LAST_VT >= APAC_MAX_VT
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - APAC_E#_LAST_VT >= APAC_MAX_VT
 *              FALSE - APAC_E#_LAST_VT < APAC_MAX_VT
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN ValidateMenu()
{
  return(APACMgr_IF_ValidateManual(NULL,NULL,NULL,NULL,APAC_IF_MAX));
}

/******************************************************************************
 * Function:    RunningPAC
 *
 * Description: Verify that the current PAC is still running.
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - PAC Complete
 *              False - PAC Running
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RunningPAC()
{
  BOOLEAN                    bResult;
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, pac_Status;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction    = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A17 - 100);
  pScreen    = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retFalse;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pac_Status = pVar + pScreen->var1;
  bResult    = 
  APACMgr_IF_RunningPAC(pac_Status->strInsert, NULL,NULL,NULL,APAC_IF_MAX);
  
  // Update APAC Processing status
  pStatus->bAPACProcessing = TRUE;
  
  return(bResult);
}

/******************************************************************************
 * Function:    PACResult
 *
 * Description: Determine whether the completed PAC passed or failed.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACResult()
{
  return(APACMgr_IF_CompletedPAC(NULL,NULL,NULL,NULL,APAC_IF_MAX));
}

/******************************************************************************
 * Function:    PACSuccess
 *
 * Description: Returns indication if 50 Engine Run Hrs has transpired since 
 *              last successful Manual PAC Validation
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACSuccess()
{ 
  m_DispProcApp_Status.bAPACProcessing = FALSE;
  return(APACMgr_IF_ValidateManual(NULL,NULL,NULL,NULL,APAC_IF_MAX));
}

/******************************************************************************
 * Function:    PACFail
 *
 * Description: Acquires the PAC Failure message from the APACMgr.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN ErrorMessage()
{
  DISPLAY_VARIABLE_TABLE_PTR pVar, error_Msg, fail_Param;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pAction    = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A20 - 100);
  pScreen    = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  error_Msg  = pVar + pScreen->var1;
  fail_Param = pVar + pScreen->var2;
  m_DispProcApp_Status.bAPACProcessing = FALSE;

  // Acquire APAC error message
  APACMgr_IF_ErrMsg(error_Msg->strInsert, fail_Param->strInsert, 
                    NULL, NULL, APAC_IF_MAX);
  return(TRUE);
}

/******************************************************************************
 * Function:    AquirePACData
 *
 * Description: acquires the engine number, category, NG, and ITT for a PAC
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN AquirePACData()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, eng_Num, eng_Num2, cat, cat2, 
                             ng_Margin, itt_Margin;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  if (pStatus->currentScreen == M8)
  {
    pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + (A21 - 100);
  }
  else
  {
    pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] +
              (GetNextState(pStatus->buttonInput, TRUE, NULL) - 100);
  }
  pScreen    = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pAction->retTrue;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  eng_Num    = pVar + pScreen->var1;
  cat        = pVar + pScreen->var2;
  ng_Margin  = pVar + pScreen->var3;
  itt_Margin = pVar + pScreen->var4;
  // Update display values
  APACMgr_IF_Result(cat->strInsert, ng_Margin->strInsert, 
                    itt_Margin->strInsert, eng_Num->strInsert, APAC_IF_MAX);
  
  if(cat->strInsert[0] != 'A')
  {
    cat->strInsert[0] = ' ';
  }
  if(pScreen->var5 != -1)
  {
    eng_Num2 = pVar + pScreen->var5;
    cat2     = pVar + pScreen->var6;
    
    eng_Num2->strInsert[0] = eng_Num->strInsert[0];
    cat2->strInsert[0] = cat->strInsert[0];
  }
                    
  return(TRUE);
}

/******************************************************************************
 * Function:    PACDecision
 *
 * Description: Accepts, rejects, validates, or invalidates a PAC based on
 *              current screen
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACDecision()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  
  if(pStatus->currentScreen == M13)
  {
    APACMgr_IF_ResultCommit(NULL,NULL,NULL,NULL,APAC_IF_ACPT);
  }
  else if(pStatus->currentScreen == M14)
  {
    APACMgr_IF_ResultCommit(NULL,NULL,NULL,NULL,APAC_IF_RJCT);
  }
  else if(pStatus->currentScreen == M26)
  {
    APACMgr_IF_ResultCommit(NULL,NULL,NULL,NULL,APAC_IF_VLD);
  }
  else
  {
    APACMgr_IF_ResultCommit(NULL, NULL, NULL, NULL, APAC_IF_INVLD);
  }
  return(TRUE);
}

/******************************************************************************
 * Function:    PowerSelect
 *
 * Description: Selects either Normal Power or CAT-A based on current screen.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PowerSelect()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  
  if(pStatus->currentScreen == M29)
  {
    APACMgr_IF_Select(NULL,NULL,NULL,NULL,APAC_IF_NR100);
  }
  else
  {
    APACMgr_IF_Select(NULL, NULL, NULL, NULL, APAC_IF_NR102);
  }
  return(TRUE);
}

/******************************************************************************
* Function:    DispProcessingApp_GetStatus
*
* Description: Utility function to request current Display Processing App
*              status
*
* Parameters:  None
*
* Returns:     Ptr to Display Processing Application Status data
*
* Notes:       None
*
*****************************************************************************/
DISPLAY_SCREEN_STATUS_PTR DispProcessingApp_GetStatus(void)
{
  return((DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status);
}

/******************************************************************************
* Function:    DispProcessingApp_GetDebug
*
* Description: Utility function to request current Display Processing App debug
*              state
*
* Parameters:  None
*
* Returns:     Ptr to Display Processing Application Debug Data
*
* Notes:       None
*
*****************************************************************************/
DISPLAY_DEBUG_PTR DispProcessingApp_GetDebug(void)
{
  return((DISPLAY_DEBUG_PTR) &m_DispProcApp_Debug);
}

/******************************************************************************
* Function:    DispProcessingApp_GetCfg
*
* Description: Utility function to request current Display Processing App Cfg
*              state
*
* Parameters:  None
*
* Returns:     Ptr to PWCDisp Debug Data
*
* Notes:       None
*
*****************************************************************************/
DISPLAY_SCREEN_CONFIG_PTR DispProcessingApp_GetCfg(void)
{
  return((DISPLAY_SCREEN_CONFIG_PTR) &m_DispProcApp_Cfg);
}

/******************************************************************************
 * Function:    DispProcessingApp_GetDisplayButtonInput
 *
 * Description: Returns the button pressed by the user. 
 *
 * Parameters:  None
 *
 * Returns:     buttonState - The button(or lack there of) the user pushed 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static 
DISPLAY_BUTTON_STATES DispProcessingApp_GetDisplayButtonInput(BOOLEAN bNewData)
{
  DISPLAY_RX_INFORMATION_PTR pRX_Info;
  DISPLAY_APP_DATA_PTR       pCfg;
  UINT16                     i, btnTrueCnt, dblTrueCnt, dblTimeout;
  DISPLAY_BUTTON_STATES      buttonState, dblButtonState, tempState;
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  
  pRX_Info   = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  pCfg       = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  btnTrueCnt = 0;
  dblTrueCnt = 0;
  dblTimeout = dblClickTimeout +
               ((pCfg->lastDCRate * DCRATE_CONVERSION * 2)/10);
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;

  if(bNewData == TRUE)
  {
    for(i = 0; i < BUTTON_STATE_COUNT; i++)
    {
      if(pRX_Info->buttonState[i] == TRUE)
      {
        buttonState = (DISPLAY_BUTTON_STATES)i;
        btnTrueCnt++;
      }
      if(pRX_Info->dblClickState[i] == TRUE)
      {
        dblButtonState = (DISPLAY_BUTTON_STATES)(i + 8);
        dblTrueCnt++;
      }
    }
  }
  if (btnTrueCnt == 0)
  {
    buttonState = NO_PUSH_BUTTON_DATA;
  }
  if (dblTrueCnt == 0)
  {
    dblButtonState = NO_PUSH_BUTTON_DATA;
  }
  if (btnTrueCnt > 1 || dblTrueCnt > 1 || pStatus->bButtonEnable == FALSE
      || dblTrueCnt > btnTrueCnt)
  {
    // Ignore button data if more than one button is pressed
    buttonState   = NO_PUSH_BUTTON_DATA; 
    dblClickFlag  = FALSE;
    dblClickTimer = 0;
  }
  else
  {
    if(dblClickFlag == TRUE)
    { // If the dblclick timer has expired use previous button
      if (dblClickTimer >= dblTimeout && buttonState == NO_PUSH_BUTTON_DATA)
      {
        buttonState            = previousButtonState;
        dblClickFlag           = FALSE;
        dblClickTimer          = 0;
        pStatus->bNewDebugData = TRUE;
      }
      else
      {
        dblClickTimer++;
        // If button and double click match return the dbl click.
        if (buttonState == previousButtonState && 
            dblButtonState == (buttonState + BUTTON_STATE_COUNT))
        {
          buttonState            = dblButtonState;
          dblClickFlag           = FALSE;
          dblClickTimer          = 0;
          pStatus->bNewDebugData = TRUE;
        }
        // If a different button was pressed execute first press and queue 
        // the second for dbl click testing
        else if (buttonState != NO_PUSH_BUTTON_DATA && //TODO: button state != previous button state
                 dblButtonState == NO_PUSH_BUTTON_DATA)
        {
          tempState              = buttonState;
          buttonState            = previousButtonState;
          previousButtonState    = tempState;
          dblClickTimer          = 0;
          pStatus->bNewDebugData = TRUE;
        }
        // Extraneous case where button and dbl do not match.
        else if (dblButtonState != NO_PUSH_BUTTON_DATA &&
                 (dblButtonState != (buttonState + BUTTON_STATE_COUNT) || 
                 dblButtonState != (previousButtonState + BUTTON_STATE_COUNT)))
        {
          buttonState            = NO_PUSH_BUTTON_DATA;
          dblClickFlag           = FALSE;
          dblClickTimer          = 0;
          pStatus->bNewDebugData = TRUE;
        }
      }
    }
    else
    {
      if(buttonState != NO_PUSH_BUTTON_DATA &&
         dblButtonState == NO_PUSH_BUTTON_DATA)
      {
        dblClickFlag        = TRUE;
        previousButtonState = buttonState;
      }
      buttonState = NO_PUSH_BUTTON_DATA;
    }
  }
  return(buttonState);
}

/******************************************************************************
 * Function:    FormatCharacterString
 *
 * Description: Updates a Display Character String with proper formatting. 
 *
 * Parameters:  pFormat - pointer to a string format table
 *              pScreen - pointer to Screen Table
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/

static
void FormatCharacterString(DISPLAY_STATE_PTR pScreen)
{
  DISPLAY_VARIABLE_TABLE_PTR   pVar;
  UINT8                        i, j; 
  INT8                         varIndex[6];
  varIndex[0] = pScreen->var1; varIndex[3] = pScreen->var4;
  varIndex[1] = pScreen->var2; varIndex[4] = pScreen->var5;
  varIndex[2] = pScreen->var3; varIndex[5] = pScreen->var6;
  pVar = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];

  for (i = 0; i < 6; i++)
  {
    if (varIndex[i] != -1)
    {
      for(j = 0; j < (pVar + varIndex[i])->variableLength; j++)
      {
        if ((pVar + varIndex[i])->strInsert[j] != NULL)
        {
          pScreen->menuString[(pVar + varIndex[i])->variablePosition + j] = 
          (pVar + varIndex[i])->strInsert[j];
        }
      }
    }
  }
}

/******************************************************************************
 * Function:    GetNextState
 *
 * Description: Returns the next state listed in the  MenuStateTbl according
 *              to the button state.
 *
 * Parameters:  button - The current button Input
 *              thisScreen - TRUE: use current screen. FALSE: Manual Screen
 *              pParamScreen - Manual Screen (not used if thisScreen = TRUE)
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/

static 
DISPLAY_SCREEN_ENUM GetNextState(DISPLAY_BUTTON_STATES button, 
                                 BOOLEAN thisScreen, 
                                 DISPLAY_STATE_PTR pParamScreen)
{
  DISPLAY_SCREEN_ENUM       nextScreen;
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_STATE_PTR         pScreen;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;

  if (thisScreen == TRUE)
  {
    pScreen = (DISPLAY_STATE_PTR)&MenuStateTbl[0] + pStatus->currentScreen;
  }
  else
  {
    pScreen = pParamScreen;
  }
  
  switch(button)
  {
  case UP_DBLCLICK:
    nextScreen = pScreen->dblUpScreen;
    break;
  case DOWN_DBLCLICK:
    nextScreen = pScreen->dblDownScreen;
    break;
  case LEFT_DBLCLICK:
    nextScreen = pScreen->dblLeftScreen;
    break;
  case RIGHT_DBLCLICK:
    nextScreen = pScreen->dblRightScreen;
    break;
  case ENTER_DBLCLICK:
    nextScreen = pScreen->dblEnterScreen;
    break;
  case EVENT_DBLCLICK:
    nextScreen = pScreen->dblEventScreen;
    break;
  case RIGHT_BUTTON:
    nextScreen = pScreen->rightScreen;
    break;
  case LEFT_BUTTON:
    nextScreen = pScreen->leftScreen;
    break;
  case UP_BUTTON:
    nextScreen = pScreen->upScreen;
    break;
  case DOWN_BUTTON:
    nextScreen = pScreen->downScreen;
    break;
  case ENTER_BUTTON:
    nextScreen = pScreen->enterScreen;
    break;
  case EVENT_BUTTON:
    nextScreen = pScreen->eventScreen;
    break;
  case NO_PUSH_BUTTON_DATA:
    nextScreen = pScreen->NoButtonInput;
    break;
  default:
    nextScreen = pScreen->NoButtonInput;
    break;
  };

  return nextScreen;
}


/******************************************************************************
 * Function:    NoButtonData
 *
 * Description: Utility function that sets button state data to FALSE. Only to 
 *              be used during APAC Processing when no button data is required.
 *
 * Parameters:  pFormat - pointer to a string format table
 *              pScreen - pointer to Screen Table
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
void NoButtonData(void)
{
  DISPLAY_RX_INFORMATION_PTR pbutton;
  UINT16 i;

  pbutton = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  
  for(i = 0; i < BUTTON_STATE_COUNT; i++)
  {
    pbutton->buttonState[i] = FALSE;
    pbutton->dblClickState[i] = FALSE;
  }
}

/******************************************************************************
* Function:    DispProcAppDebug_Task
*
* Description: Utility function to display Display Processing App frame data 
*
* Parameters:  pParam: Not used, only to match Task Mgr call signature
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
void DispProcAppDebug_Task(void *param)
{
  DISPLAY_DEBUG_PTR pDebug;
  
  pDebug = (DISPLAY_DEBUG_PTR)&m_DispProcApp_Debug;

  if (pDebug->bDebug == TRUE)
  {
    if(pDebug->bNewFrame == TRUE)
    {
      DISPLAY_SCREEN_STATUS_PTR  pStatus;
      DISPLAY_CHAR_TABLE_PTR     pMenuID, pButtonID, pDiscreteID;
      DISPLAY_STATE_PTR          pScreen;
      DISPLAY_APP_DATA_PTR       pCfg;
      DISPLAY_RX_INFORMATION_PTR pRXInfo;
      UINT8                      subString1[16], subString2[16], i;
      
      pStatus     = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
      pMenuID     = (DISPLAY_CHAR_TABLE_PTR)&DisplayScreenTbl[0];
      pButtonID   = (DISPLAY_CHAR_TABLE_PTR)&DisplayButtonTbl[0];
      pDiscreteID = (DISPLAY_CHAR_TABLE_PTR)&DisplayDiscreteTbl[0];
      pScreen     = (DISPLAY_STATE_PTR)&MenuStateTbl[0];
      pCfg        = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
      pRXInfo     = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
      m_Str[0]    = NULL;
      
      // Display the Current Screen ID
      memcpy(subString2, (pMenuID + pStatus->currentScreen)->Name, 3);
      snprintf((char *)m_Str, 19, "CurrentScreen   = ");
      snprintf((char *)subString1, 4, "%s\r\n", subString2);
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display the Previous Screen ID
      memcpy(subString2, (pMenuID + pStatus->previousScreen)->Name, 3);
      snprintf((char *)m_Str, 19, "PreviousScreen  = ");
      snprintf((char *)subString1, 4, "%s\r\n", subString2);
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n\n");
      m_Str[0] = NULL;
      
      // Display the completion status of PBIT
      snprintf((char *)m_Str, 22, "PBITComplete?   = %d\r\n", 
               pStatus->bPBITComplete);
      GSE_PutLine((const char *)m_Str);

      // Display the D_HLTH hexadecimal code
      snprintf((char *)m_Str, 25, "D_HLTHCode?     = 0x%02x\r\n",
               pStatus->displayHealth);
      GSE_PutLine((const char *)m_Str);
      
      // Display whether Transmission is currently enabled
      snprintf((char *)m_Str, 22, "ButtonEnable?   = %d\r\n", 
               pStatus->bButtonEnable);
      GSE_PutLine((const char *)m_Str);
      
      // Display whether or not APAC Processing is in progress
      snprintf((char *)m_Str, 22, "APACProcessing? = %d\r\n",
               pStatus->bAPACProcessing);
      GSE_PutLine((const char *)m_Str);
      
      // Display whether the button that is pressed is invalid or not
      snprintf((char *)m_Str, 21, "Invalid Key?    = %d\r\n",
               pStatus->bInvalidButton);
      GSE_PutLine((const char *)m_Str);
      m_Str[0] = NULL;
      
      // Display the Button Pressed
      memcpy(subString2, (pButtonID + pDebug->buttonInput)->Name, 16);
      snprintf((char *)m_Str, 19, "ButtonInput?    = ");
      snprintf((char *)subString1, sizeof(subString1), "%s", subString2);
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display the current Double Click Rate
      snprintf((char *)m_Str, 24, "DoubleClickRate = %d\r\n", 
               pCfg->lastDCRate);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      
      // Display the Part Number
      snprintf((char *)m_Str, 25, "Part Number     = 0x%02x\r\n", 
               pRXInfo->partNumber);
      GSE_PutLine((const char *)m_Str);
      
      // Display The Version Number Major.Minor
      snprintf((char *)m_Str, 45, "VersionNumber   = %d.%d\r\n", 
               pRXInfo->versionNumber[0], pRXInfo->versionNumber[1]);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      
      for (i = 0; i < DISPLAY_DISCRETES_COUNT; i++)
      {
        // Display the active discretes
        memcpy(subString1,  (pDiscreteID + i)->Name, 16);
        snprintf((char *)m_Str, 16, "%s", subString1);
        snprintf((char *)subString1, 5, " = %d", 
                 (pRXInfo)->discreteState[i]);
        strcat((char *)m_Str, (const char *)subString1);
        GSE_PutLine((const char *)m_Str);
        GSE_PutLine("\r\n");
        m_Str[0] = NULL;
      }
      
      // Display Menu Screen Line 1
      GSE_PutLine("\r\n");
      memcpy(m_Str, &(pScreen + pStatus->currentScreen)->menuString[0], 12);
      m_Str[12] = '\0';
      GSE_PutLine((const char *) m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display Menu Screen Line 2
      memcpy(m_Str, &(pScreen + pStatus->currentScreen)->menuString[12], 12);
      m_Str[12] = '\0';
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n\n");
      
      pDebug->bNewFrame = FALSE;
    }
  }
}

/******************************************************************************
* Function:    DispProcessingApp_CreateTransLog
*
* Description: Function creates a transition log whenever a Timeout occurs, 
*              the left button is double clicked, or the enter button is 
*              pressed for all screens excluding M1-M4.
*
* Parameters:  none
*
* Returns:     none
*
* Notes:       none
*
*****************************************************************************/
static
void DispProcessingApp_CreateTransLog(DISPLAY_LOG_RESULT_ENUM result, 
                                      BOOLEAN reset)
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_TRANSITION_LOG    log;

  pStatus           = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;
  log.result        = result;
  log.bReset        = reset; // Is this a reset? (auto abort or dblclk left)
  log.buttonPressed = (UINT16)pStatus->buttonInput;
  log.currentScreen = (UINT16)pStatus->currentScreen;

  LogWriteSystem(SYS_ID_DISPLAY_APP_TRANSITION, LOG_PRIORITY_LOW, &log,
                 sizeof(DISPLAY_TRANSITION_LOG), NULL);
}

/******************************************************************************
* Function:    DispProcessingApp_RestoreAppData
*
* Description: Utility function to restore Display Processing app data from 
*              Eeprom
*
* Parameters:  none
*
* Returns:     none
*
* Notes:       none
*
*****************************************************************************/
static 
void DispProcessingApp_RestoreAppData(void)
{
  RESULT result;
  
  result = NV_Open(NV_DISPPROC_CFG);
  if(result == SYS_OK)
  {
    NV_Read(NV_DISPPROC_CFG, 0, (void *) &m_DispProcApp_AppData,
            sizeof(DISPLAY_APP_DATA));
  }
  
  //If open failed re-init app data
  if (result != SYS_OK)
  {
    DispProcessingApp_FileInit();
  }
}

/******************************************************************************
 * Function:     DispProcessingApp_FileInit
 *
 * Description:  Reset the file to the initial state.
 *
 * Parameters:   None
 *
 * Returns:      BOOLEAN TRUE  The file reset was successful.
 *                       FALSE The file reset failed.
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN DispProcessingApp_FileInit(void)
{
  // Init App Data
  m_DispProcApp_AppData.lastDCRate = m_DispProcApp_Cfg.defaultDCRATE;
  
  // Update App Data
  NV_Write(NV_DISPPROC_CFG, 0, (void *) &m_DispProcApp_AppData,
           sizeof(DISPLAY_APP_DATA));

  return TRUE;
}

/******************************************************************************
 * Function:    PWCDispProtocol_ReturnFileHdr()
 *
 * Description: Function to return the F7X file hdr structure
 *
 * Parameters:  dest - pointer to destination buffer
 *              max_size - max size of dest buffer 
 *              ch - uart channel requested 
 *
 * Returns:     cnt - byte count of data returned 
 *
 * Notes:       none 
 *
 *****************************************************************************/
UINT16 DispProcessingApp_ReturnFileHdr(UINT8 *dest, const UINT16 max_size,
                                       UINT16 ch)
{
  DISPLAY_SCREEN_CONFIG_PTR pCfg;
  DISPLAY_FILE_HDR          fileHdr;
  
  pCfg = (DISPLAY_SCREEN_CONFIG_PTR)&m_DispProcApp_Cfg;
  
  fileHdr.defaultDCRATE        = pCfg->defaultDCRATE;
  fileHdr.invalidButtonTime_ms = pCfg->invalidButtonTime_ms;
  fileHdr.autoAbortTime_s      = pCfg->autoAbortTime_s;
  fileHdr.no_HS_Timeout_s      = pCfg->no_HS_Timeout_s;
  
  fileHdr.size = sizeof(DISPLAY_FILE_HDR);
  
  memcpy(dest, (UINT8 *) &fileHdr, sizeof(fileHdr.size));
  
  return(fileHdr.size);
}
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: DispProcessingApp.c $
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-12-02   Time: 6:38p
 * Updated in $/software/control processor/code/application
 * SCR #1303 Display App Updates from Jeremy.  
 * 
 * *****************  Version 1  *****************
 * User: Jeremy Hester Date: 10/5/15    Time:4:10p
 * Created in $software/control processor/code/system
 * Initial Check In. Implementation of the Display Processing Application
 *                   Requirements.
 *****************************************************************************/
