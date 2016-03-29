#ifndef APAC_MGR_H
#define APAC_MGR_H

/******************************************************************************
            Copyright (C) 2015-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        APACMgr.h

    Description: Contains data structures related to the APACMgr function

    VERSION
      $Revision: 16 $  $Date: 3/29/16 8:17a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <float.h>


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

#include "TaskManager.h"
#include "Evaluator.h"
#include "Sensor.h"
#include "Trend.h"
#include "APACMgr_Tables.h"
#include "cycle.h"
#include "EngineRunInterface.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
#define APAC_ESN_MAX_LEN 7  // 6 char + NULL
#define APAC_RUN_COMPUTING_DELAY_MS  1500  // 1500 msec

#define APAC_TIMEOUT_HIGELOW_SEC  15
#define APAC_TIMEOUT_STABLE_SEC   20

#define APAC_ENG_HRS_VALIDATE_SEC  (50 * 3600) // 50 Hrs in seconds

#define APAC_MENU_DISPLAY_CHAR_MAX  13  // 12 + '\0'

#define APAC_ENG_CFG_DEFAULT  SENSOR_UNUSED,          /* ITT Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /*  Ng Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /*  Nr Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /*  Tq Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /* OAT Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /* Eng Idle Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /* Eng Flt  Sensor Index, Not Init */\
                              SENSOR_UNUSED,          /* Eng EAPS IBF Sensor Indx, Not Init */\
                              EVAL_EXPR_CFG_DEFAULT,  /* start the tq criteria  */\
                              EVAL_EXPR_CFG_DEFAULT,  /* start the gnd spd criteria */\
                              TREND_UNUSED,           /* 100% Nr Trend, Uninitialized  */\
                              TREND_UNUSED,           /* 102% Nr Trend, Uninitialized  */\
                              CYCLE_UNUSED,           /* Cycle EHR Index, Not Init */\
                              ENGRUN_UNUSED           /* ESN Index from Engine Run Object */

#define APAC_SNSR_CFG_DEFAULT  SENSOR_UNUSED,         /* PAMB      Sensor Index, Not Init */\
                               SENSOR_UNUSED,         /* BaroPress Sensor Index, Not Init */\
                               SENSOR_UNUSED          /* Nr 102 or 100 Disc Sel, Not Init */\


#define APAC_TIMEOUT_CFG_DEFAULT  APAC_TIMEOUT_HIGELOW_SEC, /* Timeout HIGELow */\
                                  APAC_TIMEOUT_STABLE_SEC   /* Timeout Stable  */

#define APAC_ADJUST_CFG_DEFAULT   0.0f,     /* itt margin adjust */\
                                  0.0f      /* ng margin adjust */

#define APAC_SNSR_NAME_CFG_DEFAULT "BARO ",          /* baro (RO)      */\
                                   "PALT ",          /* palt calc (RO) */\
                                   "TQ   ",          /* tq (RO)        */\
                                   "OAT  ",          /* oat (RO)       */\
                                   "ITT  ",          /* itt (RO) */\
                                   "TQCOR",          /* tqcorr   */\
                                   "ITTMX",          /* ittMax   */\
                                   "NGMX ",          /* ngMax    */\
                                   "TQCRT",          /* TQCRIT   */\
                                   "GSCRT",          /* GNDCRIT  */\
                                   "ITTMG",          /* ittMargin*/\
                                   "NGMG "           /* ngMargin */ 

#define APAC_SNSR_TREND_NAME_CFG_DEFAULT   \
                                   "TS000",          /* snsr 00 */\
                                   "TS001",          /* snsr 01 */\
                                   "TS002",          /* snsr 02 */\
                                   "TS003",          /* snsr 03 */\
                                   "TS004",          /* snsr 04 */\
                                   "TS005",          /* snsr 05 */\
                                   "TS006",          /* snsr 06 */\
                                   "TS007",          /* snsr 07 */\
                                   "TS008",          /* snsr 08 */\
                                   "TS009",          /* snsr 09 */\
                                   "TS010",          /* snsr 10 */\
                                   "TS011",          /* snsr 11 */\
                                   "TS012",          /* snsr 12 */\
                                   "TS013",          /* snsr 13 */\
                                   "TS014",          /* snsr 14 */\
                                   "TS015"           /* snsr 15 */

#define APAC_CFG_DEFAULT    APAC_ENG_CFG_DEFAULT,         /* eng1 */\
                            APAC_ENG_CFG_DEFAULT,         /* eng2 */\
                            APAC_SNSR_CFG_DEFAULT,        /* sensors */\
                            APAC_TIMEOUT_CFG_DEFAULT,     /* timeouts  */\
                            INLET_CFG_BASIC,              /* inlet configuration */\
                            APAC_ADJUST_CFG_DEFAULT,      /* adjust */\
                            APAC_SNSR_NAME_CFG_DEFAULT,   /* names */\
                            APAC_SNSR_TREND_NAME_CFG_DEFAULT, /* namesTrend */\
                            FALSE                         /* enabled */


#define APAC_ERRMSG_TITLE_SETUP       "CONFIG ERROR"
#define APAC_ERRMSG_TITLE_HIGE_LOW    "PAC NO START"
#define APAC_ERRMSG_TITLE_STABILIZING "PAC NOT STBL"
#define APAC_ERRMSG_TITLE_SAMPLING    "PAC NOT STBL"
#define APAC_ERRMSG_TITLE_COMPUTE     "COMPUTE ERR "

#define APAC_ENG_IDLE_SET   1.0f
#define APAC_ENG_FLT_SET    1.0f

#define APAC_EAPS_IBF_SET   1.0f

#define APAC_NR102_SET      1.0f 

#define APAC_SETUP_ENGMD  "ENGMD"
#define APAC_SETUP_EAPS   "EAPS "
#define APAC_SETUP_IBF    "IBF  "

#define APAC_CAT_102      'A'
#define APAC_CAT_100      ' '

#define APAC_ENG_POS_1    '1'
#define APAC_ENG_POS_2    '2'

#define APAC_ESN_DEFAULT  "XX0000"

#define APAC_STR_RJT   "RJT"
#define APAC_STR_ACP   "ACP"
#define APAC_STR_NVL   "NVL"
#define APAC_STR_VLD   "VLD"
#define APAC_STR_ABT   "ABT"
#define APAC_STR_NCR   "NCR"

#define APAC_STR_CAT_BASIC " "
#define APAC_STR_CAT_A     "A"

#define APAC_STR_ENG_1     "E1"
#define APAC_STR_ENG_2     "E2"

#define APAC_TQCORR_MIN    86.8f
#define APAC_TQCORR_MAX   187.3f

#define APAC_OAT_MIN  -50.0f
#define APAC_OAT_MAX  +50.0f

#define APAC_ITT_MARGIN_MIN  -99.0f
#define APAC_ITT_MARGIN_MAX  +99.0f

#define APAC_NG_MARGIN_MIN  -9.9f
#define APAC_NG_MARGIN_MAX  +9.9f

#define APAC_ENG_SN_UCHAR1  'K'
#define APAC_ENG_SN_LCHAR1  'k'
#define APAC_ENG_SN_UCHAR2  'B'
#define APAC_ENG_SN_LCHAR2  'b'

#define APAC_CFG_CHECK_U8        sizeof(UINT8)
#define APAC_CFG_CHECK_U32       sizeof(UINT32)
#define APAC_CFG_CHECK_MSG_SIZE  32

#define APAC_VLD_STR_NONE    "VLD_NONE"
#define APAC_VLD_STR_50HRS   "VLD_50HRS"
#define APAC_VLD_STR_ESN     "VLD_ESN"
#define APAC_VLD_STR_CFG     "VLD_CFG"
#define APAC_VLD_STR_CYCLE   "VLD_CYCLE"
#define APAC_VLD_STR_NVM     "VLD_NVM"


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum {
  APAC_ENG_1 = 0,
  APAC_ENG_2,
  APAC_ENG_MAX
} APAC_ENG_ENUM;

typedef enum {
  APAC_NR_SEL_100=0,
  APAC_NR_SEL_102,
  APAC_NR_SEL_MAX
} APAC_NR_SEL_ENUM;

// NOTE: Must be BASIC,EAPS,IBF order.  If order change, APAC_INSTALL_ENUM must change order
//                                                       APAC_INLET_CFG_STR_CONST change order
typedef enum {
  INLET_CFG_BASIC  = 0,
  INLET_CFG_EAPS,
  INLET_CFG_IBF,
  INLET_CFG_MAX
} APAC_INLET_CFG_ENUM;

// NOTE: Must be ITT, NG order. If ordering change, APAC_INSTALL_ENUM must also change ordering
typedef enum {
  APAC_ITT = 0,
  APAC_NG,
  APAC_PARAM_MARGIN_MAX
} APAC_PARAM_ENUM;

// NOTE: If APAC_INLET_CFG_ENUM ordering changes, this enum ordering must change also.
// NOTE: If APAC_PARAM_ENUM ordering changes, this enum ordering must change also.
typedef enum {
  APAC_INSTALL_100_NORMAL = 0,
  APAC_INSTALL_100_EAPS,
  APAC_INSTALL_100_IBF,
  APAC_INSTALL_102_NORMAL,
  APAC_INSTALL_102_EAPS,
  APAC_INSTALL_102_IBF,
  APAC_INSTALL_MAX
} APAC_INSTALL_ENUM;

typedef struct {
  SENSOR_INDEX idxITT;
  SENSOR_INDEX idxNg;
  SENSOR_INDEX idxNr;
  SENSOR_INDEX idxTq;
  SENSOR_INDEX idxOAT;      // NOTE: Although only one OAT, TTO (Inlet Temp) for each Eng might
                            //      be used instead. Thus implementation provide selection
                            //      for each Eng.
  SENSOR_INDEX idxEngIdle;  // Engine Idle Discrete (1 = Idle, 0 = Not Idle)
  SENSOR_INDEX idxEngFlt;   // Engine Flight Discrete (1 = Flight, 0 = Not Flight)
  SENSOR_INDEX idxEAPS_IBF_sw;  // NOTE: EAPS and IBF are two diff switches, however,
                            //       only one can exist on a/c
                            //       Dan has 2 cfg, 1 has EAPS sensor and 1 has IBF sensor
                            //       SRS treats EAPS/IBF as single "entity".
  EVAL_EXPR  sc_TqCrit;     // RPN Expression to start the tq criteria
  EVAL_EXPR  sc_GndCrit;    // RPN Expression to start the gnd spd criteria

  TREND_INDEX idxTrend[APAC_NR_SEL_MAX];  // Index to Trend for 100% or 102% Nr Setting
  CYCLE_INDEX idxCycleHRS;  // Index to Cycle Hrs for 50 Hr Manual PAC Validation
  ENGRUN_INDEX idxESN;      // Source of Engine SN from Engine Run Object
} APAC_ENG_CFG, *APAC_ENG_CFG_PTR;

typedef struct {
  SENSOR_INDEX idxBaroPres;
  SENSOR_INDEX idxBaroCorr;
  SENSOR_INDEX idxNR102_100;  // NR102 or 100 Discrete Select
} APAC_SNSR_CFG, *APAC_SNSR_CFG_PTR;

typedef struct {
  UINT32 hIGELow_sec;  // Default is 15 sec
  UINT32 stable_sec;   // Default is 20 sec
} APAC_TIMEOUT_CFG, *APAC_TIMEOUT_CFG_PTR;

typedef struct {
   FLOAT32 itt;  // ITT Margin Adjustment -99 to +99
   FLOAT32 ng;   // Ng Margin Adjustment -9.9 to +9.9
} APAC_ADJUST_CFG, *APAC_ADJUST_CFG_PTR;

// If APAC_SNSR_ENUM changes, then this struct must be updated
#define APAC_SNSR_NAME_CHAR_MAX  6  // 5 + '\0'
typedef struct {
  CHAR barocorr[APAC_SNSR_NAME_CHAR_MAX];
  CHAR palt[APAC_SNSR_NAME_CHAR_MAX];
  CHAR tq[APAC_SNSR_NAME_CHAR_MAX];
  CHAR oat[APAC_SNSR_NAME_CHAR_MAX];
  CHAR itt[APAC_SNSR_NAME_CHAR_MAX];
  CHAR tqcorr[APAC_SNSR_NAME_CHAR_MAX];
  CHAR ittMax[APAC_SNSR_NAME_CHAR_MAX];
  CHAR ngMax[APAC_SNSR_NAME_CHAR_MAX];
  CHAR tqcrit[APAC_SNSR_NAME_CHAR_MAX];
  CHAR gndcrit[APAC_SNSR_NAME_CHAR_MAX];
  CHAR ittMg[APAC_SNSR_NAME_CHAR_MAX];
  CHAR ngMg[APAC_SNSR_NAME_CHAR_MAX];
} APAC_SNSR_NAMES_CFG, *APAC_SNSR_NAMES_CFG_PTR;

typedef struct {
  CHAR snsr[MAX_STAB_SENSORS][APAC_SNSR_NAME_CHAR_MAX];
} APAC_SNSR_TREND_NAMES_CFG, *APAC_SNSR_TREND_NAMES_CFG_PTR;

typedef struct {
  APAC_ENG_CFG eng[APAC_ENG_MAX];
  APAC_SNSR_CFG snsr;
  APAC_TIMEOUT_CFG timeout;
  APAC_INLET_CFG_ENUM inletCfg;// inlet Cfg for both engines
  APAC_ADJUST_CFG adjust;      // adjustments to Margin Calc
  APAC_SNSR_NAMES_CFG names;   // 5 char name for internal calc params
  APAC_SNSR_TREND_NAMES_CFG namesTrend;  // 5 char name for trend sensors
  BOOLEAN enabled;             // APAC processing enabled
} APAC_CFG, *APAC_CFG_PTR;



typedef enum {
  APAC_STATE_IDLE,
  APAC_STATE_TEST_INPROGRESS,
  APAC_STATE_MAX
} APAC_STATE_ENUM;

typedef enum {
  APAC_RUN_STATE_IDLE,
  APAC_RUN_STATE_INCREASE_TQ,
  APAC_RUN_STATE_DECR_GND_SPD,
  APAC_RUN_STATE_STABILIZE,
  APAC_RUN_STATE_SAMPLING,
  APAC_RUN_STATE_COMPUTING,
  APAC_RUN_STATE_COMMIT, // COMPUTE completed successful, waiting for commit
  APAC_RUN_STATE_FAULT,  // Faulted State from _HIGE_Low, _STABLIZE, _SAMPLING, _COMPUTING
  APAC_RUN_STATE_MAX
} APAC_RUN_STATE_ENUM;

typedef enum {
  APAC_COMMIT_RJCT = 0,
  APAC_COMMIT_ACPT,
  APAC_COMMIT_INVLD,
  APAC_COMMIT_VLD,
  APAC_COMMIT_ABORT,
  APAC_COMMIT_NCR,
  APAC_COMMIT_MAX
} APAC_COMMIT_ENUM;

typedef enum {
  APAC_VLD_REASON_NONE,
  APAC_VLD_REASON_50HRS, // > 50 Hrs since last manual validation
  APAC_VLD_REASON_ESN,   // ESN has changed
  APAC_VLD_REASON_CFG,   // Cfg has changed
  APAC_VLD_REASON_CYCLE, // Cycle indicate cycle count has changed
  APAC_VLD_REASON_NVM,   // APAC NVM Data corrupted
  APAC_VLD_REASON_MAX
} APAC_VLD_REASON_ENUM;

typedef enum {
  APAC_CFG_STATE_DSB,
  APAC_CFG_STATE_OK,
  APAC_CFG_STATE_FAULT,
  APAC_CFG_STATE_MAX
} APAC_CFG_STATE_ENUM;

typedef struct {     // 50 hrs elapse time is determined by Current - Previous
  UINT32 prev_s;     // Eng Hrs Cycle Count from last Manual Validation (in sec)
  UINT32 curr_s;     // Current Eng Hrs Cycle Count (in sec)
  UINT32 start_s;    // Eng Hrs Cycle Count at Start of APAC.  If VLD run, this
                     //    will be used as the next "prev". "Prev" does not include
                     //    current test Eng run time.
} APAC_ENG_HRS_STATUS;

typedef struct {
  BOOLEAN set;    // Manual validate has been flagged.  Flag stored in NVM until
                  //   clr when VLD commit sel.
  APAC_VLD_REASON_ENUM reason;
  UINT16 reason_summary;  // All Manual Validate Reasons since last Man Validate. 
                          //      Ref APAC_VLD_BIT_ENCODE_CONST[]
} APAC_ENG_VLD_STATUS;

typedef struct {
  FLOAT64 avgVal;  // Average (over sample period) ITT or Ng used for calculation
  FLOAT64 margin;  // Calculated Margin ITT or Ng
  FLOAT64 max;     // Calculated Max ITT or Ng
  FLOAT64 c0;      // Calc c0
  FLOAT64 c1;      // Calc c1
  FLOAT64 c2;      // Calc c2
  FLOAT64 c3;      // Calc c3
  FLOAT64 c4;      // Calc c4
  FLOAT32 cfgOffset;  // Offset from cfg file
} APAC_ENG_CALC_DATA, *APAC_ENG_CALC_DATA_PTR;

typedef struct { // Calc common data used for both ITT and NG Calc
  FLOAT64 avgOAT;   // Average (over sample period) OAT value
  FLOAT64 avgBaroPres;  // Average (over sample period) Baro Press from Bus
  FLOAT64 avgTQ;    // Average (over sample period) TQ from bus
  FLOAT64 valBaroCorr;  // Value of Baro Correction Factor from Bus
  FLOAT64 convBaroCorr; // Converted Baro Correction from mBar to Ft
  FLOAT64 calcPALT;  // Calculated Pressure Altitude
  FLOAT64 coeffPALT; // Calculated Pressure Altitude Coefficient
  FLOAT64 tqCorr;    // Calculated TQ Corrected Value
} APAC_ENG_CALC_COMMON, *APAC_ENG_CALC_COMMON_PTR;

typedef enum {
  APAC_ENG_CYC_CHK_IDLE, // Engine is off
  APAC_ENG_CYC_CHK_MONITOR, // Check cycle is counting
  APAC_ENG_CYC_CHK_FAULT, // If faulted, wait till Eng is Off
  APAC_ENG_CYC_CHK_MAX
} APAC_ENG_CYC_CHK_ENUM;
#define APAC_ENG_CYC_CHK_FAIL_CNT  3  // # time cycle cnt not incr for failure. Actual=3sec

typedef struct {
  APAC_ENG_CYC_CHK_ENUM state;
  UINT32 prev_cyc_cnt; // Prev cycle count
  UINT16 fail_cnt;     // # time cycle cnt not incr
} APAC_ENG_CYC_CHK, *APAC_ENG_CYC_CHK_PTR; 

typedef struct {
  APAC_ENG_HRS_STATUS hrs;
  CHAR   esn[APAC_ESN_MAX_LEN];// Eng Serial Number.  "XX0000" indicate initial value
  APAC_COMMIT_ENUM commit;     // RJCT, ACPT, INVLD, VLD, ABORT, NCR for last successfully
                               //   completed test.  Default is NCR.
  APAC_ENG_VLD_STATUS vld;     // Manual validate status

  APAC_ENG_CALC_COMMON common;

  APAC_ENG_CALC_DATA itt;
  APAC_ENG_CALC_DATA ng;
  APAC_ENG_CYC_CHK cyc_chk; 
  ENGRUN_INDEX engineRunIndex; 
} APAC_ENG_STATUS, *APAC_ENG_STATUS_PTR;

typedef struct {
  // Eng1 or 2
    // Eng Hr Runtime since last Manual Validation ?
    // Trig Status for ER -> NOT NEEDED, IF USING TRIG
    // Trig Status for TqCrit and GndCrit -> NOT NEEDED, IF USING TRIG
    // Eng SN ("XX0000" indicate no SN received)
  // Current Status INLET CFG (update once on power up,
  //   and on every query) vs Actual -> NOT NEEDED
  APAC_ENG_STATUS eng[APAC_ENG_MAX];
  APAC_STATE_ENUM state;
  APAC_RUN_STATE_ENUM run_state;
  APAC_ENG_ENUM eng_uut;
  APAC_NR_SEL_ENUM nr_sel;
  UINT32 timeout_ms;    // General Purpose timeout
  UINT32 *menu_invld_timeout_val_ptr_ms;   // Ptr to Menu "Invalid" delay timeout value
  TIMESTAMP ts_start;   // TimeStamp of Start of Test (link to APAC START LOG and
                        //    APAC SUMMARY LOG
  APAC_CFG_STATE_ENUM cfg_state;
  CHAR cfg_err[APAC_CFG_CHECK_MSG_SIZE];
} APAC_STATUS, *APAC_STATUS_PTR;



typedef struct {
  FLOAT64 c0;
  FLOAT64 c1;
  FLOAT64 c2;
  FLOAT64 c3;
  FLOAT64 c4;
  FLOAT32 oat;
  FLOAT32 tq_min; // Min Range for TqDelta for Ng for this oat value entry, ref tables
  FLOAT32 tq_max; // Max Range for TqDelta for Ng for this oat value entry, ref tables
} APAC_TBL_ENTRY, *APAC_TBL_ENTRY_PTR;

#define APAC_INLET_CHART_MAX_LEN   6  //  5 + '\0'
#define APAC_INLET_CHART_PART_LEN  40 // 39 + '\0' 
#define APAC_INLET_CHART_REV_LEN   2  //  1 + '\0'
#define APAC_INLET_CHART_REV_1ST_CHAR  0 
#define APAC_INLET_CHART_NR_LEN    4  //  3 + '\0'
typedef struct {
  APAC_TBL_ENTRY p[APAC_TBL_MAX_ENTRIES];
  FLOAT32 param_min;  // ITTmax or Ngmax min acceptable value, reject if lower
  FLOAT32 param_max;  // ITTmax or Ngmax max acceptable value, reject if higher
  //FLOAT32 tq_rjct_min;  // TqDelta min acceptable value, reject if lower
  //FLOAT32 tq_rjct_max;  // TqDelta max acceptable value, reject if higher
  BOOLEAN minEq;  // "tq_min" value is ">=" when TRUE.  "tq_max" value is "<=" when FALSE
  UINT8 numEntries;   // Number of entries in the table
  CHAR inlet_str[APAC_INLET_CHART_MAX_LEN];  // "BASIC", "EAPS ", "IBF  "
  CHAR chart_partnum_str[APAC_INLET_CHART_PART_LEN]; //"ICN-39-A-155000-A-A0126-12214-A-03-1"
  CHAR chart_rev_str[APAC_INLET_CHART_REV_LEN];  // "F", "B", "C"
  CHAR chart_nr_str[APAC_INLET_CHART_NR_LEN];    // "102" or "100" 
} APAC_TBL, *APAC_TBL_PTR;

typedef struct {
  FLOAT32 val;
  FLOAT32 coeff;
} APAC_PALT_COEFF_ENTRY, *APAC_PALT_COEFF_ENTRY_PTR;

#define APAC_PALT_COEFF_ENTRY_MAX 20
typedef struct {
  APAC_PALT_COEFF_ENTRY p[APAC_PALT_COEFF_ENTRY_MAX];
  UINT16 numEntries;
} APAC_PALT_COEFF_TBL, *APAC_PALT_COEFF_TBL_PTR;



#define APAC_ERRMSG_CHAR_MAX  APAC_MENU_DISPLAY_CHAR_MAX   // 12 CHAR + '\0'
typedef struct {
  CHAR entry[APAC_ERRMSG_CHAR_MAX];
} APAC_ERRMSG_ENTRY, *APAC_ERRMSG_ENTRY_PTR;

#pragma pack(1)
#define APAC_ERRMSG_LIST_MAX  16  // Worst case 15 Trend Sensor Stability Sensor Defined
typedef struct {
  APAC_ERRMSG_ENTRY title;
  APAC_ERRMSG_ENTRY list[APAC_ERRMSG_LIST_MAX];
  UINT16 num;   // Number of entries in list
  UINT16 idx;   // Index into list
} APAC_ERRMSG_DISPLAY, *APAC_ERRMSG_DISPLAY_PTR;
#pragma pack()


typedef struct {
  CHAR str[APAC_ERRMSG_CHAR_MAX];
} APAC_STATE_RUN_STR, *APAC_STATE_RUN_STR_PTR;

typedef struct {
  APAC_STATE_RUN_STR entry[APAC_RUN_STATE_MAX];
} APAC_STATE_RUN_STR_BUFF, *APAC_STATE_RUN_STR_BUFF_PTR;


// If this enum changes then APAC_SNSR_NAMES_CFG must also be updated
typedef enum {
  APAC_SNSR_BARO_CORR = 0,   // 0
  APAC_SNSR_PALT_CALC,       // 1
  APAC_SNSR_TQ,              // 2
  APAC_SNSR_OAT,             // 3
  APAC_SNSR_ITT,             // 4
  APAC_SNSR_TQCORR,          // 5
  APAC_SNSR_ITTMAX,          // 6
  APAC_SNSR_NGMAX,           // 7
  APAC_SNSR_TQCRIT,          // 8
  APAC_SNSR_GNDCRIT,         // 9
  APAC_SNSR_ITTMG,           // 10
  APAC_SNSR_NGMG,            // 11
  APAC_SNSR_MAX              // 12
} APAC_SNSR_ENUM;

typedef struct {
  BOOLEAN bNotOk;                 // Item / Object is Not Ok if TRUE
  CHAR str[APAC_ERRMSG_CHAR_MAX]; // Display Msg Name
} APAC_SNSR_STATUS, *APAC_SNSR_STATUS_PTR;

typedef struct {
  CHAR chart[APAC_MENU_DISPLAY_CHAR_MAX];
  CHAR issue[APAC_MENU_DISPLAY_CHAR_MAX];
} APAC_INLET_CFG_STR, *APAC_AC_CFG_STR_PTR;

// Need NVM status command


#pragma pack(1)
typedef struct {
  APAC_ENG_ENUM eng_uut;      // Eng 1 or Eng 2
  ENGRUN_INDEX engineRunIndex;// Engine Run Cfg Index [0..3,255]  
  CHAR esn[APAC_ESN_MAX_LEN]; // ESN from data bus
  APAC_NR_SEL_ENUM nr_sel;    // 100% or 102% nr sel
  APAC_INLET_CFG_ENUM inletCfg; // Inlet selection from cfg
  BOOLEAN manual;               // Manual Verification
  APAC_VLD_REASON_ENUM reason;  // Manual Verification Reason
  APAC_COMMIT_ENUM commit;    // RJCT,ACPT,INVLD,VLD,ABORT,NCR
  UINT32 engHrs_curr_s;       // Current Eng Hrs Cycle time
  UINT32 engHrs_prev_s;       // Previous (last manual validation) Eng Hrs Cycle Time
  FLOAT64 ittMargin;
  FLOAT64 ittMax;
  FLOAT64 ittAvg;
  FLOAT64 ngMargin;
  FLOAT64 ngMax;
  FLOAT64 ngAvg;
  APAC_ENG_CALC_COMMON common;
  APAC_ENG_CALC_DATA itt;
  APAC_ENG_CALC_DATA ng;
  CHAR chart_rev_str[APAC_INLET_CHART_REV_LEN];  // "F", "B", "C" 
  UINT16 reason_summary;   // All Manual Validate Reasons since last Man Validate. 
                           //      Ref APAC_VLD_BIT_ENCODE_CONST[]
} APAC_SUMMARY_LOG, *APAC_SUMMARY_LOG_PTR;

typedef struct {
  APAC_ENG_ENUM eng_uut;      // Eng 1 or Eng 2
  ENGRUN_INDEX engineRunIndex;// Engine Run Cfg Index [0..3,255]    
  CHAR esn[APAC_ESN_MAX_LEN]; // ESN from data bus
  APAC_NR_SEL_ENUM nr_sel;    // 100% or 102% nr sel
  APAC_INLET_CFG_ENUM inletCfg;  // Inlet selection from cfg
  UINT32 engHrs_curr_s;       // Current Eng Hrs Cycle time
  UINT32 engHrs_prev_s;       // Previous (last manual validation) Eng Hrs Cycle Time
} APAC_ENG_START_LOG, *APAC_ENG_START_LOG_PTR;

typedef struct {
  APAC_COMMIT_ENUM commit;     // ABORT,NCR only
  APAC_ENG_ENUM eng_uut;       // Engine Under Test.  If APAC_ENG_MAX then Eng not selected
  ENGRUN_INDEX engineRunIndex; // Engine Run Cfg Index [0..3,255]  
  APAC_VLD_REASON_ENUM reason; // Pending Manual Validate Reason (if any and if eng selected)
  UINT16 reason_summary;       // All Manual Validate Reasons since last Man Validate. 
                               //      Ref APAC_VLD_BIT_ENCODE_CONST[]
} APAC_ABORT_NCR_LOG, *APAC_ABORT_NCR_LOG_PTR;

typedef struct {
  APAC_ENG_ENUM eng_uut;      // Eng 1 or Eng 2  or Eng_Max (if unknown)
  ENGRUN_INDEX engineRunIndex;// Engine Run Cfg Index [0..3,255]  
  APAC_NR_SEL_ENUM nr_sel;    // 100% or 102% nr sel
  APAC_INLET_CFG_ENUM inletCfg; // Inlet selection from cfg  
  APAC_ERRMSG_DISPLAY data;
} APAC_FAILURE_LOG, *APAC_FAILURE_LOG_PTR;

#define APAC_VLD_REASON_LOG_STR_MAX  48  // "curr=XXXXXXXXXX,prev=YYYYYYYYYY"
typedef struct {
  APAC_VLD_REASON_ENUM reason;
  CHAR msg[APAC_VLD_REASON_LOG_STR_MAX];
  UINT16 reason_summary;      // BIT encoding of all previous Manual Validate requests. 
                              //    Ref APAC_VLD_BIT_ENCODE_CONST[] 
} APAC_VLD_LOG, *APAC_VLD_LOG_PTR;
#pragma pack()

#pragma pack(1)
#define APAC_HIST_BLANK    0UL    // ->ts field value
typedef struct{
  TIMESTAMP ts;
  FLOAT32 ittMargin;
  FLOAT32 ngMargin;
  UINT8 flags;   // Encodes Cat A, Store Reason, Eng Pos
                 // bit 0: Eng Pos '0' = Eng 1, '1' = Eng 2
                 // bit 3-2: '000' - Reject  '001' - Accept
                 //          '010' - Invalid '011' - Valid
                 //          '100' - Abort   '101' - NCR
                 // bit 4: Cat A or Basic (102% or 100%)
                 //               '0' = Basic, '1' = A
} APAC_HIST_ENTRY, *APAC_HIST_ENTRY_PTR;

#define APAC_HIST_PARSE_ENG( x ) (x & 0x1)
#define APAC_HIST_PARSE_ACT( x ) ((x >> 1) & 0x7)
#define APAC_HIST_PARSE_CAT( x ) ((x >> 4) & 0x1)
#define APAC_HIST_ENCODE( eng, act, cat) ((cat <<4) | (act << 1) | (eng))
// Parse the Day, Month, Year field.  Ref Altair TS encoding
#define APAC_HIST_PARSE_MON_DAY( x ) ( x & 0x001FF000 )
#define APAC_HIST_PARSE_YR( x ) ((( x & 0xFF700000 ) >> 21) / 24)

typedef struct {
  CHAR esn[APAC_ESN_MAX_LEN];
  UINT32 hrs_s;  // Prev Eng Cyc Hrs since last Manual PAC Validation
                 //   50 hrs exceed = Curr Eng Hrs - Prev Eng Hrs
  BOOLEAN vld;                     // Manual Validate is flagged
  APAC_VLD_REASON_ENUM vld_reason; // Manual Validate Reason
  UINT16 vld_reason_summary;       // All Manual Validate Reasons since last Man Validate. 
                                   //      Ref APAC_VLD_BIT_ENCODE_CONST[]
} APAC_DATA_ENG_NVM, *APAC_DATA_ENG_NVM_PTR;

typedef struct {
  APAC_DATA_ENG_NVM eng[APAC_ENG_MAX];
  INT32 cfgCRC;
} APAC_DATA_NVM, *APAC_DATA_NVM_PTR;

#define APAC_HIST_ENTRY_MAX 100
typedef struct {
  APAC_HIST_ENTRY entry[APAC_HIST_ENTRY_MAX];
} APAC_HIST, *APAC_HIST_PTR;
// APAC_HIST size is 1514 + 8
// APAC_HIST size is 1500 + 22 -> 1522
#pragma pack()

typedef struct {
  UINT16 idxDate;  // Current index pt to Date Entry
  UINT16 idxDay;   // Current index pt to Day Entry
  UINT16 idxDayBegin;  // Index pt to current Day Block Begin
  UINT16 idxDayEnd;    // Index pt to current Day Block End
  UINT16 num;      // Number of entries
} APAC_HIST_STATUS, *APAC_HIST_STATUS_PTR;
#define APAC_HIST_NONE  "NONE "
#define APAC_HIST_UTC   "UTC"

#ifdef APAC_TEST_DBG
typedef struct {
  APAC_ENG_CALC_COMMON common;
  APAC_ENG_CALC_DATA itt;
  APAC_ENG_CALC_DATA ng;
} APAC_DEBUG_DATA, *APAC_DEBUG_DATA_PTR;

typedef struct {
  APAC_DEBUG_DATA data;
#ifdef APAC_TEST_SIM
  BOOLEAN simTrend;
#endif
} APAC_DEBUG, *APAC_DEBUG_PTR;
#endif

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( APAC_MGR_BODY )
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
EXPORT void APACMgr_Initialize ( void );
EXPORT BOOLEAN APACMgr_FileInitHist( void );
EXPORT BOOLEAN APACMgr_FileInitNVM( void );
EXPORT BOOLEAN APACMgr_FSMGetState( INT32 param ); 

#endif // APAC_MGR_H


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: APACMgr.h $
 * 
 * *****************  Version 16  *****************
 * User: John Omalley Date: 3/29/16    Time: 8:17a
 * Updated in $/software/control processor/code/application
 * SCR 1320 Item #10
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 3/26/16    Time: 11:43p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update FLOAT to FLOAT64.
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 3/22/16    Time: 6:17p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update FLOAT to FLOAT64.
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 3/11/16    Time: 7:15p
 * Updated in $/software/control processor/code/application
 * SCR #1317 Item #7.  Update APAC ITT/Ng Margin and Max value from
 * FLOAT32 to FLOAT64.
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 3/11/16    Time: 1:59p
 * Updated in $/software/control processor/code/application
 * SCR #1320 Item #5 Add IT/Ng Cfg Offset Adj to Summary log and GSE
 * Status
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 3/10/16    Time: 6:56p
 * Updated in $/software/control processor/code/application
 * SCR #1320 Items #1,#2,#4 APAC Processing Updates
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 2/02/16    Time: 9:43a
 * Updated in $/software/control processor/code/application
 * SCR #1308 Item #19 and #20.  Check in Cycle CBIT again.  ReOrder
 * History back to most recent when RCL selected.  
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 1/19/16    Time: 11:37a
 * Updated in $/software/control processor/code/application
 * SCR #1309 Item #3.  Remove 'apac.cfg.snsr.gndspd'  as not needed.
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 1/18/16    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR #1308. Various Updates from Reviews and Enhancements
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 11/17/15   Time: 9:27a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Code Review Updates.
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/11/15   Time: 11:48a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Add ENG ID to Failure Log
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/10/15   Time: 11:30a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing. Additional Updates from GSE/LOG Review AI.
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/05/15   Time: 6:45p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC, ESN updates
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 15-10-23   Time: 4:54p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing Additional Updates
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

