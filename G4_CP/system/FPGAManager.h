#ifndef FPGA_MANAGER_H
#define FPGA_MANAGER_H

/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:         FPGAManager.h
    
    Description:  System level functions to provide BIT support for the FPGA 
                  interface 
    
    VERSION
      $Revision: 13 $  $Date: 8/31/10 12:06p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "fpga.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct 
{
  UINT32 CbitRegCheckTimer; 
} FPGA_MGR_TASK_PARMS; 



#pragma pack(1)

// FPGA Log Data Payload Structures 
typedef struct {
  UINT16 FPGA_Version; 
  UINT16 FPGA_VersionInv; 
} DRV_FPGA_BAD_VERSION_LOG_STRUCT; 

typedef struct {
  UINT32 ResultCode; 
} DRV_FPGA_PBIT_GENERIC_LOG_STRUCT; 

// FPGA CBIT Health Status 
typedef struct 
{
  FPGA_SYSTEM_STATUS SystemStatus; 
  UINT32             CRCErrCnt; 
  UINT32             RegCheckFailCnt; 
} FPGA_CBIT_HEALTH_COUNTS; 

typedef struct 
{
  UINT32  ResultCode;  // Shadow RAM mismatch or HW Reg mismatch 
  UINT32  enumReg;     // Reg that has failed CBIT reg check 
  UINT16  nFailures;   // Number of compare failures 
  UINT16  Expected;    // The expected value 
  UINT16  Actual;      // The actual value 
} FPGA_CBIT_REG_CHECK_LOG; 

typedef struct {
  UINT32  minIntrTime; 
} FPGA_INTR_FAIL_LOG; 

#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( FPGA_MANAGER_BODY )
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
EXPORT void FPGAMgr_Initialize (void); 
EXPORT void FPGAMgr_CBIT_Task  (void *pParam );
EXPORT FPGA_CBIT_HEALTH_COUNTS FPGAMgr_GetCBITHealthStatus( void ); 
EXPORT FPGA_CBIT_HEALTH_COUNTS FPGAMgr_CalcDiffCBITHealthStatus ( FPGA_CBIT_HEALTH_COUNTS 
                                                                  PrevCount ); 
EXPORT FPGA_CBIT_HEALTH_COUNTS FPGAMgr_AddPrevCBITHealthStatus ( 
                                                          FPGA_CBIT_HEALTH_COUNTS CurrCnt,
                                                          FPGA_CBIT_HEALTH_COUNTS PrevCnt );

#ifdef GENERATE_SYS_LOGS
  EXPORT void FPGA_CreateAllInternalLogs ( void );
#endif  

#endif // FPGA_MANAGER_H        

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: FPGAManager.h $
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 12:06p
 * Updated in $/software/control processor/code/system
 * SCR #840
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 10  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 9  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 3/29/10    Time: 6:41p
 * Updated in $/software/control processor/code/system
 * SCR #517 Error in Bad FPGA Version Failed Log
 * 
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:06p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 11/20/09   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * Update FPGA Reg Check to be 2 Hz.
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:12a
 * Updated in $/software/control processor/code/system
 * SCR #315 PWEH SEU Count Processing Support
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:58a
 * Updated in $/software/control processor/code/system
 * SCR 115, 116 and FPGA CBIT requirements
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 6:13p
 * Updated in $/control processor/code/system
 * SCR #136 Pack system log structures
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:43p
 * Created in $/control processor/code/system
 * SCR 129 Misc FPGA Updates
 * 
 *
 *****************************************************************************/
                                            
