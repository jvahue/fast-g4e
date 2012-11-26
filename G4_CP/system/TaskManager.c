#define TASK_MANAGER_BODY

/******************************************************************************
            Copyright (C) 2009-2011 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:       TaskManager.c

   Description: Executive Task Manager functions
   
   Name                       Description
   ------------------------   --------------------------------------------------
   TmInitializeTaskManager    Initialize the Task Manager's status & control
                              data
   TmStartTaskManager         Start the Task Manager, PIT and Idle Task
   TmTaskCreate               Create a new DT or RMT task
   TmInitializeTaskDispatcher Initialize the DT & RMT task lists per MIF
   TmDispatchDtTasks          Schedule & dispatch DT tasks per MIF
   TmScheduleRmtTasks         Schedule RMT tasks per MIF
   TmDispatchRmtTasks         Dispatch RMT tasks per MIF
   TmTaskEnable               Enable / disable  a task to be scheduled
   TmScheduleTask             Force a specific RMT task for dispatch ASAP
  
 VERSION
      $Revision: 65 $  $Date: 12-11-13 5:46p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "assert.h"
#include "StdDefs.h"
#include "TaskManager.h"
#include "TTMR.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "LogManager.h"
#include "DIO.h"

#include "TestPoints.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define PULSE(x,s) if (x>=0 && x <=3) {DIO_SetPin((DIO_OUTPUT)x, s);}

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
#pragma pack(1)
typedef struct rmtOverrunTag {
  UINT16 taskId;
  CHAR   taskName[TASK_NAME_SIZE];
} RMT_OVERRUN_LOG;
#pragma pack()

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static TCB TcbArray[MAX_TASKS]; //TCBs to allocate from in TaskCreate()

static TM_HEALTH_COUNTS TMHealthCounts;

static SYS_MODE_IDS pendingSystemMode; // pending system mode, next MIF

// controls the transition from mode to mode
static BOOLEAN systemModeTransitions[SYS_MAX_MODE_ID][SYS_MAX_MODE_ID];

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************
 * Function:    TmInitializeTaskManager
 *
 * Description: Initialize the Task Manager's status and control and data
 *               information required to perform Task Manager functions.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmInitializeTaskManager (void)
{
    UINT16 i;

    memset( &Tm, 0, sizeof(Tm));

    Tm.nTasks            = -1;      // First task to be created as Task ID 0
    Tm.nDtTasks          = 0;
    Tm.nRmtTasks         = 0;
    Tm.bDtTaskInProgress = FALSE;
    Tm.nDtOverRuns       = 0;
    Tm.nRmtOverRuns      = 0;
    Tm.bInitialized      = FALSE;
    Tm.bStarted          = FALSE;
    Tm.nNextRmt           = 0;
    Tm.currentMIF        = MAX_MIF;
    Tm.dutyCycle.mif     = 0;
    Tm.dutyCycle.max     = 0;
    Tm.dutyCycle.cumExecTime = 0;
    Tm.dutyCycle.cur     = 0;
    Tm.nMifCount          = 0;     //Frames executed since power on.
    Tm.nMifPeriod         = TTMR_TASK_MGR_TICK_INTERRUPT_INTERVAL_uS;

    memset ( &TMHealthCounts, 0, sizeof(TM_HEALTH_COUNTS) );

    Tm.systemMode = SYS_NORMAL_ID;
    pendingSystemMode = SYS_NORMAL_ID;

    memset( systemModeTransitions, 0, sizeof(systemModeTransitions));

    // enable the valid mode transitions
    // Normal to Shutdown
    systemModeTransitions[SYS_NORMAL_ID][SYS_SHUTDOWN_ID] = TRUE;
    // Shutdown to Normal ??
    systemModeTransitions[SYS_SHUTDOWN_ID][SYS_NORMAL_ID] = TRUE;
    // Normal to Degraded
    systemModeTransitions[SYS_NORMAL_ID][SYS_DEGRADED_ID] = TRUE;

#ifdef STE_TP
    // Normal to Test Mode and back
    systemModeTransitions[SYS_NORMAL_ID][SYS_TEST_ID] = TRUE;
    systemModeTransitions[SYS_TEST_ID][SYS_NORMAL_ID] = TRUE;
#endif

    // Init the MIF timing info
    for (i = 0; i < MAX_MIF+1; ++i)
    {
      Tm.mifDc[i].min = 0xFFFFFFFF;
    }

    // no MIF0 pulse on LSS0
    mifPulse = -1;

}   // End of TmInitializeTaskManager()

/*****************************************************************************
 * Function:    TmStartTaskManager
 *
 * Description: Start the Task Manager running via the PIT.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       The Idle Task process is the lowest level Executive task.
 *              Runs in the background after all DT and RMT task have run
 *              and there are no other tasks to dispatch..
 *
 ****************************************************************************/
void TmStartTaskManager (void)
{

    Tm.bStarted = TRUE;             //Set enabled flag
    // TTMR_Init();                 //Setup and start the Task Timer
    TTMR_SetTaskManagerTickFunc(TmTick);
}

/*****************************************************************************
 * Function:    TmTaskCreate
 *
 * Description: Create a DT or RMT task for future scheduling and dispatching
 *              by the Task Dispatcher.
 *
 * Parameters:  pTaskInfo - Pointer to TCB structure with task information
 *
 * Returns:     none
 *
 * Notes:       Task can not be created after the Task Manager initialization
 *              is complete.
 *
 ****************************************************************************/
void TmTaskCreate (TCB* pTaskInfo)
{
    TCB* pTCB;
    TASK_INDEX  TaskID;

    ASSERT_MESSAGE(!Tm.bInitialized,
      "Task created after initialization (%s).", pTaskInfo->Name);
    ++Tm.nTasks;
    ASSERT_MESSAGE(Tm.nTasks < MAX_TASKS,
      "Too many tasks created: %d Max: %d",Tm.nTasks, MAX_TASKS);

    TaskID = pTaskInfo->TaskID;
    ASSERT_MESSAGE(Tm.pTaskList[TaskID] == NULL,
      "Task already created: %d Name: %s", TaskID, pTaskInfo->Name);

    pTCB = (TCB*)&TcbArray[Tm.nTasks];

    // Fill in TCB info
    strncpy_safe (pTCB->Name, sizeof(pTCB->Name), pTaskInfo->Name,_TRUNCATE);
    pTCB->TaskID      = pTaskInfo->TaskID;
    pTCB->Function    = pTaskInfo->Function;
    pTCB->modes       = pTaskInfo->modes;
    pTCB->Priority    = pTaskInfo->Priority;
    pTCB->Type        = pTaskInfo->Type;
    pTCB->state       = TASK_IDLE;
    pTCB->Enabled     = pTaskInfo->Enabled;
    pTCB->Locked      = pTaskInfo->Locked;
    pTCB->pParamBlock = pTaskInfo->pParamBlock;
    pTCB->timingPulse = -1;

    if (pTaskInfo->Type == DT)
    {
        ASSERT (++Tm.nDtTasks < MAX_DT_TASKS);
        pTCB->MIFrames = pTaskInfo->MIFrames;
    }
    else if (pTaskInfo->Type == RMT)
    {
        ASSERT(++Tm.nRmtTasks < MAX_RMT_TASKS);
        pTCB->Rmt.InitialMif       = pTaskInfo->Rmt.InitialMif;
        pTCB->Rmt.MifRate          = pTaskInfo->Rmt.MifRate;
        pTCB->Rmt.nextMif          = 0;
        pTCB->Rmt.overrunCount     = 0;
        pTCB->Rmt.interruptedCount = 0;
    }

    pTCB->lastExecutionTime    = 0;
    pTCB->minExecutionTime     = UINT32MAX;
    pTCB->maxExecutionTime     = 0;
    pTCB->maxExecutionMif      = 0;
    pTCB->executionCount       = 0;
    pTCB->totExecutionTime     = 0;
    pTCB->totExecutionTimeCnt  = 0;     // NOTE: protection is needed for divide by zero

    // Add to task list
    Tm.pTaskList[TaskID] = pTCB;
}


/*****************************************************************************
 * Function:    TmInitializeTaskDispatcher
 *
 * Description: Initialize the DT and RMT task lists for each MIF used for
 *              scheduling and dispatching of tasks.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmInitializeTaskDispatcher (void)
{
    UINT16 PriorityIndex[ MAX_MIF+1][ MAX_TASKS];
    UINT16 ElementCount [ MAX_MIF+1];

    UINT32 i;
    UINT32 RmtTaskCount;
    UINT32 Mif;
    UINT32 MiFrame;
    UINT32 InsertAt;
    UINT16 TaskPriority;
    TCB*   pTask;

    //------------------------------------------------------------------------
    // Initialize the DT and RMT Task Lists for all task created in the system
    //------------------------------------------------------------------------
    if (Tm.nTasks > -1)
    {
        //--------------------------------------------------------------------
        // Clear out arrays used to order the Task Lists by priority
        //--------------------------------------------------------------------
        memset(ElementCount,  0, sizeof(ElementCount ));
        memset(PriorityIndex, 0, sizeof(PriorityIndex));
        RmtTaskCount = 0;

        //--------------------------------------------------------------------
        // Place each task in the system in either the DT Task list or the RMT
        // Task list in descending order of priority
        //--------------------------------------------------------------------
        for ( i = 0; i < MAX_TASKS; i++)
        {
            pTask = Tm.pTaskList[i];
            if ( pTask != NULL)
            {
                //------------------------------------------------------------
                // Process DT Tasks
                //------------------------------------------------------------
                if (pTask->Type == DT)
                {
                    MiFrame = pTask->MIFrames;
                    for ( Mif = 0; (Mif <= MAX_MIF) && (MiFrame > 0); Mif++)
                    {
                        // Is this task enable for this MIF?
                        if ( MiFrame & 1 )
                        {
                            ElementCount[Mif]++;  // One more task in this MIF

                            // Insert task into list based on priority
                            InsertAt = ElementCount[Mif] - 1;
                            TaskPriority = pTask->Priority;
                            while (InsertAt > 0 &&
                                  TaskPriority < PriorityIndex[Mif][InsertAt-1])
                            {
                                // Move task down one in execution order
                                Tm.pDtTasks[Mif][InsertAt] =
                                    Tm.pDtTasks[Mif][InsertAt-1];

                                // Move task priority down one slot
                                PriorityIndex[Mif][InsertAt] =
                                    PriorityIndex[Mif][InsertAt-1];

                                // Move up the priority Index
                                 InsertAt--;
                            }    // End while

                            // Insert Task descriptor pointer
                            Tm.pDtTasks[Mif][InsertAt] = Tm.pTaskList[i];

                            // Save task priority at this index
                            PriorityIndex[Mif][InsertAt] = TaskPriority;
                        }    // End if ( minorFrames & 1 )

                        // Position next minor frame enable bit
                        MiFrame = MiFrame >> 1;
                    }    // End for (each MIF)

                }    // End if (DT task)

                //------------------------------------------------------------
                // Process RMT Tasks
                //------------------------------------------------------------
                else
                {
                    RmtTaskCount++;

                    InsertAt = RmtTaskCount - 1;
                    TaskPriority = pTask->Priority;

                    // Insert the task into the RMT task list based on
                    // priority
                    while ( InsertAt > 0 &&
                    TaskPriority < Tm.pRmtTasks[InsertAt-1]->Priority)
                    {
                        Tm.pRmtTasks[InsertAt] = Tm.pRmtTasks[InsertAt-1];
                        InsertAt--;
                    }
                    Tm.pRmtTasks[InsertAt] = pTask;

                    // Prepare for the first activation time for this task
                    pTask->Rmt.nextMif = pTask->Rmt.InitialMif;
                }
            }    // End if (pTask != NULL)
        }    // End for (all tasks)

        Tm.bInitialized = TRUE;

    }    // End if (Tm.nTasks > 0)
}

/*****************************************************************************
* Function:    TmTick
*
* Description: Function to be called by a periodic timer.  This runs the
*              task manager DT and RMT task dispatching
*
* Parameters:  [in] LastIntLvl: The core interrupt level before the timer
*                   interrupt occured.
*
* Returns:     void
*
* Notes:       None.
*
****************************************************************************/
void TmTick (UINT32 LastIntLvl)
{
    UINT32 MifStartTime;

    // Record the start time for this frame.
    // It is used for determining the execution time of interrupted RMT tasks
    MifStartTime = TTMR_GetHSTickCount();

    if (Tm.bStarted)
    {
        // Restore interrupt level to pre-interrupt level
        __RIR(LastIntLvl);

        // install the new mode on a MIF boundary, any active RMT will complete
        // even if the mode has changed
        Tm.systemMode = pendingSystemMode;

        // Check for Task Manager DT Task overrun
        if ( Tm.bDtTaskInProgress)
        {
            DT_OVERRUN_LOG dtOverrun;

            if (++Tm.nDtOverRuns >= TPU( MAX_DT_OVERRUNS, eTpNoDtOverrun))
            {
                // More than 3 sequential overruns
                FATAL("DT Overrun %d - In-Progress (MIF: %d ID: %d Name: %s )",
                    Tm.nDtOverRuns, Tm.currentMIF, Tm.nDtTaskInProgressID,
                    Tm.pTaskList[Tm.nDtTaskInProgressID]->Name);
            }

            TMHealthCounts.nDtOverRuns++;
            ++Tm.nDtRecover;

#if STE_TP
            // allows us to disable & enable recording Overruns when instrumented to ensure
            // System Log file match expected results
            // if we set eTpNoDtOverrun = 3 the system operates as normal
            // otherwise we will not record DT overruns unless we set eTpNoDtOverrun = 3
            if ( TPU( 0, eTpNoDtOverrun) == MAX_DT_OVERRUNS)
#endif
            {
                // record the fact that we had an overrun
                dtOverrun.count = Tm.nDtOverRuns;
                dtOverrun.nMIF = Tm.currentMIF;
                dtOverrun.activeTask = Tm.nDtTaskInProgressID;
                strncpy_safe( dtOverrun.activeTaskName,TASK_NAME_SIZE,
                    Tm.pTaskList[Tm.nDtTaskInProgressID]->Name,_TRUNCATE);
                LogWriteSystem( SYS_TASK_MAN_DT_OVERRUN, LOG_PRIORITY_LOW,
                    &dtOverrun, sizeof( dtOverrun), NULL);
            }

            // Display a debug message about the overrun
            GSE_DebugStr(NORMAL, TRUE,
              "DT Overrun %d - In-Progress (MIF: %d ID: %d Name: %s )",
              Tm.nDtOverRuns, Tm.currentMIF, Tm.nDtTaskInProgressID,
              Tm.pTaskList[Tm.nDtTaskInProgressID]);
        }
        else
        {
            // see if a RMT is active and accumulate its execution time
            if ( Tm.nNextRmt < Tm.nRmtTasks )
            {
                UINT8 rmt = (UINT8)Tm.nNextRmt;

                if ( Tm.pRmtTasks[rmt]->state == TASK_ACTIVE)
                {
                    Tm.pRmtTasks[rmt]->Rmt.sumExeTime +=
                     (MifStartTime - Tm.pRmtTasks[rmt]->Rmt.startExeTime)/TTMR_HS_TICKS_PER_uS;
                    ++Tm.pRmtTasks[rmt]->Rmt.interruptedCount;
                }
            }

            //-----------------------------------------------------------------------
            // Clear In Progress flag for DT Task overrun detection
            Tm.bDtTaskInProgress = TRUE;

            do
            {
                // update time and correct overrun count
                CM_RTCTick();
                if ( Tm.nDtRecover > 0)
                {
                    Tm.nDtRecover--;
                }

                // Dispatch Tasks for this MIF
                TmDispatchDtTasks();

                //Increment the number of frames executed since power on
                Tm.nMifCount++;
            } while ( Tm.nDtRecover > 0);

            // Reset Consecutive DT Overrun counter as we are done with DT tasks now
            Tm.nDtOverRuns = 0;

            // Clear In Progress flag for DT Task overrun detection
            Tm.bDtTaskInProgress = FALSE;

            //-----------------------------------------------------------------------
            // Process RMT Tasks
            TmScheduleRmtTasks();
            TmDispatchRmtTasks();
        }
    }
}

/*****************************************************************************
 * Function:    TmDispatchDtTasks
 *
 * Description: Schedule and dispatch all enabled DT tasks based on their
 *              priority for each MIF.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmDispatchDtTasks (void)
{
    UINT8   taskIndex;
    UINT32  taskTime;
    UINT32  executionTime;
    UINT32  mifStart;
    TCB**   pTaskSelect;
    TCB*    pTask;

    mifStart = TTMR_GetHSTickCount();

    //------------------------------------------------------------------------
    // Increment the current MIF counter with wraparound from MAX_MIF to 0
    //------------------------------------------------------------------------
    if (++Tm.currentMIF > MAX_MIF)
    {
        Tm.currentMIF = 0;
    }

    // Pulse LSS0 on the selected MIF
    if (Tm.currentMIF == mifPulse)
    {
      PULSE( 0, DIO_SetHigh);
    }
    else if ( mifPulse != -1)
    {
      PULSE( 0, DIO_SetLow);
    }

    //------------------------------------------------------------------------
    // DT Task Dispatcher
    //------------------------------------------------------------------------
    Tm.dutyCycle.cumExecTime = 0;

    // Dispatch each enabled DT Task for this MIF
    pTaskSelect = Tm.pDtTasks[Tm.currentMIF];
    for (taskIndex = 0; *pTaskSelect != NULL && taskIndex < MAX_DT_TASKS; taskIndex++)
    {
        pTask = *pTaskSelect;

        // if the DT task runs in the MIF and Mode
        if (pTask->Enabled && ((UINT32)pTask->modes & MODE_MASK((UINT32)Tm.systemMode)))
        {
            // Call the task's function while measuring its execution time
            ++pTask->executionCount;
            Tm.nDtTaskInProgressID = pTask->TaskID;
            PULSE(pTask->timingPulse, DIO_SetHigh);
            taskTime = TTMR_GetHSTickCount();               //Record task start time
            pTask->Function(pTask->pParamBlock);            // Call the function
            taskTime = (TTMR_GetHSTickCount() - taskTime)/
                            TTMR_HS_TICKS_PER_uS;           //Record task run time
            PULSE(pTask->timingPulse, DIO_SetLow);
            executionTime = taskTime;

            Tm.dutyCycle.cumExecTime += executionTime;

            // Calculate and save execution time stats for this task
            pTask->lastExecutionTime = executionTime;

            // Add to total execution time, reset total values when the
            // time rolls over.
            if(pTask->totExecutionTime+executionTime < pTask->totExecutionTime)
            {
              pTask->totExecutionTime = executionTime;
              pTask->totExecutionTimeCnt = 1;
            }
            else
            {
              pTask->totExecutionTime += executionTime;
              pTask->totExecutionTimeCnt++;
            }

            //Do min/max execution time calculations
            if (executionTime > pTask->maxExecutionTime)
            {
                pTask->maxExecutionTime  = executionTime;
                pTask->maxExecutionMif   = pTask->executionCount;
            }
            if (executionTime < pTask->minExecutionTime)
            {
                pTask->minExecutionTime = executionTime;
            }
        }    // End if (task enabled)

        // go to the next task in the list for this MIF
        pTaskSelect++;

    }   // End for (each task)

    Tm.mifDc[Tm.currentMIF].cur = (TTMR_GetHSTickCount() - mifStart);

    // Handle condition when cumulative execution time rolls over.
    if (Tm.mifDc[Tm.currentMIF].cumExecTime + Tm.mifDc[Tm.currentMIF].cur <
        Tm.mifDc[Tm.currentMIF].cumExecTime)
    {
      Tm.mifDc[Tm.currentMIF].cumExecTime = Tm.mifDc[Tm.currentMIF].cur;
      Tm.mifDc[Tm.currentMIF].exeCount = 1;
    }
    else
    {
      Tm.mifDc[Tm.currentMIF].cumExecTime += Tm.mifDc[Tm.currentMIF].cur;
      ++Tm.mifDc[Tm.currentMIF].exeCount;
     // GSE_DebugStr(NORMAL,TRUE,"MIF: %d Cur: %d Cum Time: %d", Tm.CurrentMIF, 
     // Tm.mifDc[Tm.CurrentMIF].Cur, Tm.mifDc[Tm.CurrentMIF].CumExecTime);
    }


    if (Tm.mifDc[Tm.currentMIF].cur > Tm.mifDc[Tm.currentMIF].max)
    {
      Tm.mifDc[Tm.currentMIF].max = Tm.mifDc[Tm.currentMIF].cur;
    }
    else if (Tm.mifDc[Tm.currentMIF].cur < Tm.mifDc[Tm.currentMIF].min)
    {
      Tm.mifDc[Tm.currentMIF].min = Tm.mifDc[Tm.currentMIF].cur;
    }
}

/*****************************************************************************
 * Function:    TmScheduleRmtTasks
 *
 * Description: Schedule all enabled RMT tasks for dispatching based on the
 *              priority for the current MIF.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmScheduleRmtTasks (void)
{
    UINT16  i;
    UINT32  TaskTime;
    UINT32  ExecutionTime;
    RMT_OVERRUN_LOG rmtLog;

    // Track execution time for this section for overall MIF duty cycle
    // calculation
    TaskTime = TTMR_GetHSTickCount();

    for ( i = 0; i < Tm.nRmtTasks; i++)
    {
        // schedule all tasks that are enable and ready to run
        if (Tm.pRmtTasks[i]->Rmt.nextMif <= Tm.nMifCount &&
            Tm.pRmtTasks[i]->Rmt.MifRate != EVENT_DRIVEN_RMT)
        {
            // reschedule True RMT tasks
            if (Tm.pRmtTasks[i]->Rmt.MifRate > EVENT_DRIVEN_RMT)
            {
                Tm.pRmtTasks[i]->Rmt.nextMif += (UINT32)Tm.pRmtTasks[i]->Rmt.MifRate;
            }
            // Help self-scheduling RMTs that are disabled keep time
            else if ( !Tm.pRmtTasks[i]->Enabled)
            {
              // See if it is enabled, if not we will move it along until it schedules itself
              // minus because MifRate is negative
              Tm.pRmtTasks[i]->Rmt.nextMif -= (UINT32)Tm.pRmtTasks[i]->Rmt.MifRate;
            }

            if (Tm.pRmtTasks[i]->Enabled)
            {
                if (Tm.pRmtTasks[i]->state == TASK_IDLE)
                {
                    if ( i < Tm.nNextRmt)
                    {
                        Tm.nNextRmt = (UINT16)i;
                    }
                    Tm.pRmtTasks[i]->state = TASK_SCHEDULED;
                }
                else
                {
                    if ( Tm.pRmtTasks[i]->Rmt.MifRate > EVENT_DRIVEN_RMT)
                    {
                        // count this task's RMT overruns
                        ++Tm.pRmtTasks[i]->Rmt.overrunCount;
                        // count all RMT over runs
                        ++Tm.nRmtOverRuns;

                        if (Tm.pRmtTasks[i]->Rmt.overrunCount == 1)
                        {
                            strncpy_safe( rmtLog.taskName,TASK_NAME_SIZE,
                                     Tm.pRmtTasks[i]->Name,_TRUNCATE);
                            rmtLog.taskId = Tm.pRmtTasks[i]->TaskID;
                            LogWriteSystem( SYS_TASK_MAN_RMT_OVERRUN, LOG_PRIORITY_LOW,
                                    &rmtLog, sizeof( rmtLog), NULL);
                            // Display a debug message about the overrun
                            GSE_DebugStr(NORMAL,TRUE,"RMT Overrun - ID: %02d Name: %s\r\n",
                                    rmtLog.taskId, rmtLog.taskName);
                        }
                    }
                }
            }   // End if (enabled)
        }   // End if (time to run)
    }   // End for(each RMT task)

    TaskTime = TTMR_GetHSTickCount() - TaskTime;

    ExecutionTime = TaskTime / TTMR_HS_TICKS_PER_uS;
    Tm.dutyCycle.cumExecTime += ExecutionTime;
}

/*****************************************************************************
 * Function:    TmDispatchRmtTasks
 *
 * Description: Dispatch all RMT tasks scheduled for the current MIF.
 *
 * Parameters:  void
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmDispatchRmtTasks (void)
{
    UINT8  CurTaskIndex;
    UINT32  ExecutionTime;
    UINT32  DutyCycle;
    TCB*    pTask;
    UINT32  IntSave;

    IntSave = __DIR();
    while ( Tm.nNextRmt < Tm.nRmtTasks)
    {
        CurTaskIndex = (UINT8)Tm.nNextRmt;
        pTask = Tm.pRmtTasks[CurTaskIndex];

        // If next task to be dispatched is already active its execution was
        // interrupted by a new MIF.  Return to it to resume execution at the
        // point it was interrupted
        if (pTask->state == TASK_ACTIVE)
        {
            // Resume the RMT task
            pTask->Rmt.startExeTime = TTMR_GetHSTickCount();
            __RIR(IntSave);
            return;
        }

        if (pTask->state == TASK_SCHEDULED )
        {
          if ( pTask->modes & MODE_MASK(Tm.systemMode))
          {
            // Indicate that the task is now active
            pTask->state = TASK_ACTIVE;

            // Execute the RMT task
            ++pTask->executionCount;
            __RIR(IntSave);

            //Save the start time in a globally accessible location.  This
            //time will be adjusted if overruns occur, maintaining only the
            //total execution time of this RMT.
            pTask->Rmt.sumExeTime = 0;
            PULSE(pTask->timingPulse, DIO_SetHigh);
            pTask->Rmt.startExeTime = TTMR_GetHSTickCount();

            // Call the function
            pTask->Function(pTask->pParamBlock);

            //Disable Interrupts for timing calculation, restored using the
            //int level at the top of this function.
            __DI();

            //Calculate execution time.
            ExecutionTime =
              (TTMR_GetHSTickCount() - pTask->Rmt.startExeTime)/TTMR_HS_TICKS_PER_uS;

            PULSE(pTask->timingPulse, DIO_SetLow);

            // Indicate the task has completed and is now idle
            pTask->state = TASK_IDLE;

            Tm.dutyCycle.cumExecTime += (pTask->Rmt.sumExeTime + ExecutionTime);

            // check if this is a self scheduling task
            if (pTask->Rmt.MifRate < EVENT_DRIVEN_RMT)
            {
                // minus because MifRate is negative
                pTask->Rmt.nextMif -= (UINT32)pTask->Rmt.MifRate;
            }

            // Calculate and save execution time stats for this task
            pTask->lastExecutionTime = ExecutionTime;

            // Add to total execution time, reset total values when the
            // time rolls over.
            if(pTask->totExecutionTime+ExecutionTime < pTask->totExecutionTime)
            {
              pTask->totExecutionTime = ExecutionTime;
              pTask->totExecutionTimeCnt = 1;
            }
            else
            {
              pTask->totExecutionTime += ExecutionTime;
              pTask->totExecutionTimeCnt++;
            }

            // check max execution time
            if (ExecutionTime > pTask->maxExecutionTime)
            {
                pTask->maxExecutionTime  = ExecutionTime;
                pTask->maxExecutionMif   = pTask->executionCount;
            }

            // check min execution time
            if (ExecutionTime < pTask->minExecutionTime)
            {
                pTask->minExecutionTime = ExecutionTime;
            }
            __RIR(IntSave);
          }
          else
          {
            // Indicate the task has completed even though we did not run it
            // ... as it was not allowed in this mode
            pTask->state = TASK_IDLE;
          }
        }   // End if (Task scheduled)

        if ( CurTaskIndex == Tm.nNextRmt)
        {
            Tm.nNextRmt++;
        }

    }   // End while ( Tm.NextRmt < Tm.nRmtTasks)

    // Calculate MIF duty cycle (% loading)
    DutyCycle = Tm.dutyCycle.cumExecTime;
    if (DutyCycle > Tm.dutyCycle.max)
    {
        Tm.dutyCycle.max = DutyCycle;
        Tm.dutyCycle.mif = Tm.nMifCount;
    }
    Tm.dutyCycle.cur     = DutyCycle;

    __RIR(IntSave);
}

/*****************************************************************************
 * Function:    TmTaskEnable
 *
 * Description: Enable / disable the specified task to be scheduled for
 *              dispatching in the next MIF the task is assigned to.
 *
 * Parameters:  TaskID - Task Identifier Number
 *              State  - Enable / disable state
 *
 * Returns:     void
 *
 * Notes:       The task must not be locked to be able to change its enable
 *              / disable state.
 *
 ****************************************************************************/
void TmTaskEnable (TASK_INDEX TaskID, BOOLEAN State)
{
  if ( (Tm.pTaskList[TaskID] != NULL) && (!Tm.pTaskList[TaskID]->Locked))
  {
      Tm.pTaskList[TaskID]->Enabled = State;
  }
}

/*****************************************************************************
 * Function:    TmScheduleTask
 *
 * Description: Manually force a specific RMT task for dispatch as soon as
 *              possible.
 *
 * Parameters:  TaskID - Task Identifier Number
 *
 * Returns:     void
 *
 * Notes:       None.
 *
 ****************************************************************************/
void TmScheduleTask (TASK_INDEX TaskID)
{
  if (Tm.pTaskList[TaskID] != NULL)
  {
    ASSERT_MESSAGE(Tm.pTaskList[TaskID]->Rmt.MifRate == EVENT_DRIVEN_RMT,
                   "Attempt to schedule Task [%s]",
                   Tm.pTaskList[TaskID]->Name);

    if ( Tm.pTaskList[TaskID]->Enabled )
    {
        Tm.pTaskList[TaskID]->state = TASK_SCHEDULED;
        Tm.nNextRmt = 0;
    }
  }
}

/*****************************************************************************
* Function:    TmSetMode
*
* Description: Request a new mode.  This function verifies the transition is
* allowed.  If it is then the new mode is copied to the pending mode and on
* the next MIF the system mode will be set to the pendingMode.
*
* Parameters:  newMode (i) - the request new system mode
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void TmSetMode(const SYS_MODE_IDS newMode)
{
    if ( newMode < SYS_MAX_MODE_ID &&
         systemModeTransitions[Tm.systemMode][newMode])
    {
        pendingSystemMode = newMode;
    }
}

/*****************************************************************************
 * Function:    Tm_GetTMHealthStatus
 *
 * Description: Returns current TM Health Status Counts
 *
 * Parameters:  none
 *
 * Returns:     Current TMHealtCounts status
 *
 * Notes:       none
 *
 ****************************************************************************/
TM_HEALTH_COUNTS Tm_GetTMHealthStatus (void)
{
  TMHealthCounts.nRmtOverRuns = Tm.nRmtOverRuns;

  return (TMHealthCounts);
}

/*****************************************************************************
 * Function:    Tm_CalcDiffTMHealthStatus
 *
 * Description:  Calculate the difference in TM Health Counts to for PWEH SEU
 *               (Used to support determining SEU count during data capture)
 *
 * Parameters:   PrevCnt - Initial count value
 *
 * Returns:      Difference in TM_HEALTH_COUNTS from (Current - PrevCnt)
 *
 * Notes:        None
 *
 ****************************************************************************/
TM_HEALTH_COUNTS Tm_CalcDiffTMHealthStatus (TM_HEALTH_COUNTS PrevCnt)
{
  TM_HEALTH_COUNTS diffCount;
  TM_HEALTH_COUNTS currCount;


  currCount = Tm_GetTMHealthStatus();

  diffCount.nDtOverRuns = currCount.nDtOverRuns - PrevCnt.nDtOverRuns;
  diffCount.nRmtOverRuns = currCount.nRmtOverRuns - PrevCnt.nRmtOverRuns;

  return ( diffCount );
}


/******************************************************************************
 * Function:     Tm_AddPrevTMHealthStatus
 *
 * Description:  Add TM Health Counts to support PWEH SEU
 *               (Used to support determining SEU count during data capture)
 *
 * Parameters:   CurrCnt - Current count value
 *               PrevCnt - Previous count value
 *
 * Returns:      Add TM_HEALTH_COUNTS from (CurrCnt + PrevCnt)
 *
 * Notes:        None
 *
 *****************************************************************************/
TM_HEALTH_COUNTS Tm_AddPrevTMHealthStatus ( TM_HEALTH_COUNTS CurrCnt,
                                            TM_HEALTH_COUNTS PrevCnt )
{
  TM_HEALTH_COUNTS addCount;

  addCount.nDtOverRuns = CurrCnt.nDtOverRuns + PrevCnt.nDtOverRuns;
  addCount.nRmtOverRuns = CurrCnt.nRmtOverRuns + PrevCnt.nRmtOverRuns;

  return ( addCount );
}


#ifdef ENV_TEST
/*vcast_dont_instrument_start*/
/*****************************************************************************
* Function:    TmGetTaskId
*
* Description: Return the TaskIndex of a task with the name specified
* Parameters:  name - that task name
*
* Returns:     taskId if found, (MAX_TASKS+1) if not found
*
* Notes:       None.
*
****************************************************************************/
TASK_INDEX TmGetTaskId(char* name)
{
    TASK_INDEX i;
    TASK_INDEX id = (MAX_TASKS+1);

    for ( i= 0; i < MAX_TASKS; ++i)
    {
        if (strncmp( name, Tm.pTaskList[i]->Name, TASK_NAME_SIZE) == 0)
        {
            id = Tm.pTaskList[i]->TaskID;
            break;
        }
    }

    return id;
}
/*vcast_dont_instrument_end*/
#endif

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*************************************************************************
 *  MODIFICATIONS
 *    $History: TaskManager.c $
 * 
 * *****************  Version 65  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 64  *****************
 * User: John Omalley Date: 12-11-12   Time: 2:09p
 * Updated in $/software/control processor/code/system
 * SCR 1099 - Code Review Updates
 *
 * *****************  Version 63  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:19p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 61  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 59  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:51p
 * Updated in $/software/control processor/code/system
 * SCR #1099 Task Statistics SCR # 1087 Update copyright
 *
 * *****************  Version 58  *****************
 * User: Jeff Vahue   Date: 9/22/11    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR# 1077 - inhibit DT over logs, correctly
 *
 * *****************  Version 57  *****************
 * User: Jeff Vahue   Date: 9/22/11    Time: 3:51p
 * Updated in $/software/control processor/code/system
 * SCR# 1077 - disallow recording DT overruns when instrumented, which
 * also stops resets ... yea I like it.
 *
 * *****************  Version 56  *****************
 * User: Jeff Vahue   Date: 9/22/11    Time: 3:11p
 * Updated in $/software/control processor/code/system
 * SCR# 1077 - Test Points
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 9/22/11    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR# 1077 - Test Point addition to inhibit DT overrun when instrumented
 *
 * *****************  Version 54  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 5:13p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 *
 * *****************  Version 53  *****************
 * User: Jeff Vahue   Date: 7/19/10    Time: 6:38p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Code Coverage TP
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 7/19/10    Time: 5:24p
 * Updated in $/software/control processor/code/system
 * SCR 708 - Made DT Overrun count >= 3
 *
 * *****************  Version 51  *****************
 * User: Jim Mood     Date: 6/28/10    Time: 2:18p
 * Updated in $/software/control processor/code/system
 * SCR 648
 *
 * *****************  Version 50  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:08p
 * Updated in $/software/control processor/code/system
 * SCR 648, change to restore pre-interrupt interrupt level instead of
 * setting interrupt level to zero with __EI()
 *
 * *****************  Version 49  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 *
 * *****************  Version 48  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:14p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 5/25/10    Time: 4:49p
 * Updated in $/software/control processor/code/system
 * SCR# 609 - Fix NextMif value of self-scheduling RMT tasks in disabled
 * state, overwritten/removed by Version 46
 *
 * *****************  Version 46  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * Removed ID translation table. Coding standard fixes
 *
  * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 5/24/10    Time: 6:47p
 * Updated in $/software/control processor/code/system
 * SCR# 609 - Fix NextMif value of self-scheduling RMT tasks in disabled
 * state
 *
* *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 5/12/10    Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR# 587 - correct DataManager not being initialized properly
 *
 * *****************  Version 43  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 42  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:05p
 * Updated in $/software/control processor/code/system
 * SCR #536 Avg Task Execution Time Incorrect
 *
 * *****************  Version 41  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 39  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 5:56p
 * Updated in $/software/control processor/code/system
 * SCR# 484 - MIF pulse  on any MIF#, not just MIF0
 *
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 2/26/10    Time: 10:29a
 * Updated in $/software/control processor/code/system
 * SCR# 467 - 3 consecutive DT overruns cause an assert, not 3 occurrances
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 2/12/10    Time: 12:30p
 * Updated in $/software/control processor/code/system
 * SCR #449 - Add control of system mode via user command, display system
 * mode, clean up sys.get.task.x processing, provide GHS target debugging
 * hook to bypass DT overrun code.
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 5:42p
 * Updated in $/software/control processor/code/system
 * SCR# 326 - init the pending mode to NORMAL
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR# 405 - Add DT overrun Log on each occurance, save actual MIF of max
 * Duty Cycle vs. 0-32, so we can tell if issues happen at startup.
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 2/01/10    Time: 1:40p
 * Updated in $/software/control processor/code/system
 * SCR# 434 - Pulse LSSx on task execution, clean up sys cmds and
 * processing
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 1/23/10    Time: 3:15p
 * Updated in $/software/control processor/code/system
 * SCR# 405
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:57p
 * Updated in $/software/control processor/code/system
 * SCR# 400
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:14p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 12/29/09   Time: 3:28p
 * Updated in $/software/control processor/code/system
 * SCR# 329 Pack RMT Overrun Log
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 12/23/09   Time: 2:07p
 * Updated in $/software/control processor/code/system
 * SCR# 329 - implement SRS-1706
 *
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 12/23/09   Time: 12:09p
 * Updated in $/software/control processor/code/system
 * SCR# 329
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 5:23p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:49p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR  #326, #329
 *
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:42p
 * Updated in $/software/control processor/code/system
 * SCR 350 Misc - Preliminary Code Review
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 11/20/09   Time: 10:40a
 * Updated in $/software/control processor/code/system
 * SCR 337
 * Added Fatal on three DT Task Overruns
 * Added Debug string on the first 2 DT Overruns
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 3:22p
 * Updated in $/software/control processor/code/system
 * SCR #315 SEU counts for TM Health Counts
 *
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 10/08/09   Time: 6:13p
 * Updated in $/software/control processor/code/system
 * SCR #283
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 10/01/09   Time: 4:12p
 * Updated in $/software/control processor/code/system
 * SCR #283 Misc Code Review Updates
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:22p
 * Updated in $/software/control processor/code/system
 * Comment out TTMR_Init() call.  Not necessary !
 *
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 *
 ***************************************************************************/

