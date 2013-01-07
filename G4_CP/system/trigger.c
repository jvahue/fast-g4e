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
  $Revision: 85 $  $Date: 12-12-13 5:42p $

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
#define MAX_COND_STRING 30
#define MATCH_ANY       FALSE

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static BITARRAY128       m_triggerFlags;                 // Bit flags used to
                                                       // indicate when a
                                                       // trigger has been
                                                       // activated
static BITARRAY128       m_triggerValidFlags;            // Bit(array) flags used to
                                                       // indicate when one or
                                                       // more of the sensors
                                                       // used in the trigger is
                                                       // invalid
static TRIGGER_DATA       m_triggerData  [MAX_TRIGGERS]; // Runtime data related
                                                       // to the trigger
static TRIGGER_LOG        m_triggerLog   [MAX_TRIGGERS]; // Trigger Log Container

static TRIGGER_CONFIG     m_TriggerCfg[MAX_TRIGGERS];  // Trigger Cfg Array

#include "triggerUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    TriggerUpdateTask    ( void *pParam);
static void    TriggerProcess       ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static BOOLEAN TriggerCheckStart    (const TRIGGER_CONFIG *pTrigCfg,
                                     const TRIGGER_DATA *pTrigData,
                                     BOOLEAN *IsValid);
static BOOLEAN TriggerCheckDuration ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static TRIG_END_TYPE TriggerCheckEnd( const TRIGGER_CONFIG *pTrigCfg,
									                    const TRIGGER_DATA *pTrigData );
static void    TriggerLogEnd        ( TRIGGER_CONFIG *pTrigCfg, TRIGGER_DATA *pTrigData );
static void    TriggerReset         ( TRIGGER_DATA *pTrigData );
static void TriggerUpdateData       ( TRIGGER_DATA *pTrigData );

static void    TriggersEnd          ( void );
static void    TriggerLogUpdate     ( TRIGGER_DATA *pTrigData);

static void TriggerConvertLegacyCfg(INT32 trigIdx);

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     TriggerInitialize
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
  INT32          i;
  UINT8           tsi;

  TRIGGER_CONFIG  *pTrigCfg;
  TRIGGER_DATA    *pTrigData;
  TCB             tcb;

  // Add Trigger Tables to the User Manager
  User_AddRootCmd(&rootTriggerMsg);

  // Reset the Trigger configuration storage array
  memset(m_TriggerCfg, 0, sizeof(m_TriggerCfg));


  // Load the current configuration to the sensor configuration array.
  memcpy(m_TriggerCfg,
         CfgMgr_RuntimeConfigPtr()->TriggerConfigs,
         sizeof(m_TriggerCfg));

  // Clear any Trigger Flags
  CLR_TRIGGER_FLAGS(m_triggerFlags);
  CLR_TRIGGER_FLAGS(m_triggerValidFlags);

  // Loop through the Configured triggers
  for ( i = 0; i < MAX_TRIGGERS; i++)
  {
    // Set a pointer to the current trigger
    pTrigCfg = &m_TriggerCfg[i];

    // Set a pointer to the runtime data table
    pTrigData = &m_triggerData[i];

    // Clear the runtime data table
    pTrigData->triggerIndex      = (TRIGGER_INDEX)i;
    pTrigData->state             = TRIG_NONE;
    pTrigData->nStartTime_ms     = 0;
    pTrigData->nDuration_ms      = 0;
    pTrigData->nRateCounts       = (UINT16)(MIFs_PER_SECOND / pTrigCfg->rate);
    pTrigData->nRateCountdown    = (INT16)((pTrigCfg->nOffset_ms / MIF_PERIOD_mS) + 1);
    pTrigData->bStartCompareFail = FALSE;
    pTrigData->bEndCompareFail   = FALSE;
    pTrigData->bLegacyConfig     = FALSE;
    pTrigData->endType           = TRIG_NO_END;

    // Init the array of monitored sensors for legacy configs
    pTrigData->nTotalSensors     = 0;
    memset(pTrigData->sensorMap, 0, sizeof(pTrigData->sensorMap));

    // If start AND end expressions are empty, but a sensor IS defined in the cfg...
    // this is a legacy-style cfg. Create a start and end cfg expression.
    if( (pTrigCfg->startExpr.size == 0 &&
         pTrigCfg->endExpr.size == 0)  &&
         SENSOR_UNUSED != pTrigCfg->trigSensor[0].sensorIndex )
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
        if ( SENSOR_UNUSED != pTrigCfg->trigSensor[tsi].sensorIndex )
        {
          // Flag this sensor for tracking during trigger-active
          SetBit( (INT32)pTrigCfg->trigSensor[tsi].sensorIndex,
                  pTrigData->sensorMap,
                  sizeof(pTrigData->sensorMap) );
        }
      }

      // Setup the sensor summary array based on whatever was set in the SensorMap above.
      pTrigData->nTotalSensors = SensorInitSummaryArray ( pTrigData->snsrSummary,
                                                          MAX_TRIG_SENSORS,
                                                          pTrigData->sensorMap,
                                                          sizeof(pTrigData->sensorMap) );
    }

    // There should always be a configuration by the time we get here
    // (legacy, expression, expression-from-legacy) unless nothing was defined.

    if(TriggerIsConfigured((TRIGGER_INDEX)i))
    {
      // Trigger has a valid sensor configuration
      // put the trigger in the start state
      TriggerReset(pTrigData);
    }
  }

  // Create Trigger Task - DT
  memset(&tcb, 0, sizeof(tcb));
  strncpy_safe(tcb.Name, sizeof(tcb.Name),"Trigger Task",_TRUNCATE);
  tcb.TaskID          = (TASK_INDEX) Trigger_Task;
  tcb.Function        = TriggerUpdateTask;
  tcb.Priority        = taskInfo[Trigger_Task].priority;
  tcb.Type            = taskInfo[Trigger_Task].taskType;
  tcb.modes           = taskInfo[Trigger_Task].modes;
  tcb.MIFrames        = taskInfo[Trigger_Task].MIFframes;
  tcb.Enabled         = TRUE;
  tcb.Locked          = FALSE;
  tcb.Rmt.InitialMif  = taskInfo[Trigger_Task].InitialMif;
  tcb.Rmt.MifRate     = taskInfo[Trigger_Task].MIFrate;
  tcb.pParamBlock     = NULL;

  TmTaskCreate (&tcb);
}


/******************************************************************************
 * Function:     TriggerIsActive
 *
 * Description:  The Trigger Is Active function will compared a passed in
 *               test-mask against the trigger flags and return a TRUE if any of the bits
 *               in the test-mask are active in the triggerFlags array, otherwise FALSE.
 *
 * Parameters:   UINT32 Flags - Mask to check if any triggers are active
 *
 * Returns:      TRUE  - if any flag in the mask is set
 *               FALSE - if none of the flags in the mask are set in triggerFlags
 *
 * Notes:        None;
 *
 *****************************************************************************/
BOOLEAN TriggerIsActive (BITARRAY128 Flags)
{
  // Local Data
  BOOLEAN bActive;

  // Local Data Init
  bActive = FALSE;

  // Check if ANY flag is set
  if (TestBits(Flags, sizeof(BITARRAY128 ),
               &m_triggerFlags[0], sizeof(BITARRAY128 ), MATCH_ANY))
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
   TRIGGER_HDR triggerHdr[MAX_TRIGGERS];
   UINT16      triggerIndex;
   INT8        *pBuffer;
   UINT16      nRemaining;
   UINT16      nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( triggerHdr, 0, sizeof(triggerHdr) );

   // Loop through all the triggers
   for ( triggerIndex = 0;
         ((triggerIndex < MAX_TRIGGERS) && (nRemaining > sizeof (TRIGGER_HDR)));
         triggerIndex++ )
   {
      // Copy the trigger names
      strncpy_safe( triggerHdr[triggerIndex].name,
                    sizeof(triggerHdr[triggerIndex].name),
                    m_TriggerCfg[triggerIndex].triggerName,
                    _TRUNCATE);
      // Copy the sensor indexes
      triggerHdr[triggerIndex].sensorIndexA =
         m_TriggerCfg[triggerIndex].trigSensor[0].sensorIndex;
      triggerHdr[triggerIndex].sensorIndexB =
         m_TriggerCfg[triggerIndex].trigSensor[1].sensorIndex;
      triggerHdr[triggerIndex].sensorIndexC =
         m_TriggerCfg[triggerIndex].trigSensor[2].sensorIndex;
      triggerHdr[triggerIndex].sensorIndexD =
         m_TriggerCfg[triggerIndex].trigSensor[3].sensorIndex;
      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (TRIGGER_HDR);
      nRemaining -= sizeof (TRIGGER_HDR);
   }
   // Copy the Trigger header to the buffer
   memcpy ( pBuffer, triggerHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     TriggerGetState
 *
 * Description:  Returns trigger active/inactive state.  Function call signature
 *               satisfies the GetState interface to FastStateMgr module.
 *
 * Parameters:   [in] trigIdx: Index of the Trigger [0..63]
 *                    if the trigger is invalid , it returns inactive.
 *                    Out of range parameter will always return inactive.
 *
 * Returns:      active/inactive state of the passed trigger.
 *
 * Notes:        None.
 *
 *****************************************************************************/
BOOLEAN TriggerGetState( TRIGGER_INDEX trigIdx )
{
  BOOLEAN status = FALSE;

  if (trigIdx != TRIGGER_UNUSED)
  {
    status = (TRIG_ACTIVE == m_triggerData[trigIdx].state);
  }
  return status;
}

/******************************************************************************
 * Function:     TriggerValidGetState
 *
 * Description:  Returns trigger valid/invalid state.
 *
 * Parameters:   [in] trigIdx of the Trigger[0..63]
 *
 * Returns:      active/inactive state of the passed trigger.
 *
 * Notes:        None.
 *
 *****************************************************************************/
BOOLEAN TriggerValidGetState( TRIGGER_INDEX trigIdx )
{
  BOOLEAN status = FALSE;

  if (trigIdx != TRIGGER_UNUSED)
  {
    status =  GetBit((INT32)trigIdx, m_triggerValidFlags, sizeof(m_triggerValidFlags));
  }
  return status;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

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
     TmTaskEnable( Trigger_Task, FALSE );
   }
   else // Normal task execution.
   {
     // Loop through all available triggers
     for ( nTrigger = 0; nTrigger < MAX_TRIGGERS; nTrigger++ )
     {
       // Set pointers to Trigger configuration and Trigger Data Storage
       pTrigCfg  = &m_TriggerCfg[nTrigger];
       pTrigData = &m_triggerData[nTrigger];

       // Determine if current trigger is being used
       if (pTrigData->state != TRIG_NONE)
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
   BOOLEAN startCriteriaMet;
   BOOLEAN isStartValid;

   switch (pTrigData->state)
   {
      // Trigger start is looking for beginning of trigger
      case TRIG_START:

         // Check if all trigger sensors have reached thresh-hold
         startCriteriaMet = TriggerCheckStart(pTrigCfg, pTrigData, &isStartValid);

         if( isStartValid )
         {
           SetBit((INT32)pTrigData->triggerIndex,
                   m_triggerValidFlags,
                   sizeof(m_triggerValidFlags));
         }
         else
         {
           ResetBit((INT32)pTrigData->triggerIndex,
                     m_triggerValidFlags,
                     sizeof(m_triggerValidFlags));
         }

         if (TRUE == startCriteriaMet)
         {
            // Record Trigger data for each sensor
            TriggerUpdateData( pTrigData);
            // Check duration
            if (TRUE == TriggerCheckDuration(pTrigCfg, pTrigData))
            {
               // Change state to Trigger Active
               pTrigData->state  = TRIG_ACTIVE;
               // Set global trigger flag
               SetBit((INT32)pTrigData->triggerIndex, m_triggerFlags, sizeof(m_triggerFlags));
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

         pTrigData->endType = TriggerCheckEnd(pTrigCfg, pTrigData);

         // Check if any end criteria has happened
         if (TRIG_NO_END != pTrigData->endType)
         {
            // Record the time the trigger ended
            CM_GetTimeAsTimestamp(&pTrigData->endTime);
            // Change trigger state back to start
            pTrigData->state  = TRIG_START;

            // Reset trigger flag
            ResetBit((INT32)pTrigData->triggerIndex, m_triggerFlags, sizeof(m_triggerFlags));

            // Set the validity based on endType
            if( pTrigData->endType == TRIG_SENSOR_INVALID )
            {
                ResetBit((INT32)pTrigData->triggerIndex,
                         m_triggerValidFlags,
                         sizeof(m_triggerValidFlags));
            }
            else
            {
                SetBit((INT32)pTrigData->triggerIndex,
                       m_triggerValidFlags,
                       sizeof(m_triggerValidFlags));
            }

            // Log the end - if we are a legacy trigger
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
            TriggerUpdateData( pTrigData);
         }
         break;

      case TRIG_NONE:
      default:
         // FATAL ERROR WITH FILTER TYPE
         FATAL("Trigger State Fail: %d - %s - State = %d",
                  pTrigData->triggerIndex, pTrigCfg->triggerName, pTrigData->state);
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
static BOOLEAN TriggerCheckStart(const TRIGGER_CONFIG *pTrigCfg,
                                 const TRIGGER_DATA *pTrigData, BOOLEAN *IsValid)
{
  // Local Data
  BOOLEAN startCriteriaMet;
  BOOLEAN validTmp;

  //Catch IsValid passed as null, preset to true
  IsValid = IsValid == NULL ? &validTmp : IsValid;
  *IsValid = TRUE;

  // Set validity of 'this' trigger.
  // This supports self-referencing triggers to recover if a referenced sensor
  // is marked as failed from a prior run and may have healed.
  SetBit((INT32)pTrigData->triggerIndex, m_triggerValidFlags, sizeof(m_triggerValidFlags));

  // Call the configured Trigger-start check rule.
  // Criteria is in return value.
  // Validity of sensors is in IsValid pointer.
  startCriteriaMet = (BOOLEAN)EvalExeExpression( EVAL_CALLER_TYPE_TRIGGER,
                                                 (INT32)pTrigData->triggerIndex,
                                                 &pTrigCfg->startExpr,
                                                 IsValid);

  return startCriteriaMet;
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
    CM_GetTimeAsTimestamp(&pTrigData->durationMetTime);

    bDurationMet = TRUE;
    // Check if flag is already set
    if ( FALSE == GetBit( (INT32)pTrigData->triggerIndex,
						   m_triggerFlags,
						   sizeof(m_triggerFlags) ) )
    {
      // If this trigger is a legacy definition(has sensors defined) start the log.
      if (pTrigData->bLegacyConfig)
      {
        // Not Set Record the TRIGGER start Log
        nTriggerStorage = (UINT32)pTrigData->triggerIndex;
        LogWriteSystem (SYS_ID_INFO_TRIGGER_STARTED,
                        LOG_PRIORITY_LOW,
                        &nTriggerStorage,
                        sizeof(nTriggerStorage),
                        NULL);
      }
      GSE_DebugStr(NORMAL,TRUE,"Trigger start (%d) %s ", pTrigData->triggerIndex,
                   pTrigCfg->triggerName );
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
static TRIG_END_TYPE TriggerCheckEnd (const TRIGGER_CONFIG *pTrigCfg,
							   const TRIGGER_DATA *pTrigData)
{
   // Local Data
  BOOLEAN        validity;
  TRIG_END_TYPE  endType = TRIG_NO_END; // Assume/default to still running
  INT32          result;

  // Process the end expression.
  // Exit criteria is in return value, sensor validity in 'validity'.
  // Together they define how the trigger ended.

  // If end criteria is empty(i.e. un-configured),
  // then just check to see if start criteria is no longer active.
  // Note: The result is an INT32 where < 0 is error, 0 is false and 1 is true

  if(pTrigCfg->endExpr.size > 0)
  {
    result = EvalExeExpression( EVAL_CALLER_TYPE_TRIGGER,
                                (INT32)pTrigData->triggerIndex,
                                &pTrigCfg->endExpr,
                                &validity );
  }
  else
  {
    result = EvalExeExpression(EVAL_CALLER_TYPE_TRIGGER,
                               (INT32)pTrigData->triggerIndex,
                               &pTrigCfg->startExpr,
                               &validity );
    // If the returned value is 0 or 1, do a boolean inversion.
    result = (result >= 0) ?  (INT32) ! (BOOLEAN)(result) : result;
  }

  if(result > 0 && validity)
  {
    endType = TRIG_END_CRITERIA;
  }
  else if(!validity)
  {
    endType = TRIG_SENSOR_INVALID;
  }

  return endType;
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
   GSE_DebugStr(NORMAL,TRUE,"Trigger end (%d) %s", pTrigData->triggerIndex,
                                       pTrigCfg->triggerName );

   //Log the data collected
   TriggerLogUpdate( pTrigData );
   LogWriteSystem (SYS_ID_INFO_TRIGGER_ENDED,
                   LOG_PRIORITY_LOW,
                   &m_triggerLog[pTrigData->triggerIndex],
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
   pTrigData->state         = TRIG_START;
   pTrigData->nStartTime_ms = 0;
   pTrigData->nDuration_ms  = 0;
   pTrigData->endType       = TRIG_NO_END;

   SensorResetSummaryArray (pTrigData->snsrSummary, pTrigData->nTotalSensors);
}

/******************************************************************************
 * Function:     TriggerUpdateData
 *
 * Description:  Initialize the Trigger data and continue logging a Trigger.
 *
 * Parameters:   TRIGGER_DATA   *pTrigData - Dynamic Trigger Data
 *
 *
 * Returns:      None.
 *
 * Notes:        None
 *
 *****************************************************************************/
static void TriggerUpdateData( TRIGGER_DATA *pTrigData )
{

   // if duration == 0 the Trigger needs to be initialized
   if ( 0 == pTrigData->nStartTime_ms )
   {
      // general Trigger initialization
      pTrigData->nStartTime_ms   = CM_GetTickCount();
      CM_GetTimeAsTimestamp(&pTrigData->criteriaMetTime);
      memset(&pTrigData->durationMetTime,0, sizeof(TIMESTAMP));
      memset(&pTrigData->endTime        ,0, sizeof(TIMESTAMP));
   }

   SensorUpdateSummaries(pTrigData->snsrSummary, pTrigData->nTotalSensors);

   // update the Trigger duration
   pTrigData->nDuration_ms = CM_GetTickCount() - pTrigData->nStartTime_ms;
}

/******************************************************************************
 * Function:     TriggerLogUpdate
 *
 * Description:  The trigger log update function creates a system log
 *               containing the trigger data.
 *
 * Parameters:   [in]  *pTrigData TRIGGER_DATA
 *
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void TriggerLogUpdate( TRIGGER_DATA *pTrigData)
{
  // Local Data
  TRIGGER_LOG     *pLog;
  SNSR_SUMMARY    *pSummary;
  UINT8           i;

  // Initialize Local Data
  pLog = &m_triggerLog[pTrigData->triggerIndex];

  // update the Trigger duration
  pLog->nDuration_ms  = CM_GetTickCount() - pTrigData->nStartTime_ms;

  // Update the trigger end log
  pLog->triggerIndex = pTrigData->triggerIndex;
  pLog->endType         = pTrigData->endType;
  pLog->criteriaMetTime = pTrigData->criteriaMetTime;
  pLog->durationMetTime = pTrigData->durationMetTime;
  pLog->endTime         = pTrigData->endTime;

  SensorCalculateSummaryAvgs(pTrigData->snsrSummary,pTrigData->nTotalSensors);

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
      pLog->sensor[i].sensorIndex   = pSummary->SensorIndex;
      pLog->sensor[i].fTotal        = pSummary->fTotal;
      pLog->sensor[i].fMinValue     = pSummary->fMinValue;
      pLog->sensor[i].fMaxValue     = pSummary->fMaxValue;
      pLog->sensor[i].fAvgValue     = pSummary->fAvgValue;
      pLog->sensor[i].bValid        = pSummary->bValid;
    }
    else // Sensor Not Used
    {
      pLog->sensor[i].sensorIndex  = SENSOR_UNUSED;
      pLog->sensor[i].fTotal       = 0.0;
      pLog->sensor[i].fMinValue    = 0.0;
      pLog->sensor[i].fMaxValue    = 0.0;
      pLog->sensor[i].fAvgValue    = 0.0;
      pLog->sensor[i].bValid       = FALSE;
    }
  }
}

/******************************************************************************
 * Function:     TriggersEnd
 *
 * Description:  Force end all active Triggers
 *
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
    pTrigSensor = &(pTrigCfg->trigSensor[0]);
    pTrigData   = &(m_triggerData[i]);

    if ( pTrigData->state == TRIG_ACTIVE )
    {
      CM_GetTimeAsTimestamp(&pTrigData->endTime);
      if (( pTrigSensor->sensorIndex != SENSOR_UNUSED ) &&
          ( SensorIsUsed(pTrigSensor->sensorIndex) ))
      {
        pTrigData->endType = TRIG_COMMANDED_END;
        if( pTrigData->bLegacyConfig )
        {
          TriggerLogUpdate ( pTrigData );
          LogWriteSystem (SYS_ID_INFO_TRIGGER_ENDED,
            LOG_PRIORITY_LOW,
            &m_triggerLog[pTrigData->triggerIndex],
            sizeof(TRIGGER_LOG),
            NULL);
        }

      }
    }
  }
}


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
BOOLEAN TriggerIsConfigured( TRIGGER_INDEX trigIdx )
{
   return( 0 != m_TriggerCfg[trigIdx].startExpr.size );
}


/******************************************************************************
 * Function:      TriggerConvertLegacyCfg
 *
 * Description:
 *
 * Parameters:    [in] trigIdx- The index of the trigger to be converted from
 *                              legacy format to Eval Expression.
 *
 *
 * Returns:      None
 *
 * Notes:
 *
 *
 *****************************************************************************/

static void TriggerConvertLegacyCfg(INT32 trigIdx )
{
  INT32   i;
  INT32   exprCnt;
  INT32   bytesMoved;
  INT32   strToBinResult;
  CHAR    logicalOp;
  CHAR    exprString[EVAL_MAX_EXPR_STR_LEN];
  CHAR*   pStr;
  FLOAT32 tempFloat;

  TRIGGER_CONFIG  configTriggerTemp;
  TRIGGER_CONFIG* pTrigCfg = &configTriggerTemp;
  EVAL_EXPR*      pExpr;
  TRIG_CRITERIA*  pCriteria;

  // KEEP THIS LIST IN THE SAME ORDER AS enum COMPARISON in trigger.h
  const CHAR* trigSensorComparison[] = {"<", ">", "==", "!=", "!P"};
  const CHAR* critString[2] = {"start","end"};

  // Work in a local temp buffer so we don't change the config
  // in the event a valid criteria string can't be built.
  memcpy(&configTriggerTemp,
         &CfgMgr_ConfigPtr()->TriggerConfigs[trigIdx],
         sizeof(configTriggerTemp));


  // Make two Eval Expressions; start (0) and end (1)
  for (exprCnt = 0; exprCnt < 2; ++exprCnt)
  {
    pStr = exprString;
    *pStr = '\0';

    // Select the logical operator to use
    // for start (&) and end (|)
    if (0 == exprCnt)
    {
      logicalOp = '&';
      pExpr     = &pTrigCfg->startExpr;
    }
    else
    {
      logicalOp = '|';
      pExpr     = &pTrigCfg->endExpr;
    }

    // For each eval expression, check all 4 trig sensors.
    for(i = 0; i < MAX_TRIG_SENSORS; ++i)
    {
      // Get a pointer to the start/end criteria for this trig-sensor
      pCriteria = ( 0 == exprCnt) ? &pTrigCfg->trigSensor[i].start :
                                    &pTrigCfg->trigSensor[i].end;

      // Only build expressions for active trig-sensor entries
      if(pTrigCfg->trigSensor[i].sensorIndex != SENSOR_UNUSED)
      {
        tempFloat = pCriteria->fValue;

        // Ensure the COMPARISON type is in range.
        // This can only ASSERT when cfg is corrupted.
        ASSERT_MESSAGE(pCriteria->compare >= LT && pCriteria->compare <= NO_COMPARE,
                           "Legacy Comparison not defined: %d", pCriteria->compare);

        // Format NO_COMPARE condition as a 'FALSE' const
        if (NO_COMPARE == pCriteria->compare)
        {
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "FALSE ");
        }

        // Format expression ( sensor compare value)
        else if (NOT_EQUAL_PREV != pCriteria->compare)
        {
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "SVLU%03d %f %s ",
                                  pTrigCfg->trigSensor[i].sensorIndex,
                                  tempFloat,
                                  trigSensorComparison[pCriteria->compare] );
        }
        else
        {
          // Format a (sensor not-equal-prev) expression
          bytesMoved = snprintf( pStr,MAX_COND_STRING, "SVLU%03d %s ",
                                  pTrigCfg->trigSensor[i].sensorIndex,
                                  trigSensorComparison[pCriteria->compare] );
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
    strToBinResult = EvalExprStrToBin( EVAL_CALLER_TYPE_TRIGGER, trigIdx,
                                       exprString, pExpr, MAX_TRIG_EXPR_OPRNDS);
    if ( strToBinResult >= 0)
    {
      // Update the local m_TriggerCfg working area ONLY. This change is not propagated to
      // ShadowRAM/EEPROM
      memcpy(&m_TriggerCfg[trigIdx], &configTriggerTemp, sizeof(configTriggerTemp));

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
                     EvalGetMsgFromErrCode(strToBinResult), exprString);
    }



  } // for each criteria
}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: trigger.c $
 *
 * *****************  Version 85  *****************
 * User: John Omalley Date: 12-12-13   Time: 5:42p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Update
 *
 * *****************  Version 84  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:32p
 * Updated in $/software/control processor/code/system
 * SCR 1167 - Sensor Summary Init optimization
 *
 * *****************  Version 83  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 82  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 81  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:44p
 * Updated in $/software/control processor/code/system
 * SCR #1167 SensorSummary
 *
 * *****************  Version 80  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 8:03p
 * Updated in $/software/control processor/code/system
 * Code Review trigger restore static storage class
 *
 * *****************  Version 79  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 7:57p
 * Updated in $/software/control processor/code/system
 * Code Review trigger
 *
 * *****************  Version 78  *****************
 * User: Contractor V&v Date: 11/14/12   Time: 4:01p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 77  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * Code Review
 *
 * *****************  Version 76  *****************
 * User: John Omalley Date: 12-10-23   Time: 2:54p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 75  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:35p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 74  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 1:36p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
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

