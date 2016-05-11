#define APAC_MGR_BODY
/******************************************************************************
            Copyright (C) 2015-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        APACMgr.c

    Description: Contains all functions and data related to the APAC Function.

    VERSION
      $Revision: 30 $  $Date: 5/11/16 7:18p $

******************************************************************************/

//#define APAC_TIMING_TEST 1
//#define APAC_TEST_SIM 1
//#define APAC_TEST_DBG 1
//#define APACMGR_CYCLE_CBIT_INCR 1

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <math.h>
#include <stdio.h>
#include <float.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "APACMgr.h"
#include "Evaluator.h"
#include "ClockMgr.h"
#include "Sensor.h"
#include "GSE.h"
#include "Assert.h"

EXPORT void DispProcessingApp_Initialize(BOOLEAN bEnable);

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define APAC_CALC_BARO_CORR(x)      \
     (1 - pow((x / 1013.25),0.190284)) * 145366.45;

#define APAC_CALC_PALT_CORR(x,y) (x - y)

#define APAC_CALC_INTERPOLATE(val,x1,x2,y1,y2)    \
     (y1 + ( ((val - x1)/(x2 - x1)) * (y2 - y1) ))

#define APAC_CALC_MARGIN_COEFF( c0,c1,c2,c3,c4,tq )     \
     ( (c4 * pow(tq,4.0f)) + (c3 * pow(tq,3.0f)) + (c2 * pow(tq,2.0f)) + (c1 * tq) + c0)

#define APACMgr_DebugStr( DbgLevel, Timestamp, ...)  \
            { snprintf ( gse_OutLine, sizeof(gse_OutLine), __VA_ARGS__ ); \
              GSE_DebugStr( DbgLevel, Timestamp, gse_OutLine); }

#define APAC_ESN_CHECK_TICK   1000 // Check every 1 second, if ESN has changed
#define APAC_CYCLE_CHECK_TICK 1000 // Check every 1 second, if Cycle CBIT check
#define APAC_CYCLE_INC_CHECK_TICK 1000 // Check every 1 second, if Cycle Inc CBIT check

#define APAC_SNSR_NOT_FOUND_VAL -9999.0f  // Snsr Idx not found in Trend data

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef enum {
  APAC_DATA_AVG_OAT = 0,  //0
  APAC_DATA_AVG_BAROPRES, //1
  APAC_DATA_AVG_TQ,       //2
  APAC_DATA_VAL_BAROCORR, //3
  APAC_DATA_AVG_ITT,      //4
  APAC_DATA_AVG_NG,       //5
  APAC_DATA_AVG_MAX       //6
} APAC_DATA_AVG_ENUM;

typedef struct {
  SENSOR_INDEX idx;
  FLOAT64 *dest_ptr;
} APAC_DATA_AVG_ENTRY, *APAC_DATA_AVG_ENTRY_PTR;

typedef struct {
  APAC_DATA_AVG_ENTRY avg[APAC_DATA_AVG_MAX];
} APAC_DATA_AVG, *APAC_DATA_AVG_PTR;

typedef enum {
  ENG_INLET_STATE = 0, 
  ENG_INLET_VLD, 
  ENG_FLIGHT_STATE, 
  ENG_FLIGHT_VLD, 
  ENG_IDLE_STATE, 
  ENG_IDLE_VLD, 
  ENG_DISC_STATE_MAX
} APAC_ENG_DISC_STATE_ENUM; 

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static APAC_CFG m_APAC_Cfg;
static APAC_STATUS m_APAC_Status;
static APAC_TBL m_APAC_Tbl[APAC_PARAM_MARGIN_MAX][APAC_INSTALL_MAX];
static APAC_PALT_COEFF_TBL m_APAC_PALT_Coeff;
static APAC_ERRMSG_DISPLAY m_APAC_ErrMsg;
static APAC_SNSR_STATUS m_APAC_Snsr_Status[APAC_SNSR_MAX];
static APAC_HIST m_APAC_Hist;    // Approx 1500 bytes
static APAC_DATA_NVM m_APAC_NVM; // Approx 12 * 2 bytes
static APAC_HIST_STATUS m_APAC_Hist_Status;
static APAC_DATA_AVG m_APAC_DataAvg[APAC_ENG_MAX];

static UINT32 m_APAC_ESN_Check_tick;
static UINT32 m_APAC_Cycle_Check_tick;
#ifdef APACMGR_CYCLE_CBIT_INCR
  static UINT32 m_APAC_Cycle_Inc_Check_tick; 
#endif

#ifdef APAC_TEST_DBG
static APAC_DEBUG m_APAC_Debug;
static APAC_ENG_STATUS m_APAC_Eng_Debug;
static APAC_INLET_CFG_ENUM m_APAC_Eng_Debug_Inlet;
static APAC_NR_SEL_ENUM m_APAC_Eng_Debug_NR_Sel;
#endif

static APAC_VLD_LOG m_APAC_VLD_Log;

// Test Timing
// #define APAC_TIMING_TEST 1
#ifdef APAC_TIMING_TEST
  #define APAC_TIMING_BUFF_MAX 200
  static UINT32 m_APACTiming_Buff[APAC_TIMING_BUFF_MAX];
  static UINT32 m_APACTiming_Start;
  static UINT32 m_APACTiming_End;
  static UINT16 m_APACTiming_Index;
 /*
   m_APACTiming_Start = TTMR_GetHSTickCount();
   m_APACTiming_End = TTMR_GetHSTickCount();
   m_APACTiming_Buff[m_APACTiming_Index] = (m_APACTiming_End - m_APACTiming_Start);
   m_APACTiming_Index = (++m_APACTiming_Index) % APAC_TIMING_BUFF_MAX;
 */
#endif
// Test Timing


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static APAC_STATUS_PTR APACMgr_GetStatus (void);
static BOOLEAN APACMgr_CalcTqCorr (FLOAT64 baro_corr, FLOAT64 baro_pres, FLOAT64 tq,
                                   APAC_ENG_CALC_COMMON_PTR common_ptr );

static BOOLEAN APACMgr_CalcMargin (FLOAT64 oat, FLOAT64 tqDelta, FLOAT64 val,
                                   APAC_TBL_PTR tbl_ptr, FLOAT32 adj, FLOAT64 *margin_ptr,
                                   FLOAT64 *max_ptr, APAC_ENG_CALC_DATA_PTR c_ptr);

static void APACMgr_Task ( void *pParam ) ;
static void APACMgr_Running ( void );

static void APACMgr_errMsg_Computing ( BOOLEAN bPaltCalc, BOOLEAN bITTMax, BOOLEAN bNgMax,
                                       BOOLEAN bTqCorr, BOOLEAN bOAT, BOOLEAN bITTMg, 
                                       BOOLEAN bNGMg);
static void APACMgr_errMsg_HIGE_LoW ( BOOLEAN bTqCrit, BOOLEAN bGndCrit );
static void APACMgr_errMsg_Create ( const CHAR *title );

static void APACMgr_errMsg_Setup ( BOOLEAN eng1, BOOLEAN eng2, BOOLEAN eaps_ibf_1,
                                   BOOLEAN eaps_ibf_2 );
static void APACMgr_errMsg_Stabilizing ( void );

static void APACMgr_LogSummary ( APAC_COMMIT_ENUM commit );
static void APACMgr_LogEngStart ( void );
static void APACMgr_LogFailure ( void );
static void APACMgr_LogAbortNCR ( APAC_COMMIT_ENUM commit );

static void APACMgr_ClearStatus (void);
static void APACMgr_GetDataValues (void);

static BOOLEAN APACMgr_RestoreAppDataNVM ( void );
static BOOLEAN APACMgr_RestoreAppDataHist( void );
static void APACMgr_NVMAppendHistData( void );
static void APACMgr_NVMHistRemoveEngData (APAC_ENG_ENUM eng );
static void APACMgr_NVMUpdate ( APAC_ENG_ENUM eng_uut, APAC_VLD_REASON_ENUM reason,
                                APAC_VLD_LOG_PTR plog );
static void APACMgr_InitDataAvgStructs ( void );

static void APACMgr_CheckCfg ( void );
static void APACMgr_InitDispNames ( void );

#ifdef APACMGR_CYCLE_CBIT_INCR
  static void APACMgr_CheckCycle ( APAC_ENG_ENUM i );
  static void APACMgr_CheckCycleClear ( APAC_ENG_ENUM i );
#endif

static void APACMgr_RestoreAppDataHistOrder( void ); 

static void APACMgr_CheckHIGE_LOW( BOOLEAN *bResult, BOOLEAN *bResult1 ); 


// TEST
#ifdef APAC_TEST_SIM
static UINT32 test_menu_invld_timeout_val;
static UINT32 simNextTick;
static void APACMgr_Simulate ( void );
static BOOLEAN APACMgr_Test_TrendAppStartTrend( TREND_INDEX idx, BOOLEAN bStart,
                                                TREND_CMD_RESULT* rslt);
static void APACMgr_Test_TrendGetStabilityState ( TREND_INDEX idx,
            STABILITY_STATE* pStabState, UINT32* pDurMs,  SAMPLE_RESULT* pSampleResult);

typedef enum {
  APAC_SIM_IDLE,
  APAC_SIM_CONFIG,
  APAC_SIM_SELECT,
  APAC_SIM_SETUP,
  APAC_SIM_MANUAL_VALID,
  APAC_SIM_RUNNING,
  APAC_SIM_COMPLETED,
  APAC_SIM_RESULT,
  APAC_SIM_RESULT_COMMIT,
  APAC_SIM_RESET,
  APAC_SIM_RESTART,
  APAC_SIM_MAX
} APAC_SIM_STATES;

APAC_SIM_STATES simState;

static const UINT32 simStateDelay[APAC_SIM_MAX] =
{
//15000,  // APAC_SIM_IDLE
//15000,  // APAC_SIM_CONFIG
//15000,  // APAC_SIM_SELECT
5000,  // APAC_SIM_IDLE
5000,  // APAC_SIM_CONFIG
5000,  // APAC_SIM_SELECT
    500,  // APAC_SIM_SETUP
    500,  // APAC_SIM_MANUAL_VALID
  45000,  // APAC_SIM_RUNNING
  10000,  // APAC_SIM_COMPLETED
   5000,  // APAC_SIM_RESULT
   5000,  // APAC_SIM_RESULT_COMMIT
   5000,  // APAC_SIM_RESET
   1000   // APAC_SIM_RESTART
};
#endif // APAC_TEST_SIM
// TEST


/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/
static const APAC_TBL APAC_ITT_CONST_TBL[APAC_INSTALL_MAX] =
{
  { APAC_TBL_ITT_NORMAL_100PCNT_DEF },
  { APAC_TBL_ITT_EAPS_100PCNT_DEF   },
  { APAC_TBL_ITT_IBF_100PCNT_DEF    },
  { APAC_TBL_ITT_NORMAL_102PCNT_DEF },
  { APAC_TBL_ITT_EAPS_102PCNT_DEF   },
  { APAC_TBL_ITT_IBF_102PCNT_DEF    }
};

static const APAC_TBL APAC_NG_CONST_TBL[APAC_INSTALL_MAX] =
{
  { APAC_TBL_NG_NORMAL_100PCNT_DEF },
  { APAC_TBL_NG_EAPS_100PCNT_DEF   },
  { APAC_TBL_NG_IBF_100PCNT_DEF    },
  { APAC_TBL_NG_NORMAL_102PCNT_DEF },
  { APAC_TBL_NG_EAPS_102PCNT_DEF   },
  { APAC_TBL_NG_IBF_102PCNT_DEF    }
};

static const APAC_PALT_COEFF_TBL APAC_PALT_COEFF_CONST_TBL =
{
  APAC_TBL_COEFF_DEF
};

// The ordering below must match APAC_RUN_STATE_ENUM
static const APAC_STATE_RUN_STR APAC_STATE_RUN_STR_BUFF_CONST[APAC_RUN_STATE_MAX] =
{
  "IDLE        ",// APAC_RUN_STATE_IDLE
  "INCREASE TQ.",// APAC_RUN_STATE_INCREASE_TQ
  "DECR GND SPD",// APAC_RUN_STATE_DECR_GND_SPD
  "STABILIZING.",// APAC_RUN_STATE_STABILIZE
  "SAMPLING....",// APAC_RUN_STATE_SAMPLING
  "COMPUTING...",// APAC_RUN_STATE_COMPUTING
  "COMMIT      ",// APAC_RUN_STATE_COMMIT
  "FAULT       " // APAC_RUN_STATE_FAULT
};

static const APAC_INSTALL_ENUM m_APAC_Tbl_Mapping[APAC_NR_SEL_MAX][INLET_CFG_MAX] =
{
  { APAC_INSTALL_100_NORMAL, APAC_INSTALL_100_EAPS, APAC_INSTALL_100_IBF },
  { APAC_INSTALL_102_NORMAL, APAC_INSTALL_102_EAPS, APAC_INSTALL_102_IBF }
};

static const CHAR APAC_CAT_STR[APAC_NR_SEL_MAX] =
{
  APAC_CAT_100,
  APAC_CAT_102
};

static const CHAR APAC_ENG_POS_STR[APAC_ENG_MAX] =
{
  APAC_ENG_POS_1,
  APAC_ENG_POS_2
};

#define APAC_MONTH_MAX 13 // Month is 0..12, but "0' not used for '1'..'12' tot=13
static const CHAR APAC_HIST_MONTH_STR_CONST[APAC_MONTH_MAX][4]=
{
  "JAN", // Not Valid, Month is from 1..12
  "JAN",
  "FEB",
  "MAR",
  "APR",
  "MAY",
  "JUN",
  "JUL",
  "AUG",
  "SEP",
  "OCT",
  "NOV",
  "DEC"
};

static const CHAR APAC_HIST_ACT_STR_CONST[APAC_COMMIT_MAX][4]=
{
   APAC_STR_RJT,
   APAC_STR_ACP,
   APAC_STR_NVL,
   APAC_STR_VLD,
   APAC_STR_ABT,
   APAC_STR_NCR
};

static const CHAR APAC_HIST_CAT_STR_CONST[APAC_NR_SEL_MAX][2]=
{
  APAC_STR_CAT_BASIC,
  APAC_STR_CAT_A
};

static const CHAR APAC_HIST_ENG_STR_CONST[APAC_ENG_MAX][3]=
{
  APAC_STR_ENG_1,
  APAC_STR_ENG_2
};

typedef struct
{
  void   *ptr;       // Ptr to Cfg Param Field
  UINT8  size;       // Cfg Param Field size
  UINT32 check_val;  // Value to check against
  CHAR   msg[APAC_CFG_CHECK_MSG_SIZE];  // Err Msg to display
} APAC_CFG_CHECK, *APAC_CFG_CHECK_PTR;

static const APAC_CFG_CHECK APAC_CFG_CHECK_TBL[] =
{
  {&m_APAC_Cfg.snsr.idxBaroPres, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "SNSR.BAROPRES"},
  {&m_APAC_Cfg.snsr.idxBaroCorr, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "SNSR.BAROCORR"},
  {&m_APAC_Cfg.snsr.idxNR102_100, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "SNSR.NR102"},

  {&m_APAC_Cfg.eng[APAC_ENG_1].idxITT, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].ITT"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxNg, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].NG"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxNr, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].NR"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxTq, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].TQ"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxOAT, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].OAT"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxEngIdle, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].IDLE"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxEngFlt, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].FLT"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxEAPS_IBF_sw, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[0].EAPS_IBF"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxTrend[APAC_NR_SEL_100], sizeof(TREND_INDEX), TREND_UNUSED, "ENG[0].TREND_100"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxTrend[APAC_NR_SEL_102], sizeof(TREND_INDEX), TREND_UNUSED, "ENG[0].TREND_102"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].sc_TqCrit.size, sizeof(UINT8), 0, "ENG[0].TQCRIT"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].sc_GndCrit.size, sizeof(UINT8), 0, "ENG[0].GNDCRIT"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxCycleHRS, sizeof(CYCLE_INDEX), CYCLE_UNUSED, "ENG[0].ENG_CYC"},
  {&m_APAC_Cfg.eng[APAC_ENG_1].idxESN, sizeof(ENGRUN_INDEX), ENGRUN_UNUSED, "ENG[0].ESN_IDX"},

  {&m_APAC_Cfg.eng[APAC_ENG_2].idxITT, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].ITT"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxNg, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].NG"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxNr, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].NR"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxTq, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].TQ"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxOAT, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].OAT"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxEngIdle, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].IDLE"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxEngFlt, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].FLT"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxEAPS_IBF_sw, sizeof(SENSOR_INDEX), SENSOR_UNUSED, "ENG[1].EAPS_IBF"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxTrend[APAC_NR_SEL_100], sizeof(TREND_INDEX), TREND_UNUSED, "ENG[1].TREND_100"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxTrend[APAC_NR_SEL_102], sizeof(TREND_INDEX), TREND_UNUSED, "ENG[1].TREND_102"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].sc_TqCrit.size, sizeof(UINT8), 0, "ENG[1].TQCRIT"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].sc_GndCrit.size, sizeof(UINT8), 0, "ENG[1].GNDCRIT"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxCycleHRS, sizeof(CYCLE_INDEX), CYCLE_UNUSED, "ENG[1].ENG_CYC"},
  {&m_APAC_Cfg.eng[APAC_ENG_2].idxESN, sizeof(ENGRUN_INDEX), ENGRUN_UNUSED, "ENG[1].ESN_IDX"},

  { NULL, NULL, NULL, NULL }
};

// NOTE: APAC_VLD_REASON_ENUM changes, the ordering below must change as well to match
static const CHAR *APAC_VLD_STR_CONST[] =
{
  APAC_VLD_STR_NONE,   // APAC_VLD_REASON_NONE
  APAC_VLD_STR_50HRS,  // APAC_VLD_REASON_50HRS
  APAC_VLD_STR_ESN,    // APAC_VLD_REASON_ESN
  APAC_VLD_STR_CFG,    // APAC_VLD_REASON_CFG
  APAC_VLD_STR_CYCLE,  // APAC_VLD_REASON_CYCLE
  APAC_VLD_STR_NVM     // APAC_VLD_REASON_NVM
};

// NOTE: APAC_VLD_REASON_ENUM changes, the ordering below must change as well to match
static const UINT16 APAC_VLD_BIT_ENCODE_CONST[] = 
{
  0x0000,              // APAC_VLD_REASON_NONE
  0x0001,              // APAC_VLD_REASON_50HRS
  0x0002,              // APAC_VLD_REASON_ESN
  0x0004,              // APAC_VLD_REASON_CFG
  0x0008,              // APAC_VLD_REASON_CYCLE
  0x0010               // APAC_VLD_REASON_NVM
}; 

static const UINT16 APAC_ENG_DISC_STATE_POS[APAC_ENG_MAX][ENG_DISC_STATE_MAX] = 
{
  { 0x0001,              // E1 INLET DISC STATE
    0x0002,              // E1 INLET DISC VALIDITY
    0x0004,              // E1 FLIGHT DISC STATE
    0x0008,              // E1 FLIGHT DISC VALIDITY
    0x0010,              // E1 IDLE DISC STATE
    0x0020 },            // E1 IDLE DISC VALIDITY
  
  { 0x0100,              // E2 INLET DISC STATE
    0x0200,              // E2 INLET DISC VALIDITY
    0x0400,              // E2 FLIGHT DISC STATE
    0x0800,              // E2 FLIGHT DISC VALIDITY
    0x1000,              // E2 IDLE DISC STATE
    0x2000 }             // E2 IDLE DISC VALIDITY
}; 

#include "APACMgr_Interface.c"
#include "APACMgrUserTables.c"

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    APACMgr_Initialize
 *
 * Description: Initializes the APAC Mgr functionality
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void APACMgr_Initialize ( void )
{
  UINT32 i;
  UINT32 *ltemp, tick;
  TCB tcbTaskInfo;

  tick = CM_GetTickCount();

  // Save menu_delay_timeout_val as this might have been set by Menu App before hand
  ltemp = m_APAC_Status.menu_invld_timeout_val_ptr_ms;

  memset ( (void *) &m_APAC_Status, 0, sizeof(m_APAC_Status) );
  memset ( (void *) &m_APAC_Cfg, 0, sizeof(m_APAC_Cfg) );
  memset ( (void *) &m_APAC_ErrMsg, 0, sizeof(m_APAC_ErrMsg) );
  memset ( (void *) &m_APAC_Hist, 0, sizeof(m_APAC_Hist) );
  memset ( (void *) &m_APAC_Hist_Status, 0, sizeof(m_APAC_Hist_Status) );

#ifdef APAC_TEST_DBG
  /*vcast_dont_instrument_start*/  
  memset ( (void *) &m_APAC_Eng_Debug, 0, sizeof(m_APAC_Eng_Debug) );
  m_APAC_Eng_Debug_Inlet = INLET_CFG_BASIC;
  m_APAC_Eng_Debug_NR_Sel = APAC_NR_SEL_100;
  memset ( (void *) &m_APAC_Debug, 0, sizeof(m_APAC_Debug));
  /*vcast_dont_instrument_end*/
#endif  
  memset ( (void *) &m_APAC_VLD_Log, 0, sizeof(m_APAC_VLD_Log) );

  // Restore saved menu_delay_timeout_val as this might have been set by Menu App before hand
  m_APAC_Status.menu_invld_timeout_val_ptr_ms = ltemp;
  strncpy_safe (m_APAC_Status.cfg_err, APAC_CFG_CHECK_MSG_SIZE, "NONE", _TRUNCATE);

// Test
#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/ 
  m_APAC_Status.menu_invld_timeout_val_ptr_ms = &test_menu_invld_timeout_val;  // TEST
  test_menu_invld_timeout_val = 1500; // TEST
  /*vcast_dont_instrument_end*/
#endif
// Test

  // Add an entry in the user message handler table
  User_AddRootCmd(&apacMgrRootTblPtr);

  // Copy data from CfgMgr, to be used as the run time copy
  memcpy( (void *) &m_APAC_Cfg, (const void *) &CfgMgr_RuntimeConfigPtr()->APACConfig,
          sizeof(m_APAC_Cfg));

#ifdef APAC_TIMING_TEST
  /*vcast_dont_instrument_start*/
  for (i=0;i<APAC_TIMING_BUFF_MAX;i++)
  {
    m_APACTiming_Buff[i] = 0;
  }
  m_APACTiming_Index = 0;
  m_APACTiming_Start = 0;
  m_APACTiming_End = 0;
  /*vcast_dont_instrument_end*/
#endif

  // Create Processing Task if enabled
  if (m_APAC_Cfg.enabled == TRUE)
  {
    memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
    strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name), "APAC Manager",_TRUNCATE);
    tcbTaskInfo.TaskID      = APAC_Manager;
    tcbTaskInfo.Function    = APACMgr_Task;
    tcbTaskInfo.Priority    = taskInfo[APAC_Manager].priority;
    tcbTaskInfo.Type        = taskInfo[APAC_Manager].taskType;
    tcbTaskInfo.modes       = taskInfo[APAC_Manager].modes;
    tcbTaskInfo.MIFrames    = taskInfo[APAC_Manager].MIFframes;
    tcbTaskInfo.Rmt.InitialMif = taskInfo[APAC_Manager].InitialMif;
    tcbTaskInfo.Rmt.MifRate = taskInfo[APAC_Manager].MIFrate;
    tcbTaskInfo.Enabled     = TRUE;
    tcbTaskInfo.Locked      = FALSE;
    tcbTaskInfo.pParamBlock = APACMgr_Task;
    TmTaskCreate (&tcbTaskInfo);

    APACMgr_CheckCfg();
  }

  // Initialize APAC internal tables from ROM or NVRAM
  for (i=0;i<(UINT16) APAC_INSTALL_MAX;i++)
  {
    m_APAC_Tbl[APAC_ITT][i] = APAC_ITT_CONST_TBL[i];
    m_APAC_Tbl[APAC_NG][i] = APAC_NG_CONST_TBL[i];
  }
  m_APAC_PALT_Coeff = APAC_PALT_COEFF_CONST_TBL;

  APACMgr_ClearStatus();
  strncpy_safe ( m_APAC_Status.eng[APAC_ENG_1].esn, APAC_ESN_MAX_LEN, APAC_ESN_DEFAULT,
                 _TRUNCATE );
  strncpy_safe ( m_APAC_Status.eng[APAC_ENG_2].esn, APAC_ESN_MAX_LEN, APAC_ESN_DEFAULT,
                 _TRUNCATE );

  // Setup APAC_SNSR Display Names from CFG
  APACMgr_InitDispNames();

  APACMgr_RestoreAppDataHist();
  APACMgr_RestoreAppDataNVM();

  m_APAC_ESN_Check_tick =  tick + APAC_ESN_CHECK_TICK;
  m_APAC_Cycle_Check_tick = (tick + 50) + APAC_CYCLE_CHECK_TICK;  // Offset by 50 ms
#ifdef APACMGR_CYCLE_CBIT_INCR  
  /*vcast_dont_instrument_start*/
  m_APAC_Cycle_Inc_Check_tick = (tick + 100) + APAC_CYCLE_INC_CHECK_TICK;  // Offset by 100 ms
  /*vcast_dont_instrument_end*/
#endif  

  APACMgr_InitDataAvgStructs();
  
  // Call Display Processing App and Info that APAC is enb or dsb
  DispProcessingApp_Initialize(m_APAC_Cfg.enabled);
  
  // Update engineRunIndex of each engine
  for (i=(UINT32) APAC_ENG_1;i< (UINT32) APAC_ENG_MAX;i++) {
    if (m_APAC_Cfg.eng[i].idxCycleHRS != CYCLE_UNUSED) {
      m_APAC_Status.eng[i].engineRunIndex = 
        CfgMgr_RuntimeConfigPtr()->CycleConfigs[m_APAC_Cfg.eng[i].idxCycleHRS].nEngineRunId; 
    }
  }

// Test
#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/  
  //simState = APAC_SIM_IDLE;
  simState = APAC_SIM_MAX;
  simNextTick = CM_GetTickCount() + simStateDelay[simState];
  /*vcast_dont_instrument_end*/  
#endif
// Test
  
}


/******************************************************************************
 * Function:     APACMgr_FileInitHist
 *
 * Description:  Reset the file to the initial state.
 *
 * Parameters:   none
 *
 * Returns:      BOOLEAN TRUE is always returned
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN APACMgr_FileInitHist(void)
{
  // Init App data
  memset ( (void *) &m_APAC_Hist, 0, sizeof(m_APAC_Hist) );

  // Update App data
  NV_Write(NV_APAC_HIST, 0, (void *) &m_APAC_Hist, sizeof(m_APAC_Hist));

  return TRUE;
}


/******************************************************************************
 * Function:     APACMgr_FileInitNVM
 *
 * Description:  Reset the file to the initial state if APAC NVM Data is bad
 *
 * Parameters:   none
 *
 * Returns:      BOOLEAN TRUE is always returned
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN APACMgr_FileInitNVM(void)
{
  APAC_ENG_ENUM i;
  APAC_DATA_ENG_NVM_PTR eng_nvm_ptr;

  // Init App data
  memset ( (void *) &m_APAC_NVM, 0, sizeof(m_APAC_NVM) );
  
  m_APAC_NVM.cfgCRC = NV_GetFileCRC(NV_CFG_MGR);
  
  for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++) {
     eng_nvm_ptr = (APAC_DATA_ENG_NVM_PTR) &m_APAC_NVM.eng[i];
    // Update ESN to "unitialized" state
    strncpy_safe (eng_nvm_ptr->esn, APAC_ESN_MAX_LEN, APAC_ESN_DEFAULT, _TRUNCATE );
    if ( m_APAC_Cfg.enabled == TRUE ) { // Only when APAC Processing is enb
      snprintf (m_APAC_VLD_Log.msg,sizeof(m_APAC_VLD_Log.msg),"E%d:",i);
      APACMgr_NVMUpdate ( i, APAC_VLD_REASON_NVM, &m_APAC_VLD_Log );
    }
    else  { // Either NVM corrupted when APAC not enb or factory reset, just reset data
      NV_Write(NV_APAC_NVM, 0, (void *) &m_APAC_NVM, sizeof(m_APAC_NVM));
    }
  }
  
  return TRUE;
}


/******************************************************************************
 * Function:     APACMgr_FSMGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                     | FSM
 *
 * Description:  Returns the APAC inprogress status.  
 *               This function has the call signature required by the
 *               Fast State Manager's Task interface.
 *
 * Parameters:   [in] Param: Not used, for matching call sig. only.
 *
 * Returns:
 *
 *****************************************************************************/
BOOLEAN APACMgr_FSMGetState( INT32 param )
{
  return ( !(m_APAC_Status.run_state == APAC_RUN_STATE_IDLE) || 
            (m_APAC_Status.run_state == APAC_RUN_STATE_FAULT) );
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    APACMgr_Task
 *
 * Description: Main processing loop for APAC test functionality.
 *
 * Parameters:  *pParam - ptr to m_APAC_Status
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_Task (void *pParam)
{
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_ENG_ENUM i;
  UINT32 tick;
  CHAR esn[APAC_ESN_MAX_LEN];

  tick = CM_GetTickCount();

  // Update current Eng Run Cycle Value for each Eng
  if (tick >= m_APAC_Cycle_Check_tick)
  {
    m_APAC_Cycle_Check_tick = tick + APAC_CYCLE_CHECK_TICK;  // Set next timeout
    for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++)
    {
      eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[i];
      eng_ptr->hrs.curr_s = (m_APAC_Cfg.eng[i].idxCycleHRS != CYCLE_UNUSED) ?
                             CycleGetCount(m_APAC_Cfg.eng[i].idxCycleHRS) :
                             eng_ptr->hrs.curr_s;
      // If Eng Hrs Curr is < Hrs Prev then Cycle Count must have changed / reset,
      //   force manual APAC
      if ((eng_ptr->hrs.curr_s < eng_ptr->hrs.prev_s) &&
          (m_APAC_Cfg.eng[i].idxCycleHRS != CYCLE_UNUSED))
      {
        snprintf ( m_APAC_VLD_Log.msg, sizeof(m_APAC_VLD_Log.msg), "E%d: curr=%d,prev=%d",
                   i, eng_ptr->hrs.curr_s, eng_ptr->hrs.prev_s );
        eng_ptr->hrs.prev_s = eng_ptr->hrs.curr_s; // Reset prev counter to current
        m_APAC_NVM.eng[i].hrs_s = eng_ptr->hrs.prev_s;  // Update NVM cpy. Might not be necess
        // Set manual validate flag.  Indicate Eng Cycle count changed as Validate Reason
        APACMgr_NVMUpdate ( i, APAC_VLD_REASON_CYCLE, &m_APAC_VLD_Log );
      }
    } // end for loop APAC_ENG_MAX
  } // end of if (tick >= m_APAC_Cycle_Check_tick)

  // Check if ESN has been updated or changed
  if (tick >= m_APAC_ESN_Check_tick)
  {
    m_APAC_ESN_Check_tick = tick + APAC_ESN_CHECK_TICK;  // Set next timeout
    for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++)
    {
      if ( m_APAC_Cfg.eng[i].idxESN != ENGRUN_UNUSED )
      {
        // NOTE: Currently only UART port has ESN decode logic
        if (EngRunGetSN ((UINT8) m_APAC_Cfg.eng[i].idxESN, &esn[0], APAC_ESN_MAX_LEN))
        { // Have ESN to process
          if ( (strncmp ( m_APAC_Status.eng[i].esn, esn, APAC_ESN_MAX_LEN) != 0) &&
               ((esn[0] == APAC_ENG_SN_UCHAR1) || (esn[0] == APAC_ENG_SN_LCHAR1)) &&
               ((esn[1] == APAC_ENG_SN_UCHAR2) || (esn[1] == APAC_ENG_SN_LCHAR2)) )
          { // "Valid" ESN has changed.  "Valid" == "KB9999"
            memcpy ( m_APAC_Status.eng[i].esn, esn, APAC_ESN_MAX_LEN); // quicker with memcpy
            // Compare to prev in NVM and if diff, flag as needing manual validate
            if (strncmp ( m_APAC_NVM.eng[i].esn, esn, APAC_ESN_MAX_LEN) != 0)
            { // ESN has changed compare to NVM
              GSE_DebugStr (NORMAL,TRUE,
                            "APACMgr: APACMgr_Task() ESN Changed (new=%s,prev=%s)\r\n",
                            esn, m_APAC_NVM.eng[i].esn);
              snprintf (m_APAC_VLD_Log.msg, sizeof(m_APAC_VLD_Log.msg), "E%d: curr=%s,prev=%s",
                         i, esn, m_APAC_NVM.eng[i].esn );
              memcpy ( m_APAC_NVM.eng[i].esn, esn, APAC_ESN_MAX_LEN );
              APACMgr_NVMHistRemoveEngData ( i );
              APACMgr_NVMUpdate ( i, APAC_VLD_REASON_ESN, &m_APAC_VLD_Log );
            } // end if esn has changed from NVM
          } // end if esn has changed
        } // end if ESN exists
      } // end if cfg for ESN decode from Port
    } // end loop thru all ENG to check ESN change
  } // end if timeout to check ESN change
  
  // Check if Cycle Inc CBIT 
#ifdef APACMGR_CYCLE_CBIT_INCR
  /*vcast_dont_instrument_start*/
  if (tick >= m_APAC_Cycle_Inc_Check_tick) 
  {
    m_APAC_Cycle_Inc_Check_tick = tick + APAC_CYCLE_INC_CHECK_TICK;  // Set next timeout
    for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++) {
      APACMgr_CheckCycle ( i ); 
    }
  }
  /*vcast_dont_instrument_end*/
#endif  

  if ( m_APAC_Status.run_state != APAC_RUN_STATE_IDLE ){
    APACMgr_Running();
  }

// Test
#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/  
  APACMgr_Simulate();
  /*vcast_dont_instrument_end*/  
#endif
// Test

}


/******************************************************************************
 * Function:    APACMgr_Running
 *
 * Description: APAC Running State in progress
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_Running (void)
{
  BOOLEAN bResult, bResult1, bResult2, bResult3, bResult4, bResult5, bResult6;
  BOOLEAN bResult7;
  UINT32 tick, temp;
  APAC_TBL_PTR tbl_ptr;
  APAC_ENG_ENUM eng_uut;
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_INSTALL_ENUM tblIdx;
  APAC_ENG_CALC_COMMON_PTR common_ptr;
  APAC_ENG_CALC_DATA_PTR itt_ptr, ng_ptr;
  TREND_CMD_RESULT rslt;
  STABILITY_STATE state_stability;
  SAMPLE_RESULT state_sample;


  ASSERT ( !((m_APAC_Status.eng_uut == APAC_ENG_MAX) &&
             (m_APAC_Status.run_state != APAC_RUN_STATE_FAULT)) );
  ASSERT ( m_APAC_Status.state == APAC_STATE_TEST_INPROGRESS );
  ASSERT ( m_APAC_Status.nr_sel != APAC_NR_SEL_MAX );

  tick = CM_GetTickCount();

  eng_uut = m_APAC_Status.eng_uut;
  bResult1 = bResult2 = bResult3 = bResult4 = bResult5 = bResult6 = bResult7 = TRUE;

  switch ( m_APAC_Status.run_state )
  {
    case APAC_RUN_STATE_INCREASE_TQ:
    case APAC_RUN_STATE_DECR_GND_SPD:
      
      // Execute HIGE_LOW criteria to determine if cond met ?
      APACMgr_CheckHIGE_LOW ( &bResult, &bResult1 ); 
      
      // .run_state used for status display on menu for "INCREASE TQ." or "DECR GND SPD"
      m_APAC_Status.run_state = (bResult == FALSE) ? APAC_RUN_STATE_INCREASE_TQ : 
                                                     APAC_RUN_STATE_DECR_GND_SPD ; 
      if ( (bResult == TRUE) && (bResult1 == TRUE) )
      {
        // Kick off trend object
#ifdef APAC_TEST_SIM
        /*vcast_dont_instrument_start*/  
        bResult = APACMgr_Test_TrendAppStartTrend(
                    m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel], TRUE, &rslt);
        /*vcast_dont_instrument_end*/        
#else
        bResult = TrendAppStartTrend(m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel],
                                     TRUE, &rslt);
#endif
        // If TREND_NOT_COMMANDABLE rslt, output GSE Debug
        if ( rslt == TREND_NOT_COMMANDABLE ) {
          GSE_DebugStr(NORMAL,TRUE,
                       "APACMgr: APACMgr_Running() ENG[%d] Trend Not Commandable\r\n",eng_uut);
        }
        if ( rslt == TREND_NOT_CONFIGURED ) {
          GSE_DebugStr(NORMAL,TRUE,
                       "APACMgr: APACMgr_Running() ENG[%d] Trend Not Cfg\r\n",eng_uut);
        }
        // Always move to stablize state.  If Trend Not Cfg, will timeout in stablize state
        m_APAC_Status.timeout_ms = tick + CM_DELAY_IN_SECS(m_APAC_Cfg.timeout.stable_sec);
        m_APAC_Status.run_state = APAC_RUN_STATE_STABILIZE;
        GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Stablizing \r\n");
      } // end of if TqCrit and GndCrit are TRUE, start Trend Obj
      else if (tick >= m_APAC_Status.timeout_ms)
      {
        // Note: APACMgr_errMsg_HIGE_LoW ( bHIGE, bLoW )
        APACMgr_errMsg_HIGE_LoW( bResult, bResult1 );
        APACMgr_LogFailure();
        m_APAC_Status.run_state = APAC_RUN_STATE_FAULT;
        GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() HIGE_LoW Timeout\r\n");
      }
      break;

    case APAC_RUN_STATE_STABILIZE:
      // If Trend stabilizeS
      // if (APACMgr_Test_TrendStatus() == TRUE)
#ifdef APAC_TEST_SIM
      /*vcast_dont_instrument_start*/  
      APACMgr_Test_TrendGetStabilityState(
          m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel], &state_stability, &temp,
          &state_sample);
      /*vcast_dont_instrument_end*/      
#else
      TrendGetStabilityState(m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel],
                             &state_stability, &temp, &state_sample);
#endif
      if ( state_stability == STB_STATE_SAMPLE )
      {
        m_APAC_Status.run_state = APAC_RUN_STATE_SAMPLING;
        GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Sampling \r\n");
      }
      else if (tick >= m_APAC_Status.timeout_ms)
      {
        // Command Trend to Off. Note: Will call TrendAppStartTrend(FALSE) until Trend
        //   trans to STB_STATE_IDLE.  This should not be a problem
        bResult = TrendAppStartTrend(m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel],
                                     FALSE, &rslt);
        // Note: Trend will take another MIF frame before TrendGetStabilityHistory() is
        //       available. Wait for state_stability -> STB_STATE_IDLE.
        if (state_stability == STB_STATE_IDLE)
        {
          APACMgr_errMsg_Stabilizing();
          APACMgr_LogFailure();
          m_APAC_Status.run_state = APAC_RUN_STATE_FAULT;
          GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Stabilize Timeout\r\n");
        }
      }
      break;

    case APAC_RUN_STATE_SAMPLING:
#ifdef APAC_TEST_SIM
      /*vcast_dont_instrument_start*/  
      APACMgr_Test_TrendGetStabilityState(
           m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel], &state_stability, &temp,
           &state_sample);
      /*vcast_dont_instrument_end*/      
#else
      TrendGetStabilityState(m_APAC_Cfg.eng[eng_uut].idxTrend[m_APAC_Status.nr_sel],
                             &state_stability, &temp, &state_sample);
#endif
      // if (APACMgr_Test_TrendStatus() == TRUE)
      if ( state_sample == SMPL_RSLT_SUCCESS )
      {
        APACMgr_GetDataValues();
        m_APAC_Status.run_state = APAC_RUN_STATE_COMPUTING;
        m_APAC_Status.timeout_ms = tick + APAC_RUN_COMPUTING_DELAY_MS;
        GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Computing \r\n");
      }
      //else if (APACMgr_Test_TrendSamplingFault() == TRUE)
      else if ( state_sample == SMPL_RSLT_FAULTED )
      {
        APACMgr_errMsg_Stabilizing();
        APACMgr_LogFailure();
        m_APAC_Status.run_state = APAC_RUN_STATE_FAULT;
        GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Sampling Faulted\r\n");
      }
      break;

    case APAC_RUN_STATE_COMPUTING:
      // Add 2 sec delay here for jv testing, can see on display "COMPUTING". Internal timeout.
      if ( tick >=  m_APAC_Status.timeout_ms )
      {
        // This s/b be very quick computation.
        eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[eng_uut];
        itt_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->itt;
        itt_ptr->cfgOffset = m_APAC_Cfg.adjust.itt;
        ng_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->ng;
        ng_ptr->cfgOffset = m_APAC_Cfg.adjust.ng; 
        common_ptr = (APAC_ENG_CALC_COMMON_PTR) &eng_ptr->common;
        bResult1 = bResult2 = bResult3 = bResult4 = bResult5 = TRUE;  // Init all to TRUE
        bResult1 = APACMgr_CalcTqCorr ( common_ptr->valBaroCorr, common_ptr->avgBaroPres,
                                        common_ptr->avgTQ, common_ptr);
        // If _CalcTqDelta() fails then don't check TqDelta Range, as PAMB calc had failed
        //    in _CalcTqDelta() call.
        bResult4 = (bResult1 == FALSE) ? TRUE :
                 ((common_ptr->tqCorr >= APAC_TQCORR_MIN) &&
                  (common_ptr->tqCorr <= APAC_TQCORR_MAX));
        bResult5 = (common_ptr->avgOAT >= APAC_OAT_MIN) &&
                   (common_ptr->avgOAT <= APAC_OAT_MAX);
        if (bResult1 && bResult4 && bResult5)
        {
          tblIdx = m_APAC_Tbl_Mapping[m_APAC_Status.nr_sel][m_APAC_Cfg.inletCfg];
          tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_ITT][tblIdx];
          bResult2 = APACMgr_CalcMargin( common_ptr->avgOAT, common_ptr->tqCorr,
                     itt_ptr->avgVal, tbl_ptr, m_APAC_Cfg.adjust.itt, &itt_ptr->margin,
                     &itt_ptr->max, itt_ptr );
          // Check ITT Margin Range, if Margin had been calc successfully
          bResult6 = (bResult2 == FALSE) ? TRUE :
                     ((itt_ptr->margin >= APAC_ITT_MARGIN_MIN) &&
                      (itt_ptr->margin <= APAC_ITT_MARGIN_MAX));
          tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_NG][tblIdx];
          bResult3 = APACMgr_CalcMargin( common_ptr->avgOAT, common_ptr->tqCorr,
                     ng_ptr->avgVal, tbl_ptr, m_APAC_Cfg.adjust.ng, &ng_ptr->margin,
                     &ng_ptr->max, ng_ptr );
          // Check Ng Margin Range, if Margin had been calc successfully
          bResult7 = (bResult3 == FALSE) ? TRUE :
                     ((ng_ptr->margin >= APAC_NG_MARGIN_MIN) &&
                      (ng_ptr->margin <= APAC_NG_MARGIN_MAX));
        } // end if APACMgr_CalcTqDelta() was successful, calc ITT Margin and calc Ng Margin
        // bResult1 = PAMB, bResult2 = ITT, bResult3 = Ng, bResult4 = TqCorr, bResult5 = 0AT
        // (Result6 && Result7) = ITT or Ng Margin Range
        if ( bResult1 && bResult2 && bResult3 && bResult4 && bResult5 &&
             (bResult6 && bResult7) )
        { // No failures, trans to wait for commit from crew
          m_APAC_Status.run_state = APAC_RUN_STATE_COMMIT;
          GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_Running() Comp Success \r\n");
        } // end if calculation was SUCCESS
        else
        { // Failure detected.
          // Note: APACMgr_errMsg_Computing ( bPAMB, bITTMax, bNgMax, bTqDelta, bOAT, bITTMg, 
          //                                  bNGMg )
          APACMgr_errMsg_Computing( bResult1, bResult2, bResult3, bResult4, bResult5,
                                    bResult6, bResult7 );
          APACMgr_LogFailure();
          m_APAC_Status.run_state = APAC_RUN_STATE_FAULT;
          GSE_DebugStr(NORMAL,TRUE,
                 "APACMgr: APACMgr_Running() Comp Fault ('%s','%s','%s','%s','%s','%s')\r\n",
                 m_APAC_ErrMsg.list[0].entry, m_APAC_ErrMsg.list[1].entry,
                 m_APAC_ErrMsg.list[2].entry, m_APAC_ErrMsg.list[3].entry,
                 m_APAC_ErrMsg.list[4].entry, m_APAC_ErrMsg.list[5].entry);
        } // end something failed
      } // end if timeout did not expire
      break;

    case APAC_RUN_STATE_COMMIT:
      // Wait for APACMgr_IF_ResultCommit() to be called,  to commit results.
    case APAC_RUN_STATE_IDLE:
    case APAC_RUN_STATE_FAULT:
    case APAC_RUN_STATE_MAX:
    default:
      //
      break;
  } // end of switch

}


/******************************************************************************
 * Function:    APACMgr_errMsg_Computing
 *
 * Description: Utility function to set the m_APAC_ErrMsg for Menu Err Display
 *              for PAC Computing Fault detected
 *
 * Parameters:  bPaltCalc - FALSE if PALT Calc failed
 *              bITTMax - FALSE if ITT Max Calc failed
 *              bNgMax - FALSE if Ng Max Calc failed
 *              bTqCorr - FALSE if Tq Corrected Calc failed
 *              bOAT - FALSE if OAT is out of range
 *              bITTMg - FALSE if ITT Margin Calc is out of range
 *              bNGMg - FALSE if Ng Margin Calc is out of range
 *
 * Returns:     None
 *
 * Notes:
 *   - TRUE passed in indicates Calc success or not performed
 *
 *****************************************************************************/
static void APACMgr_errMsg_Computing ( BOOLEAN bPaltCalc, BOOLEAN bITTMax, BOOLEAN bNgMax,
                                       BOOLEAN bTqCorr, BOOLEAN bOAT, BOOLEAN bITTMg, 
                                       BOOLEAN bNGMg)
{
  m_APAC_Snsr_Status[APAC_SNSR_TQCORR].bNotOk = (bTqCorr == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_ITTMAX].bNotOk = (bITTMax == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_NGMAX].bNotOk = (bNgMax == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_PALT_CALC].bNotOk = (bPaltCalc == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_OAT].bNotOk = (bOAT == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_ITTMG].bNotOk = (bITTMg == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[APAC_SNSR_NGMG].bNotOk = (bNGMg == FALSE) ? TRUE : FALSE;

  APACMgr_errMsg_Create( APAC_ERRMSG_TITLE_COMPUTE );
}


/******************************************************************************
 * Function:    APACMgr_errMsg_HIGE_LoW
 *
 * Description: Utility function to set the m_APAC_ErrMsg for Menu Err Display
 *              for HIGE LoW Fault detected
 *
 * Parameters:  bTqCrit  - FALSE - Tq Crit faulted / not met
 *              bGndCrit - FALSE - GndSpd Crit faulted / not met
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_errMsg_HIGE_LoW ( BOOLEAN bTqCrit, BOOLEAN bGndCrit )
{
  m_APAC_Snsr_Status[ APAC_SNSR_TQCRIT].bNotOk = (bTqCrit == FALSE) ? TRUE : FALSE;
  m_APAC_Snsr_Status[ APAC_SNSR_GNDCRIT].bNotOk = (bGndCrit == FALSE) ? TRUE : FALSE;

  APACMgr_errMsg_Create(APAC_ERRMSG_TITLE_HIGE_LOW);
}


/******************************************************************************
 * Function:    APACMgr_errMsg_Create
 *
 * Description: Utility function to set the m_APAC_ErrMsg for Menu Err Display
  *
 * Parameters:  *title - ptr to title string to print
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_errMsg_Create ( const CHAR *title )
{
  APAC_ERRMSG_ENTRY_PTR entry_ptr;
  APAC_SNSR_STATUS_PTR snsr_status_ptr;
  UINT16 i;

  memset ( (void *) &m_APAC_ErrMsg, 0, sizeof(m_APAC_ErrMsg) );
  strncpy ( (char *) m_APAC_ErrMsg.title.entry, title, APAC_MENU_DISPLAY_CHAR_MAX );

  entry_ptr = (APAC_ERRMSG_ENTRY_PTR) m_APAC_ErrMsg.list;
  snsr_status_ptr = (APAC_SNSR_STATUS_PTR) &m_APAC_Snsr_Status[0];
  for (i=0;i< (UINT16) APAC_SNSR_MAX;i++)
  {
    if ( snsr_status_ptr->bNotOk == TRUE )
    {
      memcpy ((void *) entry_ptr->entry,(void *) snsr_status_ptr->str, APAC_ERRMSG_CHAR_MAX);
      m_APAC_ErrMsg.num++;
      entry_ptr++;
    } // end if ( snsr_status_ptr->bNotOk == TRUE )
    snsr_status_ptr++;
  } // end for looop

  // ASSERT if *menu_invld_timeout_val_ptr_ms == NULL
  m_APAC_Status.timeout_ms = *m_APAC_Status.menu_invld_timeout_val_ptr_ms + CM_GetTickCount();
}


/******************************************************************************
 * Function:    APACMgr_errMsg_Setup
 *
 * Description: Utility function to set the m_APAC_ErrMsg for Menu Err Display
 *              for Setup Fault detected
 *
 * Parameters:  eng1 - Eng 1 (TRUE) Flt and not IDLE  OR  (FALSE) not Flt and IDLE
 *              eng2 - Eng 2 (TRUE) Flt and not IDLE  OR  (FALSE) not Flt and IDLE
 *              eaps_ibf_1 - Eng 1 EAPS/IBF sw is set
 *              eaps_ibf_2 - Eng 2 EAPS/IBF sw is set
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_errMsg_Setup ( BOOLEAN eng1, BOOLEAN eng2, BOOLEAN eaps_ibf_1,
                                   BOOLEAN eaps_ibf_2 )
{
  APAC_ERRMSG_ENTRY_PTR entry_ptr;

  memset ( (void *) &m_APAC_ErrMsg, 0, sizeof(m_APAC_ErrMsg) );
  strncpy ( (char *) m_APAC_ErrMsg.title.entry, APAC_ERRMSG_TITLE_SETUP,
            APAC_MENU_DISPLAY_CHAR_MAX );
  entry_ptr = (APAC_ERRMSG_ENTRY_PTR) m_APAC_ErrMsg.list;

  if ( (eng1 && eng2) || (!eng1 && !eng2) )
  { // ENG not in proper state
    strncpy ( (CHAR *) entry_ptr->entry, APAC_SETUP_ENGMD, APAC_ERRMSG_CHAR_MAX );
    m_APAC_ErrMsg.num++;
    entry_ptr++;
  } // end if ( (eng1 && eng2) || (!eng1 && !eng2) )

  if ( (eng1 && !eaps_ibf_1) || (eng2 && !eaps_ibf_2) ||
       ( (!eng1 && !eng2) && (!eaps_ibf_1 || !eaps_ibf_2)) )
  { // EAPS/IBF sw not in proper state for Eng that is Flt Active
    if (m_APAC_Cfg.inletCfg == INLET_CFG_EAPS) {
      strncpy ( (CHAR *) entry_ptr->entry, APAC_SETUP_EAPS, APAC_ERRMSG_CHAR_MAX );
    }
    else {
      strncpy ( (CHAR *) entry_ptr->entry, APAC_SETUP_IBF, APAC_ERRMSG_CHAR_MAX );
    }
    m_APAC_ErrMsg.num++;
  } // end if ( (eng1 && !eaps_ibf_1) || (eng2 && !eaps_ibf_2) )

}


/******************************************************************************
 * Function:    APACMgr_errMsg_Stabilizing
 *
 * Description: Utility function to set the m_APAC_ErrMsg for Menu Err Display
 *              for PAC Stabilizing Fault detected
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_errMsg_Stabilizing ( void )
{
  STABLE_HISTORY data;
  APAC_ENG_ENUM eng_uut;
  APAC_NR_SEL_ENUM nr_sel;
  APAC_SNSR_TREND_NAMES_CFG_PTR names_ptr;
  APAC_ERRMSG_ENTRY_PTR entry_ptr;
  UINT16 i;
  BOOLEAN bResult;

  eng_uut = m_APAC_Status.eng_uut;
  nr_sel = m_APAC_Status.nr_sel;
  bResult = TrendGetStabilityHistory(m_APAC_Cfg.eng[eng_uut].idxTrend[nr_sel], &data);

  memset ( (void *) &m_APAC_ErrMsg, 0, sizeof(m_APAC_ErrMsg) );
  strncpy ( (char *) m_APAC_ErrMsg.title.entry, APAC_ERRMSG_TITLE_STABILIZING,
            APAC_MENU_DISPLAY_CHAR_MAX );
  names_ptr = (APAC_SNSR_TREND_NAMES_CFG_PTR) &m_APAC_Cfg.namesTrend;
  entry_ptr = (APAC_ERRMSG_ENTRY_PTR) m_APAC_ErrMsg.list;

  if (bResult == TRUE)
  {
    for (i=0;i<MAX_STAB_SENSORS;i++)
    {
      if ((data.bStable[i] == FALSE) && (data.sensorID[i] != SENSOR_UNUSED))
      {
        memcpy ((void *) entry_ptr->entry, names_ptr->snsr[i], APAC_SNSR_NAME_CHAR_MAX);
        m_APAC_ErrMsg.num++;
        entry_ptr++;
      }
    }
  }

  // ASSERT if *menu_invld_timeout_val_ptr_ms == NULL
  m_APAC_Status.timeout_ms = *m_APAC_Status.menu_invld_timeout_val_ptr_ms + CM_GetTickCount();
}


/******************************************************************************
 * Function:    APACMgr_LogSummary
 *
 * Description: Create APAC Summary (test completed successful) log
 *
 * Parameters:  commit - APAC_COMMIT_ENUM (RJCT,ACPT,INVLD,VLD,ABORT,NCR)
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_LogSummary ( APAC_COMMIT_ENUM commit )
{
  APAC_SUMMARY_LOG alog;
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_TBL_PTR tbl_ptr;
  APAC_INSTALL_ENUM tblIdx;
  

  m_APAC_Status.eng[m_APAC_Status.eng_uut].commit = commit;

  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[m_APAC_Status.eng_uut];
  
  tblIdx = m_APAC_Tbl_Mapping[m_APAC_Status.nr_sel][m_APAC_Cfg.inletCfg];
  tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_ITT][tblIdx];  

  alog.eng_uut = m_APAC_Status.eng_uut;
  alog.engineRunIndex = m_APAC_Status.eng[m_APAC_Status.eng_uut].engineRunIndex;
  memcpy ( (void *) alog.esn, (void *) eng_ptr->esn, APAC_ESN_MAX_LEN);
  alog.nr_sel = m_APAC_Status.nr_sel;
  alog.inletCfg = m_APAC_Cfg.inletCfg;
  memcpy ((void *) alog.chart_rev_str, tbl_ptr->chart_rev_str, APAC_INLET_CHART_REV_LEN);
  alog.manual = eng_ptr->vld.set;
  alog.reason = eng_ptr->vld.reason;
  alog.reason_summary = eng_ptr->vld.reason_summary; 
  alog.engHrs_prev_s = eng_ptr->hrs.prev_s;
  alog.engHrs_curr_s = eng_ptr->hrs.curr_s;
  alog.ittMargin = eng_ptr->itt.margin;
  alog.ittMax = eng_ptr->itt.max;
  alog.ittAvg = eng_ptr->itt.avgVal;
  alog.ngMargin = eng_ptr->ng.margin;
  alog.ngMax = eng_ptr->ng.max;
  alog.ngAvg = eng_ptr->ng.avgVal;
  alog.commit = commit;
  alog.common = eng_ptr->common;
  alog.itt = eng_ptr->itt;
  alog.ng = eng_ptr->ng;

  LogWriteETM (APP_ID_APAC_SUMMARY, LOG_PRIORITY_LOW, &alog, sizeof(alog), NULL);

  // If this is a VLD commit, then update Eng Hrs and Clr Valid Flag
  if (commit == APAC_COMMIT_VLD) {
    APACMgr_NVMUpdate( m_APAC_Status.eng_uut, APAC_VLD_REASON_NONE, &m_APAC_VLD_Log );
  }
  APACMgr_NVMAppendHistData(); // NOTE: Eng Hrs will be updates also if VLD commit type
}


/******************************************************************************
 * Function:    APACMgr_LogEngStart
 *
 * Description: Create APAC Eng Start Test Log
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_LogEngStart ( void )
{
  APAC_ENG_START_LOG alog;
  APAC_ENG_STATUS_PTR eng_ptr;

  CM_GetTimeAsTimestamp(&m_APAC_Status.ts_start);

  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[m_APAC_Status.eng_uut];

  alog.eng_uut = m_APAC_Status.eng_uut;
  alog.engineRunIndex = m_APAC_Status.eng[m_APAC_Status.eng_uut].engineRunIndex;
  memcpy ( (void *) alog.esn, (void *) eng_ptr->esn, APAC_ESN_MAX_LEN);
  alog.nr_sel = m_APAC_Status.nr_sel;
  alog.inletCfg = m_APAC_Cfg.inletCfg;
  alog.engHrs_prev_s = eng_ptr->hrs.prev_s;
  alog.engHrs_curr_s = eng_ptr->hrs.curr_s;

  LogWriteETM (APP_ID_APAC_START, LOG_PRIORITY_LOW, &alog, sizeof(alog), NULL);
}

/******************************************************************************
 * Function:    APACMgr_LogFailure
 *
 * Description: Create APAC Failure Test Log.
 *              From "Config Error", "PAC No Start", "PAC No Stabl", "Compute Err"
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_LogFailure ( void )
{
  APAC_FAILURE_LOG alog;
  APAC_ENG_ENUM i; 
  UINT16 disc; 
  FLOAT32 fval;
  BOOLEAN vld; 
  APAC_ENG_CFG_PTR eng_ptr;         

  memcpy ( (void *) &alog.data, (void *) &m_APAC_ErrMsg, sizeof(alog.data) );
  alog.eng_uut        = m_APAC_Status.eng_uut;
  alog.engineRunIndex = (m_APAC_Status.eng_uut != APAC_ENG_MAX) ? 
                         m_APAC_Status.eng[m_APAC_Status.eng_uut].engineRunIndex : 
                         ENGRUN_UNUSED; 
  alog.nr_sel         = m_APAC_Status.nr_sel;
  alog.inletCfg       = m_APAC_Cfg.inletCfg;
  
  disc = 0; 
  for ( i = APAC_ENG_1; i < APAC_ENG_MAX ; i++ )
  {
    eng_ptr = (APAC_ENG_CFG_PTR) &m_APAC_Cfg.eng[i]; 
    vld = SensorGetValueEx( eng_ptr->idxEAPS_IBF_sw, (FLOAT32 *) &fval);
    disc = (fval == 1.0) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_INLET_STATE])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_INLET_STATE]) ;
    disc = (vld == TRUE) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_INLET_VLD])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_INLET_VLD]) ;
    vld = SensorGetValueEx( eng_ptr->idxEngFlt, (FLOAT32 *) &fval);
    disc = (fval == 1.0) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_FLIGHT_STATE])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_FLIGHT_STATE]) ;
    disc = (vld == TRUE) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_FLIGHT_VLD])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_FLIGHT_VLD]) ;
    vld = SensorGetValueEx( eng_ptr->idxEngIdle, (FLOAT32 *) &fval);
    disc = (fval == 1.0) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_IDLE_STATE])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_IDLE_STATE]) ;
    disc = (vld == TRUE) ? (disc | APAC_ENG_DISC_STATE_POS[i][ENG_IDLE_VLD])  :
                           (disc & ~APAC_ENG_DISC_STATE_POS[i][ENG_IDLE_VLD]) ;
  }
  alog.discState = disc; 
  LogWriteETM (APP_ID_APAC_FAILURE, LOG_PRIORITY_LOW, &alog, sizeof(alog), NULL);
}


/******************************************************************************
 * Function:    APACMgr_LogAbortNCR
 *
 * Description: Create APAC Abort or NCR (No Crew Response) Log
 *
 * Parameters:  commit - APAC_COMMIT_ENUM (ABORT,NCR)
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_LogAbortNCR ( APAC_COMMIT_ENUM commit )
{
  APAC_ABORT_NCR_LOG alog;

  alog.commit = commit;
  alog.eng_uut = m_APAC_Status.eng_uut; 
  alog.engineRunIndex = (m_APAC_Status.eng_uut != APAC_ENG_MAX) ? 
                         m_APAC_Status.eng[m_APAC_Status.eng_uut].engineRunIndex : 
                         ENGRUN_UNUSED; 
  
  alog.reason = (m_APAC_Status.eng_uut != APAC_ENG_MAX) ?
                 m_APAC_Status.eng[m_APAC_Status.eng_uut].vld.reason :
                 APAC_VLD_REASON_NONE ;
   
  alog.reason_summary = (m_APAC_Status.eng_uut != APAC_ENG_MAX) ?
                 m_APAC_Status.eng[m_APAC_Status.eng_uut].vld.reason_summary :
                 APAC_VLD_REASON_NONE ;
   
  LogWriteETM (APP_ID_APAC_ABORT_NCR, LOG_PRIORITY_LOW, &alog, sizeof(alog), NULL);
}


/******************************************************************************
 * Function:    APACMgr_ClearStatus
 *
 * Description: Utility function to Clear the APAC Status to initialized state
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_ClearStatus (void)
{
  UINT16 i;
  APAC_SNSR_STATUS_PTR snsr_status_ptr;

  m_APAC_Status.run_state = APAC_RUN_STATE_IDLE;
  m_APAC_Status.state = APAC_STATE_IDLE;
  m_APAC_Status.eng_uut = APAC_ENG_MAX;    // Set to "un-initialized"
  m_APAC_Status.nr_sel = APAC_NR_SEL_MAX;  // Set to "un-initialzed"
  m_APAC_Status.ts_start.Timestamp = 0;
  m_APAC_Status.ts_start.MSecond = 0;
  for (i=0;i<(UINT16) APAC_ENG_MAX;i++) {
    m_APAC_Status.eng[i].commit = APAC_COMMIT_MAX;
    memset ( (void *) &m_APAC_Status.eng[i].common, 0, sizeof(APAC_ENG_CALC_COMMON) );
    memset ( (void *) &m_APAC_Status.eng[i].itt, 0, sizeof(APAC_ENG_CALC_DATA) );
    memset ( (void *) &m_APAC_Status.eng[i].ng,  0, sizeof(APAC_ENG_CALC_DATA) );
  }
  snsr_status_ptr = (APAC_SNSR_STATUS_PTR) &m_APAC_Snsr_Status[0];
  for (i=0;i<APAC_SNSR_MAX;i++) {
    snsr_status_ptr->bNotOk = FALSE; 
    snsr_status_ptr++; 
  }
}


/******************************************************************************
 * Function:    APACMgr_GetDataValues
 *
 * Description: Utility function to Get the Eng Data avg parameter values from
 *              the Trned object / function
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_GetDataValues (void)
{
  TREND_SAMPLE_DATA data;
  APAC_DATA_AVG_ENTRY_PTR pDataEntry;
  APAC_DATA_AVG_ENUM i;
  SNSR_SUMMARY *snsr_ptr;
  UINT16 j;
#ifdef APAC_TEST_SIM
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_ENG_CALC_DATA_PTR itt_ptr, ng_ptr;
  APAC_ENG_CFG_PTR eng_cfg_ptr;
  APAC_ENG_CALC_COMMON_PTR eng_common_ptr;
  APAC_SNSR_CFG_PTR snsr_cfg_ptr;
  APAC_ENG_ENUM eng_uut;
#endif

#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/ 
  if (m_APAC_Debug.simTrend == TRUE)
  {
    eng_uut = m_APAC_Status.eng_uut;
    eng_cfg_ptr = (APAC_ENG_CFG_PTR) &m_APAC_Cfg.eng[eng_uut];
    snsr_cfg_ptr = (APAC_SNSR_CFG_PTR) &m_APAC_Cfg.snsr;
    eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[eng_uut];
    itt_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->itt;
    ng_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->ng;
    eng_common_ptr = (APAC_ENG_CALC_COMMON_PTR) &m_APAC_Status.eng[eng_uut].common;

    eng_common_ptr->avgOAT = SensorGetValue(eng_cfg_ptr->idxOAT);
    eng_common_ptr->avgBaroPres = SensorGetValue(snsr_cfg_ptr->idxBaroPres);
    eng_common_ptr->avgTQ = SensorGetValue(eng_cfg_ptr->idxTq);
    eng_common_ptr->valBaroCorr = SensorGetValue(snsr_cfg_ptr->idxBaroCorr);
    itt_ptr->avgVal = SensorGetValue(eng_cfg_ptr->idxITT);
    ng_ptr->avgVal = SensorGetValue(eng_cfg_ptr->idxNg);
  }
  else
  {
  /*vcast_dont_instrument_end*/  
#endif
    TrendGetSampleData(m_APAC_Cfg.eng[m_APAC_Status.eng_uut].idxTrend[m_APAC_Status.nr_sel],
                       &data);
    // Loop thru each element in m_APAC_DataAvg[]
    pDataEntry = (APAC_DATA_AVG_ENTRY_PTR) m_APAC_DataAvg[m_APAC_Status.eng_uut].avg;
    for (i=APAC_DATA_AVG_OAT;i<APAC_DATA_AVG_MAX;i++)
    {
      snsr_ptr = (SNSR_SUMMARY *) &data.snsrSummary[0];
      for (j=0;j<MAX_TREND_SENSORS;j++)
      {
        if (snsr_ptr->SensorIndex == pDataEntry->idx)
        {
          *pDataEntry->dest_ptr = (FLOAT64) snsr_ptr->fAvgValue;
          break; // exit inner loop, and go to next APAC_DATA_AVG_ENUM
        } // end if (snsr_ptr->SensorIndex == pDataAvg->avg.idx)
        snsr_ptr++;
      } // inner loop thru MAX_TREND_SENSORS
      if (j == MAX_TREND_SENSORS)
      { // Sensor Index not found, just move onto next APAC_DATA_AVG_ENUM
        // and set an out of range value, as calc will fail.
        if ( i != APAC_DATA_VAL_BAROCORR) {
          GSE_DebugStr(NORMAL,TRUE,
                "APACMgr: APACMgr_GetDataValues() Snsr Idx Not Found Eng=%d,Idx=%d\r\n",
                m_APAC_Status.eng_uut, pDataEntry->idx);
          *pDataEntry->dest_ptr = (FLOAT64) APAC_SNSR_NOT_FOUND_VAL;
        } // end if ( i != APAC_DATA_VAL_BARO)
        else { // Get APAC_DATA_VAL_BARO val directly
          *pDataEntry->dest_ptr = (FLOAT64) SensorGetValue(pDataEntry->idx);
        }
      }
      pDataEntry++; // Move to next element in APAC_DATA_AVG_ENUM
    } // outer loop thru APAC_DATA_AVG_ENUM
#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/ 
  }
  /*vcast_dont_instrument_end*/
#endif

}


/******************************************************************************
 * Function:    APACMgr_RestoreAppDataHist
 *
 * Description: Restores the APAC App Hist Data from EEPROM
 *
 * Parameters:  None
 *
 * Returns:     True always returned
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN APACMgr_RestoreAppDataHist( void )
{
  RESULT result;
  BOOLEAN bOk;


  bOk = TRUE;

  result = NV_Open(NV_APAC_HIST);

  if ( result == SYS_OK )
  {
    NV_Read(NV_APAC_HIST, 0, (void *) &m_APAC_Hist, sizeof(m_APAC_Hist));
  }

  // If open failed, re-init app data
  if ( result != SYS_OK )
  {
    bOk = APACMgr_FileInitHist();
    bOk = FALSE;  // Always return fail if we have to re-init File
    // Note: m_APAC_Hist_Status already cleared at this point
  }
  else // History Data retrieved
  { 
    APACMgr_RestoreAppDataHistOrder(); 
  } // end else History Data retrieved

  return (bOk);
}


/******************************************************************************
 * Function:    APACMgr_RestoreAppDataHistOrder
 *
 * Description: Puts the restores the APAC App Hist Data from EEPROM into 
 *              order (most recent date then most recent time)
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       
 *  - Loop thru and find # of entries in history.  After a period of time, this
 *    list should be 100.
 *  - Point to the last entry, which is the most recent Date
 *  - Find the block (Begin and End) of the tests withing this Date
 *    Worst case all 100 entries were performed on same day
 * 
 *****************************************************************************/
static
void APACMgr_RestoreAppDataHistOrder( void )
{
  UINT16 i;
  APAC_HIST_ENTRY_PTR entry_ptr, begin_ptr;
  UINT32 curr_mon_day, curr_yr;

  // Assume data in order sequence from idx[0] to idx[99]. ->ts field will be
  // "0" if not used / initialized
  m_APAC_Hist_Status.num = 0;
  entry_ptr = (APAC_HIST_ENTRY_PTR) m_APAC_Hist.entry;
  for (i=0;i<APAC_HIST_ENTRY_MAX;i++) {
    if ( ( entry_ptr->ts.Timestamp == APAC_HIST_BLANK ) &&
         ( entry_ptr->ts.MSecond == APAC_HIST_BLANK ) )
    {
      break;  // Exit loop.  Blank entry found
    } // end if ts == APAC_HIST_BLANK
    m_APAC_Hist_Status.num++;
    entry_ptr++;
  } // end for loop i thru APAC_HIST_ENTRY_MAX
  // Adj to point the last entry, not next empty
  i = (m_APAC_Hist_Status.num == 0) ? 0 : (m_APAC_Hist_Status.num - 1);
  m_APAC_Hist_Status.idxDate = i;
  m_APAC_Hist_Status.idxDay = i;

  // Setup idxDayEnd and idxDayBegin
  m_APAC_Hist_Status.idxDayEnd = m_APAC_Hist_Status.idxDay;
  entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[m_APAC_Hist_Status.idxDayEnd];
  begin_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[0];
  curr_mon_day = APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp);
  curr_yr = APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp);
  i = 0;
  // Loop thru to find idxDayBegin
  while (entry_ptr > begin_ptr) {
    entry_ptr--;
    if ((curr_mon_day != APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp)) ||
        (curr_yr != APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp)) ) {
      break;
    }
    i++;
  }
  m_APAC_Hist_Status.idxDayBegin = m_APAC_Hist_Status.idxDayEnd - i;
  
}


/******************************************************************************
 * Function:    APACMgr_RestoreAppDataNVM
 *
 * Description: Restores the APAC App Data from EEPROM
 *
 * Parameters:  None
 *
 * Returns:     True always returned
 *
 * Notes:
 *  - m_APAC_Cfg s/b be restored from NVM before calling this func
 *
 *****************************************************************************/
static
BOOLEAN APACMgr_RestoreAppDataNVM ( void )
{
  RESULT result;
  BOOLEAN bOk;
  APAC_ENG_ENUM i;
  APAC_VLD_REASON_ENUM reason[APAC_ENG_MAX];
  INT32 crc,prev_crc;

  bOk = TRUE;

  result = NV_Open(NV_APAC_NVM);
  if ( result == SYS_OK )
  {
    NV_Read(NV_APAC_NVM, 0, (void *) &m_APAC_NVM, sizeof(m_APAC_NVM));
  }
  // If open failed, re-init app data
  if ( result != SYS_OK )
  {
    bOk = APACMgr_FileInitNVM();
    bOk = FALSE;  // Always return fail if we have to re-init File
  }
  else // NVM Data retrieved
  {
    // Check if Cfg CRC has changed
    reason[APAC_ENG_1] = reason[APAC_ENG_2] = APAC_VLD_REASON_NONE;
    crc = NV_GetFileCRC(NV_CFG_MGR);
    prev_crc = m_APAC_NVM.cfgCRC;
    // Only when APAC is enabled will the cfg changed be detected.
    if ( (m_APAC_NVM.cfgCRC != crc) && (m_APAC_Cfg.enabled == TRUE) )
    {
      reason[APAC_ENG_1] = reason[APAC_ENG_2] = APAC_VLD_REASON_CFG;
      m_APAC_NVM.cfgCRC = crc;
    }
    for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++) {
      m_APAC_Status.eng[i].hrs.prev_s = m_APAC_NVM.eng[i].hrs_s;
      if (reason[i] != APAC_VLD_REASON_NONE) {
       snprintf (m_APAC_VLD_Log.msg,sizeof(m_APAC_VLD_Log.msg),"E%d: curr=0x%04X,prev=0x%04X",
                  i, crc, prev_crc );
        APACMgr_NVMUpdate( i, reason[i], &m_APAC_VLD_Log );
      }
      m_APAC_Status.eng[i].vld.set = m_APAC_NVM.eng[i].vld;
      m_APAC_Status.eng[i].vld.reason = m_APAC_NVM.eng[i].vld_reason;
      m_APAC_Status.eng[i].vld.reason_summary = m_APAC_NVM.eng[i].vld_reason_summary;
    } // end for loop thru engs and if need to update __NVMUpdate()
  } // end else NVM Data retrieved

  return (bOk);
}


/******************************************************************************
 * Function:    APACMgr_NVMAppendHistData
 *
 * Description: Add rec to end of Hist Buff and Store the updated APAC App Data
 *              to EEPROM
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void APACMgr_NVMAppendHistData( void )
{
  APAC_HIST_ENTRY_PTR entry_ptr;
  UINT16 idxLast;
  APAC_ENG_STATUS_PTR eng_ptr;
  UINT32 prev_mon_day, prev_yr, curr_mon_day, curr_yr;
  TIMESTAMP ts;

  // If m_APAC_Hist_Status.num == APAC_HIST_ENTRY_MAX
  //   cpy idx1 to idx99 TO idx0 to idx98 (takes 320 us)
  if (m_APAC_Hist_Status.num == APAC_HIST_ENTRY_MAX)
  {
    memcpy ( (void *) m_APAC_Hist.entry, (void *) &m_APAC_Hist.entry[1],
              sizeof(APAC_HIST_ENTRY) * (APAC_HIST_ENTRY_MAX - 1) );
    idxLast = APAC_HIST_ENTRY_MAX - 1;
  }
  else
  {
    idxLast = m_APAC_Hist_Status.num++;
  }

  // Cpy current data to idx99
  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[m_APAC_Status.eng_uut];
  entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[idxLast];
  CM_GetTimeAsTimestamp(&entry_ptr->ts);
  entry_ptr->ittMargin = (FLOAT32) eng_ptr->itt.margin;
  entry_ptr->ngMargin = (FLOAT32) eng_ptr->ng.margin;
  entry_ptr->flags = (UINT8) APAC_HIST_ENCODE((UINT8) m_APAC_Status.eng_uut,
                             (UINT8) eng_ptr->commit, (UINT8) m_APAC_Status.nr_sel);
  // Update NVM App Data
  NV_Write(NV_APAC_HIST, 0, (void *) &m_APAC_Hist, sizeof(m_APAC_Hist));

  // update internal ptr
  //  if idxDate == prev last entry and if Date of curr has not changed, then update idxDate
  //      and idxDayEnd, idxDay
  if ( m_APAC_Hist_Status.num > 1 )
  {
    ts = m_APAC_Hist.entry[m_APAC_Hist_Status.idxDate].ts;
    prev_mon_day = APAC_HIST_PARSE_MON_DAY(ts.Timestamp);
    prev_yr = APAC_HIST_PARSE_YR(ts.Timestamp);
    curr_mon_day = APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp);
    curr_yr = APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp);
    if ( (m_APAC_Hist_Status.idxDate == (idxLast - 1)) && (prev_mon_day == curr_mon_day) &&
         (prev_yr == curr_yr) )
    { // Move idx to curr entry
      m_APAC_Hist_Status.idxDate = idxLast;
      m_APAC_Hist_Status.idxDay = idxLast;
      m_APAC_Hist_Status.idxDayEnd = idxLast;
    }
  }

}


/******************************************************************************
 * Function:    APACMgr_NVMHistRemoveEngData
 *
 * Description: Utility function to remove the Eng Data from the History Buffer
 *              in NVM
 *
 * Parameters:  eng - Eng of the data to remove
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void APACMgr_NVMHistRemoveEngData (APAC_ENG_ENUM eng )
{
  APAC_HIST_ENTRY_PTR entry_ptr, tmp_ptr, begin_ptr;
  UINT16 i, num, num_keep;
  UINT32 curr_mon_day, curr_yr;
  APAC_HIST_ENTRY buff[APAC_HIST_ENTRY_MAX];


  entry_ptr = (APAC_HIST_ENTRY_PTR) m_APAC_Hist.entry;
  tmp_ptr = (APAC_HIST_ENTRY_PTR) buff;
  num = m_APAC_Hist_Status.num;
  num_keep = 0;
  // Keep all the Eng Data from Eng where ESN has not changed
  for (i=0;i<num;i++)
  {
    if ( (APAC_ENG_ENUM) APAC_HIST_PARSE_ENG(entry_ptr->flags) != eng ) {
      *tmp_ptr++ = *entry_ptr;  // Save this Eng Data
      num_keep++;
    }
    entry_ptr++;
  }

  GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_HistRemoveEngData() Eng=%d,Tot=%d,Del=%d\r\n",
               eng, num, num-num_keep);

  if (num_keep != num) // If num_keep == num, then no data thrown away, thus no update req
  {
    // Copy the "kept" Eng data back to Hist Buff
    memcpy ( (void *) m_APAC_Hist.entry, (void *) buff, sizeof(APAC_HIST_ENTRY) * num_keep);
    entry_ptr = (APAC_HIST_ENTRY_PTR) m_APAC_Hist.entry;
    // Delete the rest of buff
    memset ((void *) (entry_ptr + num_keep), 0, sizeof(APAC_HIST_ENTRY) * (num - num_keep));
    // Update NVM App Data
    NV_Write(NV_APAC_HIST, 0, (void *) &m_APAC_Hist, sizeof(m_APAC_Hist));
    // Update internal ptr / idx
    m_APAC_Hist_Status.num = num_keep;
    i = (m_APAC_Hist_Status.num  == 0) ? 0 : (m_APAC_Hist_Status.num  - 1);
    m_APAC_Hist_Status.idxDate = i;
    m_APAC_Hist_Status.idxDay = i;

    // Setup idxDayEnd and idxDayBegin
    m_APAC_Hist_Status.idxDayEnd = m_APAC_Hist_Status.idxDay;
    entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[m_APAC_Hist_Status.idxDayEnd];
    begin_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[0];
    curr_mon_day = APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp);
    curr_yr = APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp);
    i = 0;
    // Loop thru to find idxDayBegin
    while (entry_ptr > begin_ptr) {
      entry_ptr--;
      if ((curr_mon_day != APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp)) ||
          (curr_yr != APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp)) ) {
        break;
      }
      i++;
    }
    m_APAC_Hist_Status.idxDayBegin = m_APAC_Hist_Status.idxDayEnd - i;
  } // end if num_keep != num

}


/******************************************************************************
 * Function:    APACMgr_NVMUpdate
 *
 * Description: Updates the APAC App Data in EEPROM
 *
 * Parameters:  eng_uut - Eng Selected
 *              reason - APAC_VLD_REASON_ENUM for manual validate
 *              plog - APAC_VLD_LOG_PTR to log data to write
 *
 * Returns:     None
 *
 * Notes:
 *  - m_APAC_VLD_Log.msg s/b initialized before call to this func
 *
 *****************************************************************************/
static
void APACMgr_NVMUpdate ( APAC_ENG_ENUM eng_uut, APAC_VLD_REASON_ENUM reason,
                         APAC_VLD_LOG_PTR plog )
{
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_DATA_ENG_NVM_PTR eng_nvm_ptr;

  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[eng_uut];
  eng_nvm_ptr = (APAC_DATA_ENG_NVM_PTR) &m_APAC_NVM.eng[eng_uut];

  if ( reason != APAC_VLD_REASON_NONE ) {
    eng_ptr->vld.set = eng_nvm_ptr->vld = TRUE;  // Set manual validate flag
    eng_nvm_ptr->vld_reason_summary |= APAC_VLD_BIT_ENCODE_CONST[reason]; 
    // Rec Sys Log
    plog->reason = reason;
    plog->reason_summary = eng_nvm_ptr->vld_reason_summary;
    LogWriteETM (APP_ID_APAC_VLD_MANUAL, LOG_PRIORITY_LOW, plog,
                 sizeof(m_APAC_VLD_Log), NULL);
  }
  else {
    // On Manual Validate Action, reset the next 50 hrs
    eng_ptr->hrs.prev_s = eng_nvm_ptr->hrs_s = eng_ptr->hrs.start_s;
    eng_ptr->vld.set = eng_nvm_ptr->vld = FALSE; // Clr manual validate flag
    eng_nvm_ptr->vld_reason_summary = APAC_VLD_BIT_ENCODE_CONST[reason]; // reason == NONE
  }
  eng_ptr->vld.reason = eng_nvm_ptr->vld_reason = reason;
  eng_ptr->vld.reason_summary = eng_nvm_ptr->vld_reason_summary; 
  // Update App data
  NV_Write(NV_APAC_NVM, 0, (void *) &m_APAC_NVM, sizeof(m_APAC_NVM));

  GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_NVMUpdate() Eng=%d,Rea=%s,PrevEngHrs_s=%d\r\n",
                  eng_uut, APAC_VLD_STR_CONST[reason], eng_ptr->hrs.prev_s);

}


/******************************************************************************
* Function:    APACMgr_InitDataAvgStructs
*
* Description: Initializes m_APAC_DataAvg[] on startup.
*
* Parameters:  None
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void APACMgr_InitDataAvgStructs ( void )
{
  APAC_DATA_AVG_PTR pDataAvg;

  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_ENG_CALC_DATA_PTR itt_ptr, ng_ptr;
  APAC_ENG_CFG_PTR eng_cfg_ptr;
  APAC_ENG_CALC_COMMON_PTR eng_common_ptr;
  APAC_SNSR_CFG_PTR snsr_cfg_ptr;
  APAC_ENG_ENUM i;


  snsr_cfg_ptr = (APAC_SNSR_CFG_PTR) &m_APAC_Cfg.snsr;

  for (i=APAC_ENG_1;i<APAC_ENG_MAX;i++)
  {
    pDataAvg = (APAC_DATA_AVG_PTR) &m_APAC_DataAvg[i];
    eng_cfg_ptr = (APAC_ENG_CFG_PTR) &m_APAC_Cfg.eng[i];

    eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[i];
    itt_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->itt;
    ng_ptr = (APAC_ENG_CALC_DATA_PTR) &eng_ptr->ng;
    eng_common_ptr = (APAC_ENG_CALC_COMMON_PTR) &eng_ptr->common;

    pDataAvg->avg[APAC_DATA_AVG_OAT].idx = eng_cfg_ptr->idxOAT;
    pDataAvg->avg[APAC_DATA_AVG_OAT].dest_ptr = &eng_common_ptr->avgOAT;
    pDataAvg->avg[APAC_DATA_AVG_BAROPRES].idx = snsr_cfg_ptr->idxBaroPres;
    pDataAvg->avg[APAC_DATA_AVG_BAROPRES].dest_ptr = &eng_common_ptr->avgBaroPres;
    pDataAvg->avg[APAC_DATA_AVG_TQ].idx = eng_cfg_ptr->idxTq;
    pDataAvg->avg[APAC_DATA_AVG_TQ].dest_ptr = &eng_common_ptr->avgTQ;
    pDataAvg->avg[APAC_DATA_VAL_BAROCORR].idx = snsr_cfg_ptr->idxBaroCorr;
    pDataAvg->avg[APAC_DATA_VAL_BAROCORR].dest_ptr = &eng_common_ptr->valBaroCorr;
    pDataAvg->avg[APAC_DATA_AVG_ITT].idx = eng_cfg_ptr->idxITT;
    pDataAvg->avg[APAC_DATA_AVG_ITT].dest_ptr = &itt_ptr->avgVal;
    pDataAvg->avg[APAC_DATA_AVG_NG].idx = eng_cfg_ptr->idxNg;
    pDataAvg->avg[APAC_DATA_AVG_NG].dest_ptr = &ng_ptr->avgVal;
  }

}


/******************************************************************************
 * Function:    APACMgr_InitDispNames
 *
 * Description: Initializes the structures used for displaying names when
 *              failure occurs
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void APACMgr_InitDispNames ( void )
{
  CHAR   strTemp[APAC_SNSR_NAME_CHAR_MAX];
  UINT16 i;
  APAC_SNSR_TREND_NAMES_CFG_PTR names_ptr;

  memset ( (void *) m_APAC_Snsr_Status, 0, sizeof(m_APAC_Snsr_Status) );
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.barocorr );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_BARO_CORR].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.palt );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_PALT_CALC].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.tq );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_TQ].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.oat );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_OAT].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.itt );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_ITT].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.tqcorr );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_TQCORR].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.ittMax );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_ITTMAX].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.ngMax );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_NGMAX].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.tqcrit );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_TQCRIT].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.gndcrit );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_GNDCRIT].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.ittMg );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_ITTMG].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);
  snprintf ( strTemp, APAC_SNSR_NAME_CHAR_MAX, "%-5s", m_APAC_Cfg.names.ngMg );
  strncpy_safe ( m_APAC_Snsr_Status[APAC_SNSR_NGMG].str, APAC_SNSR_NAME_CHAR_MAX,
                 strTemp, _TRUNCATE);

  // Loop thru Trend Sensor Names and Pad with <space> char.
  names_ptr = (APAC_SNSR_TREND_NAMES_CFG_PTR) &m_APAC_Cfg.namesTrend;
  for (i=0;i<MAX_STAB_SENSORS;i++)
  {
    snprintf( names_ptr->snsr[i], APAC_SNSR_NAME_CHAR_MAX, "%-5s", names_ptr->snsr[i] );
  }

}


/******************************************************************************
* Function:    APACMgr_CheckCfg
*
* Description: Checks the APAC cfg to ensure all necessary cfg items has been cfg
*              (not default case).
*
* Parameters:  None
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void APACMgr_CheckCfg ( void )
{
  APAC_CFG_CHECK_PTR pCfg;
  UINT8  *byte_ptr;
  UINT32 *long_ptr;
  BOOLEAN apacCfgOk;
  CHAR *msg_ptr;

  pCfg = (APAC_CFG_CHECK_PTR) &APAC_CFG_CHECK_TBL[0];
  apacCfgOk = TRUE;
  m_APAC_Status.cfg_state = APAC_CFG_STATE_OK;  // Def to Cfg is Ok.
  msg_ptr = NULL;

  while ( ( pCfg->ptr != NULL ) && (apacCfgOk) )
  {
    byte_ptr = (UINT8 *) pCfg->ptr;
    long_ptr = (UINT32 *) pCfg->ptr;
    // More complex 'if' to limit number of branches to help with Statement Coverage
    if ( ((pCfg->size == APAC_CFG_CHECK_U8) && (*byte_ptr == (UINT8) pCfg->check_val)) ||
         ((pCfg->size == APAC_CFG_CHECK_U32) && (*long_ptr == (UINT32) pCfg->check_val)) )
    {
      apacCfgOk = FALSE;
      msg_ptr = (CHAR *) pCfg->msg;
      break;
    }
    pCfg++;
  } // end while ( pCfg->ptr != NULL )

  if (!apacCfgOk) // If err, update cfg_state and cfg_err[] msg
  {
    m_APAC_Status.cfg_state = APAC_CFG_STATE_FAULT;
    strncpy_safe ( m_APAC_Status.cfg_err, APAC_CFG_CHECK_MSG_SIZE, msg_ptr, _TRUNCATE );
    GSE_DebugStr(NORMAL,TRUE,"APACMgr: APACMgr_CheckCfg() Cfg Err (%s)\r\n",msg_ptr);
  }

  ASSERT ( apacCfgOk != FALSE );

}


#ifdef APACMGR_CYCLE_CBIT_INCR
/*vcast_dont_instrument_start*/ 
/******************************************************************************
* Function:    APACMgr_CheckCycle
*
* Description: When APAC Test is Running, check to ensure the Engine Cycle is counting
*              to ensure proper cycle configuration
*
* Parameters:  i - APAC_ENG_1 or APAC_ENG_2
*
* Returns:     None
*
* Notes:      
*  - When APAC Test is Running (_STABILIZE, _SAMPLING, _COMPUTING) check
*    Eng Flt Discrete against Eng Cycle
*  - If Eng Flt Discrete and Eng Cycle do not agree after 2 seconds, declare
*    APAC_VLD_REASON_CYCLE
*****************************************************************************/
static void APACMgr_CheckCycle ( APAC_ENG_ENUM i )
{
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_ENG_CYC_CHK_PTR cyc_ptr; 
  UINT32 cyc_cnt; 
  
  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[i];
  cyc_ptr = (APAC_ENG_CYC_CHK_PTR) &eng_ptr->cyc_chk; 
  
  if ( ( m_APAC_Status.run_state == APAC_RUN_STATE_STABILIZE) ||
       ( m_APAC_Status.run_state == APAC_RUN_STATE_SAMPLING) ||
       ( m_APAC_Status.run_state == APAC_RUN_STATE_COMPUTING) )
  {
    switch (cyc_ptr->state)
    {
      case APAC_ENG_CYC_CHK_IDLE:
        cyc_ptr->state = APAC_ENG_CYC_CHK_MONITOR; 
        cyc_ptr->prev_cyc_cnt = CycleGetCount(m_APAC_Cfg.eng[i].idxCycleHRS); 
        break;
       
      case APAC_ENG_CYC_CHK_MONITOR:
        if ( SensorIsValid(m_APAC_Cfg.eng[i].idxEngFlt ) &&
            (SensorGetValue(m_APAC_Cfg.eng[i].idxEngFlt) == APAC_ENG_FLT_SET) )
        {
          cyc_cnt = CycleGetCount(m_APAC_Cfg.eng[i].idxCycleHRS); 
          if (  cyc_cnt <= cyc_ptr->prev_cyc_cnt ) {
            if ( cyc_ptr->fail_cnt >= APAC_ENG_CYC_CHK_FAIL_CNT ) {
              snprintf ( m_APAC_VLD_Log.msg, sizeof(m_APAC_VLD_Log.msg), "E%d: curr=%d",
                         i, cyc_cnt );
              APACMgr_NVMUpdate ( i, APAC_VLD_REASON_CYCLE, &m_APAC_VLD_Log );
              cyc_ptr->state = APAC_ENG_CYC_CHK_FAULT; 
            }
            cyc_ptr->fail_cnt++; 
          }
          else {
            cyc_ptr->fail_cnt = 0;  // cyc_cnt incremented,clear fault cnt 
          }
          cyc_ptr->prev_cyc_cnt = cyc_cnt; // cyc cnt should inc once per sec when eng is ON
        }
        break;

      default:
      case APAC_ENG_CYC_CHK_FAULT:
        // Do Nothing. Wait for m_APAC_Status.run_state to complete Testing In Progress
        break;
    }
  }
  else if (cyc_ptr->state != APAC_ENG_CYC_CHK_IDLE)
  {
    APACMgr_CheckCycleClear(i); 
  }

}
/*vcast_dont_instrument_end*/
#endif


#ifdef APACMGR_CYCLE_CBIT_INCR
/*vcast_dont_instrument_start*/ 
/******************************************************************************
* Function:    APACMgr_CheckCycleClear
*
* Description: Clear / Re-init Check Cycle Variables
*
* Parameters:  i - APAC_ENG_1 or APAC_ENG_2
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void APACMgr_CheckCycleClear ( APAC_ENG_ENUM i )
{
  APAC_ENG_STATUS_PTR eng_ptr;
  APAC_ENG_CYC_CHK_PTR cyc_ptr;
  
  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[i];
  cyc_ptr = (APAC_ENG_CYC_CHK_PTR) &eng_ptr->cyc_chk; 
  
  memset ( (void *) cyc_ptr, 0, sizeof(APAC_ENG_CYC_CHK) ); 
}
/*vcast_dont_instrument_end*/
#endif


/******************************************************************************
 * Function:    APACMgr_GetStatus
 *
 * Description: Utility function to request current APACMgr Status
 *
 * Parameters:  None
 *
 * Returns:     Ptr to APACMgr Status Data
 *
 * Notes:       None
 *
 *****************************************************************************/
static
APAC_STATUS_PTR APACMgr_GetStatus (void)
{
  return ( (APAC_STATUS_PTR) &m_APAC_Status );
}


/******************************************************************************
 * Function:    APACMgr_CalcTqCorr
 *
 * Description:
 *
 * Parameters:  baro_corr - baro correction constant
 *              baro_pres - baro pressue (ave over sample period)
 *              tq - current tq (ave over sample period)
 *              common_ptr - ptr to APAC_ENG_CALC_COMMON_PTR
 *
 * Returns:     TRUE if tqCorr calculated
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN APACMgr_CalcTqCorr (FLOAT64 baro_corr, FLOAT64 baro_pres, FLOAT64 tq,
                            APAC_ENG_CALC_COMMON_PTR common_ptr)
{
  FLOAT64 baro_conv, palt_corr;
  FLOAT64 ax1,ax2,ay1,ay2;
  APAC_PALT_COEFF_ENTRY_PTR entry_ptr;
  UINT16 i,j;
  BOOLEAN ok;

  ok = FALSE;
  baro_conv = APAC_CALC_BARO_CORR(baro_corr);
  palt_corr = APAC_CALC_PALT_CORR( baro_pres, baro_conv );
  common_ptr->tqCorr = 0.0f;

  entry_ptr = (APAC_PALT_COEFF_ENTRY_PTR) m_APAC_PALT_Coeff.p;
  j = m_APAC_PALT_Coeff.numEntries;
  for (i=0;i<(j-1);i++) {
    if ( (palt_corr >= entry_ptr->val) && (palt_corr <= (entry_ptr + 1)->val) ) {
      ok = TRUE;
      ax1 = entry_ptr->val;
      ax2 = (entry_ptr + 1)->val;
      ay1 = entry_ptr->coeff;
      ay2 = (entry_ptr + 1)->coeff;
      common_ptr->coeffPALT = APAC_CALC_INTERPOLATE(palt_corr,ax1,ax2,ay1,ay2);
      common_ptr->tqCorr = common_ptr->coeffPALT * tq;
      break;
    }
    entry_ptr++;
  }

  common_ptr->convBaroCorr = baro_conv;
  common_ptr->calcPALT = palt_corr;

  GSE_DebugStr (NORMAL,TRUE,
     "APACMgr: APACMgr_CalcTqCorr() bi=%1.1f,pi=%1.1f,ti=%1.1f,bc=%1.1f,pc=%1.1f,co=%1.4f,t=%1.1f,ok=%d\r\n",
     baro_corr, baro_pres,tq,baro_conv,palt_corr,common_ptr->coeffPALT,common_ptr->tqCorr,ok);

  return (ok);
}


/******************************************************************************
* Function:    APACMgr_CalcMargin
*
* Description:
*
* Parameters:  oat - outside air temperature (ave over sample period)
*              tqDelta - calculated tqDelta value
*              val - param (ITT or Ng) current value (ave over sample period)
*              tbl_ptr - ptr to ITT or Ng margin calc table (1 of 12)
*              adj - adjustment to calc margin value
*              *margin_ptr - ptr to location to return calc ITT or Ng Margin
*              *max_ptr - ptr to location to return calc ITT or Ng Max Value
*              *c_ptr - ptr to location to return the calc c0,c1,c2,c3,c4 vals
*
* Returns:     TRUE if Margin successfull calculated
*
* Notes:
*  - Assume oat range is valid for table.  External app must ensure range as no
*    checking in this function
*  - Only two tq range per oat will be supported.
*  - Calc margin within range
*
*****************************************************************************/
static
BOOLEAN APACMgr_CalcMargin (FLOAT64 oat, FLOAT64 tqDelta, FLOAT64 val, APAC_TBL_PTR tbl_ptr,
                            FLOAT32 adj, FLOAT64 *margin_ptr, FLOAT64 *max_ptr,
                            APAC_ENG_CALC_DATA_PTR c_ptr)
{
  APAC_TBL_ENTRY_PTR entry1_ptr, entry2_ptr;
  FLOAT64 c0,c1,c2,c3,c4,res,margin;
  UINT16 i,j;
  BOOLEAN ok;

  ok = FALSE;
  c0 = c1 = c2 = c3 = c4 = res = margin = 0;

  entry1_ptr = (APAC_TBL_ENTRY_PTR) tbl_ptr->p;
  entry2_ptr = entry1_ptr + 1;
  j = (tbl_ptr->numEntries - 1);

  for (i=0;i<j;i++)
  {
    //if (entry1_ptr->oat == entry2_ptr->oat) {
    if ( fabs(entry2_ptr->oat - entry1_ptr->oat) < FLT_EPSILON) {
      entry2_ptr++;
      i++;  // Duplicate oat, move to next
    }
    if ( (oat >= entry1_ptr->oat) && (oat <= entry2_ptr->oat) )
    {
      // For tables with mult OAT for tq range, find the correct oat.
      // Only two tq range will be supported.
      // if ( (entry1_ptr->oat) == ((entry1_ptr + 1)->oat) )
      if ( fabs((entry1_ptr + 1)->oat - entry1_ptr->oat) < FLT_EPSILON )
      {
        if ( tbl_ptr->minEq == TRUE ) {
          if ( ((tqDelta >= (entry1_ptr + 1)->tq_min)) &&
               (tqDelta < (entry1_ptr + 1)->tq_max) ) {
            entry1_ptr++; // Next entry has the proper tq range
          }
        }
        else {
          if (((tqDelta > (entry1_ptr + 1)->tq_min)) && (tqDelta <= (entry1_ptr + 1)->tq_max))
          {
            entry1_ptr++; // Next entry has the proper tq range
          }
        } // end else minEq == FALSE
      } // end mult OAT for tq range
      // if ( (entry2_ptr->oat) == ((entry2_ptr + 1)->oat) )
      if ( fabs((entry2_ptr +1)->oat - entry2_ptr->oat) < FLT_EPSILON )
      {
        if ( tbl_ptr->minEq == TRUE ) {
          if (((tqDelta >= (entry2_ptr + 1)->tq_min)) && (tqDelta < (entry2_ptr + 1)->tq_max))
          {
            entry2_ptr++; // Next entry has the proper tq range
          }
        }
        else {
          if (((tqDelta > (entry2_ptr + 1)->tq_min)) && (tqDelta <= (entry2_ptr + 1)->tq_max))
          {
            entry2_ptr++; // Next entry has the proper tq range
          }
        } // end else minEq == FALSE
      } // end mult OAT for tq range

      // Calc c0,c1,c2,c3,c4
      c0 = APAC_CALC_INTERPOLATE(oat,entry1_ptr->oat,entry2_ptr->oat,entry1_ptr->c0,
                                 entry2_ptr->c0);
      c1 = APAC_CALC_INTERPOLATE(oat,entry1_ptr->oat,entry2_ptr->oat,entry1_ptr->c1,
                                 entry2_ptr->c1);
      c2 = APAC_CALC_INTERPOLATE(oat,entry1_ptr->oat,entry2_ptr->oat,entry1_ptr->c2,
                                 entry2_ptr->c2);
      c3 = APAC_CALC_INTERPOLATE(oat,entry1_ptr->oat,entry2_ptr->oat,entry1_ptr->c3,
                                 entry2_ptr->c3);
      c4 = APAC_CALC_INTERPOLATE(oat,entry1_ptr->oat,entry2_ptr->oat,entry1_ptr->c4,
                                 entry2_ptr->c4);

      res = APAC_CALC_MARGIN_COEFF(c0,c1,c2,c3,c4,tqDelta);

      GSE_DebugStr (NORMAL,TRUE,"APACMgr: APACMgr_CalcMargin() p=%d,c0=%1.6f,c1=%1.6f,c2=%1.6f,c3=%1.6f,c4=%1.6f\r\n",
                        i, c0, c1, c2, c3, c4 );

      // Calc margin within range
      if ( (res >= tbl_ptr->param_min) && (res <= tbl_ptr->param_max) ) {
        ok = TRUE;
        margin = (res - val) + adj; // Calc the margin and include adjustment
        break;
      }
      else {
        GSE_DebugStr (NORMAL,TRUE,"APACMgr: APACMgr_CalcMargin() max=%1.2f Range Failed (%1.2f,%1.2f)\r\n",
                          res,tbl_ptr->param_min,tbl_ptr->param_max);
        break;
      }
    }
    else // oat not within range
    {
      entry1_ptr = entry2_ptr;
      entry2_ptr++;
    }
  } // end for i loop

  *margin_ptr = margin;
  *max_ptr = res;

  GSE_DebugStr (NORMAL,TRUE,"APACMgr: APACMgr_CalcMargin() o=%1.2f,t=%1.2f,v=%1.2f,mrg=%1.2f(%1.2f),max=%1.2f,ok=%d\r\n",
                    oat, tqDelta, val, *margin_ptr, adj, *max_ptr, ok);

  c_ptr->c0 = c0;
  c_ptr->c1 = c1;
  c_ptr->c2 = c2;
  c_ptr->c3 = c3;
  c_ptr->c4 = c4;

  return (ok);
}


/******************************************************************************
* Function:    APACMgr_CheckHIGE_LOW
*
* Description: When APAC Test is Running, check to Tq and Gnd Speed are within
*              acceptable range
*
* Parameters:  *bResult - Ptr to return Tq Criteria Met (TRUE)
*              *bResult1 - Ptr to return GndSpd Criteria Met (TRUE)
*
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void APACMgr_CheckHIGE_LOW ( BOOLEAN *bResult, BOOLEAN *bResult1 )
{
  BOOLEAN validity;
  
  // Execute HIGE_LOW criteria to determine if cond met ?
  *bResult =  (BOOLEAN) EvalExeExpression ( EVAL_CALLER_TYPE_APAC,
              (INT32) m_APAC_Status.eng_uut,
              &m_APAC_Cfg.eng[m_APAC_Status.eng_uut].sc_TqCrit,
              (BOOLEAN *) &validity );
  *bResult = *bResult && validity;
  *bResult1 = (BOOLEAN) EvalExeExpression  ( EVAL_CALLER_TYPE_APAC,
              (INT32) m_APAC_Status.eng_uut,
              &m_APAC_Cfg.eng[m_APAC_Status.eng_uut].sc_GndCrit,
              (BOOLEAN *) &validity );
  *bResult1 = *bResult1 && validity;
}


/*****************************************************************************/
/* TEST Functions                                                            */
/*****************************************************************************/
#ifdef APAC_TEST_SIM
  /*vcast_dont_instrument_start*/ 
  static BOOLEAN APACMgr_Test_TrendAppStartTrend( TREND_INDEX idx, BOOLEAN bStart,
                                                  TREND_CMD_RESULT* rslt)
  {
    BOOLEAN bOk;

    if (m_APAC_Debug.simTrend == TRUE) {
      bOk = TRUE;
      *rslt = TREND_CMD_OK;
    }
    else {
      bOk = TrendAppStartTrend( idx, bStart, rslt );
    }
    return (bOk);
  }


  static void APACMgr_Test_TrendGetStabilityState ( TREND_INDEX idx,
      STABILITY_STATE* pStabState, UINT32* pDurMs,  SAMPLE_RESULT* pSampleResult)
  {
    if (m_APAC_Debug.simTrend == TRUE) {
      *pStabState = STB_STATE_SAMPLE;
      *pSampleResult = SMPL_RSLT_SUCCESS;
    }
    else {
      TrendGetStabilityState( idx, pStabState, pDurMs, pSampleResult );
    }
  }
  /*vcast_dont_instrument_end*/
#endif



#ifdef APAC_TEST_SIM
/*vcast_dont_instrument_start*/
static void APACMgr_Simulate ( void )
{
  UINT32 tick;
  static CHAR msg1[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg2[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg3[APAC_MENU_DISPLAY_CHAR_MAX];
  static CHAR msg4[APAC_MENU_DISPLAY_CHAR_MAX];

  tick = CM_GetTickCount();

  if ( ( tick >= simNextTick ) && (simState != APAC_SIM_MAX))
  {
    simState++;
    simNextTick = tick + simStateDelay[simState];

    memset ( msg1, 0, APAC_MENU_DISPLAY_CHAR_MAX );
    memset ( msg2, 0, APAC_MENU_DISPLAY_CHAR_MAX );
    memset ( msg3, 0, APAC_MENU_DISPLAY_CHAR_MAX );
    memset ( msg4, 0, APAC_MENU_DISPLAY_CHAR_MAX );

    switch (simState)
    {
      case APAC_SIM_CONFIG:
        APACMgr_IF_Config ( msg1, msg2, msg3, msg4, APAC_IF_MAX );
        break;

      case APAC_SIM_SELECT:
        APACMgr_IF_Select ( msg1, msg2, msg3, msg4, APAC_IF_NR102 );
        break;

      case APAC_SIM_SETUP:
        if ( APACMgr_IF_Setup ( msg1, msg2, msg3, msg4, APAC_IF_MAX ) == FALSE ) {
          // Setup Error, go to _SIM_RUNNING which will return TRUE to allow ErrMsg Display
          simState++; // Skip _MANUAL_VALID state
          simState++; // go to _SIM_RUNNING
          simNextTick = tick + simStateDelay[simState];
        }
        break;

      case APAC_SIM_MANUAL_VALID:
        APACMgr_IF_ValidateManual ( msg1, msg2, msg3, msg4, APAC_IF_MAX );
        break;

      case APAC_SIM_RUNNING:
        APACMgr_IF_RunningPAC ( msg1, msg2, msg3, msg4, APAC_IF_MAX );
        break;

      case APAC_SIM_COMPLETED:
        if (APACMgr_IF_CompletedPAC ( msg1, msg2, msg3, msg4, APAC_IF_MAX ) == FALSE)
          //simState += 2; // Force skip of _RESULT and _RESET_COMMIT and issue _RESET
          simState = APAC_SIM_CONFIG;  // Force back to _SELECT to re-run. Yes set to _CONFIG
                                       //   to do this as _CONFIG will be skipped to _SELECT
        break;

      case APAC_SIM_RESULT:
        APACMgr_IF_Result ( msg1, msg2, msg3, msg4, APAC_IF_MAX );
        break;

      case APAC_SIM_RESULT_COMMIT:
        APACMgr_IF_ResultCommit ( msg1, msg2, msg3, msg4, APAC_IF_VLD );
        break;

      case APAC_SIM_RESET:
        APACMgr_IF_Reset ( msg1, msg2, msg3, msg4, APAC_IF_DCLK );
        simState = APAC_SIM_MAX; // Uncomment to END Test, Comment out to perf repeating test
        break;

      case APAC_SIM_RESTART:
        simState = APAC_SIM_IDLE;
        simNextTick = CM_GetTickCount() + simStateDelay[simState];
        break;

      case APAC_SIM_IDLE:
      default:
        break;
    }
  }

  if (simState == APAC_SIM_RUNNING )
  {
    // Call _running constantly
    if ( APACMgr_IF_RunningPAC ( msg1, msg2, msg3, msg4, APAC_IF_MAX ) == TRUE )
    {
      if ( m_APAC_Status.run_state == APAC_RUN_STATE_FAULT ) {
        // Call to display failure msg
        APACMgr_IF_ErrMsg ( msg1, msg2, msg3, msg4, APAC_IF_MAX );
      }
      else if ( m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT ) {
        simNextTick = tick;  // Force exit of this state to next
      }
    }
  }

}
/*vcast_dont_instrument_end*/
#endif // APAC_TEST_SIM

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: APACMgr.c $
 * 
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 5/11/16    Time: 7:18p
 * Updated in $/software/control processor/code/application
 * SCR-1331 Fix my commit comment
 * 
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 5/11/16    Time: 7:08p
 * Updated in $/software/control processor/code/application
 * SCR-1331 Add  missing -vcast_dont_instrument_start&end-  
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 4/15/16    Time: 6:19p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #13 add disc state to FAILURE log. 
 * 
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 4/15/16    Time: 6:09p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #11 ABT/NCR Log has incorrect VLD Manual Reason.
 * Item #13 add disc state to FAILURE log. 
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 4/11/16    Time: 7:37p
 * Updated in $/software/control processor/code/application
 * Code Review Updates
 * 
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 4/01/16    Time: 5:15p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #9.   BAD Chart Value in APAC SUMMARY LOG 
 * 
 * *****************  Version 24  *****************
 * User: John Omalley Date: 3/29/16    Time: 8:17a
 * Updated in $/software/control processor/code/application
 * SCR 1320 Item #10
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 3/26/16    Time: 11:43p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update FLOAT to FLOAT64.
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 3/22/16    Time: 6:17p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update FLOAT to FLOAT64.
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 3/11/16    Time: 7:15p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update APAC ITT/Ng Margin and Max value from
 * FLOAT32 to FLOAT64.
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 3/11/16    Time: 1:59p
 * Updated in $/software/control processor/code/application
 * SCR #1320 Item #5 Add IT/Ng Cfg Offset Adj to Summary log and GSE
 * Status
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 3/10/16    Time: 6:56p
 * Updated in $/software/control processor/code/application
 * SCR #1320 Items #1,#2,#4 APAC Processing Updates
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 2/24/16    Time: 6:13p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #4 Comment out APAC Mgr Cycle CBIT Increment test.
 * Added a few vcast for commented out code. 
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 2/16/16    Time: 6:55p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #1 and #2.  GNDSP CRIT bad display and 1 frame _INCR_TQ
 * when not necessary.
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 2/02/16    Time: 9:43a
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #19 and #20.  Check in Cycle CBIT again.  ReOrder
 * History back to most recent when RCL selected.  
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 1/29/16    Time: 12:02p
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #18 Call DispProcessing Init thru APAC Mgr
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 1/19/16    Time: 2:28p
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #3.1 
 * *apac.status.eng.commit[0/1] default*
 * GSE indicates value is set to None on startup, code returns NCR
 * 
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 1/19/16    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR #1309 Item #3.  Remove 'apac.cfg.snsr.gndspd'  as not needed.
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 1/18/16    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR #1308. Various Updates from Reviews and Enhancements
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 1/16/16    Time: 6:50p
 * Updated in $/software/control processor/code/application
 * SCR #1309 Item #2. NVMgr warning msg on startup for APAC Mgr. 
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 1/04/16    Time: 6:20p
 * Updated in $/software/control processor/code/application
 * SCR 1308 - Updated per AI #12, added missing "..."
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 11/16/15   Time: 7:08p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing, Code Review Updates
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 11/11/15   Time: 11:48a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Add ENG ID to Failure Log
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 11/10/15   Time: 11:30a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing. Additional Updates from GSE/LOG Review AI.
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/05/15   Time: 6:45p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC, ESN updates
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/03/15   Time: 2:17p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing additional minor updates
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 15-10-23   Time: 4:54p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Additional Updates
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-10-20   Time: 1:16p
 * Updated in $/software/control processor/code/application
 * SCR #1304 Disable test debug stuff
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 15-10-19   Time: 9:35p
 * Updated in $/software/control processor/code/application
 * SCR #1304 Additional APAC Processing development
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 15-10-14   Time: 9:19a
 * Created in $/software/control processor/code/application
 * SCR #1304 APAC Processing Initial Check In
 *
 *****************************************************************************/
