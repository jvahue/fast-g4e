#define DISPLAY_PROCESSING_BODY

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        DispProcessingApp.c

    Description: Contains all functions and data related to the Display  
                 Processing Application 
    
    VERSION
      $Revision: 10 $  $Date: 2/10/16 5:36p $     

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
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 },
  { NULL, -1, -1 }
};
static DISPLAY_VARIABLE_TABLE m_VariableTable[MAX_SCREEN_VARIABLE_COUNT] =
{//                  strInsert Pos Len
/* 0 AC_CFG     */ { "",        7,  5 },
/* 1 CHART_REV  */ { "",       23,  1 },
/* 2 PAC_STATUS */ { "",       12, 12 },
/* 3 ENG_NUM    */ { "",        1,  1 },
/* 4 ITT_MARGIN */ { "",        8,  3 },
/* 5 CAT        */ { "",        2,  1 },
/* 6 NG_MARGIN  */ { "",       19,  4 },
/* 7 ERROR_MSG  */ { "",        0, 12 },
/* 8 FAIL_PARAM */ { "",       12,  5 },
/* 9 DATE       */ { "",        0, 11 },
/*10 TIME       */ { "",       12, 12 },
/*11 STATE      */ { "",        0, 12 },
/*12 PAC_VALUES */ { "",       12, 12 },
/*13 DCRATE     */ { "",       21,  3 },
/*14 CHANGED    */ { "",       23,  1 },
/*15 NR_SPD     */ { "",       12,  3 }
};

static UARTMGR_PROTOCOL_DATA  m_PWCDisp_RXData[PWCDISP_RX_ELEMENT_COUNT];
static DISPLAY_SCREEN_STATUS  m_DispProcApp_Status;
static DISPLAY_SCREEN_CONFIG  m_DispProcApp_Cfg;
static DISPLAY_RX_INFORMATION m_DispProcRX_Info;
static DISPLAY_APP_DATA       m_DispProcApp_AppData;
static DISPLAY_DEBUG          m_DispProcApp_Debug;
static UINT16                 auto_Abort_Timer      = 0;
static UINT16                 invalid_Button_Timer  = 0;
static UINT16                 dblClickTimer         = 0;
static UINT16                 newDataTimer          = 0;
static BOOLEAN                dblClickFlag          = FALSE;
static BOOLEAN                bValidatePAC          = FALSE;
static BOOLEAN                bPACDecision          = FALSE;
static BOOLEAN                discreteDIOStatusFlag = FALSE;

static UINT8* dataLossFlag = NULL;
static DISPLAY_DEBUG_TASK_PARAMS dispProcAppDisplayDebugBlock;
static DISPLAY_APP_TASK_PARAMS   dispProcAppBlock;
static INT8 scaledDCRATE = 0; // The scaled value of DCRATE for M21
static INT8 savedDCRATE;      // The saved DCRATE setting on the display
static DISPLAY_BUTTON_STATES prevButtonState = NO_PUSH_BUTTON_DATA;
static UINT16 invalidButtonTime_Converted; //converted to ticks
static UINT16 autoAbortTime_Converted;     //converted to ticks
static UINT16 no_HS_Timeout_Converted;     //converted to ticks
static UINT8 dio_Validity_Status = (UINT8)DIO_DISPLAY_HEALTH_CODE_FAIL;
static DISPLAY_SCREEN_ENUM invalidButtonScreen;

static DISPLAY_BUTTON_TABLE validButtonTable[DISPLAY_VALID_BUTTONS] =
{
  {DOWN_BUTTON,   "", 1},
  {UP_BUTTON,     "", 1},
  {ENTER_BUTTON,  "", 1},
  {LEFT_BUTTON,   "", 1},
  {RIGHT_BUTTON,  "", 1},
  {EVENT_BUTTON,  "", 2},
  {DOWN_DBLCLICK, "", 2},
  {UP_DBLCLICK,   "", 2},
  {ENTER_DBLCLICK,"", 2},
  {LEFT_DBLCLICK, "", 2},
  {RIGHT_DBLCLICK,"", 2},
  {EVENT_DBLCLICK,"", 2}
};

static UINT8 m_Str[MAX_DEBUG_STRING_SIZE];

/********************************/
/*      Enumerator Tables       */
/********************************/

static DISPLAY_CHAR_TABLE displayScreenTbl[DISPLAY_SCREEN_COUNT] =
{
  { "M00" }, { "M01" }, { "M02" }, { "M03" }, { "M04" }, 
  { "M05" }, { "M06" }, { "M07" }, { "M08" }, { "M09" }, 
  { "M10" }, { "M11" }, { "M12" }, { "M13" }, { "M14" }, 
  { "M15" }, { "M16" }, { "M17" }, { "M18" }, { "M19" }, 
  { "M20" }, { "M21" }, { "M22" }, { "M23" }, { "M24" }, 
  { "M25" }, { "M26" }, { "M27" }, { "M28" }, { "M29" }, 
  { "M30" }
};

static DISPLAY_CHAR_TABLE displayButtonTbl[(UINT16)DISPLAY_BUTTONS_COUNT + 2] =
{
  { "RIGHT_BUTTON    " }, { "LEFT_BUTTON     " }, { "ENTER_BUTTON    " },
  { "UP_BUTTON       " }, { "DOWN_BUTTON     " }, { "UNUSED1_BUTTON  " },
  { "UNUSED2_BUTTON  " }, { "EVENT_BUTTON    " }, { "RIGHT_DBLCLICK  " },
  { "LEFT_DBLCLICK   " }, { "ENTER_DBLCLICK  " }, { "UP_DBLCLICK     " },
  { "DOWN_DBLCLICK   " }, { "UNUSED1_DBLCLICK" }, { "UNUSED2_DBLCLICK" },
  { "EVENT_DBLCLICK  " }, { "BUTTON_COUNT    " }, { "NO_PUSH_BUTTON  " },
};

static DISPLAY_CHAR_TABLE displayDiscreteTbl[DISCRETE_STATE_COUNT] =
{
  { "DISC_ZERO " }, { "DISC_ONE  " }, { "DISC_TWO  " }, { "DISC_THREE" },
  { "DISC_FOUR " }, { "DISC_FIVE " }, { "DISC_SIX  " }, { "DISC_SEVEN" },
};
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

static void                  DispProcessingApp_InitStrings(void);
static BOOLEAN               DispProcessingApp_GetNewDispData( void );
static DISPLAY_STATE_PTR     DispProcessingApp_AutoAbortValid(DISPLAY_STATE_PTR pScreen,
	                                                          UINT16 buttonInput);
static DISPLAY_SCREEN_ENUM   DispProcessingApp_PerformAction(DISPLAY_SCREEN_ENUM nextAction);
static void                  DispProcessingApp_D_HLTHcheck(UINT8 dispHealth, 
                                                           BOOLEAN bNewData);
static void                  DispProcessingApp_D_HLTHResult(UINT8 dispHealth, 
                                                            UINT32 no_HS_Timeout_s);
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
static BOOLEAN               PACManual();
static BOOLEAN               ErrorMessage();
static BOOLEAN               AquirePACData();
static BOOLEAN               PACDecision();
static BOOLEAN               PACAcceptOrValidate();
static BOOLEAN               PACRejectORInvalidate();
static void                  DispProcessingApp_SendNewMonData(CHAR characterString[] );
static DISPLAY_BUTTON_STATES DispProcessingApp_GetBtnInput(BOOLEAN bNewData);
static void                  DispProcessingApp_FormatString(DISPLAY_STATE_PTR pScreen);
static DISPLAY_SCREEN_ENUM   DispProcessingApp_GetNextState(DISPLAY_BUTTON_STATES button, 
                                                            BOOLEAN thisScreen,
                                                            DISPLAY_STATE_PTR pParamScreen);
static void                  DispProcessingApp_NoButtonData(void);
static void                  DispProcessingApp_CreateTransLog(DISPLAY_LOG_RESULT_ENUM result,
                                                              DISPLAY_SCREEN_ENUM previousScreen,
                                                              DISPLAY_SCREEN_ENUM currentScreen,
                                                              BOOLEAN reset);
static void                  DispProcessingApp_RestoreAppData(void);
#include "DispProcessingUserTables.c"

static DISPLAY_STATE menuStateTbl[DISPLAY_SCREEN_COUNT] =
{
//Menu  Menu String                 DblClkUP DblClkDOWN DblClkLEFT DblClkRIGHT DblClkENTER DblClkEVENT UP   DOWN LEFT  RIGHT ENTER EVENT      None  Var:  1   2   3   4   5   6
/*M0 */{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M1 */{"TO BE INITIALIZED       ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, M04,  M02,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M2 */{"TO BE INITIALIZED       ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, M01,  M03,  M06,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M3 */{"TO BE INITIALIZED       ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, M02,  M04,  A02,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M4 */{"TO BE INITIALIZED       ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, M03,  M01,  A08,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M5 */{"TO BE INITIALIZED       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, M06,  A00,  M07,  NO_ACTION, A01,        0, 15,  1, -1, -1, -1},
/*M6 */{"HTR/COND SOVOFF CONFIRM?", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, M02,  A00,  A01,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1}, 
/*M7 */{"SEL ENGINE  START PAC?  ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A14,  A00,  A16,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M8 */{"RUNNING PAC 111111111111", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, A18,        2, -1, -1, -1, -1, -1},
/*M9 */{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M10*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M11*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M12*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M13*/{"TO BE INITIALIZED       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A23,  A23,  A27,  NO_ACTION, NO_ACTION,  3,  5,  4,  6, -1, -1},
/*M14*/{"TO BE INITIALIZED       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A22,  A22,  A28,  NO_ACTION, NO_ACTION,  3,  5,  4,  6, -1, -1},
/*M15*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M16*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M17*/{"11111111111122222 RETRY?", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  M02,  NO_ACTION, A21,        7,  8, -1, -1, -1, -1},
/*M18*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M19*/{"11111111111 22222       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A03, A04, M03,  A00,  A05,  NO_ACTION, NO_ACTION,  9, 10, -1, -1, -1, -1},
/*M20*/{"11111 222 334 55555 6666", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A06, A07, A02,  A00,  A00,  NO_ACTION, NO_ACTION, 11, 12, -1, -1, -1, -1},
/*M21*/{"DBL CLICK   RATE     112", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A09, A10, M04,  A00,  A11,  NO_ACTION, NO_ACTION, 13, 14, -1, -1, -1, -1},
/*M22*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M23*/{"PRFRM MANUALPAC   READY?", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A18,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M24*/{"VALIDATE PACTO COMPUTED?", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A24,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M25*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M26*/{"TO BE INITIALIZED       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A25,  A25,  A27,  NO_ACTION, NO_ACTION,  3,  5,  4,  6, -1, -1},
/*M27*/{"TO BE INITIALIZED       ", A00,     A00,       A13,       A00,        A00,        NO_ACTION,  A00, A00, A24,  A24,  A28,  NO_ACTION, NO_ACTION,  3,  5,  4,  6, -1, -1},
/*M28*/{"VALID KEYS:             ", A00,     A00,       A00,       A00,        A00,        A00,        A00, A00, A00,  A00,  A00,  A00,       A00,       -1, -1, -1, -1, -1, -1},
/*M29*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
/*M30*/{"UNUSED                  ", A00,     A00,       A00,       A00,        A00,        NO_ACTION,  A00, A00, A00,  A00,  A00,  NO_ACTION, NO_ACTION, -1, -1, -1, -1, -1, -1},
};

static DISPLAY_ACTION_TABLE m_ActionTable[MAX_ACTIONS_COUNT] =
{
//A#    Function,              Return True, Return False
/*A00*/{InvalidButtonInput,    NO_ACTION,   M28},
/*A01*/{GetConfig,             M05,         M05},
/*A02*/{RecallDate,            M19,         M19},
/*A03*/{RecallNextDate,        A02,         A02},
/*A04*/{RecallPreviousDate,    A02,         A02},
/*A05*/{RecallPAC,             M20,         M20},
/*A06*/{RecallNextPAC,         A05,         A05},
/*A07*/{RecallPreviousPAC,     A05,         A05},
/*A08*/{SavedDoubleClick,      M21,         M21},
/*A09*/{IncDoubleClick,        A08,         A08},
/*A10*/{DecDoubleClick,        A08,         A08},
/*A11*/{SaveDoubleClick,       A08,         A08},
/*A12*/{ResetPAC,              M02,         M02},
/*A13*/{ResetPAC,              M01,         M01},
/*A14*/{ResetPAC,              A01,         A01},
/*A15*/{ResetPAC,              M06,         M06},
/*A16*/{CorrectSetup,          A17,         A21},
/*A17*/{ValidateMenu,          M23,         A18},
/*A18*/{RunningPAC,            A19,         M08},
/*A19*/{PACResult,             A20,         A21},
/*A20*/{PACManual,             M24,         A22},
/*A21*/{ErrorMessage,          M17,         M17},
/*A22*/{AquirePACData,         M13,         M13},
/*A23*/{AquirePACData,         M14,         M14},
/*A24*/{AquirePACData,         M26,         M26},
/*A25*/{AquirePACData,         M27,         M27},
/*A26*/{PACDecision,           M02,         M02},
/*A27*/{PACAcceptOrValidate,   A26,         A26},
/*A28*/{PACRejectORInvalidate, A26,         A26}

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
 * Parameters:  BOOLEAN bEnable - Only initializes the task if the APAC manager
 *                                is active.
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void DispProcessingApp_Initialize(BOOLEAN bEnable)
{  
  DISPLAY_SCREEN_STATUS_PTR pStatus = 
      (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;
  
  // Local Data
  TCB dispTaskInfo, debugTaskInfo;
  
  // Sets both Current and previous screen to M1 and 
  // both bNewData and bPBITComplete to FALSE
  memset(&m_DispProcApp_Status, 0,      sizeof(m_DispProcApp_Status));
  memset(&m_DispProcRX_Info,    0,      sizeof(m_DispProcRX_Info)   );
  memset(&m_DispProcApp_Cfg,    0,      sizeof(m_DispProcApp_Cfg)   );
  m_DispProcApp_Status.buttonInput    = NO_PUSH_BUTTON_DATA;
  m_DispProcApp_Status.currentScreen  = (DISPLAY_SCREEN_ENUM)HOME_SCREEN;
  m_DispProcApp_Status.previousScreen = (DISPLAY_SCREEN_ENUM)HOME_SCREEN;
  m_DispProcApp_Status.nextScreen     = (DISPLAY_SCREEN_ENUM)HOME_SCREEN;
  m_DispProcApp_Status.displayHealth  = D_HLTH_PBIT_ACTIVE;
  m_DispProcApp_Status.partNumber     = PART_NUMBER_INIT;
  snprintf((char*)m_DispProcApp_Status.versionNumStr, 
           DISP_PRINT_PARAM(VERSION_NUMBER_INIT));

  User_AddRootCmd(&dispProcAppRootTblPtr);

  memcpy(&m_DispProcApp_Cfg, &CfgMgr_RuntimeConfigPtr()->DispProcAppConfig,
      sizeof(m_DispProcApp_Cfg));
  // Set the DIO address and Sensor validation flag for the Display Discretes.
  DIO_DispProtocolSetAddress((const char*)&m_DispProcRX_Info.discreteState[0],
                             (const char*)&discreteDIOStatusFlag,
                             (const char*)&m_DispProcApp_Status.displayHealth,
                             (const char*)&dio_Validity_Status);
  dataLossFlag = PWCDispProtocol_DataLossFlag();
  // Restore App Data
  DispProcessingApp_RestoreAppData();
  savedDCRATE = m_DispProcApp_AppData.lastDCRate;
  // Assign the Invalid button time to the APAC Manager
  APACMgr_IF_InvldDelayTimeOutVal(&m_DispProcApp_Cfg.invalidButtonTime_ms);
  // Convert configurable times to ticks.
  invalidButtonTime_Converted = (UINT16)(m_DispProcApp_Cfg.invalidButtonTime_ms
                                / INVBUTTON_TIME_CONVERSION);
  autoAbortTime_Converted     = (UINT16)(m_DispProcApp_Cfg.autoAbortTime_s 
                                * AUTO_ABORT_CONVERSION);
  no_HS_Timeout_Converted     = (UINT16)(m_DispProcApp_Cfg.no_HS_Timeout_s 
                                * NO_HS_TIMEOUT_CONVERSION);

  m_DispProcApp_Debug.bDebug     = FALSE;
  m_DispProcApp_Debug.bNewFrame  = FALSE;
  
  // Initialize Character Strings that require specific formatting.
  DispProcessingApp_InitStrings();

  m_DispProcRX_Info.displayHealth = D_HLTH_PBIT_ACTIVE;
  m_DispProcRX_Info.partNumber    = DEFAULT_PART_NUMBER;
  m_DispProcRX_Info.versionNumber[VERSION_MAJOR_NUMBER] = 
    DEFAULT_VERSION_NUMBER_MJR;
  m_DispProcRX_Info.versionNumber[VERSION_MINOR_NUMBER] = 
    DEFAULT_VERSION_NUMBER_MNR;
  
  // Update Status
  CM_GetTimeAsTimestamp((TIMESTAMP *) &pStatus->lastFrameTS);
  pStatus->lastFrameTime = CM_GetTickCount();

  // Create Display Processing App Task
  if (bEnable){
    // Create Display Processing App Task
    memset(&dispTaskInfo, 0, sizeof(dispTaskInfo));
    strncpy_safe(dispTaskInfo.Name, sizeof(dispTaskInfo.Name),
                 "Display Processing Application", _TRUNCATE);
    dispTaskInfo.TaskID         = DispProcApp_Handler;
    dispTaskInfo.Function       = DispProcessingApp_Handler;
    dispTaskInfo.Priority       = taskInfo[DispProcApp_Handler].priority;
    dispTaskInfo.Type           = taskInfo[DispProcApp_Handler].taskType;
    dispTaskInfo.modes          = taskInfo[DispProcApp_Handler].modes;
    dispTaskInfo.MIFrames       = taskInfo[DispProcApp_Handler].MIFframes;
    dispTaskInfo.Rmt.InitialMif = taskInfo[DispProcApp_Handler].InitialMif;
    dispTaskInfo.Rmt.MifRate    = taskInfo[DispProcApp_Handler].MIFrate;
    dispTaskInfo.Enabled        = TRUE;
    dispTaskInfo.Locked         = FALSE;
    dispTaskInfo.pParamBlock    = &dispProcAppBlock;
    TmTaskCreate(&dispTaskInfo);
    
    // Create Display Processing App Display Task
    memset(&debugTaskInfo, 0, sizeof(debugTaskInfo));
    strncpy_safe(debugTaskInfo.Name, sizeof(debugTaskInfo.Name),
                 "Display Processing Debug",_TRUNCATE);
    debugTaskInfo.TaskID         = DispProcApp_Debug;
    debugTaskInfo.Function       = DispProcAppDebug_Task;
    debugTaskInfo.Priority       = taskInfo[DispProcApp_Debug].priority;
    debugTaskInfo.Type           = taskInfo[DispProcApp_Debug].taskType;
    debugTaskInfo.modes          = taskInfo[DispProcApp_Debug].modes;
    debugTaskInfo.MIFrames       = taskInfo[DispProcApp_Debug].MIFframes;
    debugTaskInfo.Rmt.InitialMif = taskInfo[DispProcApp_Debug].InitialMif;
    debugTaskInfo.Rmt.MifRate    = taskInfo[DispProcApp_Debug].MIFrate;
    debugTaskInfo.Enabled        = TRUE;
    debugTaskInfo.Locked         = FALSE;
    debugTaskInfo.pParamBlock    = &dispProcAppDisplayDebugBlock;
    TmTaskCreate (&debugTaskInfo);
  }
}


/******************************************************************************
 * Function:    DispProcessingApp_Handler
 *
 * Description: Processes the decoded data from the PWC Display Protocol and 
 *              provides messages to be encoded for transmission. 
 *
 * Parameters:  pParam - Not used in this function.
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
  DISPLAY_SCREEN_ENUM        frameScreen;
  DISPLAY_DEBUG_PTR          pDebug;
  UINT16                     i;
  
  pStatus     = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  bNewData    = (BOOLEAN*)&pStatus->bNewData;
  pScreen     = (DISPLAY_STATE_PTR)(&(menuStateTbl[0]) + 
                (UINT16)pStatus->currentScreen);
  frameScreen = pStatus->currentScreen;
  pDebug      = (DISPLAY_DEBUG_PTR)&m_DispProcApp_Debug;

  // Look for new RX data
  *bNewData = DispProcessingApp_GetNewDispData();
  
  // Get available button input
  pStatus->buttonInput = DispProcessingApp_GetBtnInput(*bNewData);

  // Handle the Auto Abort timer and the Manual return to M1.
  pScreen = DispProcessingApp_AutoAbortValid(pScreen, 
                                             (UINT16)pStatus->buttonInput);
  //Process MenuScreen based on button input received by the protocol
  pStatus->nextScreen = DispProcessingApp_GetNextState(pStatus->buttonInput, 
    TRUE, NULL);

  if (pStatus->nextScreen != NO_ACTION){ 
    // Update Previous screen
    pStatus->previousScreen = pStatus->currentScreen;

    while((UINT16)pStatus->nextScreen >= ACTION_ENUM_OFFSET){ 
      // If the next screen requires an action process the action.
      pStatus->nextScreen = 
        DispProcessingApp_PerformAction(pStatus->nextScreen);
    }
    // Update status and format the new screen.
    pStatus->currentScreen = pStatus->nextScreen;
    pScreen = (DISPLAY_STATE_PTR)(&(menuStateTbl[0]) + 
              (UINT16)pStatus->currentScreen);
    DispProcessingApp_FormatString(pScreen);
    pScreen = (DISPLAY_STATE_PTR)(&(menuStateTbl[0]) +
        (UINT16)pStatus->currentScreen);
  }

  if (pStatus->buttonInput != NO_PUSH_BUTTON_DATA ||
      frameScreen != pStatus->currentScreen){
    pStatus->bNewDebugData = TRUE;
    DispProcessingApp_CreateTransLog(DISPLAY_LOG_TRANSITION, 
      pStatus->previousScreen, pStatus->currentScreen, FALSE);
  }

  // Update Debug menu based on if the menu string has changed.
  for (i = 0; i < MAX_SCREEN_SIZE; i++){
    if (pStatus->bNewDebugData != TRUE && pDebug->bDebug == TRUE){
      pStatus->bNewDebugData = (pStatus->charString[i]==pScreen->menuString[i])
                                ? pStatus->bNewDebugData : TRUE;
    }
    else{
      break;
    }
  }

  // Copy the menu string into the status string.
  memcpy((void *)pStatus->charString, (const void*)pScreen->menuString, 
         MAX_SCREEN_SIZE);
  // Update the Debug display if there is any new significant RX data.
  if (pDebug->bNewFrame != TRUE){
    pDebug->bNewFrame = pStatus->bNewDebugData;
    pDebug->buttonInput = pStatus->buttonInput;
  }
  pStatus->bNewDebugData = FALSE;
  
  // Update the Protocol with new character string and DCRATE
  DispProcessingApp_SendNewMonData(pScreen->menuString);

  // Update Status
  CM_GetTimeAsTimestamp((TIMESTAMP *)&pStatus->lastFrameTS);
  pStatus->lastFrameTime = CM_GetTickCount();
}
/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/******************************************************************************
 * Function:    DispProcessingApp_InitStrings
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
void DispProcessingApp_InitStrings()
{
  DISPLAY_STATE_PTR        pScreen;
  DISPLAY_BUTTON_TABLE_PTR pButtons;
  UINT16                   i;
  
  pScreen  = (DISPLAY_STATE_PTR) &menuStateTbl[0];
  pButtons = (DISPLAY_BUTTON_TABLE_PTR)&validButtonTable[0];
  
  snprintf((pScreen + (UINT16)M01)->menuString, 
           MAX_SCREEN_SIZE + 1, "ETM ACTIVE  %c%c CMD?     ",  0x04, 0x02);
  snprintf((pScreen + (UINT16)M02)->menuString, 
           MAX_SCREEN_SIZE + 1, "ETM ACTIVE  %c%c RUN PAC? ",  0x04, 0x02);
  snprintf((pScreen + (UINT16)M03)->menuString, 
           MAX_SCREEN_SIZE + 1, "ETM ACTIVE  %c%c RCL PAC? ",  0x04, 0x02);
  snprintf((pScreen + (UINT16)M04)->menuString,
           MAX_SCREEN_SIZE + 1, "ETM ACTIVE  %c%c SETTINGS?",  0x04, 0x02);
  snprintf((pScreen + (UINT16)M05)->menuString,
           MAX_SCREEN_SIZE + 1, "CONFRM 11111222%c ISSUE 3",   0x25);
  snprintf((pScreen + (UINT16)M13)->menuString, 
           MAX_SCREEN_SIZE + 1, "E12 ITT 344CACP NG 5666%c",   0x25);
  snprintf((pScreen + (UINT16)M14)->menuString, 
           MAX_SCREEN_SIZE + 1, "E12 ITT 344CRJT NG 5666%c",   0x25);
  snprintf((pScreen + (UINT16)M26)->menuString, 
           MAX_SCREEN_SIZE + 1, "E12 ITT 344CVLD NG 5666%c",   0x25);
  snprintf((pScreen + (UINT16)M27)->menuString, 
           MAX_SCREEN_SIZE + 1, "E12 ITT 344CNVL NG 5666%c",   0x25);

  // Format stings for Invalid Push Button Screen
  for (i = 0; i < DISPLAY_VALID_BUTTONS; i++){
    switch (pButtons->id){
      case RIGHT_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "%c", 0x02);
        break;
      case LEFT_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "%c", 0x04);
        break;
      case ENTER_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "%c", 0x05);
        break;
      case UP_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "%c", 0x01);
        break;
      case DOWN_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "%c", 0x03);
        break;
      case EVENT_BUTTON:
        snprintf((char *)pButtons->name, pButtons->size + 1, "E");
        break;
      case RIGHT_DBLCLICK:
        snprintf((char *)pButtons->name, sizeof(pButtons->name), "%c%c", 0x02,
                 0x02);
        break;
      case LEFT_DBLCLICK:
        snprintf((char *)pButtons->name, sizeof(pButtons->name), "%c%c", 0x04,
                 0x04);
        break;
      case ENTER_DBLCLICK:
        snprintf((char *)pButtons->name, sizeof(pButtons->name), "%c%c", 0x05,
                 0x05);
        break;
      case UP_DBLCLICK:
        snprintf((char *)pButtons->name, sizeof(pButtons->name), "%c%c", 0x01,
                 0x01);
        break;
      case DOWN_DBLCLICK:
        snprintf((char *)pButtons->name, sizeof(pButtons->name), "%c%c", 0x03,
                 0x03);
        break;
      case EVENT_DBLCLICK:
        snprintf((char *)pButtons->name, pButtons->size + 1, "EE");
        break;
      default:
        break;
    }
    pButtons++;
  }
}

/******************************************************************************
 * Function:    DispProcessingApp_GetNewDispData
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
BOOLEAN DispProcessingApp_GetNewDispData( void )
{
  BOOLEAN bNewData, bButtonReset;
  UINT16 i;
  UINT8 tempChar;
  UARTMGR_PROTOCOL_DATA_PTR  pData;
  DISPLAY_RX_INFORMATION_PTR pDest;
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_SCREEN_CONFIG_PTR  pCfg;

  pData    = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_RXData[0];
  pDest    = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  pStatus  = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pCfg     = (DISPLAY_SCREEN_CONFIG_PTR)&m_DispProcApp_Cfg;
  bNewData = PWCDispProtocol_Read_Handler(pData, 
                                         (UINT32)UARTMGR_PROTOCOL_PWC_DISPLAY,
                                         (UINT16)RX_PROTOCOL, 
                                         (UINT16)PWCDISP_RX_ELEMENT_COUNT);
  bButtonReset = TRUE;
  
  if(bNewData == TRUE){
    // Check to see if all new button presses are FALSE and update bButtonReset
    for(i = 1; i <= DISPLAY_VALID_BUTTONS; i++){
      if((BOOLEAN)(pData + i)->data.value != FALSE){
        bButtonReset = FALSE;
      }
    }
    
    // If all previous button states were FALSE then accept new presses
    if(pStatus->bButtonReset == TRUE && bButtonReset == FALSE){
      for (i = 0; i < BUTTON_STATE_COUNT; i++){
        if( i >= UNUSED1_BUTTON){
          if(i == EVENT_BUTTON){
            pDest->buttonState[i]   = 
              (BOOLEAN)(pData + (UINT16)BUTTONSTATE_EVENT)->data.value;
            pDest->dblClickState[i] = 
              (BOOLEAN)(pData + (UINT16)DOUBLECLICK_EVENT)->data.value;
          }
        }
        else{
          pDest->buttonState[i]   = 
            (BOOLEAN)(pData + (UINT16)BUTTONSTATE_RIGHT + i)->data.value;
          pDest->dblClickState[i] = 
            (BOOLEAN)(pData + (UINT16)DOUBLECLICK_RIGHT + i)->data.value;
        }
      }
    }
    else{                                                                          
      // There were button states set to true for the last set of data received
      // Ignore all button presses.                                            
        DispProcessingApp_NoButtonData();
    }                                                                          
                                                                               
    for(i = 0; i < (UINT16)DISCRETE_STATE_COUNT; i++){
      if (pDest->discreteState[i] != 
          (BOOLEAN)(pData + (UINT16)DISCRETE_ZERO + i)->data.value){
        // Update Debug Display if there are discrete changes
        pStatus->bNewDebugData = TRUE;
      }
      pDest->discreteState[i] = (BOOLEAN)(pData + 
                                (UINT16)DISCRETE_ZERO + i)->data.value;
    }
    
    pDest->displayHealth = (UINT8)(pData + (UINT16)DISPLAY_HEALTH)->data.value;
    
    // Update button reset status
    pStatus->bButtonReset = bButtonReset;
    
    pDest->partNumber = (UINT8)((pData + (UINT16)PART_NUMBER)->data.value);
    pStatus->partNumber = pDest->partNumber;

    pDest->versionNumber[VERSION_MAJOR_NUMBER] = 
     (UINT8) ((pData + (UINT16)VERSION_NUMBER)->data.value >> 4);

    tempChar = (UINT8)((pData + (UINT16)VERSION_NUMBER)->data.value << 4);
    
    pDest->versionNumber[VERSION_MINOR_NUMBER] = (UINT8)tempChar >> 4;
    pStatus->versionNumber = (UINT8)(pData+(UINT16)VERSION_NUMBER)->data.value;
    snprintf((char*)pStatus->versionNumStr, MAX_SUBSTRING_SIZE, "%d.%d", 
             pDest->versionNumber[VERSION_MAJOR_NUMBER], 
             pDest->versionNumber[VERSION_MINOR_NUMBER]);
    newDataTimer = 0;
  }
  else{
    //set the discrete DIO Status Flag to FALSE if no RX messages are received
    //within 150 ms.
    if(++newDataTimer >= (pCfg->disc_Fail_ms/10)){
      discreteDIOStatusFlag = FALSE; 
      dio_Validity_Status   = *dataLossFlag;
    }
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
* Returns:     DISPLAY_STATE_PTR pScreen - The current screen reset or unreset
*
* Notes:       None
*
*****************************************************************************/
static
DISPLAY_STATE_PTR DispProcessingApp_AutoAbortValid(DISPLAY_STATE_PTR pScreen,
	                                               UINT16 buttonInput)
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;

  pStatus = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;

  if (buttonInput == (UINT16)NO_PUSH_BUTTON_DATA){
    // Increment Auto Abort Timer
    auto_Abort_Timer++;
    
    if (auto_Abort_Timer == autoAbortTime_Converted){
      // Update Transition Log, Reset APAC.
      if(pStatus->currentScreen != (DISPLAY_SCREEN_ENUM)HOME_SCREEN){
        DispProcessingApp_CreateTransLog(DISPLAY_LOG_TIMEOUT, 
          pStatus->currentScreen, (DISPLAY_SCREEN_ENUM)HOME_SCREEN, TRUE);
        APACMgr_IF_Reset(NULL, NULL, NULL, NULL, APAC_IF_TIMEOUT);
        pStatus->previousScreen = pStatus->currentScreen;
      }
      // Auto Abort Timer has expired. Return to Screen M1. 
      pStatus->currentScreen  = (DISPLAY_SCREEN_ENUM)HOME_SCREEN;
      auto_Abort_Timer        = 0;
      pScreen = (DISPLAY_STATE_PTR)&(menuStateTbl[0]) +
	             (UINT16)pStatus->currentScreen;
    }
  }
  else{
    // reset the auto abort timer
    auto_Abort_Timer = 0;
  }
  return(pScreen);
}

/******************************************************************************
* Function:    DispProcessingApp_PerformAction
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
DISPLAY_SCREEN_ENUM 
  DispProcessingApp_PerformAction(DISPLAY_SCREEN_ENUM nextAction)
{
  DISPLAY_ACTION_TABLE_PTR pAction;

  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
            ((UINT16)nextAction - ACTION_ENUM_OFFSET);
  return(pAction->action() == TRUE ? pAction->retTrue : pAction->retFalse);
}

/******************************************************************************
 * Function:    DispProcessingApp_D_HLTHcheck
 *
 * Description: Function simply checks the D_HLTH byte and logs an error if 
 *              it has been nonzero too long.
 *
 * Parameters:  dispHealth - the D_HLTH byte from the encoded rx packet.
 *              bNewData   - Flag signalling new data from the Protocol.
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

  if(dispHealth == D_HLTH_ACTIVE) {
    pStatus->dispHealthTimer = 0;
    if (bNewData == TRUE){ 
      // Ensure that these elements are not set at startup to TRUE
      pStatus->bPBITComplete   = TRUE;
      pStatus->bButtonEnable   = TRUE;
      discreteDIOStatusFlag    = TRUE;
      dio_Validity_Status      = (UINT8)DIO_DISPLAY_VALID;
    }
    if (pStatus->displayHealth != dispHealth){ 
      pStatus->displayHealth = dispHealth;
      pStatus->bNewDebugData = TRUE;
    }
  }
  // For critical D_HLTH codes, call D_HLTH inspector 
  else if (dispHealth == D_HLTH_COM_RX_FAULT      || 
           dispHealth == D_HLTH_INOP_SIGNAL_FAULT ||
           dispHealth == D_HLTH_MON_INOP_FAULT    || 
           dispHealth == D_HLTH_DISPLAY_FAULT){
    if(pStatus->displayHealth != dispHealth){ //Fresh critical D_HLTH code.
      // Update the Debug Display, Status, and Log
      pStatus->displayHealth = dispHealth;
      pStatus->bNewDebugData = TRUE;
      DispProcessingApp_D_HLTHResult(dispHealth, 0);
    }
    pStatus->bButtonEnable = FALSE;
    discreteDIOStatusFlag  = FALSE;
  	pStatus->dispHealthTimer++;
    dio_Validity_Status    = (UINT8)DIO_DISPLAY_HEALTH_CODE_FAIL;
  }
  // Set timer for PBIT and diagnostic D_HLTH codes
  else{
    pStatus->displayHealth = dispHealth;
  	// Report error if the Max D_HLTH nonzero wait time is reached
  	if (pStatus->dispHealthTimer == no_HS_Timeout_Converted){
      DispProcessingApp_D_HLTHResult(dispHealth, pCfg->no_HS_Timeout_s);

      // Update the Debug Display when D_HLTH is nonzero too long.
      pStatus->bNewDebugData   = TRUE;
  	}
    pStatus->dispHealthTimer++;  	
    pStatus->bButtonEnable = FALSE;
    discreteDIOStatusFlag  = FALSE;
  }
}

/******************************************************************************
* Function:    DispProcessingApp_D_HLTHResult
*
* Description: Function simply checks the D_HLTH byte and returns the proper 
               RESULT.
*
* Parameters:  dispHealth      - the D_HLTH byte from the encoded rx packet.
*              no_HS_Timeout_s - The max nonzero D_HLTH time before timeout.
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static
void DispProcessingApp_D_HLTHResult(UINT8 dispHealth, UINT32 no_HS_Timeout_s)
{
  DISPLAY_LOG_RESULT_ENUM result;
  DISPLAY_DHLTH_LOG       displayHealthTOLog;
  switch (dispHealth){
    case D_HLTH_PBIT_ACTIVE:
      result = DISPLAY_LOG_PBIT_ACTIVE;
      break;
    case D_HLTH_PBIT_PASS:
      result = DISPLAY_LOG_PBIT_PASS;
      break;
    case D_HLTH_MON_INOP_FAULT:
      result = DISPLAY_LOG_INOP_MON_FAULT;
      break;
    case D_HLTH_INOP_SIGNAL_FAULT:
      result = DISPLAY_LOG_INOP_SIGNAL_FAULT;
      break;
    case D_HLTH_COM_RX_FAULT:
      result = DISPLAY_LOG_RX_COM_FAULT;
      break;
    case D_HLTH_DISPLAY_FAULT:
      result = DISPLAY_LOG_DISPLAY_FAULT;
      break;
    case D_HLTH_DIAGNOSTIC:
      result = DISPLAY_LOG_DIAGNOSTIC_MODE;
      break;
    default:
      result = DISPLAY_LOG_D_HLTH_UNKNOWN;
      break;
  }
   
  // Process Log
  displayHealthTOLog.result          = result;
  displayHealthTOLog.d_HLTHcode      = dispHealth;
  displayHealthTOLog.no_HS_Timeout_s = no_HS_Timeout_s;
  CM_GetTimeAsTimestamp((TIMESTAMP *)&displayHealthTOLog.lastFrameTime);

  LogWriteETM(SYS_ID_DISPLAY_APP_DHLTH_TO, LOG_PRIORITY_LOW,
      &displayHealthTOLog, sizeof(displayHealthTOLog), NULL);
}

/******************************************************************************
 * Function:    DispProcessingApp_SendNewMonData
 *
 * Description: Utility function that sends out updated data to be encoded by
 *              the PWC Display Protocol.
 *
 * Parameters:  CHAR characterString - contains the unencoded character 
 *                                        string for the current display  
 *                                        screen                                          
 *
 * Returns:     None
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
void DispProcessingApp_SendNewMonData(CHAR characterString[])
{
  UARTMGR_PROTOCOL_DATA_PTR pProtData, pCharData;
  DISPLAY_APP_DATA_PTR      pData;
  UINT8                    *pString;
  UINT16                    i;
  
  pProtData     = (UARTMGR_PROTOCOL_DATA_PTR)&m_PWCDisp_TXData[0];
  pData         = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pString       = (UINT8*) &characterString[0];
  pCharData     = pProtData + (UINT16)CHARACTER_0;
  
  // Copy character string into the outgoing TX packet
  for(i = 0; i < MAX_SCREEN_SIZE; i++){
    pCharData->data.value = *pString;
    pCharData++; pString++;
  }
  
  (pProtData + (UINT16)DC_RATE)->data.value =
    (UINT32)(pData->lastDCRate * DCRATE_CONVERSION);
   
  PWCDispProtocol_Write_Handler(pProtData, 
                                (UINT32)UARTMGR_PROTOCOL_PWC_DISPLAY,
                                (UINT16)TX_PROTOCOL, 
                                (UINT16)PWCDISP_TX_ELEMENT_CNT_COMPLETE);
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
  DISPLAY_STATE_PTR         pScreen, pInvalidScreen;
  DISPLAY_BUTTON_TABLE_PTR    pButtons;
  DISPLAY_SCREEN_ENUM       checkState;
  UINT16                    i, size, charCount, max;
  CHAR                      strInsert[MAX_SCREEN_SIZE] = "";

  pStatus         = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction         = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
                    ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen         = (DISPLAY_STATE_PTR)&menuStateTbl[0] + 
                    (UINT16)pAction->retFalse;
  pButtons        = (DISPLAY_BUTTON_TABLE_PTR)&validButtonTable[0];
  size            = MAX_SCREEN_SIZE / 2;
  charCount       = 0;

  if (invalid_Button_Timer >= invalidButtonTime_Converted){
    // timer expired. return to the previous screen. Update previous screen
    pAction->retTrue        = invalidButtonScreen;
    pStatus->previousScreen = pStatus->currentScreen;
    invalid_Button_Timer    = 0;
    pStatus->bInvalidButton = FALSE;
  }
  else{
    invalidButtonScreen = (pStatus->previousScreen != pAction->retFalse) 
                        ?  pStatus->previousScreen :  invalidButtonScreen;
    // return to M28 regardless of button press
    pAction->retTrue        = pAction->retFalse;
    pStatus->bInvalidButton = TRUE;
    // Increment the invalid button timer
    invalid_Button_Timer++;
  }
  //Update Invalid Screen Pointer
  pInvalidScreen = (DISPLAY_STATE_PTR)&menuStateTbl[0] +
      (UINT16)invalidButtonScreen;
  // Perform check to see what buttons are valid and add them to display.
  for (i = 0; i < DISPLAY_VALID_BUTTONS; i++){
    checkState = DispProcessingApp_GetNextState(
      (DISPLAY_BUTTON_STATES)pButtons->id, FALSE, pInvalidScreen);
    if (checkState != pStatus->nextScreen && checkState != NO_ACTION){
      if (charCount == 0){
        // Add symbol minus the comma
        strcat(strInsert, (const char *)pButtons->name);
        charCount += pButtons->size;
      }
      else if ((charCount + pButtons->size + 1) <= size){
        // Add symbol plus the comma
        strcat(strInsert, (const char *)(","));
        strcat(strInsert, (const char *)pButtons->name);
        charCount += (pButtons->size + 1);
      }
    }
    // Check next possible button
    pButtons++;
  }
  // Update Max
  max = size + (charCount - 1);

  for (i = size; i < MAX_SCREEN_SIZE; i++){
    if (i <= max){
      pScreen->menuString[i] = strInsert[i - size];
    }
    else{
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
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN GetConfig()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_VARIABLE_TABLE_PTR pVar, ac_Cfg, chart_Rev, power;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus   = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pVar      = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction   = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
              ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen   = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  ac_Cfg    = pVar + pScreen->var1;
  power     = pVar + pScreen->var2;
  chart_Rev = pVar + pScreen->var3;

  return(APACMgr_IF_Config(ac_Cfg->strInsert, chart_Rev->strInsert,
                           power->strInsert, NULL, APAC_IF_MAX));
}

/******************************************************************************
 * Function:    RecallDate
 *
 * Description: Acquires the current recalled Date.
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallDate()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, date, time;
  DISPLAY_STATE_PTR pScreen;
  DISPLAY_ACTION_TABLE_PTR pAction;
  BOOLEAN bResult;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
            ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  date    = pVar + pScreen->var1;
  time    = pVar + pScreen->var2;

  if (pStatus->currentScreen != pAction->retTrue){
    bResult = APACMgr_IF_HistoryReset(NULL, NULL, NULL, NULL, APAC_IF_MAX);
  }

  bResult = APACMgr_IF_HistoryData(date->strInsert, time->strInsert, NULL, NULL,
      APAC_IF_DATE);

  return(bResult);
}

/******************************************************************************
 * Function:    RecallNextDate
 *
 * Description: Selects the next saved Date in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallNextDate()
{
  return(APACMgr_IF_HistoryAdv(NULL, NULL, NULL, NULL, APAC_IF_DATE));
}

/******************************************************************************
 * Function:    RecallPreviousDate
 *
 * Description: Selects the previous saved Date in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPreviousDate()
{
  return(APACMgr_IF_HistoryDec(NULL, NULL, NULL, NULL, APAC_IF_DATE));
}

/******************************************************************************
 * Function:    RecallPAC
 *
 * Description: Acquires the current recalled PAC.
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPAC()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, line1, line2;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
            ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  line1   = pVar + pScreen->var1;
  line2   = pVar + pScreen->var2;
  
  return(APACMgr_IF_HistoryData(line1->strInsert, line2->strInsert, NULL, NULL,
                                APAC_IF_DAY));
}

/******************************************************************************
 * Function:    RecallNextPAC
 *
 * Description: Selects the next saved PAC in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallNextPAC()
{
  return(APACMgr_IF_HistoryAdv(NULL, NULL, NULL, NULL, APAC_IF_DAY));
}

/******************************************************************************
 * Function:    RecallPreviousPAC
 *
 * Description: Selects the Previous saved PAC in the Buffer
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RecallPreviousPAC()
{
  return(APACMgr_IF_HistoryDec(NULL, NULL, NULL, NULL, APAC_IF_DAY));
}

/******************************************************************************
 * Function:    SavedDoubleClick
 *
 * Description: Acquires the double click data and saves it to the proper 
 *              proper variable storage.
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN SavedDoubleClick()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_APP_DATA_PTR       pData;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_VARIABLE_TABLE_PTR pVar, dcRate, changed;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pData   = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pVar    = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pAction = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
            ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  dcRate  = pVar + pScreen->var1;
  changed = pVar + pScreen->var2;
  
  if (pStatus->previousScreen != pAction->retTrue){
    scaledDCRATE = savedDCRATE;
  }
  if(scaledDCRATE > 0){
    sprintf(dcRate->strInsert, "+%d", scaledDCRATE);
  }
  else if (scaledDCRATE == 0){
    sprintf(dcRate->strInsert, " %d", scaledDCRATE);
  }
  else{
    sprintf(dcRate->strInsert, "%d", scaledDCRATE);
  }
  if (pData->lastDCRate == scaledDCRATE){
      changed->strInsert[0] = ' '; 
  }
  else{
    snprintf((char *)(changed)->strInsert,
             sizeof((changed)->strInsert), "%c", CHANGED_CHARACTER);
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
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN IncDoubleClick()
{
  if(scaledDCRATE < DISPLAY_DCRATE_MAX){
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
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN DecDoubleClick()
{
  if(scaledDCRATE > (DISPLAY_DCRATE_MAX * -1)){
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
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN SaveDoubleClick(){
  DISPLAY_APP_DATA_PTR pData;
  
  pData             = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
  pData->lastDCRate = (INT8)(scaledDCRATE);
  savedDCRATE       = scaledDCRATE;
  
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
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN ResetPAC()
{
  return(APACMgr_IF_Reset(NULL, NULL, NULL, NULL, APAC_IF_DCLK));
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
  bValidatePAC = APACMgr_IF_ValidateManual(NULL,NULL,NULL,NULL,APAC_IF_MAX);
  return(bValidatePAC);
}

/******************************************************************************
 * Function:    RunningPAC
 *
 * Description: Verify that the current PAC is still running.
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - PAC Complete
 *              FALSE - PAC Running
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN RunningPAC()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, pac_Status;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction; 
  
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction    = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
               ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen    = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retFalse;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  pac_Status = pVar + pScreen->var1;
  // Update APAC Processing status
  pStatus->bAPACProcessing = TRUE;

  // Run / Continue running PAC.
  return(APACMgr_IF_RunningPAC(pac_Status->strInsert, 
                               NULL, NULL, NULL, APAC_IF_MAX));
}

/******************************************************************************
 * Function:    PACResult
 *
 * Description: Determine whether the completed PAC passed or failed.
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - if the PAC passed
 *              FALSE - if the PAC did not pass 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACResult()
{ 
  m_DispProcApp_Status.bAPACProcessing = FALSE;
  return(APACMgr_IF_CompletedPAC(NULL,NULL,NULL,NULL,APAC_IF_MAX));
}

/******************************************************************************
 * Function:    PACManual
 *
 * Description: Returns indication if 50 Engine Run Hrs has transpired since 
 *              last successful Manual PAC Validation
 *
 * Parameters:  None
 *
 * Returns:     TRUE  - 50 Engine Run Hrs have transpired
 *              FALSE - 50 Engine Run Hrs have not transpired 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACManual()
{ 
  return(bValidatePAC);
}

/******************************************************************************
 * Function:    ErrorMessage
 *
 * Description: Acquires the PAC Failure message from the APACMgr.
 *
 * Parameters:  None
 *
 * Returns:     TRUE
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN ErrorMessage()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, error_Msg, fail_Param;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction    = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
               ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen    = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  error_Msg  = pVar + pScreen->var1;
  fail_Param = pVar + pScreen->var2;
  m_DispProcApp_Status.bAPACProcessing = FALSE;

  // Acquire APAC error message
  return(APACMgr_IF_ErrMsg(error_Msg->strInsert, fail_Param->strInsert,
                           NULL, NULL, APAC_IF_MAX));
}

/******************************************************************************
 * Function:    AquirePACData
 *
 * Description: acquires the engine number, category, NG, and ITT for a PAC
 *
 * Parameters:  None
 *
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN AquirePACData()
{
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  DISPLAY_VARIABLE_TABLE_PTR pVar, eng_Num, cat, ng_Margin, itt_Margin;
  DISPLAY_STATE_PTR          pScreen;
  DISPLAY_ACTION_TABLE_PTR   pAction;
  
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
  pAction    = (DISPLAY_ACTION_TABLE_PTR)&m_ActionTable[0] + 
               ((UINT16)pStatus->nextScreen - ACTION_ENUM_OFFSET);
  pScreen    = (DISPLAY_STATE_PTR)&menuStateTbl[0] + (UINT16)pAction->retTrue;
  pVar       = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];
  eng_Num    = pVar + pScreen->var1;
  cat        = pVar + pScreen->var2;
  itt_Margin = pVar + pScreen->var3;
  ng_Margin  = pVar + pScreen->var4;
  // Update display values
  APACMgr_IF_Result(cat->strInsert, ng_Margin->strInsert,
                    itt_Margin->strInsert, eng_Num->strInsert, APAC_IF_MAX);
  
  if(cat->strInsert[0] != 'A'){
    cat->strInsert[0] = ' ';
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
 * Returns:     TRUE 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static BOOLEAN PACDecision()
{
  APAC_IF_ENUM decision;

  if (bValidatePAC == FALSE){
    decision = (bPACDecision == TRUE) ? APAC_IF_ACPT : APAC_IF_RJCT;
  }
  else{
    decision = (bPACDecision == TRUE) ? APAC_IF_VLD : APAC_IF_INVLD;
  }
  return(APACMgr_IF_ResultCommit(NULL, NULL, NULL, NULL, decision));
}

/******************************************************************************
* Function:    PACAcceptOrValidate
*
* Description: Sets the bPACDecision flag to TRUE
*
* Parameters:  None
*
* Returns:     TRUE
*
* Notes:       None
*
*****************************************************************************/
static BOOLEAN PACAcceptOrValidate()
{
  bPACDecision = TRUE;
  return(bPACDecision);
}

/******************************************************************************
* Function:    PACRejectORInvalidate
*
* Description: Sets the bPACDecision flag to FALSE
*
* Parameters:  None
*
* Returns:     TRUE
*
* Notes:       None
*
*****************************************************************************/
static BOOLEAN PACRejectORInvalidate()
{
    bPACDecision = FALSE;
    return(bPACDecision);
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
 * Function:    DispProcessingApp_GetBtnInput
 *
 * Description: Returns the button pressed by the user. 
 *
 * Parameters:  bNewData - Flag indicating new data from the protocol
 *
 * Returns:     buttonState - The button(or lack there of) the user pushed 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static 
DISPLAY_BUTTON_STATES DispProcessingApp_GetBtnInput(BOOLEAN bNewData)
{
  DISPLAY_RX_INFORMATION_PTR pRX_Info;
  DISPLAY_APP_DATA_PTR       pData;
  UINT16                     i, btnTrueCnt, dblTrueCnt, dblTimeout;
  INT8                       lastDCRate;
  DISPLAY_BUTTON_STATES      buttonState, dblButtonState, tempState;
  DISPLAY_SCREEN_STATUS_PTR  pStatus;
  
  pRX_Info       = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  pData          = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
                   //Convert lastDCRate to Ticks.
  lastDCRate     = (pData->lastDCRate * DCRATE_CONVERSION * 2) / 10;
  buttonState    = NO_PUSH_BUTTON_DATA;
  dblButtonState = NO_PUSH_BUTTON_DATA;
  btnTrueCnt     = 0;
  dblTrueCnt     = 0;
  dblTimeout = (lastDCRate < 0) ? DBLCLICK_TIMEOUT + (UINT16)(lastDCRate * -1):
                                  DBLCLICK_TIMEOUT + (UINT16)lastDCRate;
  pStatus    = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;

  if(bNewData == TRUE){
    for(i = 0; i < (UINT16)BUTTON_STATE_COUNT; i++){
      if(pRX_Info->buttonState[i] == TRUE){
        buttonState = (DISPLAY_BUTTON_STATES)i;
        btnTrueCnt++;
      }
      if(pRX_Info->dblClickState[i] == TRUE){
        dblButtonState = (DISPLAY_BUTTON_STATES)(i + BUTTON_STATE_COUNT);
        dblTrueCnt++;
      }
    }
  }
  if (btnTrueCnt == 0){
    buttonState = NO_PUSH_BUTTON_DATA;
  }
  if (dblTrueCnt == 0){
    dblButtonState = NO_PUSH_BUTTON_DATA;
  }
  if (btnTrueCnt > 1 || dblTrueCnt > 1 || pStatus->bButtonEnable == FALSE
      || dblTrueCnt > btnTrueCnt){
    // Ignore button data if more than one button is pressed
    buttonState   = NO_PUSH_BUTTON_DATA; 
    dblClickFlag  = FALSE;
    dblClickTimer = 0;
  }
  else{
    if(dblClickFlag == TRUE){ 
      // If the dblclick timer has expired use previous button
      if (dblClickTimer >= dblTimeout && buttonState == NO_PUSH_BUTTON_DATA){
        buttonState            = prevButtonState;
        dblClickFlag           = FALSE;
        dblClickTimer          = 0;
      }
      else{
        dblClickTimer++;
        // If button and double click match return the dbl click.
        if (buttonState == prevButtonState && 
            dblButtonState == (buttonState + BUTTON_STATE_COUNT)){
          buttonState            = dblButtonState;
          dblClickFlag           = FALSE;
          dblClickTimer          = 0;
        }
        // If a different button was pressed execute first press and queue 
        // the second for dbl click testing
        else if (buttonState != NO_PUSH_BUTTON_DATA &&
                 dblButtonState == NO_PUSH_BUTTON_DATA){
          tempState              = buttonState;
          buttonState            = prevButtonState;
          prevButtonState        = tempState;
          dblClickTimer          = 0;
        }
        // Extraneous case where button and dbl do not match.
        else if (dblButtonState != NO_PUSH_BUTTON_DATA &&
                (dblButtonState != 
                (buttonState + (DISPLAY_BUTTON_STATES)BUTTON_STATE_COUNT) || 
                dblButtonState != 
                (prevButtonState + (DISPLAY_BUTTON_STATES)BUTTON_STATE_COUNT))){
          buttonState            = NO_PUSH_BUTTON_DATA;
          dblClickFlag           = FALSE;
          dblClickTimer          = 0;
        }
      }
    }
    else{
      if(buttonState != NO_PUSH_BUTTON_DATA &&
         dblButtonState == NO_PUSH_BUTTON_DATA){
        dblClickFlag        = TRUE;
        prevButtonState     = buttonState;
      }
      buttonState = NO_PUSH_BUTTON_DATA;
    }
  }
  pStatus->bNewDebugData = (buttonState != NO_PUSH_BUTTON_DATA) 
                           ? TRUE : pStatus->bNewDebugData;
  return(buttonState);
}

/******************************************************************************
 * Function:    DispProcessingApp_FormatString
 *
 * Description: Updates a Display Character String with proper formatting. 
 *
 * Parameters:  pScreen - pointer to Screen Table
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/

static
void DispProcessingApp_FormatString(DISPLAY_STATE_PTR pScreen)
{
  DISPLAY_VARIABLE_TABLE_PTR   pVar;
  UINT8                        i, j; 
  INT8                         varIndex[MAX_VARIABLES_PER_SCREEN];
  varIndex[0] = pScreen->var1; varIndex[3] = pScreen->var4;
  varIndex[1] = pScreen->var2; varIndex[4] = pScreen->var5;
  varIndex[2] = pScreen->var3; varIndex[5] = pScreen->var6;
  pVar = (DISPLAY_VARIABLE_TABLE_PTR)&m_VariableTable[0];

  for (i = 0; i < MAX_VARIABLES_PER_SCREEN; i++){
    if (varIndex[i] != -1){
      for(j = 0; j < (pVar + varIndex[i])->variableLength; j++){
        pScreen->menuString[(pVar + varIndex[i])->variablePosition + j] = 
          ((pVar + varIndex[i])->strInsert[j] != NULL) ?
          (pVar + varIndex[i])->strInsert[j] : ' ';
      }
    }
  }
}

/******************************************************************************
 * Function:    DispProcessingApp_GetNextState
 *
 * Description: Returns the next state listed in the  MenuStateTbl according
 *              to the button state.
 *
 * Parameters:  button - The current button Input
 *              thisScreen - TRUE: use current screen. FALSE: Manual Screen
 *              pParamScreen - Manual Screen (not used if thisScreen = TRUE)
 *
 * Returns:     DISPLAY_SCREEN_ENUM nextScreen - the next screen based on the 
 *                                               button input 
 *              
 * Notes:       None
 *
 *****************************************************************************/

static 
DISPLAY_SCREEN_ENUM DispProcessingApp_GetNextState(
                                 DISPLAY_BUTTON_STATES button,
                                 BOOLEAN thisScreen, 
                                 DISPLAY_STATE_PTR pParamScreen)
{
  DISPLAY_SCREEN_ENUM       nextScreen;
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_STATE_PTR         pScreen;
  
  pStatus = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;

  if (thisScreen == TRUE){
    pScreen = (DISPLAY_STATE_PTR)&menuStateTbl[0] +
              (UINT16)pStatus->currentScreen;
  }
  else{
    pScreen = pParamScreen;
  }
  
  switch(button){
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
    nextScreen = pScreen->noButtonInput;
    break;
  case UNUSED1_BUTTON:
  case UNUSED2_BUTTON:
  case UNUSED1_DBLCLICK:
  case UNUSED2_DBLCLICK:
  case DISPLAY_BUTTONS_COUNT:
  default:
    nextScreen = pScreen->noButtonInput;
    break;
  };

  return nextScreen;
}


/******************************************************************************
 * Function:    DispProcessingApp_NoButtonData
 *
 * Description: Utility function that sets button state data to FALSE. Only to 
 *              be used during APAC Processing when no button data is required.
 *
 * Parameters:  None
 *
 * Returns:     None 
 *              
 * Notes:       None
 *
 *****************************************************************************/
static
void DispProcessingApp_NoButtonData(void)
{
  DISPLAY_RX_INFORMATION_PTR pbutton;
  UINT16 i;

  pbutton = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
  
  for(i = 0; i < BUTTON_STATE_COUNT; i++){
    pbutton->buttonState[i]   = FALSE;
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

  if (pDebug->bDebug == TRUE){
    if(pDebug->bNewFrame == TRUE){
      DISPLAY_SCREEN_STATUS_PTR  pStatus;
      DISPLAY_CHAR_TABLE_PTR     pMenuID, pButtonID, pDiscreteID;
      DISPLAY_STATE_PTR          pScreen;
      DISPLAY_APP_DATA_PTR       pData;
      DISPLAY_RX_INFORMATION_PTR pRXInfo;
      UINT8                      subString1[MAX_SUBSTRING_SIZE], i;
      
      pStatus     = (DISPLAY_SCREEN_STATUS_PTR)&m_DispProcApp_Status;
      pMenuID     = (DISPLAY_CHAR_TABLE_PTR)&displayScreenTbl[0];
      pButtonID   = (DISPLAY_CHAR_TABLE_PTR)&displayButtonTbl[0];
      pDiscreteID = (DISPLAY_CHAR_TABLE_PTR)&displayDiscreteTbl[0];
      pScreen     = (DISPLAY_STATE_PTR)&menuStateTbl[0];
      pData       = (DISPLAY_APP_DATA_PTR)&m_DispProcApp_AppData;
      pRXInfo     = (DISPLAY_RX_INFORMATION_PTR)&m_DispProcRX_Info;
      m_Str[0]    = NULL;
      
      // Display the Current Screen ID
      snprintf((char *)m_Str, DISP_PRINT_PARAM("CurrentScreen   = "));
      snprintf((char *)subString1, DISP_PRINT_PARAMF("%s\r\n", 
               (pMenuID + (UINT16)pStatus->currentScreen)->name));
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display the Previous Screen ID
      snprintf((char *)m_Str, DISP_PRINT_PARAM("PreviousScreen  = "));
      snprintf((char *)subString1, DISP_PRINT_PARAMF("%s\r\n", 
               (pMenuID + (UINT16)pStatus->previousScreen)->name));
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n\n");
      m_Str[0] = NULL;
      
      // Display the completion status of PBIT
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("PBITComplete?   = %d\r\n", 
               pStatus->bPBITComplete));
      GSE_PutLine((const char *)m_Str);

      // Display the D_HLTH hexadecimal code
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("D_HLTHCode?     = 0x%02x\r\n",
               pStatus->displayHealth));
      GSE_PutLine((const char *)m_Str);
      
      // Display whether Transmission is currently enabled
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("ButtonEnable?   = %d\r\n", 
               pStatus->bButtonEnable));
      GSE_PutLine((const char *)m_Str);
      
      // Display whether or not APAC Processing is in progress
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("APACProcessing? = %d\r\n",
               pStatus->bAPACProcessing));
      GSE_PutLine((const char *)m_Str);
      
      // Display whether the button that is pressed is invalid or not
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("Invalid Key?    = %d\r\n",
               pStatus->bInvalidButton));
      GSE_PutLine((const char *)m_Str);
      m_Str[0] = NULL;
      
      // Display the Button Pressed
      snprintf((char *)m_Str, DISP_PRINT_PARAM("ButtonInput?    = "));
      snprintf((char *)subString1, sizeof(subString1), "%s", 
               (pButtonID + (UINT16)pDebug->buttonInput)->name);
      strcat((char *)m_Str, (const char *)subString1);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display the current Double Click Rate
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("DoubleClickRate = %d\r\n",
               pData->lastDCRate));
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      
      // Display the Part Number
      snprintf((char*)m_Str, DISP_PRINT_PARAMF("Part Number     = 0x%02x\r\n",
               pRXInfo->partNumber));
      GSE_PutLine((const char *)m_Str);
      
      // Display The Version Number Major.Minor
      snprintf((char*)m_Str, DISP_PRINT_PARAM("VersionNumber   = %d.%d\r\n"),
               pRXInfo->versionNumber[VERSION_MAJOR_NUMBER], 
               pRXInfo->versionNumber[VERSION_MINOR_NUMBER]);
      GSE_PutLine((const char *)m_Str);
      GSE_PutLine("\r\n");
      
      for (i = 0; i < (UINT16)DISCRETE_STATE_COUNT; i++){
        // Display the active discretes
        snprintf((char *)m_Str, sizeof(m_Str), 
                 (const char*)(pDiscreteID + i)->name);
        snprintf((char*)subString1, 
                 DISP_PRINT_PARAMF(" = %d", (pRXInfo)->discreteState[i]));
        strcat((char *)m_Str, (const char *)subString1);
        GSE_PutLine((const char *)m_Str);
        GSE_PutLine("\r\n");
        m_Str[0] = NULL;
      }
      
      // Display Menu Screen Line 1
      GSE_PutLine("\r\n");
      snprintf((char *)m_Str, (UINT16)(MAX_LINE_LENGTH + 1), 
               (const char *)&(pScreen + 
               (UINT16)pStatus->currentScreen)->menuString[0]);
      PWCDispProtocol_TranslateArrows((CHAR *)m_Str, (UINT16)MAX_LINE_LENGTH);
      GSE_PutLine((const char *) m_Str);
      GSE_PutLine("\r\n");
      m_Str[0] = NULL;
      
      // Display Menu Screen Line 2
      memcpy((void*)m_Str, (const void*)&(pScreen + 
             (UINT16)pStatus->currentScreen)->menuString[MAX_LINE_LENGTH], 
             MAX_LINE_LENGTH);
      PWCDispProtocol_TranslateArrows((CHAR *)m_Str, MAX_LINE_LENGTH);
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
* Parameters:  result         - The purpose of the log.
*              previousScreen - The previous screen before the button input
*              currentScreen  - The current screen after the button input
*              reset          - Reset transition to home screen flag.
*
* Returns:     none
*
* Notes:       none
*
*****************************************************************************/
static
void DispProcessingApp_CreateTransLog(DISPLAY_LOG_RESULT_ENUM result,
                                      DISPLAY_SCREEN_ENUM previousScreen,
                                      DISPLAY_SCREEN_ENUM currentScreen,
                                      BOOLEAN reset)
{
  DISPLAY_SCREEN_STATUS_PTR pStatus;
  DISPLAY_TRANSITION_LOG    log;

  pStatus            = (DISPLAY_SCREEN_STATUS_PTR) &m_DispProcApp_Status;
  log.result         = result;
  log.bReset         = reset; // Is this a reset? (auto abort or dblclk left)
  log.buttonPressed  = pStatus->buttonInput;
  log.previousScreen = (UINT16)previousScreen;
  log.currentScreen  = (UINT16)currentScreen;

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
  if(result == SYS_OK){
    NV_Read(NV_DISPPROC_CFG, 0, (void *) &m_DispProcApp_AppData,
            sizeof(DISPLAY_APP_DATA));
  }
  
  //If open failed re-init app data
  if (result != SYS_OK){
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
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void DispProcessingApp_FileInit(void)
{
  // Init App Data
  m_DispProcApp_AppData.lastDCRate = DISPLAY_DEFAULT_DCRATE;
  
  // Update App Data
  NV_Write(NV_DISPPROC_CFG, 0, (void *) &m_DispProcApp_AppData,
           sizeof(DISPLAY_APP_DATA));

}

/******************************************************************************
 * Function:    DispProcessingApp_ReturnFileHdr()
 *
 * Description: Function to return the Display file hdr structure
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
  DISPLAY_FILE_HDR_PTR      pFileHdr;
  
  pFileHdr = (DISPLAY_FILE_HDR_PTR)dest;
  pCfg = (DISPLAY_SCREEN_CONFIG_PTR)&m_DispProcApp_Cfg;
  
  pFileHdr->invalidButtonTime_ms = pCfg->invalidButtonTime_ms;
  pFileHdr->autoAbortTime_s      = pCfg->autoAbortTime_s;
  pFileHdr->no_HS_Timeout_s      = pCfg->no_HS_Timeout_s;
  pFileHdr->disc_Fail_ms         = pCfg->disc_Fail_ms;
  pFileHdr->size                 = sizeof(DISPLAY_FILE_HDR);
  
  return(pFileHdr->size);
}
/******************************************************************************
* Function:     DispProcApp_DisableLiveStream
*
* Description:  Disables the outputting the live stream for the Display 
*               Processing App
*
* Parameters:   None
*
* Returns:      None.
*
* Notes:
*  1) Used for debugging only
*
*
*****************************************************************************/
void DispProcApp_DisableLiveStream(void)
{
    m_DispProcApp_Debug.bDebug = FALSE;
}
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: DispProcessingApp.c $
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 2/10/16    Time: 5:36p
 * Updated in $/software/control processor/code/application
 * SCR 1303 Removed GSE Commands
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 2/10/16    Time: 9:09a
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
 * User: Jeremy Hester Date: 10/5/15    Time:4:10p
 * Created in $software/control processor/code/system
 * Initial Check In. Implementation of the Display Processing Application
 *                   Requirements.
 *****************************************************************************/
