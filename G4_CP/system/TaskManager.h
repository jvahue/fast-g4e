#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

/******************************************************************************
         Copyright (C) 2003-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.
 
  File:        TaskManager.h
 
  Description: Task Manager definitions.
 
  VERSION
     $Revision: 65 $  $Date: 12-11-12 4:46p $  
******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                               */
/*****************************************************************************/
// None



/*****************************************************************************/
/* Software Specific Includes                                               */
/*****************************************************************************/
#include "TTMR.h"

/******************************************************************************
                              Package Defines
******************************************************************************/
// Task Related constants
#define MAX_DT_TASKS       31
#define MAX_RMT_TASKS      45
#define MAX_TASKS          (MAX_DT_TASKS+MAX_RMT_TASKS-1)  // (0 to xx)

#define MAX_MIF            31        // 32 MIFs  [0 - 31]

#define MAX_DT_OVERRUNS    3        //  Max of 3 sequential DT task overruns

#define EVENT_DRIVEN_RMT   0

#define TASK_TIMER_CHAN    TPU_CHAN_0

#define MIF_PERIOD_mS (TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL_uS/1000) 
#define MIFs_PER_SECOND (1000/MIF_PERIOD_mS)

// convert seconds to MIFs
#define S2M(x) ((x)*MIFs_PER_SECOND)


//Conversion factor to changing DutyCycle members of Tm to percent
#define TM_DUTY_CYCLE_TO_PCT (TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL_uS/100)

#define TASK_NAME_SIZE 25

#define MODE_MASK(x) (1U << x)

/******************************************************************************
                              Package Typedefs
******************************************************************************/
// Pointer to a function receiving a void pointer parameter & returning a void
typedef void (*FUNCTION_PTR)(void*);
//----------------------------------------------------------------------------
// NOTE: For Verification the highest and lowest priority should be set to
//       0 and 255 respectively.  This can be done for both DT and RMT task.
//----------------------------------------------------------------------------

#define TASK_ENTRY(Name,Pri,Type,Modes,DTrate,RMToffset,RMTrate) Name,
#define TASK_LIST\
/* Deterministic Tasks ----------------------------------------------------------------------------------*/\
/*         ID                          Priority  Type Modes          DT Rate,    RMT Offset, RMT Rate*   */\
/*         -----------------------     --------- ---- -------------- ----------- ----------- ------------*/\
TASK_ENTRY(SPI_Manager,                  0,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Pm_Task,                     10,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(DIO_Update_DINs,             20,      DT,   SYS_MODE_GP1, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Arinc429_Process_Msgs,       30,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(QAR_Read_Sub_Frame,          40,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(UART_Mgr0,                   45,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(UART_Mgr1,                   45,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(UART_Mgr2,                   45,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(UART_Mgr3,                   45,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Sensor_Task,                 50,      DT,   SYS_MODE_GP1, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Trigger_Task,                60,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(EngRun_Task,                 60,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Trend_Task,                  62,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Event_Task,                  62,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Action_Task,                 63,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(FAST_Manager,                65,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(FSM_Task,                    67,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Data_Mgr0,                   70,      DT,   SYS_MODE_GP0, 0x11111111,   0,          0          )\
TASK_ENTRY(Data_Mgr1,                   70,      DT,   SYS_MODE_GP0, 0x22222222,   0,          0          )\
TASK_ENTRY(Data_Mgr2,                   70,      DT,   SYS_MODE_GP0, 0x44444444,   0,          0          )\
TASK_ENTRY(Data_Mgr3,                   70,      DT,   SYS_MODE_GP0, 0x88888888,   0,          0          )\
TASK_ENTRY(Data_Mgr4,                   70,      DT,   SYS_MODE_GP0, 0x11111111,   0,          0          )\
TASK_ENTRY(Data_Mgr5,                   70,      DT,   SYS_MODE_GP0, 0x22222222,   0,          0          )\
TASK_ENTRY(Data_Mgr6,                   70,      DT,   SYS_MODE_GP0, 0x44444444,   0,          0          )\
TASK_ENTRY(Data_Mgr7,                   70,      DT,   SYS_MODE_GP0, 0x88888888,   0,          0          )\
TASK_ENTRY(Log_Manage_Task,             90,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(TH_Task_ID,                  95,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(Creep,                      100,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(TP_SUPPORT_DT1,             253,      DT,   SYS_MODE_GP0, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(TP_SUPPORT_DT2,             254,      DT,   SYS_MODE_GP2, 0xFFFFFFFF,   0,          0          )\
TASK_ENTRY(CBIT_Manager,               255,      DT,   SYS_MODE_GP1, 0x11111111,   0,          0          )\
/* RMT Tasks -------------------------------------------------------------------------------*/\
/*                ORDER THESE RMT TASKS IN PRIORITY BASED ON THEIR EXECUTION RATE           */\
/* RMT Tasks -------------------------------------------------------------------------------*/\
TASK_ENTRY(Arinc429_Disp_SW_Buff,        0,      RMT,  SYS_MODE_GP1, 0x00000000,   1,         10          )\
TASK_ENTRY(FPGA_Manager,                 5,      RMT,  SYS_MODE_GP1, 0x00000000,  17,         10          )\
TASK_ENTRY(System_Log_Manage_Task,      10,      RMT,  SYS_MODE_GP0, 0x00000000,   0,         10          )\
TASK_ENTRY(Sensor_Live_Data_Task,       15,      RMT,  SYS_MODE_GP1, 0x00000000,   0,         25          )\
TASK_ENTRY(QAR_Manager,                 20,      RMT,  SYS_MODE_GP1, 0x00000000,  16,         25          )\
TASK_ENTRY(Arinc429_BIT,                25,      RMT,  SYS_MODE_GP1, 0x00000000, 100,         32          )\
TASK_ENTRY(FAST_WDOG1,                  30,      RMT,  SYS_MODE_GP0, 0x00000000,   0,         S2M(1)      )\
TASK_ENTRY(FAST_WDOG2,                  35,      RMT,  SYS_MODE_GP0, 0x00000000,  50,         S2M(1)      )\
TASK_ENTRY(PWEHSEU_Manager,             40,      RMT,  SYS_MODE_GP0, 0x00000000,  18,         S2M(1)      )\
TASK_ENTRY(QAR_Monitor,                 45,      RMT,  SYS_MODE_GP1, 0x00000000,   2,         S2M(1)      )\
TASK_ENTRY(Uart_BIT,                    47,      RMT,  SYS_MODE_GP1, 0x00000000,   4,         S2M(1)      )\
TASK_ENTRY(MS_Status,                   50,      RMT,  SYS_MODE_GP1, 0x00000000,   0,         S2M(1)      )\
TASK_ENTRY(FAST_UL_Chk,                 55,      RMT,  SYS_MODE_GP1, 0x00000000, 100,         S2M(1)      )\
TASK_ENTRY(F7X_Disp_Debug,              60,      RMT,  SYS_MODE_GP1, 0x00000000,  20,         S2M(1)      )\
TASK_ENTRY(Uart_Disp_Debug,             65,      RMT,  SYS_MODE_GP1, 0x00000000,  20,         S2M(1)      )\
TASK_ENTRY(TP_SUPPORT_RMT,              70,      RMT,  SYS_MODE_GP2, 0x00000000, 500,         S2M(2)      )\
TASK_ENTRY(Clock_CBIT,                  75,      RMT,  SYS_MODE_GP1, 0x00000000, 500,         S2M(10*60)  )\
/* Self-scheduled and pure event tasks -----------------------------------------------------*/\
TASK_ENTRY(AC_Task_ID,                 100,      RMT,  SYS_MODE_GP1, 0x00000000, 500,        -4           )\
TASK_ENTRY(MSI_Packet_Handler,         110,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(Log_Command_Erase_Task,     115,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(Log_Mark_State_Task,        120,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(Log_Monitor_Erase_Task,     125,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -50          )\
TASK_ENTRY(Mem_Clear_Task,             130,      RMT,  SYS_MODE_GP0, 0x00000000,   0,        -2           )\
TASK_ENTRY(Mem_Erase_Task,             135,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -2           )\
TASK_ENTRY(Mem_Program_Task,           140,      RMT,  SYS_MODE_GP0, 0x00000000,   0,         0           )\
TASK_ENTRY(NV_Mem_Manager,             145,      RMT,  SYS_MODE_GP0, 0x00000000,   0,        -1           )\
TASK_ENTRY(UL_Log_Collection,          150,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(UL_Log_File_Upload,         155,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(UL_Log_Check,               160,      RMT,  SYS_MODE_GP0, 0x00000000,   0,        -S2M(1)      )\
TASK_ENTRY(Box_Background_Processing,  165,      RMT,  SYS_MODE_GP0, 0x00000000,   1,        -S2M(1)      )\
TASK_ENTRY(User_Msg_Proc,              170,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(FAST_TxTestID,              175,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -50          )\
TASK_ENTRY(MS_GSE_File_XFR,            180,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
TASK_ENTRY(Monitor_Task,               253,      RMT,  SYS_MODE_GP1, 0x00000000,   0,        -1           )\
/* ---------------------------- Super Slow RMT Tasks ---------------------------------------*/\
TASK_ENTRY(CRC_CBIT_Task,              254,      RMT,  SYS_MODE_GP1, 0x00000000,   500,       S2M(10*60)  )\
TASK_ENTRY(Ram_CBIT_Task,              255,      RMT,  SYS_MODE_GP1, 0x00000000,  1000,       S2M(10*60)  )

//*Negative RMT rates for self-scheduled tasks indicates the number of frames after the current frame 
// the task will run again.

typedef enum { 
  TASK_LIST
  MAX_TASK
}TASK_ID;

typedef INT16 TASK_INDEX;

// Task States
typedef enum
{
    TASK_IDLE      = 0,
    TASK_SCHEDULED,
    TASK_ACTIVE,
    MAX_TASK_STATE
} TASK_STATE;

// Task Types
typedef enum
{
    DT  = 0,
    RMT,
    MAX_TASK_TYPE
} TASK_TYPE;

// Task Modes - use with the MODE_MASK macro 
typedef enum {
  SYS_NORMAL_ID   = 0,
  SYS_SHUTDOWN_ID = 1,
  SYS_DEGRADED_ID = 2,
#ifdef STE_TP
  SYS_TEST_ID     = 3,
#endif
  SYS_MAX_MODE_ID, // NOT A MODE - DO NOT USE
} SYS_MODE_IDS;

typedef enum {
  SYS_MODE_NORMAL   = MODE_MASK(SYS_NORMAL_ID),
  SYS_MODE_SHUTDOWN = MODE_MASK(SYS_SHUTDOWN_ID),
  SYS_MODE_DEGRADED = MODE_MASK(SYS_DEGRADED_ID),
#ifdef STE_TP
  SYS_MODE_TEST     = MODE_MASK(SYS_TEST_ID),
  SYS_MODE_GP0      = ( SYS_MODE_NORMAL | SYS_MODE_DEGRADED | SYS_MODE_SHUTDOWN | SYS_MODE_TEST),
  SYS_MODE_GP1      = ( SYS_MODE_NORMAL | SYS_MODE_DEGRADED | SYS_MODE_TEST),
#else
  SYS_MODE_GP0      = ( SYS_MODE_NORMAL | SYS_MODE_DEGRADED | SYS_MODE_SHUTDOWN ),
  SYS_MODE_GP1      = ( SYS_MODE_NORMAL | SYS_MODE_DEGRADED),
#endif
  SYS_MODE_GP2      = ( SYS_MODE_NORMAL )
} SYSTEM_MODE;

typedef struct taskInfoTag {
    UINT16      priority;
    TASK_TYPE   taskType;
    SYSTEM_MODE modes;
    UINT32      MIFframes;
    UINT32      InitialMif;
    INT32       MIFrate;     // 0 indicates don't schedule event driven, 
                             // Negative indicates schedule N after completion
} TaskInformation;                             

// Task Control Block
typedef struct
{
    UINT32          InitialMif;
    INT32           MifRate;          // 0 indicates don't schedule event driven,      
                                      // Negative indicates schedule N after completion
    UINT32          nextMif;
    UINT32          startExeTime;     //RMT execution start time.
    UINT32          sumExeTime;       //Sum of execution time on interruption 
    UINT32          overrunCount;
    UINT32          interruptedCount; // count of MIF interruptions
} RMT_ATTRIBUTE;

typedef struct
{
    CHAR            Name[TASK_NAME_SIZE];
    TASK_INDEX      TaskID;
    FUNCTION_PTR    Function;
    UINT16          Priority;
    TASK_TYPE       Type;
    UINT32          MIFrames;
    SYSTEM_MODE     modes;               // use with the MODE_MASK macro 
    TASK_STATE      state;
    BOOLEAN         Enabled;
    BOOLEAN         Locked;
    void*           pParamBlock;
    INT32           timingPulse;         // DOUT (LSS0-3) to pulse when task active (-1 off)
    RMT_ATTRIBUTE   Rmt;
    UINT32          lastExecutionTime;
    UINT32          minExecutionTime;
    UINT32          maxExecutionTime;
    UINT32          totExecutionTime;    // Both used for computing the 
    UINT32          totExecutionTimeCnt; // average execution time
    UINT32          maxExecutionMif;
    UINT32          executionCount;
} TCB;

// Task manager Data / Control Structure
typedef struct
{
  UINT32    mif;         // MIF with worst case duty cycle ...
                         // MUST be UINT32 as this tells us when this happened
  UINT32    max;         // Worst case duty cycle %
  UINT32    cumExecTime; // Cumulative duty cycle execution time
                         // in usec for the current MIF
  UINT32    cur;         // Current duty cycle %
} DUTY_CYCLE;

// Task Manager Data / Control Structure
typedef struct
{
  UINT32    cumExecTime;            // Cumulative duty cycle execution time
  UINT32    cur;                    // Current execution time
  UINT32    min;                    // Min execution time
  UINT32    max;                    // Max execution time
  UINT32    exeCount;               // number of times this MIF has executed
} MIF_DUTY_CYCLE;


typedef struct
{
    INT16          nTasks;                           // Total number of tasks [0:MAX_TASKS]
                                                     // -1 = No tasks created
    UINT16         nDtTasks;                         // Number of Deterministic Tasks
    UINT16         nRmtTasks;                        // Number of Rate Monotonic tasks
    TCB*           pTaskList[MAX_TASKS];             // List of TCBs for ALL DT+RMT tasks 
    TCB*           pDtTasks[MAX_MIF+1][MAX_DT_TASKS];// List of DT tasks by MIF, priority 
                                                     //    sorted
    TCB*           pRmtTasks[MAX_RMT_TASKS];         // List of RMT task priority sorted
    BOOLEAN        bDtTaskInProgress;                // TRUE if DT Tasks are being dispatched
    TASK_INDEX     nDtTaskInProgressID;              // ID of the DT task in progress
    UINT8          nDtOverRuns;                      // Number of consecutive times a DT task
                                                     // has overrun the end of the MIF  
    UINT16         nDtRecover;                       // # of DT overruns seen
    UINT16         nRmtOverRuns;                     // Number of consecutive times a RMT task
                                                     // has overrun the end of the MIF  
    BOOLEAN        bInitialized;                     // TRUE when all TM task scheduling has 
                                                     // been completed
    BOOLEAN        bStarted;                         // TRUE when the TM has been started
    UINT8          currentMIF;                       // Current MIF [0 - MAX_MIF]
    SYS_MODE_IDS   systemMode;                       // The current system mode
    UINT16         nNextRmt;                          // Index into Rmt Task list of next
                                                     // RMT task to execute
    DUTY_CYCLE     dutyCycle;                        // MIF duty cycle info
    MIF_DUTY_CYCLE mifDc[MAX_MIF+1];                 // MIF duty cycle info
    UINT32         nMifPeriod;                       // Measured MIF period in uSec
    UINT32         nMifCount;      
} TM_DATA;

#pragma pack(1)
// DT Overrun Log 
typedef struct 
{
  UINT8 count;
  UINT8 nMIF;                           // MIF that over ran
  TASK_INDEX activeTask;                // MIF that over ran
  CHAR  activeTaskName[TASK_NAME_SIZE]; // Task Active right now 
} DT_OVERRUN_LOG;

// TM Health Status 
typedef struct 
{
  UINT32  nDtOverRuns;       // Number of Dt Over Runs 
  UINT32  nRmtOverRuns;      // Number of Rmt Over Runs
} TM_HEALTH_COUNTS; 


#pragma pack() 

/******************************************************************************
                              Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( TASK_MANAGER_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif



/******************************************************************************
                              Package Exports Variables
******************************************************************************/
EXPORT TM_DATA Tm; 

EXPORT UINT16  MaxStackSize;

EXPORT INT32 mifPulse;

extern const TaskInformation taskInfo[MAX_TASK];

///////////////////////////////////////////////////////////////////////////////////////////
// This is only created when included into TaskManager.c
///////////////////////////////////////////////////////////////////////////////////////////
#if defined( TASK_MANAGER_BODY )

#undef  TASK_ENTRY
#define TASK_ENTRY(Name,Pri,Type,Modes,DTrate,RMToffset,RMTrate) {Pri,Type,Modes,DTrate,RMToffset,RMTrate },

const TaskInformation taskInfo[MAX_TASK] = {TASK_LIST};


#endif

/******************************************************************************
                              Package Exports Functions
*******************************************************************************/
EXPORT void       TmInitializeTaskManager   (void);
EXPORT void       TmInitializeTaskDispatcher(void);
EXPORT void       TmStartTaskManager      (void);
EXPORT void       TmDispatchDtTasks       (void);
EXPORT void       TmScheduleRmtTasks      (void);
EXPORT void       TmDispatchRmtTasks      (void);
EXPORT void       TmTaskCreate            (TCB *pTaskInfo);
EXPORT void       TmTaskEnable            (TASK_INDEX TaskID, BOOLEAN State);
EXPORT void       TmScheduleTask          (TASK_INDEX TaskID);
EXPORT void       TmTick                  (UINT32 LastIntLvl);
EXPORT void       TmSetMode               (const SYS_MODE_IDS newMode);


EXPORT TM_HEALTH_COUNTS Tm_GetTMHealthStatus (void); 
EXPORT TM_HEALTH_COUNTS Tm_CalcDiffTMHealthStatus ( TM_HEALTH_COUNTS PrevCnt ); 
EXPORT TM_HEALTH_COUNTS Tm_AddPrevTMHealthStatus  ( TM_HEALTH_COUNTS CurrCnt,
                                                    TM_HEALTH_COUNTS PrevCnt ); 


#ifdef ENV_TEST
EXPORT TASK_INDEX TmGetTaskId             (char* name);
#endif


#endif // TASK_MANAGER_H 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: TaskManager.h $
 * 
 * *****************  Version 65  *****************
 * User: John Omalley Date: 12-11-12   Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR 1142 - Formatting Error
 * 
 * *****************  Version 64  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1099 - Code Review Updates
 * 
 * *****************  Version 63  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1190 Creep Requirements
 * 
 * *****************  Version 62  *****************
 * User: Jim Mood     Date: 9/06/12    Time: 6:04p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Time History implementation changes
 * 
 * *****************  Version 61  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Add Trend to task list
 * 
 * *****************  Version 59  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Changed the Event Action Task to Action Task
 * 
 * *****************  Version 58  *****************
 * User: John Omalley Date: 12-07-13   Time: 3:59p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Output task added to Task Manager
 * 
 * *****************  Version 57  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:05p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:02p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Engine Run
 * 
 * *****************  Version 55  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 *  * the Fast State Machine)
 * 
 * *****************  Version 54  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 5:16p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 * 
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 8/29/10    Time: 12:57p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - Fix Modes for instrumented builds
 * 
 * *****************  Version 52  *****************
 * User: Peter Lee    Date: 8/20/10    Time: 6:13p
 * Updated in $/software/control processor/code/system
 * SCR #807 Update FAST_TxTestID, Monitor_Task and AC_Task_ID to _GRP1
 * 
 * *****************  Version 51  *****************
 * User: Jeff Vahue   Date: 8/03/10    Time: 2:50p
 * Updated in $/software/control processor/code/system
 * SCR #766 - Increase MSI Packet handler priority
 * 
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:45p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 * 
 * *****************  Version 49  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 5:14p
 * Updated in $/software/control processor/code/system
 * SCR 760 MS_GSE_File_XFR task priority for YMODEM timing issues
 * 
 * *****************  Version 48  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR 327 fast transmission test updates
 * 
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 8:08p
 * Updated in $/software/control processor/code/system
 * SCR #698 Code Review Updates. 
 * 
 * *****************  Version 46  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:12p
 * Updated in $/software/control processor/code/system
 * SCR #698 -- Fixes for code review findings
 * 
 * *****************  Version 45  *****************
 * User: Jim Mood     Date: 7/27/10    Time: 7:24p
 * Updated in $/software/control processor/code/system
 * SCR 327 "maintainence" or "txtest" functionality
 * 
 * *****************  Version 44  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #151 Delete Upload FVT row after the good Ground CRC is received
 * from JMessenger
 * 
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:39p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Code Coverage TP
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 6/27/10    Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR# 635 - Missing task info for Uart_Disp_Debug added this in 
 * 
 * *****************  Version 41  *****************
 * User: Contractor3  Date: 6/25/10    Time: 9:46a
 * Updated in $/software/control processor/code/system
 * SCR #662 - Changes based on code review
 * 
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #634 Distribute reg check loading. 
 * 
 * *****************  Version 39  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR 648, change to restore pre-interrupt interrupt level instead of
 * setting interrupt level to zero with __EI()
 * 
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 9:43a
 * Updated in $/software/control processor/code/system
 * SCR 623 - Batch Configuration mode changes
 * 
 * *****************  Version 37  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:15p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR #618 SW-SW Integration problems. Task Mgr updated to allow only one
 * Task Id creation. 
 * 
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * Removed ID translation table. Coding standard fixes
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 5/12/10    Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR# 587 - correct DataManager not being initialized properly
 * 
 * *****************  Version 32  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 4/23/10    Time: 6:52p
 * Updated in $/software/control processor/code/system
 * SCR #563 - clean up System Log ID values and insert comment into
 * TaskManager.h
 * 
 * *****************  Version 30  *****************
 * User: Contractor2  Date: 4/23/10    Time: 2:08p
 * Updated in $/software/control processor/code/system
 * Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM Tests
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 4/06/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 525 - Implement periodic CBIT CRC test.
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 4/01/10    Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR# 520 - ahh fix priority after the re-ordering
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 4/01/10    Time: 5:16p
 * Updated in $/software/control processor/code/system
 * SCR# 520 - fix task data table ordering to match enumeration sequence.
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/30/10    Time: 9:54a
 * Updated in $/software/control processor/code/system
 * SCR# 520 - WD Tasks execute at 1Hz 500Ms apart in time SRS-2231
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 5:56p
 * Updated in $/software/control processor/code/system
 * SCR# 484 - MIF pulse  on any MIF#, not just MIF0
 * 
 * *****************  Version 23  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:05p
 * Updated in $/software/control processor/code/system
 * SCR 67 Interrupted SPI Access (Multiple / Nested SPI Access)
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 1:26p
 * Updated in $/software/control processor/code/system
 * SCR# 463 - WD Tasks are now RMT to achieve required interval
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 2/24/10    Time: 12:40p
 * Updated in $/software/control processor/code/system
 * SCR# 463 - 2 WD Tasks to pulse 500 ms apart
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 12:30p
 * Updated in $/software/control processor/code/system
 * SCR #449 - Add control of system mode via user command, display system
 * mode, clean up sys.get.task.x processing, provide GHS target debugging
 * hook to bypass DT overrun code.
 * 
 * *****************  Version 17  *****************
 * User: Jeff Vahue   Date: 2/11/10    Time: 12:41p
 * Updated in $/software/control processor/code/system
 * SCR# 445 - for verification set min/max priorities to specified limits
 * for both DT and RMT task groups
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR# 405 - Add DT overrun Log on each occurance, save actual MIF of max
 * Duty Cycle vs. 0-32, so we can tell if issues happen at startup.
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 1:40p
 * Updated in $/software/control processor/code/system
 * SCR# 434 - Pulse LSSx on task execution, clean up sys cmds and
 * processing
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:15p
 * Updated in $/software/control processor/code/system
 * SCR# 405
 * 
 * *****************  Version 13  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:57p
 * Updated in $/software/control processor/code/system
 * SCR# 400
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #315 SEU counts for TM Health Counts 
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * Updated for sensor processing
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 4:37p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:19p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 ***************************************************************************/

