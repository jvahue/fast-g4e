#define TEST_POINT_BODY
/******************************************************************************
Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
Altair Engine Diagnostic Solutions
All Rights Reserved. Proprietary and Confidential.


File:        TestPoint.c  

Description:  This file implements the test point capability used during
  certification.  This allows the test scripts to cause abnormal control path
  execution (i.e., hardware failures, memory corruption, etc.) that the code
  is designed to handle.


 VERSION
    $Revision: 23 $  $Date: 12/08/12 1:56p $  

******************************************************************************/
#ifdef STE_TP

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <string.h>
//#include "mcf5xxx.h"
#include "mcf548x_psc.h"


/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/
#include "TestPoints.h"
#include "alt_basic.h"
#include "Gse.h"
#include "user.h"
#include "DIO.h"

#include "TaskManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define TP_STR_REP(v)  {#v, v}
#define TP_USER(s,t,a,n) {s, NO_NEXT_TABLE, TpUserMsg, t, USER_RW|USER_NO_LOG, a, 0, eTpMaxValue, NO_LIMIT, n}
/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
BOOLEAN enableSdDump;  // enable the output of coverage data on shutdown
UINT32 tpRmtDelay;

UINT32 divisor;
FLOAT32 returnValue;
UINT32 tpiMaxStartup;
UINT32 tpiMaxValue;

USER_ENUM_TBL TpTypeNames[TP_ENUM_MAX+1] = { 
    TP_STR_REP( TP_INACTIVE),
    TP_STR_REP( TP_VALUE),
    TP_STR_REP( TP_INC),
    TP_STR_REP( TP_TOGGLE),
    TP_STR_REP( TP_TOGGLE_X),
    TP_STR_REP( TP_TRIGGER),
    { NULL,       0 }                                   
};

USER_HANDLER_RESULT TpUserMsg(USER_DATA_TYPE DataType,
                              USER_MSG_PARAM Param,
                              UINT32 Index,
                              const void *SetPtr,
                              void **GetPtr);

TEST_POINT_DATA tpEdit;

extern void ReportCoverage(void);
USER_HANDLER_RESULT DumpCoverage(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

static USER_MSG_TBL TestPointRoot[] =
{
    TP_USER("TYPE",   USER_TYPE_ENUM,   &tpEdit.type,        TpTypeNames),
    TP_USER("VALUE",  USER_TYPE_UINT32, &tpEdit.rVal.uVal,   NULL),
    TP_USER("WAIT",   USER_TYPE_INT32,  &tpEdit.wait,        NULL),
    TP_USER("REPEAT", USER_TYPE_INT32,  &tpEdit.repeat,      NULL),
    TP_USER("CMDID",  USER_TYPE_UINT32, &tpEdit.cmdId,       NULL),
    TP_USER("TOGGLE", USER_TYPE_UINT32, &tpEdit.toggleState, NULL),
    TP_USER("TGLMAX", USER_TYPE_UINT32, &tpEdit.toggleMax,   NULL),
    TP_USER("TGLLIM", USER_TYPE_UINT32, &tpEdit.toggleLim,   NULL),
    TP_USER("TGLCUR", USER_TYPE_UINT32, &tpEdit.toggleCur,   NULL),
    { "SD_DUMP", NO_NEXT_TABLE, User_GenericAccessor, USER_TYPE_YESNO,  USER_RW|USER_NO_LOG, &enableSdDump, -1, -1, NO_LIMIT, NULL},
    { "DUMPCOV", NO_NEXT_TABLE, DumpCoverage,         USER_TYPE_ACTION, USER_RO|USER_NO_LOG, NULL,          -1, -1, NO_LIMIT, NULL},
    { NULL,NULL,NULL,NO_HANDLER_DATA}
};

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
void TpSupportDt(  void* pParam);
void TpSupportRmt( void* pParam);

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

//--------------------------------------------------------------------------------------------
//  1. overrun on cmd
//  2. Switch to Test mode on MIF0
//  3. Switch to Normal mode on MIF0
void TpSupportDt1(  void* pParam)
{

    UINT32 StartTime = TTMR_GetHSTickCount();

    // 1. overrun on cmd
    UINT32 duration = TPU( 0, eTpDtOverrun);
    if ( duration > 0)
    {
        UINT32 StartTime1 = TTMR_GetHSTickCount();
        while((TTMR_GetHSTickCount() - StartTime1) < (duration* TICKS_PER_mSec));
    }

    // MODE SWITCHING
    // 2. toggle mode between TEST and NORMAL as commanded
    // 3. Verify RMT task run to complete when started on mode switch
    //   To see mode switch on scope 
    //   a. trigger on MIF0 pulse
    //   b. enable task pulse for these two DT tasks 1 run 2 stops
    if ( Tm.currentMIF == 0 && TPU( 0, eTpModeTest))
    {
        TmSetMode(SYS_TEST_ID);
    }

    if ( Tm.currentMIF == 0 && TPU( 0, eTpModeNormal))
    {
        TmSetMode(SYS_NORMAL_ID);
    }

    // cause an unexpected interrupt
    if ( TPU( 0, eTpUnexpected))
    {
       StartTime /= divisor;
    }

    while((TTMR_GetHSTickCount() - StartTime) < (50*TICKS_PER_uSec));
}

//--------------------------------------------------------------------------------------------
// 50us task - actual around 62us
void TpSupportDt2(  void* pParam)
{
    UINT32 StartTime = TTMR_GetHSTickCount();
    while((TTMR_GetHSTickCount() - StartTime) < (50*TICKS_PER_uSec));
}

//--------------------------------------------------------------------------------------------
// This task is configured to run for 500ms, once every 2s and only in Normal mode
// It can do the following:
//  1. overrun on cmd - run for 3s
//  2. attempt to create tasks after startup on cmd
//  3. run through a mode switch - run for 1.75s
//--------------------------------------------------------------------------------------------
void TpSupportRmt( void* pParam)
{
    TCB TaskInfo;
    UINT32 StartTime = TTMR_GetHSTickCount();

    //  1. overrun on cmd - run for 3s
    if ( TPU( 0, eTpRmtOverrun))
    {
        // cause an RMT overrun - run for 3s
        UINT32 StartTime1 = TTMR_GetHSTickCount();
        while((TTMR_GetHSTickCount() - StartTime1) < (3 * TICKS_PER_Sec));
    }

    //  2. attempt to create tasks after startup on cmd
    if ( TPU( 0, eTpCreateTask))
    {
        // Test Procedure will ensure the DataManager0 is open
        memset(&TaskInfo, 0, sizeof(TaskInfo));
        strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"INVALID TASK",_TRUNCATE);
        TaskInfo.TaskID          = Data_Mgr0;
        TaskInfo.Function        = TpSupportRmt;
        TaskInfo.Priority        = taskInfo[Data_Mgr0].priority;
        TaskInfo.Type            = taskInfo[Data_Mgr0].taskType;
        TaskInfo.modes           = taskInfo[Data_Mgr0].modes;
        TaskInfo.MIFrames        = taskInfo[Data_Mgr0].MIFframes;
        TaskInfo.Enabled         = TRUE;
        TaskInfo.Locked          = FALSE;
        TaskInfo.Rmt.InitialMif  = taskInfo[Data_Mgr0].InitialMif;
        TaskInfo.Rmt.MifRate     = taskInfo[Data_Mgr0].MIFrate;
        TaskInfo.pParamBlock     = NULL;
        TmTaskCreate (&TaskInfo);
    }

    //  3. run through a mode switch - run for 1.75s
    if ( TPU( 0, eTpRmtMode))
    {
        // First switch to test mode so Dt2 and this can't can't be dispatched
        // run for 1.75s
        UINT32 StartTime1 = TTMR_GetHSTickCount();
        
        TmSetMode(SYS_TEST_ID);

        while((TTMR_GetHSTickCount() - StartTime1) < (1750 * TICKS_PER_mSec));
    }

    // Re-enable the GSE port when the code fails it SRS-3711 Testing
    //MCF_PSC_CR(0) =(MCF_PSC_CR_RX_ENABLED | MCF_PSC_CR_TX_ENABLED);

    // otherwise burn <tpRmtDelay>ms so we can see a blip every 2s
    // and holdoff other RMT tasks from running
    while((TTMR_GetHSTickCount() - StartTime) < (tpRmtDelay * TICKS_PER_mSec));
}

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/******************************************************************************
* Function:    TestPointInit
*
* Description: Setup the initial state for the Test Point Code.
*              *Clear and init the list of tasks
*              *Create a new task to run the test manager every frame
* Parameters:  
*              
* Returns: 
*
* Notes:       
*              
*****************************************************************************/
void TestPointInit(void)
{
    TCB             TaskInfo;
    USER_MSG_TBL TestPtTblPtr = { "TP", TestPointRoot, NULL, NO_HANDLER_DATA};

    //Add an entry in the user message handler table
    User_AddRootCmd(&TestPtTblPtr);

    memset( tpData, 0, sizeof(tpData));

    // simple check to see if PyScripts match C code values
    tpiMaxStartup = eTpMaxStartUp;
    tpiMaxValue   = eTpMaxValue;

    // 5 ms standard delay
    tpRmtDelay = 5;

    // add a couple bad tasks into the system to do things to cause problems
    // A DT task to:
    //  1. overrun on cmd
    //  2. toggle mode switching to verify mode switch on MIF boundaries
    //  3. Verify RMT task run to complete when started on mode switch
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"TP Support DT1",_TRUNCATE);
    TaskInfo.TaskID          = TP_SUPPORT_DT1;
    TaskInfo.Function        = TpSupportDt1;
    TaskInfo.Priority        = taskInfo[TP_SUPPORT_DT1].priority;
    TaskInfo.Type            = taskInfo[TP_SUPPORT_DT1].taskType;
    TaskInfo.modes           = taskInfo[TP_SUPPORT_DT1].modes;
    TaskInfo.MIFrames        = taskInfo[TP_SUPPORT_DT1].MIFframes;
    TaskInfo.Enabled         = TRUE;
    TaskInfo.Locked          = FALSE;
    TaskInfo.Rmt.InitialMif  = taskInfo[TP_SUPPORT_DT1].InitialMif;
    TaskInfo.Rmt.MifRate     = taskInfo[TP_SUPPORT_DT1].MIFrate;
    TaskInfo.pParamBlock     = &returnValue;

    TmTaskCreate (&TaskInfo);

    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"TP Support DT2",_TRUNCATE);
    TaskInfo.TaskID          = TP_SUPPORT_DT2;
    TaskInfo.Function        = TpSupportDt2;
    TaskInfo.Priority        = taskInfo[TP_SUPPORT_DT2].priority;
    TaskInfo.Type            = taskInfo[TP_SUPPORT_DT2].taskType;
    TaskInfo.modes           = taskInfo[TP_SUPPORT_DT2].modes;
    TaskInfo.MIFrames        = taskInfo[TP_SUPPORT_DT2].MIFframes;
    TaskInfo.Enabled         = TRUE;
    TaskInfo.Locked          = FALSE;
    TaskInfo.Rmt.InitialMif  = taskInfo[TP_SUPPORT_DT2].InitialMif;
    TaskInfo.Rmt.MifRate     = taskInfo[TP_SUPPORT_DT2].MIFrate;
    TaskInfo.pParamBlock     = &returnValue;

    TmTaskCreate (&TaskInfo);

    // A RMT task to:
    //  1. overrun on cmd
    //  2. attempt to create tasks after startup on cmd
    //  3. run through a mode switch
    memset(&TaskInfo, 0, sizeof(TaskInfo));
    strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"TP Support RMT",_TRUNCATE);
    TaskInfo.TaskID          = TP_SUPPORT_RMT;
    TaskInfo.Function        = TpSupportRmt;
    TaskInfo.Priority        = taskInfo[TP_SUPPORT_RMT].priority;
    TaskInfo.Type            = taskInfo[TP_SUPPORT_RMT].taskType;
    TaskInfo.modes           = taskInfo[TP_SUPPORT_RMT].modes;
    TaskInfo.MIFrames        = taskInfo[TP_SUPPORT_RMT].MIFframes;
    TaskInfo.Enabled         = TRUE;
    TaskInfo.Locked          = FALSE;
    TaskInfo.Rmt.InitialMif  = taskInfo[TP_SUPPORT_RMT].InitialMif;
    TaskInfo.Rmt.MifRate     = taskInfo[TP_SUPPORT_RMT].MIFrate;
    TaskInfo.pParamBlock     = NULL;

    TmTaskCreate (&TaskInfo);

    // for un-handled exception
    divisor = 0;

    // allow data dump on shutdown
    enableSdDump = FALSE;
}

/******************************************************************************
* Function:    GetDinValue
*
* Description: This function construct the value encoded by DIN0-7 at startup
*              to drive the startup test point functionality.
*              
*****************************************************************************/
UINT32 GetDinValue( void)
{
    UINT8 x;
    UINT32 value = 0;


    for (x = 0; x < 8; x++)
    {
        if ( DIO_ReadPin( (DIO_INPUT)x))
        {
            value |= 1 << x;
        }
    }

    return value;
}

/******************************************************************************
* Function:    StartupTpUint
*
* Description: This functions return an UINT32 Test Point value
*              
*
* Parameters:  [in] actual : Actual Value or variable value
*              [in] tpId: test point identifier
*              
* Returns:     UINT32: The test point value          
*
* Notes:       
*              
*****************************************************************************/
UINT32 StartupTpUint( UINT32 actual, TEST_POINTS tpId)
{
    UINT32 setTp = GetDinValue();
    if ( setTp == tpId)
    {
        return actual ^ 0xFFFFFFFF;
    }
    else
    {
        return actual;
    }
}

/******************************************************************************
* Function:    StartupTpUintInit
*
* Description: This functions return an UINT32 Test Point value with a 
*              specified value
*              
*
* Parameters:  [in] actual : Actual Value or variable value
*              [in] tpId: test point identifier
*              [in] iValue: initial value
*              
* Returns:     UINT32: The test point value          
*
* Notes:       
*              
*****************************************************************************/
UINT32 StartupTpUintInit( UINT32 actual, TEST_POINTS tpId, UINT32 iValue)
{
    UINT32 setTp = GetDinValue();
    if ( setTp == tpId)
    {
        return iValue;
    }
    else
    {
        return actual;
    }
}

/******************************************************************************
* Function:    TpInt
*
* Description: This functions return an UINT32 Test Point value
*              
*
* Parameters:  [in] actual : Actual Value or variable value
*              [in] tpId: test point identifier
*              
* Returns:     UINT32: The test point value          
*
* Notes:       
*              
*****************************************************************************/
UINT32 TpUint( UINT32 actual, TEST_POINTS tpId)
{
    BOOLEAN cycleDone = TRUE;
    UINT32  tpValue = actual;

    // handle after M calls for all TP
    if (tpData[tpId].type != TP_INACTIVE && tpData[tpId].wait > 0)
    {
        tpData[tpId].wait--;
    }
    else
    {
        switch (tpData[tpId].type)
        {
        case TP_INACTIVE:
            break;

        case TP_VALUE:
            tpValue = tpData[tpId].rVal.uVal;
            break;

        case TP_INC:
            tpValue = tpData[tpId].rVal.uVal++;
            break;

        case TP_TOGGLE:
            // toggle the result every other call between the actual and the value
            if (!tpData[tpId].toggleState)
            {
                tpValue = tpData[tpId].rVal.uVal;
            }
            // toggle the state
            tpData[tpId].toggleState ^= 1;
            break;

        case TP_TOGGLE_X:
            cycleDone = FALSE;
            // Complex Toggle Toggle Max Cycle 
            if (tpData[tpId].toggleCur++ >= tpData[tpId].toggleLim)
            {
                // (+) TP value until Limit
                tpValue = tpData[tpId].toggleMax >= 0 ? actual : tpData[tpId].rVal.uVal;
            }
            else
            {
                // (-) Actual Value until Limit
                tpValue = tpData[tpId].toggleMax >= 0 ? tpData[tpId].rVal.uVal : actual;
            }

            if (tpData[tpId].toggleMax > 0 && tpData[tpId].toggleCur == tpData[tpId].toggleMax)
            {
                tpData[tpId].toggleCur = 0;
                cycleDone = TRUE;
            }
            else if (tpData[tpId].toggleMax < 0 && tpData[tpId].toggleCur == -tpData[tpId].toggleMax)
            {
                tpData[tpId].toggleCur = 0;
                cycleDone = TRUE;
            }
            break;
        case TP_TRIGGER:
            // set the target TP's type, i.e., enable it
            tpData[ tpData[tpId].cmdId].type = tpData[ tpId].rVal.uVal;
            // shut ourself off
            tpData[ tpId].type = TP_INACTIVE;
        }

        // clear the TP after N
        if (cycleDone && tpData[tpId].type != TP_INACTIVE && tpData[tpId].repeat > 0)
        {
            tpData[tpId].repeat--;
            if (tpData[tpId].repeat == 0)
            {
                tpData[tpId].type = TP_INACTIVE;
            }
        }
    }

    return tpValue;  
}

/******************************************************************************
* Function:    TpUserMsg
*
* Description: This functions handles user interaction.
*              
*
* Parameters:  [in] DataType : Actual Value or variable value
*              [in] Param: test point identifier
*              
* Returns:     UINT32: The test point value          
*
* Notes:       
*              
*****************************************************************************/
USER_HANDLER_RESULT TpUserMsg(USER_DATA_TYPE DataType,
                              USER_MSG_PARAM Param,
                              UINT32 Index,
                              const void *SetPtr,
                              void **GetPtr)
{
    USER_HANDLER_RESULT result = USER_RESULT_OK;

    //Get sensor structure based on index
    memcpy( &tpEdit, &tpData[Index], sizeof(TEST_POINT_DATA));

    // If "get" command, point the GetPtr to the sensor config element defined by
    // Param
    if( SetPtr == NULL )
    {
        *GetPtr = Param.Ptr;
    }
    else
    {
        result = User_GenericAccessor(DataType, Param, Index, SetPtr, GetPtr);

        if(USER_RESULT_OK == result)
        {
            memcpy( &tpData[Index], &tpEdit, sizeof(TEST_POINT_DATA));
        }
    }

    return result;
}

/******************************************************************************
* Function:    DumpCoverage
*
* Description: This functions handles user interaction.
*              
*
* Parameters:  [in] DataType : Actual Value or variable value
*              [in] Param: test point identifier
*              
* Returns:     UINT32: The test point value          
*
* Notes:       
*              
*****************************************************************************/
USER_HANDLER_RESULT DumpCoverage(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
#ifndef WIN32

    TmTaskEnable( Monitor_Task, FALSE);
    ReportCoverage();
    TmTaskEnable( Monitor_Task, TRUE);

#else
    GSE_DebugStr(OFF, FALSE, "Coverage Done");
#endif

    return USER_RESULT_OK;
}

#endif  // STE_TP

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TestPoints.c $
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/08/12   Time: 1:56p
 * Updated in $/software/control processor/code/test
 * Update forf v2.0.0
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 11/12/10   Time: 9:54p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 11/09/10   Time: 9:08p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 11/05/10   Time: 9:48p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 11/01/10   Time: 4:05p
 * Updated in $/software/control processor/code/test
 * Mod for Coverage Data Collection
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 10/19/10   Time: 7:25p
 * Updated in $/software/control processor/code/test
 * SCR# 848 - Code Coverage
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 10/01/10   Time: 6:14p
 * Updated in $/software/control processor/code/test
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 9/23/10    Time: 5:40p
 * Updated in $/software/control processor/code/test
 * Code Cov Update
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 9/10/10    Time: 4:47p
 * Updated in $/software/control processor/code/test
 * Remove mod to reenable UARTs in TP RMT 
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:45p
 * Updated in $/software/control processor/code/test
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 9/03/10    Time: 8:23p
 * Updated in $/software/control processor/code/test
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 4:40p
 * Updated in $/software/control processor/code/test
 * SCR #707 - Coverage Mods
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 8/22/10    Time: 1:44p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - No log for all (including Dump Coverage) test point commands
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 7/24/10    Time: 5:09p
 * Updated in $/software/control processor/code/test
 * Reduce Tp RMT task duration form 500ms to 5ms
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 7/23/10    Time: 8:14p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - More TP code 
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 7/21/10    Time: 7:51p
 * Updated in $/software/control processor/code/test
 * Mark all TP to disable recording of TP user modification
 * 
 * *****************  Version 7  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:51p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/test
 * SCR# 707 - Add TestPoints for code coverage.
 * 
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 7/16/10    Time: 7:25p
 * Updated in $/software/control processor/code/test
 * Code Coverage development
 * 
 * *****************  Version 4  *****************
 * User: Contractor2  Date: 3/02/10    Time: 2:09p
 * Updated in $/software/control processor/code/test
 * SCR# 472 - Fix file/function header
 * 
 *************************************************************************/

