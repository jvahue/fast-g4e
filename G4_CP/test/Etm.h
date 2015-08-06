#ifndef ETM_H
#define ETM_H

/******************************************************************************
Copyright (C) 2009 Pratt & Whitney Engine Services, Inc. 
Altair Engine Diagnostic Solutions
All Rights Reserved. Proprietary and Confidential.

File: Etm.h

Description: This file defines the interfaces and control structures required
by the Environmental Test Manager (ETM).  The ETM code is enable when the 
define macro ENV_TEST is defined during compilation.  The code works in 
conjunction with the Environment Test Control (ETC) Windows Application to 
perform Environmental Testing.

VERSION
$Revision: 3 $  $Date: 7/14/15 10:27a $   

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"

#include "PowerManager.h"

/******************************************************************************
Package Defines
******************************************************************************/
// define or undef to include/exclude ETM functionality from the build
#ifdef ENV_TEST
#define MAX_MSG_SIZE 64

#define RESULT_ROW  {0,0,""}
#define ETM_DEFAULT 0,0,0,0,\
                    {\
                    RESULT_ROW,  /* SP    */\
                    RESULT_ROW,  /* 429   */\
                    RESULT_ROW,  /* 717   */\
                    RESULT_ROW,  /* LSS   */\
                    RESULT_ROW,  /* GPIO  */\
                    RESULT_ROW,  /* SDRAM */\
                    RESULT_ROW,  /* DPRAM */\
                    RESULT_ROW,  /* ADC   */\
                    RESULT_ROW,  /* Reset */\
                    RESULT_ROW   /* Net   */\
                    }
/******************************************************************************
Package Typedefs
******************************************************************************/
// NOTE: This definition must match the definition in the ETC see ETCDlg.h for
// the definition of enum TestId.
typedef enum {
    eSp,         // Serial Port Test
    e429,
    e717,
    eLss,
    eGpio,
    eSdram,
    eDpram,
    eAdc,
    eReset,
    eNet,
    eMaxTestId
} ENV_TEST_ID;

typedef enum {
    eBatt,
    eBus,
    eLiBat,
    eTemp,
    eMaxAdcSignal
} ADC_SIGNALS;

typedef struct TestResultsTag {
    UINT32 pass;
    UINT32 fail;
    char   msg[MAX_MSG_SIZE+1];
} TestResults;

typedef struct TestControlTag {
    BOOLEAN active;          // is the ETM testing active
    UINT32  tests;           // which tests are turned on
    SINT32  reset;           // should we reset the tests
    UINT32  msStartupTimer;  // two minute timer until the MS is up and running
    TestResults results[eMaxTestId];  // results
} TestControl;

/******************************************************************************
Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ETM_BODY )
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
EXPORT void InitEtm(void);

EXPORT void HeartbeatFailSignal(void);
EXPORT BOOLEAN EnvNormalShutDown(PM_APPSHUTDOWN_REASON reason);

/*****************************************************************************
*  MODIFICATIONS
*    $History: Etm.h $
 * 
 * *****************  Version 3  *****************
 * User: Jeff Vahue   Date: 7/14/15    Time: 10:27a
 * Updated in $/software/control processor/code/test
 * Update to run with v2.1.0
 * 
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 9/15/09    Time: 5:33p
 * Updated in $/software/control processor/code/test
* 
*
*****************************************************************************/
#endif // ETM defined

#endif // ETM_H                             
