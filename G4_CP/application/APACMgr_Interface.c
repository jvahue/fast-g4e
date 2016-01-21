#define APAC_MGR_INTERFACE_BODY
/******************************************************************************
            Copyright (C) 2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9E991

    File:        APACMgr_Interface.c

    Description: Contains all functions and data related to the APAC Mgr
                 Interface Function and the Menu Processing

    VERSION
      $Revision: 10 $  $Date: 1/18/16 5:49p $

******************************************************************************/
#ifndef APAC_MGR_BODY
#error APACMgr_Interface.c should only be included by APACMgr.c
#endif


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "APACMgr_Interface.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#ifdef APAC_TEST_SIM
  #define APACMgr_IF_DebugStr( DbgLevel, Timestamp, ...)  \
              { snprintf ( gse_OutLine1, sizeof(gse_OutLine1), __VA_ARGS__ ); \
                GSE_DebugStr( DbgLevel, Timestamp, gse_OutLine1); }
#else
    #define APACMgr_IF_DebugStr( DbgLevel, Timestamp, ...)
#endif


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static CHAR gse_OutLine1[128];

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/


/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    APACMgr_IF_Config
 *
 * Description: Returns the current FAST APAC Configuration
 *
 * Parameters:  *msg1 - "BASIC", "EAPS", "IBF"
 *              *msg2 - "F", "B", "C"
 *              *msg3 - "102", "100"
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5062 In Screen M5 the aircraft configuration, containing the enumerated values
 *           of "BASIC", "EAPS" and "IBF", taken from the configuration file shall be
 *           displayed for the crew to confirm.
 *  SRS-5063 In Screen M5, the CHART ISSUE letter, stored in the CP firmware shall
 *           be selected based upon the aircraft configuration.
 *  SRS-5064 Default value of Chart Part / version letter for BASIC aircraft configuration
 *           is 139G0040A001 / ISSUE F and shall be displayed as 'ISSUE F'
 *  SRS-5065 Default value of Chart Part / version letter for EAPS aircraft configuration
 *           is 139G1580A001 / ISSUE B and shall be displayed as 'ISSUE B'
 *  SRS-5066 Default value of Chart Part / version letter for IBF aircraft configuration
 *           is 139G1580A021 / ISSUE A and shall be displayed as 'ISSUE A'
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_Config ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                            APAC_IF_ENUM option )
{
  APAC_TBL_PTR tbl_ptr;
  APAC_INSTALL_ENUM tblIdx;
  
  // Clear APAC status
  APACMgr_ClearStatus();
  
  // Read from NR102_100 sensor to determine 102 or 100. Ignore SensorIsValid() for disc?
  m_APAC_Status.nr_sel = (SensorGetValue(m_APAC_Cfg.snsr.idxNR102_100) == APAC_NR102_SET) ?
                         APAC_NR_SEL_102 : APAC_NR_SEL_100 ;
  
  // from APAC_TBL m_APAC_Tbl[APAC_PARAM_MARGIN_MAX][APAC_INSTALL_MAX]
  //   get chart inlet, chart issue and chart nr strings
  // NOTE:  Use ITT table as the NG table has same data for these fields
  tblIdx = m_APAC_Tbl_Mapping[m_APAC_Status.nr_sel][m_APAC_Cfg.inletCfg];
  tbl_ptr = (APAC_TBL_PTR) &m_APAC_Tbl[APAC_ITT][tblIdx];  
 
  memcpy ( (void *) msg1, tbl_ptr->inlet_str, APAC_MENU_DISPLAY_CHAR_MAX);
  memcpy ( (void *) msg2, tbl_ptr->chart_rev_str, APAC_MENU_DISPLAY_CHAR_MAX);
  memcpy ( (void *) msg3, tbl_ptr->chart_nr_str, APAC_MENU_DISPLAY_CHAR_MAX);

  m_APAC_Status.state = APAC_STATE_TEST_INPROGRESS;

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Config(m1=%s,m2=%s,m3=%s)\r\n",msg1,msg2,msg3);

  return (TRUE);
}

/******************************************************************************
 * Function:    APACMgr_IF_Select
 *
 * Description: Selects APAC Testing of 100% Nr Setting
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - APAC_IF_NR100, APAC_IF_NR102
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5144 Upon the crew member pressing the Enter (·) pushbutton switch while
 *           in Screen M29, the APAC System shall identify APAC power level is
 *           "NORMAL" (100%Nr) for trend logic
 *  SRS-5147 Upon the crew member pressing the Enter (·) pushbutton switch while
 *           in Screen M30, the APAC System shall identify APAC power level is
 *           "CAT-A" (102%Nr) for trend logic
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_Select ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                            APAC_IF_ENUM option )
{
/*  
  ASSERT ( (option == APAC_IF_NR100) || (option == APAC_IF_NR102) );
  // run_state is either APAC_RUN_STATE_IDLE for 1st time or successful prev test
  //                     APAC_RUN_STATE_FAULT for failed prev test
  ASSERT ( (m_APAC_Status.run_state == APAC_RUN_STATE_IDLE ) ||
           (m_APAC_Status.run_state == APAC_RUN_STATE_FAULT )  );

  m_APAC_Status.nr_sel = (option == APAC_IF_NR100) ? APAC_NR_SEL_100 : APAC_NR_SEL_102;

  m_APAC_Status.run_state = APAC_RUN_STATE_IDLE;  // Set or clear run state back to _IDLE

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Select(o=%d)-cat='%c'\r\n",
                   option, APAC_CAT_STR[m_APAC_Status.nr_sel]);
*/
  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_Setup
 *
 * Description: Checks intial setup before beginning PAC test
 *
 * Parameters:  *msg1 - Not Used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE if setup is OK
 *              FALSE if setup if Not OK
 *
 * Notes:
 *  SRS-5072 From the values of EEC#Flight and EEC#Idle, where one engine is idle and
 *           the other is in Flight mode, the APAC Engine under test shall be determined
 *           by which of the two is in Flight Mode.
 *  SRS-5074 If both engines have EEC#Flight set to TRUE or FALSE, or if both engines have
 *           EEC#Idle set to TRUE or FALSE, the APAC Configuration shall be declared to be
 *           Invalid "ENGMD" and provided to display processing for M18.
 *  SRS-5075 If the aircraft is configured for EAPS and the EAPS switch for the Engine under
 *           test is FALSE, the APAC Configuration shall be declared to be Invalid "EAPS" and
 *           provided to display processing for M18.
 *  SRS-5076 If the aircraft is configured for IBF and the EAPS switch for the Engine under
 *           test is TRUE, the APAC Configuration shall be declared to be Invalid "IBF" and
 *           provided to display processing for M18.
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_Setup ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                           APAC_IF_ENUM option )
{
  BOOLEAN ok, eng1, eng2, eaps_ibf_1, eaps_ibf_2;

  ASSERT ( m_APAC_Status.state == APAC_STATE_TEST_INPROGRESS );
  ASSERT ( m_APAC_Status.run_state == APAC_RUN_STATE_IDLE );

  // Check that each eng sensor is valid
  eng1 = SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_1].idxEngFlt) &&
         SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_1].idxEngIdle);
  eng2 = SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_2].idxEngFlt) &&
         SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_2].idxEngIdle) &&
         eng1;  // If eng1 is invalid then force eng2 to be invalid,so that
                // ENGMD err will be displayed

  // Check which eng is Flt and Not Idle, only when both eng snsrs are valid
  if (eng1 && eng2)
  {
    eng1 = (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_1].idxEngFlt) == APAC_ENG_FLT_SET) &&
           (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_1].idxEngIdle) != APAC_ENG_IDLE_SET);

    eng2 = (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_2].idxEngFlt) == APAC_ENG_FLT_SET) &&
           (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_2].idxEngIdle) != APAC_ENG_IDLE_SET);
  }

  // Check the eaps_ibf are in the correct state(s)
  eaps_ibf_1 = (SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_1].idxEAPS_IBF_sw) &&
               (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_1].idxEAPS_IBF_sw) ==
                APAC_EAPS_IBF_SET));

  eaps_ibf_2 = (SensorIsValid(m_APAC_Cfg.eng[APAC_ENG_2].idxEAPS_IBF_sw) &&
               (SensorGetValue(m_APAC_Cfg.eng[APAC_ENG_2].idxEAPS_IBF_sw) ==
                APAC_EAPS_IBF_SET));

  // Note: If IBF is set, IBF switch s/b "0" or not set to be proper config
  eaps_ibf_1 = (m_APAC_Cfg.inletCfg == INLET_CFG_BASIC) ||
               ((m_APAC_Cfg.inletCfg == INLET_CFG_EAPS) && (eaps_ibf_1)) ||
               ((m_APAC_Cfg.inletCfg == INLET_CFG_IBF) && (!eaps_ibf_1));

  eaps_ibf_2 = (m_APAC_Cfg.inletCfg == INLET_CFG_BASIC) ||
               ((m_APAC_Cfg.inletCfg == INLET_CFG_EAPS) && (eaps_ibf_2)) ||
               ((m_APAC_Cfg.inletCfg == INLET_CFG_IBF) && (!eaps_ibf_2));

  ok = ( (eng1 && eaps_ibf_1) && (!eng2) ) || ( (eng2 && eaps_ibf_2) && (!eng1) );

  if (!ok) {
    APACMgr_errMsg_Setup ( eng1, eng2, eaps_ibf_1, eaps_ibf_2 );
    m_APAC_Status.run_state = APAC_RUN_STATE_FAULT;
    APACMgr_LogFailure();

    APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Setup(m1=%s,m2=%s)\r\n",
                        m_APAC_ErrMsg.list[0].entry,
                        m_APAC_ErrMsg.list[1].entry);
  }
  else {
    m_APAC_Status.eng_uut = (eng1) ? APAC_ENG_1 : APAC_ENG_2;
    APACMgr_LogEngStart();

    APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Setup( )-eng=%c\r\n",
                     APAC_ENG_POS_STR[m_APAC_Status.eng_uut]);
  }

  return (ok);
}


/******************************************************************************
 * Function:    APACMgr_IF_ErrMsg
 *
 * Description: Returns an ErrMsg for M18,  M17,M16,M15
 *
 * Parameters:  *msg1 - ErrTitle (ref Notes below)
 *              *msg2 - ErrItem  (ref Notes below)
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE always
 *
 * Notes:
 *   M18 ErrTitle - "CONFIG ERROR"
 *       ErrItem  - "ENGMD", "EAPS", "IBF"
 *                  scrolling at INVALID_BUTTON_TIME rate multiple Errors
 *                  ("EAPS" or "IBF" can be in Error inaddition to "ENGMD")
 *
 *   M17 ErrTitle - "PAC NO START"
 *       ErrItem  - "XXXXX","YYYYY"
 *                  "XXXXX" - Cfg TQ param name when TQ cond not met
 *                  "YYYYY" - Cfg GS param name when GS cond not met
 *                  scrolling at INVALID_BUTTON_TIME rate multiple Errors
 *
 *   M16 ErrTitle - "PAC NO STABL"
 *       ErrItem  - "XXXXX","YYYYY","ZZZZZ",etc..
 *                  "XXXXX" - Cfg Param Stable 1 name (ITT), when not stable
 *                  "YYYYY" - Cfg Param Stable 2 name (Ng ), when not stable
 *                  "ZZZZZ" - Cfg Param Stable 3 name (OAT), when not stable
 *                  scrolling at INVALID_BUTTON_TIME rate multiple Errors
 *
 *   M15 ErrTitle - "COMPUTE ERR"
 *       ErrItem  - "PALT","ITTMX","NGMX ","TQCOR","OAT  ","ITTMG","NGMG "
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_ErrMsg ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                          APAC_IF_ENUM option )
{
  UINT32 tick;
  BOOLEAN bUpdate = FALSE;

  ASSERT ( m_APAC_Status.run_state == APAC_RUN_STATE_FAULT );
  ASSERT ( m_APAC_Status.menu_invld_timeout_val_ptr_ms != NULL );

  tick = CM_GetTickCount();

  if (tick >= m_APAC_Status.timeout_ms)
  {
    m_APAC_ErrMsg.idx = (m_APAC_ErrMsg.num != 0) ?
                        ((++m_APAC_ErrMsg.idx) % m_APAC_ErrMsg.num) : 0;

    m_APAC_Status.timeout_ms = tick + *m_APAC_Status.menu_invld_timeout_val_ptr_ms;
    bUpdate = TRUE;
  }

  memcpy ( (void *) msg1, (void *) m_APAC_ErrMsg.title.entry, APAC_ERRMSG_CHAR_MAX );

  memcpy ( (void *) msg2, (void *) m_APAC_ErrMsg.list[m_APAC_ErrMsg.idx].entry,
           APAC_ERRMSG_CHAR_MAX );

  if (bUpdate == TRUE ) {
    APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _ErrMsg(m1=%s,m2=%s)\r\n", msg1, msg2);
  }

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_ValidateManual
 *
 * Description: Returns indication if 50 Engine Run Hrs has transpired since
 *              last Manual PAC Validation
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE if Manual PAC Validation is required (>50 hrs since last)
 *              FALSE if Manual PAC Validation is not required (<50 hrs since last)
 *
 * Notes:
 *  SRS-5088  Following the sequence of APAC Execution Menu Tree, if the PAC configuration
 *            is determined to be valid and if the APAC_ERTime (Engine run time since last
 *            validation APAC calculation per engine) exceeds MaxAPAC, the APAC system shall
 *            display Screen M23, and identify the APAC run as a Validation PAC.
 *
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_ValidateManual ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                    APAC_IF_ENUM option )
{
  BOOLEAN ok;
  APAC_ENG_STATUS_PTR eng_ptr;
  UINT32 eng_hrs;

  ASSERT ( m_APAC_Status.eng_uut != APAC_ENG_MAX );
  ASSERT ( (m_APAC_Status.run_state == APAC_RUN_STATE_IDLE) ||
           (m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT) );

  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[m_APAC_Status.eng_uut];
  eng_hrs = (m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT) ? eng_ptr->hrs.start_s :
                                                                 eng_ptr->hrs.curr_s;
  if ((m_APAC_Status.run_state != APAC_RUN_STATE_COMMIT) &&
      ((eng_hrs - eng_ptr->hrs.prev_s) >= APAC_ENG_HRS_VALIDATE_SEC))
  { // Set Manual APAC Validate Flag only on start of APAC Run test
    snprintf ( m_APAC_VLD_Log.msg, sizeof(m_APAC_VLD_Log.msg), "E%d: curr=%d,prev=%d",
               m_APAC_Status.eng_uut, eng_hrs, eng_ptr->hrs.prev_s );
    APACMgr_NVMUpdate ( m_APAC_Status.eng_uut, APAC_VLD_REASON_50HRS, &m_APAC_VLD_Log );
  }

  ok = eng_ptr->vld.set;  // NOTE: validate could have been flagged previously

  eng_ptr->hrs.start_s = eng_hrs; // Update Eng Run Time on start of test

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _ValidateManual(r=%d)-delta_hrs_s=%d,rea=%s\r\n",
                      ok,(eng_hrs - eng_ptr->hrs.prev_s),
                      APAC_VLD_STR_CONST[eng_ptr->vld.reason]);

  return (ok);
}


/******************************************************************************
 * Function:    APACMgr_IF_RunningPAC
 *
 * Description: Returns Current Status of the APAC test in progress
 *
 * Parameters:  *msg1 - "NOT LOW", "STABILIZING", "SAMPLING", "COMPUTING"
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE if PAC Test has completed (either successful or not)
 *              FALSE if PACT Test Inprogress
 *
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_RunningPAC ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                APAC_IF_ENUM option )
{
  BOOLEAN ok;
  APAC_RUN_STATE_ENUM run_state;

  static APAC_RUN_STATE_ENUM prev_run_state = APAC_RUN_STATE_MAX;

  ok = FALSE;

  ASSERT (m_APAC_Status.state == APAC_STATE_TEST_INPROGRESS);

  // Kick start APAC Run Testing if test is currently not running.
  if ( m_APAC_Status.run_state == APAC_RUN_STATE_IDLE )
  {
    m_APAC_Status.run_state = APAC_RUN_STATE_INCREASE_TQ;
    m_APAC_Status.timeout_ms  = CM_GetTickCount() +
                                CM_DELAY_IN_SECS(m_APAC_Cfg.timeout.hIGELow_sec);
  }

  // Return current state for display
  run_state = m_APAC_Status.run_state;
  memcpy ( (UINT8 *) msg1, (UINT8 *) &APAC_STATE_RUN_STR_BUFF_CONST[run_state],
           APAC_MENU_DISPLAY_CHAR_MAX );

  ok = ( (run_state == APAC_RUN_STATE_COMMIT) || (run_state == APAC_RUN_STATE_FAULT) ) ?
         TRUE : FALSE ;

  if (prev_run_state != run_state)
  {
    APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _RunningPAC(%s)\r\n", msg1);
    prev_run_state = run_state;
  }

  return (ok);
}

/******************************************************************************
 * Function:    APACMgr_IF_CompletedPAC
 *
 * Description: Returns Completed APAC status (success or not)
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - Not used
 *
 * Returns:     TRUE if PAC Test was successful
 *              FALSE if PACT Test was not successful
 *
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_CompletedPAC ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                  APAC_IF_ENUM option )
{
  BOOLEAN ok;

  ASSERT (m_APAC_Status.state == APAC_STATE_TEST_INPROGRESS);
  ASSERT ( (m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT) ||
           (m_APAC_Status.run_state == APAC_RUN_STATE_FAULT) );

  ok = (m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT) ? TRUE : FALSE;

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _CompletedPAC(r=%d)-rstate=%s\r\n",
                      ok, APAC_STATE_RUN_STR_BUFF_CONST[m_APAC_Status.run_state].str);

  return (ok);
}


/******************************************************************************
 * Function:    APACMgr_IF_Result
 *
 * Description: Returns the ITT and Ng Margin Results
 *
 * Parameters:  *msg1 - "A" or <blankd> for Cat-A or Normal
 *              *msg2 - Ng Margin  "-9.9" to "+9.9"
 *              *msg3 - ITT Margin "-99 to +99"
 *              *msg4 - "1" or "2" for Eng Pos
 *              option - Not used
 *
 * Returns:     TRUE always
 *
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_Result ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                            APAC_IF_ENUM option )
{
  APAC_ENG_STATUS_PTR eng_ptr;

  ASSERT ( m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT );

  eng_ptr = (APAC_ENG_STATUS_PTR) &m_APAC_Status.eng[m_APAC_Status.eng_uut];

  memset ( (void *) msg1, 0, APAC_MENU_DISPLAY_CHAR_MAX );
  *msg1 = APAC_CAT_STR[m_APAC_Status.nr_sel];

  snprintf ( msg2, APAC_MENU_DISPLAY_CHAR_MAX, "%+1.1f", eng_ptr->ng.margin );
  snprintf ( msg3, APAC_MENU_DISPLAY_CHAR_MAX, "%+03d",  (INT32) eng_ptr->itt.margin );

  memset ( (void *) msg4, 0, APAC_MENU_DISPLAY_CHAR_MAX );
  *msg4 = APAC_ENG_POS_STR[m_APAC_Status.eng_uut];

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Result (m1=%s,m2=%s,m3=%s,m4=%s)\r\n",
               msg1, msg2, msg3, msg4 );

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_ResultCommit
 *
 * Description: Marks the APAC result with "Reject"
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - APAC_IF_RJCT, APAC_IF_ACPT, APAC_IF_INVLD, APAC_IF_VLD
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5111 While the APAC display is displaying Screen M14 or Screen M13, if the
 *           crew member presses the Enter (·) pushbutton switch on the Display Unit,
 *           the APAC system shall record an ETM log containing the date, time and ITT,
 *           Ng margins as computed APAC values with appropriate status indicating "ACP"
 *           or "RJT" indicating the crew accepted or rejected these values.
 *  SRS-5110 While the APAC display is displaying Screen M14, if the crew member presses the
 *           Enter (·) pushbutton switch on the Display Unit, the APAC system shall store the
 *           date, time and ITT, Ng margins as computed APAC values with a "RJT" flag
 *           indicating the crew rejected these values.
 *  SRS-5108 While the APAC display is displaying Screen M13, if the crew member presses
 *           the Enter (·) pushbutton switch on the Display Unit, the APAC system shall
 *           store the date, time and ITT, Ng margins as computed APAC values with a
 *           ""ACP" flag indicating the crew accepted these values.
 *  SRS-5334 While the APAC display is displaying Screen M26 or Screen M27, if the crew
 *           member presses the Enter (·) pushbutton switch on the Display Unit, the APAC
 *           system shall record an ETM log containing the date, time and ITT, Ng margins as
 *           computed APAC values with appropriate status indicating "NVL" or "VLD" indicating
 *           the crew invalidated or validated these values.
 *  SRS-5103 While the APAC system is displaying Screen M27, if the crew member presses the
 *           Enter (·) pushbutton switch on the Display Unit, the APAC system shall store the
 *           date, time and ITT, Ng margins as computed APAC values with a "NVL" flag
 *           indicating the crew invalidated these values, to Nonvolatile memory.
 *  SRS-5104 While the APAC display is displaying Screen M26, if the crew member presses the
 *           Enter (·) pushbutton switch on the Display Unit, the APAC system shall store the
 *           date, time and ITT, Ng margins as computed APAC values with a "VLD" flag
 *           indicating the crew validated these values, to Nonvolatile memory.
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_ResultCommit ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                  APAC_IF_ENUM option )
{
  ASSERT ( (option == APAC_IF_RJCT) || (option == APAC_IF_ACPT) ||
           (option == APAC_IF_INVLD) || (option == APAC_IF_VLD) );
  ASSERT ( m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT );

  APACMgr_LogSummary ( (APAC_COMMIT_ENUM) (option - APAC_IF_RJCT) );
  m_APAC_Status.run_state = APAC_RUN_STATE_IDLE;

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _ResultCommit(o=%d)\r\n",option);

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_HistoryData
 *
 * Description: Returns the "current" Date in buffer.  Call to
 *              APACMgr_IF_HistoryAdv() moves the pointer.
 *
 * Parameters:  *msg1 - Date "APR/08/2015" format OR Time   "12:31 ACP E1"
 *              *msg2 - Time "HH:MM UTC"          OR Result "A +2.6% +15C"
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - APAC_IF_DATE, APAC_IF_DAY
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5113 Field Definitions for Screen M19 shall be as follows:
 *  SRS-5116 Display of dates shall be of the form mmm/dd/yyyy (where mmm is three
 *           letter abbreviation of the english month)
 *  SRS-5117 Display of times shall be of the form hh:mm of the FAST wall clock
 *           in 24 hour format.
 *  SRS-5125 Recalled PAC values for both engines shall be displayed in sequential recorded
 *           order group by date and time. Note: This is an enhancement from system
 *           requirement to cover the case where the clock is  bad for some period of time.
 *  SRS-5118 Each stored pair of margins computed (ITT and Ng) shall display
 *           whether it was Cat-A (Ng=102%) or <blank> indicating it was performed
 *           at 100%Ng
 *  SRS-5119 Each stored pair of margins computed (ITT and Ng) shall display the storage
 *           status having value of "ACP", "RJT", "VLD", "NVL" or "NCR".
 *  SRS-5342 Each stored pair of margins computed (ITT and Ng) shall display the engine the
 *           PAC was performed on as "E1" for engine 1 and "E2" for engine 2.
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_HistoryData ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                 APAC_IF_ENUM option )
{
  TIMESTAMP ts;
  TIMESTRUCT timeStruct;
  BOOLEAN blank = FALSE;
  UINT16 idx;
  APAC_ENG_ENUM eng;
  APAC_COMMIT_ENUM act;
  APAC_NR_SEL_ENUM nr_sel;


  ASSERT ( (option == APAC_IF_DATE) || (option == APAC_IF_DAY) );

  idx = (option == APAC_IF_DATE) ? m_APAC_Hist_Status.idxDate : m_APAC_Hist_Status.idxDay;
  ts = m_APAC_Hist.entry[idx].ts;
  if ( (ts.Timestamp == APAC_HIST_BLANK) && (ts.MSecond == APAC_HIST_BLANK) ) {
    CM_GetTimeAsTimestamp(&ts); // History list is empty, get current date time
    snprintf(msg2, APAC_MENU_DISPLAY_CHAR_MAX, "%s       ", APAC_HIST_NONE);
    blank = TRUE;
  }
  CM_ConvertTimeStamptoTimeStruct( &ts, &timeStruct );

  if ( option == APAC_IF_DATE )
  { // Return APAC_IF_DATE
    snprintf(msg1, APAC_MENU_DISPLAY_CHAR_MAX, "%s/%02d/%04d ",
             APAC_HIST_MONTH_STR_CONST[timeStruct.Month],
             timeStruct.Day, timeStruct.Year);

    if (blank == FALSE) {
      snprintf(msg2, APAC_MENU_DISPLAY_CHAR_MAX, "%02d:%02d %s   ",
               timeStruct.Hour, timeStruct.Minute, APAC_HIST_UTC);
    }
  }
  else
  { // Return APAC_IF_DAY
    snprintf(msg1, APAC_MENU_DISPLAY_CHAR_MAX, "%02d:%02d",
             timeStruct.Hour, timeStruct.Minute);
    if (blank == FALSE) {
      eng = (APAC_ENG_ENUM) APAC_HIST_PARSE_ENG(m_APAC_Hist.entry[idx].flags);
      act = (APAC_COMMIT_ENUM) APAC_HIST_PARSE_ACT(m_APAC_Hist.entry[idx].flags);
      nr_sel = (APAC_NR_SEL_ENUM) APAC_HIST_PARSE_CAT(m_APAC_Hist.entry[idx].flags);
      snprintf(msg1, APAC_MENU_DISPLAY_CHAR_MAX, "%s %s %s", msg1,
               APAC_HIST_ACT_STR_CONST[act],
               APAC_HIST_ENG_STR_CONST[eng]);
      snprintf(msg2, APAC_MENU_DISPLAY_CHAR_MAX, "%s %+1.1f%% %+03dC",
               APAC_HIST_CAT_STR_CONST[nr_sel],
               m_APAC_Hist.entry[idx].ngMargin,
               (INT32) m_APAC_Hist.entry[idx].ittMargin);
    } // end else APAC_IF_DAY
    else {
      snprintf(msg1, APAC_MENU_DISPLAY_CHAR_MAX, "%s       ", msg1);
    }
  } // end if APAC_IF_DATE

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _HistData(o=%d,m1=%s,m2=%s)\r\n", option,
                      msg1, msg2);

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_HistoryAdv
 *
 * Description: Moves the pointer to next entry in the Date Buffer.
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *               option - APAC_IF_DATE, APAC_IF_DAY
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5125 Recalled PAC values for both engines shall be displayed in sequential recorded
 *           order group by date and time. Note: This is an enhancement from system
 *           requirement to cover the case where the clock is  bad for some period of time.
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_HistoryAdv ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                APAC_IF_ENUM option )
{
  APAC_HIST_STATUS_PTR status_ptr;
  UINT16 idx, idxLast;
  APAC_HIST_ENTRY_PTR entry_ptr;
  UINT32 curr_mon_day, curr_yr;

  ASSERT ( (option == APAC_IF_DATE) || (option == APAC_IF_DAY) );

  status_ptr = (APAC_HIST_STATUS_PTR) &m_APAC_Hist_Status;
  if ( option == APAC_IF_DAY )
  {
    status_ptr->idxDay = (status_ptr->idxDay != status_ptr->idxDayEnd) ?
                         (status_ptr->idxDay + 1) : status_ptr->idxDayBegin;
  }
  else
  {
    // If Hist list has 0 or 1 entry or 1 Date Block Only, adv not necessary
    idxLast = status_ptr->num - 1;
    if ( !((status_ptr->num == 0) || (status_ptr->num == 1) ||
          ((status_ptr->idxDayBegin == 0) && (status_ptr->idxDayEnd == idxLast))))
    {
      // If idxDate is last entry in Hist List, wrap to beginning '0'
      idx = (status_ptr->idxDate != idxLast) ? (status_ptr->idxDate + 1) : 0;
      // If new idx is last entry in Hist List, then just one entry in next Date Blk
      if ( idx >= idxLast ) {
        status_ptr->idxDayBegin = idx;
      }
      else // have another Date Blk to find
      {
        entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[idx];
        curr_mon_day = APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp);
        curr_yr = APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp);
        while (idx <= idxLast)
        {
          entry_ptr++;
          // Compare to next entry, current entry
          if ( (curr_mon_day != APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp)) ||
               (curr_yr != APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp)) ||
               (idx == idxLast) )
          {
            status_ptr->idxDayBegin = ((status_ptr->idxDate + 1) > idxLast) ?
                                      0 : (status_ptr->idxDate + 1);
            break;
          }
          idx++; // point to next entry as this one has same Date
        } //loop until Date Changes
      } // else find next Date Blk
      status_ptr->idxDate = idx;
      status_ptr->idxDayEnd = idx;
      status_ptr->idxDay = idx;
    } // end if at least 2 entries with diff Date Blks
  } // end else option == APAC_IF_DATE inc

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _HistAdv(o=%d)\r\n", option);

  return (TRUE);
}


  /******************************************************************************
 * Function:    APACMgr_IF_HistoryDec
 *
 * Description: Moves the pointer to Prev entry in the Date Buffer.
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *               option - APAC_IF_DATE, APAC_IF_DAY
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5125 Recalled PAC values for both engines shall be displayed in sequential recorded
 *           order group by date and time. Note: This is an enhancement from system
 *           requirement to cover the case where the clock is  bad for some period of time.
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_HistoryDec ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                                APAC_IF_ENUM option )
{
  APAC_HIST_STATUS_PTR status_ptr;
  UINT16 idx, idxLast;
  APAC_HIST_ENTRY_PTR entry_ptr;
  UINT32 curr_mon_day, curr_yr;
  UINT16 cnt;

  ASSERT ( (option == APAC_IF_DATE) || (option == APAC_IF_DAY) );

  status_ptr = (APAC_HIST_STATUS_PTR) &m_APAC_Hist_Status;
  if ( option == APAC_IF_DAY )
  {
    status_ptr->idxDay = (status_ptr->idxDay != status_ptr->idxDayBegin) ?
                         (status_ptr->idxDay - 1) : status_ptr->idxDayEnd;
  }
  else
  {
    // If Hist list has 0 or 1 entry or 1 Date Block Only, adv not necessary
    idxLast = status_ptr->num - 1;
    if ( !((status_ptr->num == 0) || (status_ptr->num == 1) ||
           ((status_ptr->idxDayBegin == 0) && (status_ptr->idxDayEnd == idxLast))))
    {
      // If idxDate is first entry in Hist List, wrap to end
      idx = (status_ptr->idxDayBegin != 0) ? (status_ptr->idxDayBegin - 1) : idxLast;
      status_ptr->idxDate = idx;
      status_ptr->idxDayEnd = idx;
      status_ptr->idxDay = idx;
      // If new idx is beginning entry in Hist List, then just one entry in next Date Blk
      if ( idx == 0 ) {
        status_ptr->idxDayBegin = idx;
      }
      else // have to find beginning of Blk
      {
        entry_ptr = (APAC_HIST_ENTRY_PTR) &m_APAC_Hist.entry[idx];
        curr_mon_day = APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp);
        curr_yr = APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp);
        cnt = 0;
        while (cnt < APAC_HIST_ENTRY_MAX) //while (idx >= 0)
        {
          entry_ptr--;
          // Compare to next entry, current entry
          if ( (curr_mon_day != APAC_HIST_PARSE_MON_DAY(entry_ptr->ts.Timestamp)) ||
               (curr_yr !=  APAC_HIST_PARSE_YR(entry_ptr->ts.Timestamp)) ||
               (idx == 0 ) )
          {
            status_ptr->idxDayBegin = idx;
            break;
          }
          idx--; // point to next entry as this one has same Date
          cnt++;
        } //loop until Date Changes
      } // else find next Date Blk
    } // end if at least 2 entries with diff Date Blks
  } // end else option == APAC_IF_DATE inc

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _HistDec(o=%d)\r\n", option);

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_Reset
 *
 * Description: Resets the APAC Processing.  From start menu, abort "<<" and
 *              timeout
 *
 * Parameters:  *msg1 - Not used
 *              *msg2 - Not used
 *              *msg3 - Not used
 *              *msg4 - Not used
 *              option - APAC_IF_DCLK, APAC_IF_TIMEOUT
 *
 * Returns:     TRUE always
 *
 * Notes:
 *  SRS-5081 If the crew member double clicks the left arrow pushbutton switch (ÜÜ) while
 *           on any screen where it is valid or remains in any screen with a timeout exit
 *           point for greater than the configured AUTO_ABORT_TIME without pressing any
 *           pushbutton switches, the APAC System shall record an ETM log with a reason
 *           field indicating why the PAC Function aborted.
 *  SRS-5083 If the crew member double clicks the left arrow pushbutton switch (ÜÜ) while on
 *           any screen where it is valid or remains in any screen with a timeout exit point
 *           for greater than the configured AUTO_ABORT_TIME, the APAC System shall record an
 *           ETM log containing the abort reason, date, time, Ng and ITT margins for the EUT
 *           as computed APAC values with an "NCR" flag, indicating 'No Crew Response'.
 *
 *
 *****************************************************************************/
BOOLEAN APACMgr_IF_Reset ( CHAR *msg1, CHAR *msg2, CHAR *msg3, CHAR *msg4,
                           APAC_IF_ENUM option )
{
  APAC_COMMIT_ENUM commit;
  BOOLEAN bResult;
  TREND_CMD_RESULT rslt;

  ASSERT ( (option == APAC_IF_DCLK) || (option == APAC_IF_TIMEOUT) );

  commit = (option == APAC_IF_DCLK) ? APAC_COMMIT_ABORT : APAC_COMMIT_NCR;

  // DCLK or TimeOut from M26,M27,M13,M14 (VLD,INVLD,ACPT,RJCT) also for M24,M25,M12
  //    Have to generate Summary Log with ABORT or NCR
  if (m_APAC_Status.run_state == APAC_RUN_STATE_COMMIT)
  {
    APACMgr_LogSummary ( commit );
  }
  else if ( (m_APAC_Status.state == APAC_STATE_TEST_INPROGRESS) &&
            (m_APAC_Status.run_state != APAC_RUN_STATE_FAULT) )
  {
    // DCLK or TimeOut from M23,(M8,M9,M10,M11) -> ABORT (no NCR)
    // DCLK or TimeOut from M5,M6,M29,M30,M7 -> ABORT or NCR
    // DCLK or TimeOut from M18, (M22,M17,M16,M15) Dont' care as Failure Log already recorded

    // DCLK or TimeOut from m8 is when LogAbortNCR s/b recored..
    if ( m_APAC_Status.run_state != APAC_RUN_STATE_IDLE ) {
      APACMgr_LogAbortNCR ( commit );
    }
    if ((m_APAC_Status.eng_uut != APAC_ENG_MAX) && (m_APAC_Status.nr_sel != APAC_NR_SEL_MAX)){
      bResult = TrendAppStartTrend(
                  m_APAC_Cfg.eng[m_APAC_Status.eng_uut].idxTrend[m_APAC_Status.nr_sel],
                  FALSE, &rslt);
    }
  } // end else if (== APAC_STATE_TEST_INPROGRESS) &&
    // m_APAC_Status.run_state != APAC_RUN_STATE_FAULT)

  m_APAC_Status.run_state = APAC_RUN_STATE_IDLE;
  m_APAC_Status.state = APAC_STATE_IDLE;

  // DLCK or TimeOut from History.. Don't care and Test not inprogress

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _Reset\r\n");

  return (TRUE);
}


/******************************************************************************
 * Function:    APACMgr_IF_InvldDelayTimeOutVal
 *
 * Description: Setup up ptr to Menu's "Invalid Key Display TimeOut" value.
 *
 * Parameters:  *timeoutVal_ptr - ptr to Menu's "Invalid Key Display TimeOut" value
 *
 * Returns:     None
 *
 * Notes:
 *   - Menu App should call this init function once on startup
 *
 *****************************************************************************/
void APACMgr_IF_InvldDelayTimeOutVal ( UINT32 *timeoutVal_ptr )
{
  m_APAC_Status.menu_invld_timeout_val_ptr_ms = timeoutVal_ptr;

  APACMgr_IF_DebugStr(NORMAL,TRUE,"APACMgr: _InvldDelayTimeOutVal(val=%d)\r\n",
                      *timeoutVal_ptr);

}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: APACMgr_Interface.c $
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 1/18/16    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR #1308. Various Updates from Reviews and Enhancements
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 15-12-02   Time: 5:54p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Fix display of ITT Margin issue. 
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 11/17/15   Time: 9:27a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Code Review Updates.
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 11/10/15   Time: 11:54a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Comment Update.
 *
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 11/10/15   Time: 11:30a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing. Additional Updates from GSE/LOG Review AI.
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/06/15   Time: 11:28a
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing.  Disable Trend on User DCLK path.
 *
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/03/15   Time: 2:17p
 * Updated in $/software/control processor/code/application
 * SCR #1304 APAC Processing additional minor updates
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
