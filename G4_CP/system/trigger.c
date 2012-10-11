#define TRIGGER_BODY
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        trigger.c

    Description: Trigger runtime processing
     A Trigger is an indication that some measured input has exceeded
       a specified value. As implemented, the trigger is based on
       between one and three sensors, and is "activated" when those
       sensors exceed a specified comparison value.

     When activated,
       the trigger will make an entry into the log which describes the
       duration of the Trigger and some useful statistics such as the
       max and average sensor values during the Trigger.

     For example, a trigger may become active when airspeed exceeds
       a specified value. At that point, a log entry is made and the
       max and average airspeed value are kept until the trigger
       ends. Sometime later the airspeed will become less than some
       second value, and the trigger will end: the log entry is
       completed and the trigger is reset.

     Notes:
       When configured a trigger to compare NOT_EQUAL_PREV the user
       must be aware that a minimum duration of 0 must be configured.
       If the minimum duration is not zero the user must ensure that
       value of sensor being check continues to change each time the
       trigger is run. If the value does not change the trigger will
       wnd without ever meeting the duration and no log will be recorded.

  VERSION
  $Revision: 72 $  $Date: 9/15/12 7:12p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <float.h>
#include <math.h>
#include <string.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "trigger.h"
#include "assert.h"
#include "CfgManager.h"
#include "GSE.h"
#include "user.h"
#include "logmanager.h"
#include "systemlog.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static BITARRAY128       TriggerFlags;                 // Bit flags used to
                                                       // indicate when a
                                                       // trigger has been
                                                       // activated
static BITARRAY128       TriggerValidFlags;            // Bit(array) flags used to
                                                       // indicate when one or
                                                       // more of the sensors
                                                       // used in the trigger is
                                                       // invalid
static TRIGGER_DATA       TriggerData  [MAX_TRIGGERS]; // Runtime data related
                                                       // to the trigger
static TRIGGER_LOG        TriggerLog   [MAX_TRIGGERS]; // Trigger Log Container

static TRIGGER_CONFIG     m_TriggerCfg[MAX_TRIGGERS];  // Trigger Cfg Array

#include "triggerUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    TriggerUpdateTask    ( void *pParam);
static void    TriggerProcess       ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static BOOLEAN TriggerCheckStart    (TRIGGER_CONFIG *pTrigCfg,
                                     TRIGGER_DATA *pTrigData,
                                     BOOLEAN *IsValid);
static BOOLEAN TriggerCheckDuration ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static TRIG_END_TYPE TriggerCheckEnd( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static void    TriggerLogEnd        ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static void    TriggerReset         ( TRIGGER_DATA *pTrigData );
static void    TriggerUpdateData    ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static void    TriggerLogUpdate     ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static void    TriggersEnd          ( void );
static void    TriggerEnableTask    ( BOOLEAN bEnable );

static void TriggerConvertLegacyCfg(INT32 trigIdx);
#if 0
static BOOLEAN TriggerCheckStartLegacy( TRIGGER_CONFIG *pTrigCfg,
                                        TRIGGER_DATA *pTrigData,
                                        BOOLEAN *IsValid );

static TRIG_END_TYPE TriggerCheckEndLegacy  ( TRIGGER_CONFIG *pTrigCfg,
                                              TRIGGER_DATA *pTrigData );
#endif


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     TriggersInitialize
 *
 * Description:  Initializes the Trigger Data Structures and creates the
 *               Task for configured triggers.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:
 *
 *****************************************************************************/
void TriggerInitialize(void)
{
  // Local Data
  UINT32           i;
  UINT8           tsi;
  
  TRIGGER_CONFIG  *pTrigCfg;
  TRIGGER_DATA    *pTrigData;
  TCB             TaskInfo;

  // Add Trigger Tables to the User Manager
  User_AddRootCmd(&RootTriggerMsg);

  // Reset the Trigger configuration storage array
  memset(&m_TriggerCfg, 0, sizeof(m_TriggerCfg));


  // Load the current configuration to the sensor configuration array.
  memcpy(&m_TriggerCfg,
         &(CfgMgr_RuntimeConfigPtr()->TriggerConfigs),
         sizeof(m_TriggerCfg));

  // Clear any Trigger Flags
  CLR_TRIGGER_FLAGS(TriggerFlags);
  CLR_TRIGGER_FLAGS(TriggerValidFlags);

  // Loop through the Configured triggers
  for ( i = 0; i < MAX_TRIGGERS; i++)
  {
    // Set a pointer to the current trigger
    pTrigCfg = &m_TriggerCfg[i];

    // Set a pointer to the runtime data table
    pTrigData = &TriggerData[i];

    // Clear the runtime data table
    pTrigData->TriggerIndex      = (TRIGGER_INDEX)i;
    pTrigData->State             = TRIG_NONE;
    pTrigData->nStartTime_ms     = 0;
    pTrigData->nDuration_ms      = 0;
    pTrigData->nSampleCount      = 0;   /* For calculating averages */
    pTrigData->nRateCounts       = (UINT16)(MIFs_PER_SECOND / pTrigCfg->Rate);
    pTrigData->nRateCountdown    = (UINT16)((pTrigCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
    pTrigData->bStartCompareFail = FALSE;
    pTrigData->bEndCompareFail   = FALSE;
    pTrigData->bLegacyConfig     = FALSE;
    pTrigData->EndType           = TRIG_NO_END;

    // Init the array of monitored sensors for legacy configs
    pTrigData->nTotalSensors     = 0;
    memset(pTrigData->sensorMap, 0, sizeof(pTrigData->sensorMap));

//     for ( tsi = 0; tsi < MAX_TRIG_SENSORS; tsi++)
//     {
//         pTrigData->fValuePrevious[tsi] = 0;
//     }

    // If start AND end expressions are empty, but a sensor IS defined in the cfg...
    // this is a legacy-style cfg. Create a start and end cfg expression.
    if( (pTrigCfg->StartExpr.Size == 0 &&
         pTrigCfg->EndExpr.Size == 0)  &&
         SENSOR_UNUSED != pTrigCfg->TrigSensor[0].SensorIndex )
    {
      pTrigData->bLegacyConfig = TRUE;
      GSE_DebugStr(NORMAL,FALSE,
        "Trigger: Trigger[%d] - Expressions not defined, creating...", i);
      TriggerConvertLegacyCfg( i );
    }


    // If this is a legacy configuration. Check which sensors are used
    // by this trigger config the sensor map as though it was configured by the user    
    if(pTrigData->bLegacyConfig)
    {
      for ( tsi = 0; tsi < MAX_TRIG_SENSORS; tsi++)
      {
        if ( SENSOR_UNUSED != pTrigCfg->TrigSensor[tsi].SensorIndex )
        {
          // Flag this sensor for tracking during trigger-active
          SetBit( pTrigCfg->TrigSensor[tsi].SensorIndex,
                  pTrigData->sensorMap,
                  sizeof(pTrigData->sensorMap) );
        }        
      }

      // Setup the sensor summary array based on whatever was set in the SensorMap above.
#pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      pTrigData->nTotalSensors = SensorSetupSummaryArray(pTrigData->snsrSummary,
                                                         MAX_TRIG_SENSORS,
                                                         pTrigData->sensorMap,
                                                         sizeof (pTrigData->sensorMap) );
#pragma ghs endnowarning
    }

    // There should always be a configuration by the time we get here
    // (legacy, expression, expression-from-legacy) unless nothing was defined.

    if(TriggerIsConfigured(i))
    {
      // Trigger has a valid sensor configuration
      // put the trigger in the start state
      TriggerReset(pTrigData);
    }
  }

  // Create Trigger Task - DT
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Trigger Task",_TRUNCATE);
  TaskInfo.TaskID          = Trigger_Task;
  TaskInfo.Function        = TriggerUpdateTask;
  TaskInfo.Priority        = taskInfo[Trigger_Task].priority;
  TaskInfo.Type            = taskInfo[Trigger_Task].taskType;
  TaskInfo.modes           = taskInfo[Trigger_Task].modes;
  TaskInfo.MIFrames        = taskInfo[Trigger_Task].MIFframes;
  TaskInfo.Enabled         = TRUE;
  TaskInfo.Locked          = FALSE;
  TaskInfo.Rmt.InitialMif  = taskInfo[Trigger_Task].InitialMif;
  TaskInfo.Rmt.MifRate     = taskInfo[Trigger_Task].MIFrate;
  TaskInfo.pParamBlock     = NULL;

  TmTaskCreate (&TaskInfo);
}


/******************************************************************************
 * Function:     Trigger_IsActive
 *
 * Description:  The Trigger Is Active function will compared a passed in
 *               test-mask against the trigger flags and return a TRUE if any of the bits
 *               in the test-mask are active in the TriggerFlags array, otherwise FALSE.
 *
 * Parameters:   UINT32 Flags - Mask to check if any triggers are active
 *
 * Returns:      TRUE  - if any flag in the mask is set
 *               FALSE - if none of the flags in the mask are set in TriggerFlags
 *
 * Notes:        None;
 *
 *****************************************************************************/
#define MATCH_ANY  FALSE
BOOLEAN TriggerIsActive (BITARRAY128* Flags)
{
  // Local Data
  BOOLEAN bActive;

  // Local Data Init
  bActive = FALSE;

  // Check if ANY flag is set
  if (TestBits(Flags[0], sizeof(BITARRAY128 ),
               &TriggerFlags[0], sizeof(BITARRAY128 ), MATCH_ANY))
  {
    // At least one passed in flag is set return TRUE
    bActive = TRUE;
  }

  return (bActive);
}



/******************************************************************************
 * Function:     TriggerCompareValues
 *
 * Description:  Compare two passed in values using the passed in comparison
 *               type.
 *
 * Parameters:   FLOAT32 LVal       - Left Value to compare
 *               COMPARISON Compare - Type of comparison to make - LESS_THAN
 *                                    or GREATER_THAN
 *               FLOAT32 RVal       - Right Value to compare
 *               FLOAT32 PrevVal    -
 *
 * Returns:      TRUE  if LValue compares to RValue by the comparison
 *               FALSE otherwise
 *
 * Notes:        An invalid compare command value will cause an ASSERT
 *
 *****************************************************************************/
BOOLEAN TriggerCompareValues (FLOAT32 LVal, COMPARISON Compare,
                              FLOAT32 RVal, FLOAT32    PrevVal)
{
  // Local Data
  BOOLEAN bState;

  // Check the Comparison Type
  switch (Compare)
  {
    case LT:
      bState = (BOOLEAN)(LVal < RVal);
      break;
    case GT:
      bState = (BOOLEAN)(LVal > RVal);
      break;
    case EQUAL:
       //bState = (LVal == RVal);
       bState = (BOOLEAN)(fabs(LVal - RVal) <= FLT_EPSILON);
      break;
    case NOT_EQUAL:
       //bState = (LVal != RVal);
       bState = (BOOLEAN)(fabs(LVal - RVal) >= FLT_EPSILON);
       break;
    case NOT_EQUAL_PREV:
       //bState = (LVal != PrevVal)
       bState = (BOOLEAN)(fabs(LVal - PrevVal) >= FLT_EPSILON);
      break;
    case NO_COMPARE:
        bState = FALSE;
        break;
    // Make sure we're pro-active about internal errors...
    default:
      FATAL ("Trigger Compare: Bad Comparison Type %d", Compare);
      bState = FALSE; /*NEVER REACHED*/
      break;
    }

  return  bState;
}

/******************************************************************************
 * Function:     TriggerGetSystemHdr
 *
 * Description:  Retrieves the binary system header for the trigger
 *               configuration.
 *
 * Parameters:   void *pDest         - Pointer to storage buffer
 *               UINT16 nMaxByteSize - Amount of space in buffer
 *
 * Returns:      UINT16 - Total number of bytes written
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT16 TriggerGetSystemHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   TRIGGER_HDR TriggerHdr[MAX_TRIGGERS];
   UINT16      TriggerIndex;
   INT8        *pBuffer;
   UINT16      nRemaining;
   UINT16      nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( &TriggerHdr, 0, sizeof(TriggerHdr) );

   // Loop through all the triggers
   for ( TriggerIndex = 0;
         ((TriggerIndex < MAX_TRIGGERS) && (nRemaining > sizeof (TriggerHdr[TriggerIndex])));
         TriggerIndex++ )
   {
      // Copy the trigger names
      strncpy_safe( TriggerHdr[TriggerIndex].Name,
                    sizeof(TriggerHdr[TriggerIndex].Name),
                    m_TriggerCfg[TriggerIndex].TriggerName,
                    _TRUNCATE);
      // Copy the sensor indexes
      TriggerHdr[TriggerIndex].SensorIndexA =
         m_TriggerCfg[TriggerIndex].TrigSensor[0].SensorIndex;
      TriggerHdr[TriggerIndex].SensorIndexB =
         m_TriggerCfg[TriggerIndex].TrigSensor[1].SensorIndex;
      TriggerHdr[TriggerIndex].SensorIndexC =
         m_TriggerCfg[TriggerIndex].TrigSensor[2].SensorIndex;
      TriggerHdr[TriggerIndex].SensorIndexD =
         m_TriggerCfg[TriggerIndex].TrigSensor[3].SensorIndex;
      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (TriggerHdr[TriggerIndex]);
      nRemaining -= sizeof (TriggerHdr[TriggerIndex]);
   }
   // Copy the Trigger header to the buffer
   memcpy ( pBuffer, &TriggerHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}



/******************************************************************************
 * Function:     TriggerGetState | IMPLEMENTS GetState INTERFACE to
 *                               | FAST STATE MGR
 *
 * Description:  Returns trigger active/inactive state.  Function call signature
 *               satisfies the GetState interface to FastStateMgr module.
 *
 * Parameters:   [in] Index of the Trigger [0..31]
 *                    if the trigger is invalid , it returns inactive.
 *                    Out of range parameter will always return inactive.
 *
 * Returns:      active/inactive state of the passed trigger.
 *
 *****************************************************************************/
BOOLEAN TriggerGetState( INT32 TrigIdx )
{
  return ( GetBit(TrigIdx, TriggerFlags ,sizeof(TriggerFlags)));
}


/******************************************************************************
 * Function:     TriggerValidGetState | IMPLEMENTS GetState INTERFACE to
 *                                    | FAST STATE MGR
 *
 * Description:  Returns trigger valid/invalid state.  Function call signature
 *               satisfies the GetState interface to FastStateMgr module.
 *
 * Parameters:   [in] Index of the Trigger[0..31]
 *
 * Returns:      active/inactive state of the passed trigger.
 *
 *****************************************************************************/
BOOLEAN TriggerValidGetState( INT32 TrigIdx ) 
{
  return ( GetBit(TrigIdx, TriggerValidFlags, sizeof(TriggerValidFlags)) );
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/



/******************************************************************************
 * Function:     TriggerEnableTask
 *
 * Description:  Function used to enable and disable trigger
 *               processing.
 *
 * Parameters:   BOOLEAN bEnable - Enables the trigger task
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void TriggerEnableTask ( BOOLEAN bEnable )
{
   TmTaskEnable (Trigger_Task, bEnable);
}

/******************************************************************************
 * Function:     TriggerUpdateTask
 *
 * Description:  The Trigger Update Task will run periodically and check
 *               the sensor criteria for each configured trigger. When the
 *               sensor criteria is met the trigger will transition to ACTIVE,
 *               set the corresponding trigger flag and update the sensor
 *               data.
 *
 * Parameters:   void *pParam
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void TriggerUpdateTask( void *pParam )
{
   // Local Data
   TRIGGER_CONFIG *pTrigCfg;
   TRIGGER_DATA   *pTrigData;
   UINT16          nTrigger;

   // If shutdown is signaled,
   // disable all triggers, and tell Task Mgr to
   // to disable my task.
   if (Tm.systemMode == SYS_SHUTDOWN_ID)
   {
     TriggersEnd();
     TriggerEnableTask(FALSE);
   }
   else // Normal task execution.
   {
     // Loop through all available triggers
     for ( nTrigger = 0; nTrigger < MAX_TRIGGERS; nTrigger++ )
     {
       // Set pointers to Trigger configuration and Trigger Data Storage
       pTrigCfg  = &m_TriggerCfg[nTrigger];
       pTrigData = &TriggerData[nTrigger];

       // Determine if current trigger is being used
       if (pTrigData->State != TRIG_NONE)
       {
         // Countdown the number of samples until next check
         if (--pTrigData->nRateCountdown <= 0)
         {
           // Calculate the next trigger rate
           pTrigData->nRateCountdown = pTrigData->nRateCounts;
           // Process the trigger
           TriggerProcess(pTrigCfg, pTrigData);
         }
       }
     }
   }
}

/******************************************************************************
 * Function:     TriggerProcess
 *
 * Description:  Top level trigger processing where start and active states
 *               are determined.
  *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void TriggerProcess(TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
   // Local Data
   BOOLEAN StartCriteriaMet;
   BOOLEAN IsStartValid;

   switch (pTrigData->State)
   {
      // Trigger start is looking for beginning of trigger
      case TRIG_START:

         // Check if all trigger sensors have reached thresh-hold
         StartCriteriaMet = TriggerCheckStart(pTrigCfg, pTrigData, &IsStartValid);

         if( IsStartValid )
         {
           SetBit(pTrigData->TriggerIndex, TriggerValidFlags, sizeof(TriggerValidFlags));
         }
         else
         {
           ResetBit(pTrigData->TriggerIndex, TriggerValidFlags, sizeof(TriggerValidFlags));
         }

         if (TRUE == StartCriteriaMet)
         {
            // Record Trigger data for each sensor
            TriggerUpdateData(pTrigCfg, pTrigData);
            // Check duration
            if (TRUE == TriggerCheckDuration(pTrigCfg, pTrigData))
            {
               // Change state to Trigger Active
               pTrigData->State  = TRIG_ACTIVE;
               // Set global trigger flag
               SetBit(pTrigData->TriggerIndex, TriggerFlags, sizeof(TriggerFlags));
            }
         }
         // Check if trigger stopped being met before duration was met
         else if (pTrigData->nDuration_ms != 0)
         {
            // reset the trigger data
            TriggerReset(pTrigData);
         }
         break;

      // Duration has been met now the trigger is considered active
      case TRIG_ACTIVE:

         pTrigData->EndType = TriggerCheckEnd(pTrigCfg, pTrigData);

         // Check if any end criteria has happened
         if (TRIG_NO_END != pTrigData->EndType)
         {
            // Record the time the trigger ended
            CM_GetTimeAsTimestamp(&pTrigData->EndTime);
            // Change trigger state back to start
            pTrigData->State  = TRIG_START;

            // Reset trigger flag
            ResetBit(pTrigData->TriggerIndex, TriggerFlags, sizeof(TriggerFlags));
            
            // Set the validity based on EndType
            if( pTrigData->EndType == TRIG_SENSOR_INVALID )
            {
                ResetBit(pTrigData->TriggerIndex, TriggerValidFlags, sizeof(TriggerValidFlags));
            }
            else
            {
                SetBit(pTrigData->TriggerIndex, TriggerValidFlags, sizeof(TriggerValidFlags));
            }

            // Log the End - if we are a legacy trigger
            if ( pTrigData->bLegacyConfig )
            {
              TriggerLogEnd(pTrigCfg, pTrigData);
            }

            // reset the trigger data - MUST be last
            TriggerReset( pTrigData );
         }
         else // Trigger is still active
         {
            // If we're not out of the Trigger yet, then update the data.
            TriggerUpdateData( pTrigCfg, pTrigData);
         }
         break;

      default:
         // FATAL ERROR WITH FILTER TYPE
         FATAL("Trigger State Fail: %d - %s - State = %d",
                  pTrigData->TriggerIndex, pTrigCfg->TriggerName, pTrigData->State);
         break;
   }
}

/******************************************************************************
 * Function:     TriggerCheckStart
 *
 * Description:  Checks the start of a trigger. This function is the entry point
 *               for determining start trigger criteria. It uses the TRIG_EVAL_TYPE
 *               invoke the specific function.
 *
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *               [out] *IsValid  Optional boolean to return valid/not valid
 *                               status on the sensors driving this trigger
 *
 * Returns:      BOOLEAN StartCriteriaMet
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN TriggerCheckStart(TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData,
                          BOOLEAN *IsValid)
{
  // Local Data
  BOOLEAN StartCriteriaMet;
  BOOLEAN ValidTmp;

  //Catch IsValid passed as null, preset to true
  IsValid = IsValid == NULL ? &ValidTmp : IsValid;
  *IsValid = TRUE;

  // Set validity of 'this' trigger.
  // This supports self-referencing triggers to recover if a referenced sensor
  // is marked as failed from a prior run and may have healed. 
  SetBit(pTrigData->TriggerIndex, TriggerValidFlags, sizeof(TriggerValidFlags));

  // Call the configured Trigger-start check rule.
  // Criteria is in return value.
  // Validity of sensors is in IsValid pointer.
  StartCriteriaMet = (BOOLEAN)EvalExeExpression( EVAL_CALLER_TYPE_TRIGGER,
                                                 pTrigData->TriggerIndex,
                                                 &pTrigCfg->StartExpr,
                                                 IsValid);

  // If any sensor in the expression was unused/invalid, the start state must be false
  // todo DaveB move this inside the eval so apps using expressions directly will be
  // false when invalid.
  if(FALSE == *IsValid)
  {
    StartCriteriaMet = FALSE;
  }

  return StartCriteriaMet;
}


/******************************************************************************
 * Function:     TriggerCheckDuration
 *
 * Description:  Computes the current running duration of an active trigger
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 * Returns:      BOOLEAN bDurationMet
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN TriggerCheckDuration (TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
  // Local Data
  BOOLEAN bDurationMet;
  UINT32  nTriggerStorage;

  // Initialize Local Data
  bDurationMet = FALSE;

  // Check for state transition after duration is met
  if ( pTrigData->nDuration_ms >= pTrigCfg->nMinDuration_ms )
  {
    CM_GetTimeAsTimestamp(&pTrigData->DurationMetTime);

    bDurationMet = TRUE;
    // Check if flag is already set
    if ( FALSE == GetBit(pTrigData->TriggerIndex,TriggerFlags,sizeof(TriggerFlags) ) )
    {
      // If this trigger is a legacy definition(has sensors defined) start the log.
      if (pTrigData->bLegacyConfig)
      {
        // Not Set Record the TRIGGER Start Log
        nTriggerStorage = (UINT32)pTrigData->TriggerIndex;
        LogWriteSystem (SYS_ID_INFO_TRIGGER_STARTED,
                        LOG_PRIORITY_LOW,
                        &nTriggerStorage,
                        sizeof(nTriggerStorage),
                        NULL);
      }
      GSE_DebugStr(NORMAL,TRUE,"Trigger Start (%d) %s ", pTrigData->TriggerIndex,
                   pTrigCfg->TriggerName );
    }      
  }
  return bDurationMet;
}

/******************************************************************************
 * Function:     TriggerCheckEnd
 *
 * Description:  Check if trigger end criterias are met
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 * Returns:      TRIG_END_TYPE
 *
 * Notes:        None
 *
 *****************************************************************************/
TRIG_END_TYPE TriggerCheckEnd (TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
   // Local Data
  BOOLEAN        validity;
  TRIG_END_TYPE  EndType = TRIG_NO_END; // Assume/default to still running
  INT32          result;

  // Process the end expression.
  // Exit criteria is in return value, sensor validity in 'validity'.
  // Together they define how the trigger ended.

  // If end criteria is empty(i.e. un-configured),
  // then just check to see if start criteria is no longer active.
  // Note: The result is an INT32 where < 0 is error, 0 is false and 1 is true

  if(pTrigCfg->EndExpr.Size > 0)
  {
    result = EvalExeExpression( EVAL_CALLER_TYPE_TRIGGER,
                                pTrigData->TriggerIndex,
                                &pTrigCfg->EndExpr,
                                &validity );
  }
  else
  {
    result = EvalExeExpression(EVAL_CALLER_TYPE_TRIGGER,
                               pTrigData->TriggerIndex,
                               &pTrigCfg->StartExpr,
                               &validity );
    // If the returned value is 0 or 1, do a boolean inversion.
    result = (result >= 0) ?  (INT32) ! (BOOLEAN)(result) : result;
  }

  if(result > 0 && validity)
  {
    EndType = TRIG_END_CRITERIA;
  }
  else if(!validity)
  {
    EndType = TRIG_SENSOR_INVALID;
  }

  return EndType;
}

/******************************************************************************
 * Function:     TriggerLogEnd
 *
 * Description:  Log the trigger end record
 *
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void TriggerLogEnd (TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
   GSE_DebugStr(NORMAL,TRUE,"Trigger End (%d) %s", pTrigData->TriggerIndex,
                                       pTrigCfg->TriggerName );

   //Log the data collected
   TriggerLogUpdate( pTrigCfg, pTrigData );
   LogWriteSystem (SYS_ID_INFO_TRIGGER_ENDED,
                   LOG_PRIORITY_LOW,
                   &TriggerLog[pTrigData->TriggerIndex],
                   sizeof(TRIGGER_LOG),
                   NULL);
}

/******************************************************************************
 * Function:     TriggerReset
 *
 * Description:  Reset one Trigger object. Called when we transition to NOT
 *               ACTIVE, we initialize the Trigger data so that valid checks
 *               may subsequently be done.
 *
 * Parameters:   TRIGGER_DATA *pTrigData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void TriggerReset( TRIGGER_DATA *pTrigData)
{
   // Local Data
     
   // Reinit the trigger data
   pTrigData->State         = TRIG_START;
   pTrigData->nStartTime_ms = 0;
   pTrigData->nDuration_ms  = 0;
   pTrigData->nSampleCount  = 0;
   pTrigData->EndType       = TRIG_NO_END;

   pTrigData->nTotalSensors = SensorSetupSummaryArray(pTrigData->snsrSummary,
                                                      MAX_TRIG_SENSORS,
                                                      pTrigData->sensorMap,
                                                      sizeof (pTrigData->sensorMap) );   
}

/******************************************************************************
 * Function:     TriggerUpdateData
 *
 * Description:  Initialize the Trigger data and continue logging a Trigger.
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg  - Static Trigger Configuration
 *               TRIGGER_DATA   *pTrigData - Dynamic Trigger Data
 *
 * Returns:      None.
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void TriggerUpdateData( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
   // Local Data
   FLOAT32  oneOverN;
   SNSR_SUMMARY     *pSummary;
   UINT8            i;

   // if duration == 0 the Trigger needs to be initialized
   if ( 0 == pTrigData->nStartTime_ms )
   {
      // general Trigger initialization
      pTrigData->nStartTime_ms   = CM_GetTickCount();
      pTrigData->nSampleCount    = 0;
      CM_GetTimeAsTimestamp(&pTrigData->CriteriaMetTime);
      memset(&pTrigData->DurationMetTime,0, sizeof(TIMESTAMP));
      memset(&pTrigData->EndTime        ,0, sizeof(TIMESTAMP));
   }

   // update the sample count for the average calculation
   pTrigData->nSampleCount++;

   // Loop through the Triggers Sensors
   for ( i = 0; i < pTrigData->nTotalSensors; i++ )
   {
     // Set a pointer to the data summary
     pSummary    = &(pTrigData->snsrSummary[i]);

     // If the sensor is known to be invalid and was valid in the past(initialized)...
     // ignore processing for the remainder of this event.
     if( !pSummary->bValid && pSummary->bInitialized )
     {
       continue;
     }

     pSummary->bValid = SensorIsValid(pSummary->SensorIndex);

     if ( pSummary->bValid )
     {
       pSummary->bInitialized = TRUE;
       SensorUpdateSummaryItem(pSummary);
     }
     else if ( pSummary->bInitialized)
     {
       // Sensor is now invalid but had been valid
       // calculate average for valid period.
       oneOverN = (1.0f / (FLOAT32)(pTrigData->nSampleCount - 1));
       pSummary->fAvgValue = pSummary->fTotal * oneOverN;
     }
   }

   // update the Trigger duration
   pTrigData->nDuration_ms = CM_GetTickCount() - pTrigData->nStartTime_ms;
}

/******************************************************************************
 * Function:     TriggerLogUpdate
 *
 * Description:  The trigger log update function creates a system log
 *               containing the trigger data.
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void TriggerLogUpdate( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData)
{
  // Local Data
  FLOAT32         oneOverN;
  TRIGGER_LOG     *pLog;
  SNSR_SUMMARY    *pSummary;
  UINT8           i;

  // Initialize Local Data
  pLog = &TriggerLog[pTrigData->TriggerIndex];

  // Calculate the multiplier for the average
  oneOverN = (1.0f / (FLOAT32)pTrigData->nSampleCount);

  // update the Trigger duration
  pLog->nDuration_ms  = CM_GetTickCount() - pTrigData->nStartTime_ms;

  // Update the trigger end log
  pLog->TriggerIndex = pTrigData->TriggerIndex;
  pLog->EndType         = pTrigData->EndType;
  pLog->CriteriaMetTime = pTrigData->CriteriaMetTime;
  pLog->DurationMetTime = pTrigData->DurationMetTime;
  pLog->EndTime         = pTrigData->EndTime;

  // Loop through ALL possible sensors
  for ( i = 0; i < MAX_TRIG_SENSORS; i++)
  {
    // Set a pointer to the current trigger
    pSummary = &(pTrigData->snsrSummary[i]);

    // Check for a valid sensor index and the sensor is used
    if ( (SensorIsUsed(pSummary->SensorIndex)) )
    {
      pSummary->bValid = SensorIsValid (pSummary->SensorIndex);
      
      // Update the summary data for the trigger
      pLog->Sensor[i].SensorIndex   = pSummary->SensorIndex;
      pLog->Sensor[i].fTotal        = pSummary->fTotal;
      pLog->Sensor[i].fMinValue     = pSummary->fMinValue;
      pLog->Sensor[i].fMaxValue     = pSummary->fMaxValue;
      pLog->Sensor[i].fAvgValue     = pSummary->fTotal * oneOverN;
      pLog->Sensor[i].bValid        = pSummary->bValid;
    }
    else // Sensor Not Used
    {
      pLog->Sensor[i].SensorIndex  = SENSOR_UNUSED;
      pLog->Sensor[i].fTotal       = 0.0;
      pLog->Sensor[i].fMinValue    = 0.0;
      pLog->Sensor[i].fMaxValue    = 0.0;
      pLog->Sensor[i].fAvgValue    = 0.0;
      pLog->Sensor[i].bValid       = FALSE;
    }
  }
}

/******************************************************************************
 * Function:     TriggersEnd
 *
 * Description:  Force end all active Triggers
 *               TBD - Should be down when system shutsdown.
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:
 *
 *****************************************************************************/
static
void TriggersEnd( void)
{
  // Local Data
  UINT8 i;
  TRIGGER_CONFIG *pTrigCfg;
  TRIGGER_DATA   *pTrigData;
  TRIG_SENSOR    *pTrigSensor;

  for (i = 0; i < MAX_TRIGGERS; i++)
  {
    pTrigCfg    = &m_TriggerCfg[i];
    pTrigSensor = &(pTrigCfg->TrigSensor[0]);
    pTrigData   = &(TriggerData[i]);

    if ( pTrigData->State == TRIG_ACTIVE )
    {
      CM_GetTimeAsTimestamp(&pTrigData->EndTime);
      if (( pTrigSensor->SensorIndex != SENSOR_UNUSED ) &&
          ( SensorIsUsed(pTrigSensor->SensorIndex) ))
      {
        pTrigData->EndType = TRIG_COMMANDED_END;
        if( pTrigData->bLegacyConfig )
        {
          TriggerLogUpdate ( pTrigCfg, pTrigData );
          LogWriteSystem (SYS_ID_INFO_TRIGGER_ENDED,
            LOG_PRIORITY_LOW,
            &TriggerLog[pTrigData->TriggerIndex],
            sizeof(TRIGGER_LOG),
            NULL);
        }
        
      }
    }
  }
}

#if 0
/******************************************************************************
 * Function:     TriggerCheckStartLegacy
 *
 * Description:  Checks the start of a trigger using legacy approach of
 *               AND-ing the results of all defined sensor.
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *               [out] *IsValid  Optional boolean to return valid/not valid
 *                               status on the sensors driving this trigger
 *
 * Returns:      BOOLEAN StartCriteriaMet
 *
 * Notes:        None.
 *
 *****************************************************************************/
BOOLEAN TriggerCheckStartLegacy(TRIGGER_CONFIG *pTrigCfg,
                                  TRIGGER_DATA *pTrigData,
                                  BOOLEAN *IsValid)
{
  // Local Data
  BOOLEAN     StartCriteriaMet;
  UINT16      nIndex;
  TRIG_SENSOR *pTrigSensor;
  FLOAT32     LVal;

  // Initialize Local Data
  StartCriteriaMet = TRUE;

  // Loop through all comparisons for this trigger
  for ( nIndex = 0; nIndex < MAX_TRIG_SENSORS; nIndex++)
  {
    pTrigSensor = &(pTrigCfg->TrigSensor[nIndex]);

    // All unused sensors are set to 255 verify sensor index is valid
    if ( pTrigSensor->SensorIndex < MAX_SENSORS)
    {
      // Verify current sensor is being used and valid
      if ( ( SensorIsUsed(pTrigSensor->SensorIndex)) &&
        ( SensorIsValid(pTrigSensor->SensorIndex)) )
      {
        // Get Current Value
        LVal = SensorGetValue(pTrigSensor->SensorIndex);

        // Compare sensor to configured trigger parameters
        StartCriteriaMet &= TriggerCompareValues ( LVal,
          pTrigSensor->Start.Compare,
          pTrigSensor->Start.fValue,
          pTrigData->fValuePrevious[nIndex]);
        // Save the current value as previous
        pTrigData->fValuePrevious[nIndex] = LVal;
      }
      else // Sensor defined as Unused or sensor is not valid
      {
        StartCriteriaMet = FALSE;
        //SensorValid? T: Set to last value F: Set to FALSE
        *IsValid = SensorIsValid(pTrigSensor->SensorIndex) ? *IsValid : FALSE;
      }

    }
    else // Sensor Index not valid
    {
      break;
    }
  }

  return StartCriteriaMet;
}
#endif

#if 0
/******************************************************************************
 * Function:     TriggerCheckEndLegacy
 *
 * Description:  Checks the end of a trigger using legacy approach of
 *               OR-ing the results of all defined sensor.
 *
 * Parameters:   TRIGGER_CONFIG *pTrigCfg,
 *               TRIGGER_DATA   *pTrigData
 *
 *
 * Returns:      TRIG_END_TYPE EndType
 *
 * Notes:        None.
 *
 *****************************************************************************/
TRIG_END_TYPE TriggerCheckEndLegacy(TRIGGER_CONFIG *pTrigCfg,
                                    TRIGGER_DATA *pTrigData)
{
  // Local Data
  TRIG_END_TYPE EndType;
  BOOLEAN       EndCriteriaMet;
  UINT16        nIndex;
  TRIG_SENSOR   *pTrigSensor;
  FLOAT32       LVal;

  // Initialize Local Data
  EndType        = TRIG_NO_END;
  EndCriteriaMet = FALSE;

  // Loop through all defined trigger sensors
  for ( nIndex = 0; nIndex < MAX_TRIG_SENSORS; nIndex++)
  {
    // Set pointer to current sensor
    pTrigSensor = &(pTrigCfg->TrigSensor[nIndex]);

    // All unused sensors are set to 255 verify sensor index is valid
    if ( pTrigSensor->SensorIndex < MAX_SENSORS )
    {
      // Check if sensor is actually configured to be used
      if ( SensorIsUsed(pTrigSensor->SensorIndex) )
      {
        // Determine if sensor is valid
        if ( SensorIsValid(pTrigSensor->SensorIndex) )
        {
          // Store the current value to check
          LVal = SensorGetValue(pTrigSensor->SensorIndex);

          // End trigger criteria
          EndCriteriaMet |= TriggerCompareValues( LVal,
            pTrigSensor->End.Compare,
            pTrigSensor->End.fValue,
            pTrigData->fValuePrevious[nIndex] );
          // Make the current the previous
          pTrigData->fValuePrevious[nIndex] = LVal;
        }
        else // Sensor Went invalid - end trigger
        {
          EndType = TRIG_SENSOR_INVALID;
        }
      }
    }
    else // Sensor index is not valid, stop checking sensors
    {
      break;
    }
  }
  // Check if end criteria was met and no error type detected
  if ((TRUE == EndCriteriaMet) && (TRIG_NO_END == EndType))
  {
    EndType = TRIG_END_CRITERIA;
  }
  return EndType;
}

#endif


/******************************************************************************
* Function:     TriggerIsConfigured
*
* Description:  Accessor function which returns whether the trigger indicated by
*               trigIndex is defined.
*               If the start criteria is defined, the trigger is
*               considered "configured".
*
*
*
* Parameters:   [in] trigIdx  index into the trigger table,
*
*
* Returns:      BOOLEAN indicated trigger is defined.
*
* Notes:        The system assumes that sensors configured for a trigger
*               are defined starting from location '0' and allocated
*               incrementally up to location '3' without gaps.
*
*****************************************************************************/
BOOLEAN TriggerIsConfigured(INT32 trigIdx)
{
   return( 0 != m_TriggerCfg[trigIdx].StartExpr.Size );
}


/******************************************************************************
 * Function:      TriggerConvertLegacyCfg
 *
 * Description:
 *
 * Parameters:    [in]
 *
 *                [out]
 *
 * Returns:
 *
 * Notes:
 *
 *
 *****************************************************************************/
#define MAX_COND_STRING 30
static void TriggerConvertLegacyCfg(INT32 trigIdx )
{
  INT32   i;
  INT32   exprCnt;
  INT32   bytesMoved;
  INT32   StrToBinResult;
  CHAR    logicalOp;
  CHAR    exprString[255];
  CHAR*   pStr;
  FLOAT32 tempFloat;

  TRIGGER_CONFIG  ConfigTriggerTemp;
  TRIGGER_CONFIG* pTrigCfg = &ConfigTriggerTemp;
  EVAL_EXPR*      pExpr;
  TRIG_CRITERIA*  pCriteria;

  // KEEP THIS LIST IN THE SAME ORDER AS enum COMPARISON in trigger.h
  const CHAR* trigSensorComparison[] = {"<", ">", "==", "!=", "!P"};
  const CHAR* critString[2] = {"Start","End"};

  // Work in a local temp buffer so we don't change the config
  // in the event a valid criteria string can't be built.
  memcpy(&ConfigTriggerTemp,
         &CfgMgr_ConfigPtr()->TriggerConfigs[trigIdx],
         sizeof(ConfigTriggerTemp));


  // Make two Eval Expressions; start (0) and end (1)
  for (exprCnt = 0; exprCnt < 2; ++exprCnt)
  {
    pStr = exprString;
    *pStr = '\0';

    // Select the logical operator to use
    // for Start (&) and End (|)
    if (0 == exprCnt)
    {
      logicalOp = '&';
      pExpr     = &pTrigCfg->StartExpr;
    }
    else
    {
      logicalOp = '|';
      pExpr     = &pTrigCfg->EndExpr;
    }

    // For each eval expression, check all 4 trig sensors.
    for(i = 0; i < MAX_TRIG_SENSORS; ++i)
    {
      // Get a pointer to the start/end criteria for this trig-sensor
      pCriteria = ( 0 == exprCnt) ? &pTrigCfg->TrigSensor[i].Start :
                                    &pTrigCfg->TrigSensor[i].End;

      // Only build expressions for active trig-sensor entries
      if(pTrigCfg->TrigSensor[i].SensorIndex != SENSOR_UNUSED)
      {
        tempFloat = pCriteria->fValue;

        // Ensure the COMPARISON type is in range.
        // This can only ASSERT when cfg is corrupted.
        ASSERT_MESSAGE(pCriteria->Compare >= LT && pCriteria->Compare <= NO_COMPARE,
                           "Legacy Comparison not defined: %d", pCriteria->Compare);

        // Format NO_COMPARE condition as a 'FALSE' const
        if (NO_COMPARE == pCriteria->Compare)
        {
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "FALSE ");
        }

        // Format expression ( sensor Compare value)
        else if (NOT_EQUAL_PREV != pCriteria->Compare)
        {
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "SVLU%03d %f %s ",
                                  pTrigCfg->TrigSensor[i].SensorIndex,
                                  tempFloat,
                                  trigSensorComparison[pCriteria->Compare] );
        }
        else
        {
          // Format a (sensor not-equal-prev) expression
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "SVLU%03d %s ",
                                  pTrigCfg->TrigSensor[i].SensorIndex,
                                  trigSensorComparison[pCriteria->Compare] );
        }
        pStr += bytesMoved;

        // After the first two conditions are processed,
        // add the post-fix logical-operator after each condition
        // (e.g. <S_aaa float-value-a cmp-op-a> <S_bbb float-value-b cmp-op-b> lgc-op
        if ( i >=1 )
        {
          *pStr++ = logicalOp;
          *pStr++ = ' ';
        }
      }
    } //for TRIG_SENSOR

    *pStr = '\0';

    // Finished making an expression string for this criteria.
    // Send it to the evaluator for encoding and a proof-run.
    StrToBinResult = EvalExprStrToBin( EVAL_CALLER_TYPE_TRIGGER, trigIdx, 
                                       exprString, pExpr, MAX_TRIG_EXPR_OPRNDS);
    if ( StrToBinResult >= 0)
    {
      // Update the local m_TriggerCfg working area ONLY. This change is not propagated to
      // ShadowRAM/EEPROM
      memcpy(&m_TriggerCfg[trigIdx], &ConfigTriggerTemp, sizeof(ConfigTriggerTemp));

      GSE_DebugStr(NORMAL,FALSE,
                   "Trigger: Trigger[%d] - %s Criteria expression successfully created.\r\n%s",
                   trigIdx,
                   critString[exprCnt], exprString);
    }
    else
    {
      GSE_DebugStr( NORMAL,FALSE,
                    "Trigger: Trigger[%d] - %s Criteria expression failed: %s\r\n%s",
                     trigIdx,
                     critString[exprCnt],
                     EvalGetMsgFromErrCode(StrToBinResult), exprString);
    }



  } // for each criteria
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: trigger.c $
 * 
 * *****************  Version 72  *****************
 * User: Jeff Vahue   Date: 9/15/12    Time: 7:12p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 71  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:46p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trigger fix for using SensorArray
 * 
 * *****************  Version 70  *****************
 * User: Contractor V&v Date: 8/29/12    Time: 3:08p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 !P Processing
 * 
 * *****************  Version 69  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 68  *****************
 * User: Contractor V&v Date: 8/22/12    Time: 5:31p
 * Updated in $/software/control processor/code/system
 * FAST 2 Issue #15 legacy conversion
 * 
 * *****************  Version 67  *****************
 * User: Contractor V&v Date: 8/15/12    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Fixed issue #2, #6 trigger validity
 *
 * *****************  Version 66  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:28p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Update
 *
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Trigger uses SensorUpdateSummaryItem
 *
 * *****************  Version 64  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Refactor for common Sensor snsrSummary
 *
 * *****************  Version 63  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Support empty end criteria
 *
 * *****************  Version 62  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * FAST 2 Add support for FALSE boolean constant
 *
 * *****************  Version 61  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:55p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * FAST 2 reorder RPN operands and new operators
 *
 * *****************  Version 59  *****************
 * User: Contractor V&v Date: 6/07/12    Time: 7:15p
 * Updated in $/software/control processor/code/system
 * Change test for TriggerIsConfigured per EngineRun reqd review.
 *
 * *****************  Version 58  *****************
 * User: Contractor V&v Date: 4/23/12    Time: 8:02p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Evaluator cleanup
 *
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:08p
 * Updated in $/software/control processor/code/system
 * FAST2  Trigger uses Evaluator
 *
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 3/21/12    Time: 6:48p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2 Refactoring names
 *
 * *****************  Version 55  *****************
 * User: Contractor V&v Date: 3/14/12    Time: 4:52p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trigger processing
 *
 * *****************  Version 54  *****************
 * User: Jim Mood     Date: 9/19/11    Time: 2:26p
 * Updated in $/software/control processor/code/system
 * SCR 575: Forgot to save before last checkin...
 *
 * *****************  Version 53  *****************
 * User: Jim Mood     Date: 9/13/11    Time: 6:59p
 * Updated in $/software/control processor/code/system
 * SCR 575 Modfix.  Trigger Valid flags were not set properly because
 * IsValid was not properly dereferenced.
 *
 * *****************  Version 52  *****************
 * User: John Omalley Date: 8/31/11    Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR 1066 - Previous Value only initialize to 0 at power-up.
 *
 * *****************  Version 51  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 50  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 6:45p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 *
 * *****************  Version 49  *****************
 * User: John Omalley Date: 8/23/10    Time: 5:32p
 * Updated in $/software/control processor/code/system
 * SCR 813 - Removed Updating data when forcing trigger end
 *
 * *****************  Version 48  *****************
 * User: Jeff Vahue   Date: 8/21/10    Time: 5:50p
 * Updated in $/software/control processor/code/system
 * SCR# 810 - check trigger state before ending it in TriggersEnd
 *
 * *****************  Version 47  *****************
 * User: Contractor V&v Date: 7/30/10    Time: 9:38p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 46  *****************
 * User: Contractor V&v Date: 7/27/10    Time: 2:27p
 * Updated in $/software/control processor/code/system
 * SCR #282 Misc - Shutdown Processing
 *
 * *****************  Version 45  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:05a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 44  *****************
 * User: John Omalley Date: 7/15/10    Time: 5:30p
 * Updated in $/software/control processor/code/system
 * SCR 676 - Change Trigger End log to use record Sensor Unused index
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:40p
 * Updated in $/software/control processor/code/system
 * SCR# 649 - add NO_COMPARE back in for use in Signal Tests.
 *
 * *****************  Version 42  *****************
 * User: John Omalley Date: 6/14/10    Time: 3:32p
 * Updated in $/software/control processor/code/system
 * SCR 546 - Add Sensor Indexes to the Trigger End Log
 *
 * *****************  Version 41  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 40  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 *
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 5/03/10    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 530: Error: Trigger NO_COMPARE infinite logs
 * Removed NO_COMPARE from code.
 *
 * *****************  Version 37  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 36  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 34  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 1/28/10    Time: 9:23a
 * Updated in $/software/control processor/code/system
 * SCR 408
 * * Added minimum, maximum and average for all sensors.
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR 371
 *
 * *****************  Version 29  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 12/21/09   Time: 10:41a
 * Updated in $/software/control processor/code/system
 * SCR 379
 * - Moved the initialization of pSummary outside the if statement so that
 * all instances where it is used will be done after initialization.
 *
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 42 and SCR 106
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 11/19/09   Time: 2:40p
 * Updated in $/software/control processor/code/system
 * SCR 323 -
 * Removed nStartTime_ms from the trigger logs.
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:57p
 * Updated in $/software/control processor/code/system
 * Misc non code update to support WIN32 version and spelling.
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 10/26/09   Time: 2:04p
 * Updated in $/software/control processor/code/system
 * SCR 291
 * - Corrections to the sensor valid for trigger logs
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 10/26/09   Time: 1:54p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Updated the rate countdown logic because the 1st calculated countdonw
 * was incorrect.
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/21/09   Time: 4:14p
 * Updated in $/software/control processor/code/system
 * SCR 311
 * 1. Updated the previous logic to only compare to NOT_EQUALS.
 * 2. Calculated the rate countdown during initialization and then just
 * reload each time the countdown expires.
 * 3. Applied a delta to epsilon for floating point number comparisons
 * EQUAL and NOT_EQUAL.
 *
 * *****************  Version 21  *****************
 * User: John Omalley Date: 10/21/09   Time: 4:01p
 * Updated in $/software/control processor/code/system
 * SCR 291
 * - Added logic for new start and end trigger logs.
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 10/20/09   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 308
 * - Updated logic to check for  NO_COMPARE trigger configuration
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 10/14/09   Time: 5:18p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Updated the sensor and trigger rate processing.
 * - Added the BOOLEAN to the user configuration function.
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 10/07/09   Time: 5:47p
 * Updated in $/software/control processor/code/system
 * SCR 283 - Added FATAL assert to default cases
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 10/07/09   Time: 11:59a
 * Updated in $/software/control processor/code/system
 * Updated per SCR 292
 *
 * *****************  Version 16  *****************
 * User: John Omalley Date: 10/06/09   Time: 5:05p
 * Updated in $/software/control processor/code/system
 * Updates for requirments implementation
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:39p
 * Updated in $/software/control processor/code/system
 * Implemented missing requirements.
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 3:47p
 * Updated in $/control processor/code/system
 * SCR #168 Update comment from 256 to 255.
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:54a
 * Updated in $/control processor/code/system
 * SCR #87
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:23p
 * Updated in $/control processor/code/system
 * 1) SCR #87 Function Prototype
 * 2) Update call LogWriteSystem() to include NULL for ts
 *
 *
 ***************************************************************************/
 
