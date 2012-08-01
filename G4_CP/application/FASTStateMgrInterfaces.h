#ifndef FASTSTATEMGRINTERFACES_H
#define FASTSTATEMGRINTERFACES_H

/******************************************************************************
            Copyright (C) 2007-2011 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:         FASTStateMgrInterfaces.h

    Description:  

    VERSION
    $Revision: 4 $  $Date: 9/23/11 8:02p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
//List modules that define a StateMgr interface here
#include "trigger.h"
#include "uploadmgr.h"
#include "datamanager.h"
#include "MsStsCtl.h"
#include "DIOMgr.h"

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

/******************************************************************************
                                 Package Defines                              *
******************************************************************************/

/*
    Below defines a list for all:
    "Tranition Criteria" (TC)
    "Tasks"              (TASKS)
    
    Use the pre-processor to generate an enum of all TCs and TASKs, and also
    a structure containing the interface information to them.    
    
    HOW TO ADD Transition Criteria:
     1. Add a FSM_TC_ITEM macro to the FSM_TC_LIST 
    HOW TO ADD Tasks:
     1*. Add Name and GetState function to the FSM_TC_LIST
     2. Add the identical name and Control Function to the FSM_TASK_LIST
        * Can use FSM_GetStateFALSE for the GetState if the task will always return INACTIVE
     3. For Tasks that have the IsNumerated flag set, this is the maximum value of the index
        that follows the task name.
     
    LIST FIELD DEFINITIONS: 
    "Name" will become string used to string used in configuring TASKS and TCs
    "IsNumerated" allows the last X characters in name to be intepreted as a number 
                  that is passed in the parameter field of the GetState functions.
    "Control, GetState" Functions.  Function pointers to the interface.
*/
#define FSM_TC_LIST\
/*          Name IsNumerated     GetState Funciton*/\
FSM_TC_ITEM(TACT,  TRUE,         TriggerGetState)\
FSM_TC_ITEM(TVLD,  TRUE,         TriggerValidGetState)\
FSM_TC_ITEM(TIMR, FALSE,         FSM_GetIsTimerExpired)\
FSM_TC_ITEM(FILX, FALSE,         UploadMgr_FSMGetStateFilesPendingRoundTrip)\
FSM_TC_ITEM(MVPN, FALSE,         MSSC_FSMGetVPNStatus)\
FSM_TC_ITEM(MSCF, FALSE,         MSSC_FSMGetCFStatus)\
FSM_TC_ITEM(UPLD, FALSE,         UploadMgr_FSMGetState)\
FSM_TC_ITEM(AUPL, FALSE,         UploadMgr_FSMGetState)\
FSM_TC_ITEM(DNLD, FALSE,         DataMgrDownloadGetState)\
FSM_TC_ITEM(RECD, FALSE,         DataMgrRecGetState)\
FSM_TC_ITEM(ACFG, FALSE,         AC_FSMGetState)\
FSM_TC_ITEM(RFON, FALSE,         FAST_FSMRfGetState)\
FSM_TC_ITEM(ENDF, FALSE,         FSM_GetStateFALSE)\
FSM_TC_ITEM(DOUT, TRUE,          DIOMgr_FSMGetStateDIO)\
FSM_TC_ITEM(LACH, FALSE,         PmFSMAppBusyGetState)

#define FSM_TASK_LIST\
/*            Name  IsNumerated  Max Num Control Function         GetState Function*/\
FSM_TASK_ITEM(UPLD, FALSE,       0,      UploadMgr_FSMRun,        UploadMgr_FSMGetState)\
FSM_TASK_ITEM(AUPL, FALSE,       0,      UploadMgr_FSMRunAuto,    UploadMgr_FSMGetState)\
FSM_TASK_ITEM(DNLD, FALSE,       0,      DataMgrDownloadRun,      DataMgrDownloadGetState)\
FSM_TASK_ITEM(RECD, FALSE,       0,      DataMgrRecRun,           DataMgrRecGetState)\
FSM_TASK_ITEM(ACFG, FALSE,       0,      AC_FSMAutoRun,           AC_FSMGetState)\
FSM_TASK_ITEM(RFON, FALSE,       0,      FAST_FSMRfRun,           FAST_FSMRfGetState)\
FSM_TASK_ITEM(DOUT, TRUE,        4,      DIOMgr_FSMRunDIO,        DIOMgr_FSMGetStateDIO)\
FSM_TASK_ITEM(LACH, FALSE,       0,      PmFSMAppBusyRun,         PmFSMAppBusyGetState)\
FSM_TASK_ITEM(ENDF, FALSE,       0,      FAST_FSMEndOfFlightRun,  FSM_GetStateFALSE)


/******************************************************************************
                                 Package Typedefs                             *
******************************************************************************/

/******************************************************************************
                                 Package Exports                              *
******************************************************************************/
#undef EXPORT

#if defined ( FASTSTATEMGR_BODY )
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

/*************************************************************************
 *  MODIFICATIONS
 *    $History: FASTStateMgrInterfaces.h $
 * 
 * *****************  Version 4  *****************
 * User: Jim Mood     Date: 9/23/11    Time: 8:02p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix for the list of running task not being reliable in the
 * fsm.status user command.
 * 
 * *****************  Version 3  *****************
 * User: Jim Mood     Date: 9/02/11    Time: 6:08p
 * Updated in $/software/control processor/code/application
 * SCR 575 Modfix (added AUPL to the TC list)
 * 
 * *****************  Version 2  *****************
 * User: Jim Mood     Date: 7/26/11    Time: 3:05p
 * Updated in $/software/control processor/code/application
 * SCR 575 updates
 * 
 * *****************  Version 1  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:03a
 * Created in $/software/control processor/code/application
 ***************************************************************************/

#endif // FASTSTATEMGRINTERFACES_H

