#define ACTION_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        ActionManager.c

   Description: The Action manager provides a service for configuring and
                driving specific LSS output combinations based on
                action requests for other parts of the system.

   Note:        None

 VERSION
 $Revision: 11 $  $Date: 12-08-29 3:23p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "actionmanager.h"
#include "cfgmanager.h"
#include "gse.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static ACTION_CFG          m_ActionCfg;         /* Configuration Storage Object       */
static ACTION_DATA         m_ActionData;        /* Runtime Data storage object        */

static ACTION_PERSIST      m_RTC_Copy;
static ACTION_PERSIST      m_EE_Copy;

/* FIFO Definition for requested Actions                                              */
static ACTION_REQUEST_FIFO m_Request;
static ACTION_REQUEST      m_RequestStorage [MAX_ACTION_REQUESTS];

#include "ActionManagerUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void ActionInitPersist    ( void );
static void ActionTask           ( void *pParam );
static void ActionUpdateFlags    ( const ACTION_CFG  *pCfg,  ACTION_DATA *pData );
static void ActionResetFlags     ( ACTION_DATA *pData, INT8 nID, UINT16 nAction );
static void ActionSetFlags       ( const ACTION_CFG  *pCfg,  ACTION_DATA *pData,
                                   INT8 nID, UINT16 nAction, BOOLEAN bACK, BOOLEAN bLatch );
static void ActionCheckACK       ( ACTION_DATA *pData );
static void ActionClearLatch     ( ACTION_DATA *pData );
static void ActionUpdateOutputs  ( ACTION_CFG  *pCfg,  ACTION_DATA *pData );
static void ActionSetOutput      ( UINT8 nLSS,         DIO_OUT_OP state );
static void ActionRectifyOutputs ( ACTION_CFG  *pCfg,  ACTION_DATA *pData,
                                   BOOLEAN      bPersistOn );
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
* Function:     ActionsInitialize
*
* Description:  Initializes all the action variables.
*
* Parameters:   None
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
void ActionsInitialize ( void )
{
   // Local Data
   TCB tTaskInfo;
   UINT16   i;

   // Add Event Action to the User Manager
   User_AddRootCmd(&rootActionMsg);

   // Reset the Event Action cfg and storage objects
   memset((void *)&m_ActionCfg,  0, sizeof(m_ActionCfg));
   memset((void *)&m_ActionData, 0, sizeof(m_ActionData));

   // Load the current configuration to the configuration array.
   memcpy((void *)&m_ActionCfg,
          (void *)&(CfgMgr_RuntimeConfigPtr()->ActionConfig),
          sizeof(m_ActionCfg));

   for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
   {
      m_ActionData.nLSS_Priority[i] = ACTION_NONE;

      // Initialize the LSS Outputs - If Active low then init LSS to a 1
      if ( (m_ActionCfg.activeState & (1 << i)) == 0 )
      {
         ActionSetOutput ( i, DIO_SetHigh );
      }
      else // Init to 0 - Shouldn't have to do this but do it anyway for completeness
      {
         ActionSetOutput ( i, DIO_SetLow );
      }
   }

   // Event Action Initialize FIFO
   memset ( (void *) &m_Request, 0, sizeof( m_Request ) );
   FIFO_Init( &m_Request.recordFIFO, (INT8*)m_RequestStorage, sizeof(m_RequestStorage) );

   // Initialize the Persistent status from the NV Memory
   // NOTE: This must come after initializing the FIFO in case the Init Persist Check fails.
   //       When that happens it will Call Flt_SetStatus which will try and submit an
   //       Action Request to the FIFO which will cause an ASSERT
   ActionInitPersist();

   // Create Action Task - DT
   memset((void *)&tTaskInfo, 0, sizeof(tTaskInfo));
   strncpy_safe(tTaskInfo.Name, sizeof(tTaskInfo.Name),"Action",_TRUNCATE);
   tTaskInfo.TaskID          = (TASK_INDEX)Action_Task;
   tTaskInfo.Function        = ActionTask;
   tTaskInfo.Priority        = taskInfo[Action_Task].priority;
   tTaskInfo.Type            = taskInfo[Action_Task].taskType;
   tTaskInfo.modes           = taskInfo[Action_Task].modes;
   tTaskInfo.MIFrames        = taskInfo[Action_Task].MIFframes;
   tTaskInfo.Enabled         = TRUE;
   tTaskInfo.Locked          = FALSE;
   tTaskInfo.Rmt.InitialMif  = taskInfo[Action_Task].InitialMif;
   tTaskInfo.Rmt.MifRate     = taskInfo[Action_Task].MIFrate;
   tTaskInfo.pParamBlock     = NULL;

   TmTaskCreate (&tTaskInfo);

   m_ActionData.bInitialized = TRUE;
}

/******************************************************************************
* Function:     ActionInitPersist
*
* Description:  The Action Init Persist opens both the EEPROM copy and
*               RTC Copy of the persistent action and verifies they are
*               not corrupt and match. If either copy is bad a log is written
*               and then the bad copy is overwritten by the good copy. If
*               both copies are bad then a log is written and the persistent
*               action is reset.
*
* Parameters:   None
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
static
void ActionInitPersist ( void )
{
   // Local Data
   ACTION_PERSIST_NV_FAIL_LOG log;

   // Clear the intermediate copies
   memset( (void *)&m_RTC_Copy, 0, sizeof(m_RTC_Copy) );
   memset( (void *)&m_EE_Copy,  0, sizeof(m_EE_Copy)  );
   // Clear the log
   memset( (void *)&log, 0, sizeof(log) );
   // Clear the final destination
   memset( (void *)&m_ActionData.persist, 0, sizeof(m_ActionData.persist));

   m_ActionData.persist.actionNum = ACTION_NONE;

   // Retrieves the stored counters from non-voltaile
   //  Note: NV_Open() performs checksum and creates sys log if checksum fails !
   //        This means we will have duplicate indication of the failure?
   log.resultEE  = NV_Open(NV_ACT_STATUS_EE);
   log.resultRTC = NV_Open(NV_ACT_STATUS_RTC);

   NV_Read(NV_ACT_STATUS_RTC, 0, &m_RTC_Copy, sizeof(m_RTC_Copy));
   NV_Read(NV_ACT_STATUS_EE,  0, &m_EE_Copy,  sizeof(m_EE_Copy ));

   log.copyfromEE  = m_EE_Copy;
   log.copyfromRTC = m_RTC_Copy;

   if ((log.resultEE != SYS_OK) && (log.resultRTC != SYS_OK))
   {
      log.failureType = ACT_BOTH_COPIES_BAD;

      // Both locations are corrupt - Set system status to CAUTION
      Flt_SetStatus( STA_CAUTION,
                     SYS_ID_ACTION_PERSIST_RETRIEVE_FAIL,
                     &log,
                     sizeof(log));

      // Clear the persistent action data
      NV_WriteNow( NV_ACT_STATUS_RTC, 0, &m_ActionData.persist, sizeof(m_ActionData.persist) );
      NV_WriteNow( NV_ACT_STATUS_EE,  0, &m_ActionData.persist, sizeof(m_ActionData.persist) );
   }
   else if (log.resultRTC != SYS_OK)
   {
      log.failureType = ACT_RTC_COPY_BAD;
      // Write a log that the RTC was bad and restored with the EEPROM
      LogWriteSystem( SYS_ID_ACTION_PERSIST_RETRIEVE_FAIL,
                      LOG_PRIORITY_3,
                      &log,
                      sizeof(log),
                      NULL );
      // Restore the RTC version
      NV_WriteNow( NV_ACT_STATUS_RTC, 0, &m_EE_Copy, sizeof(m_EE_Copy) );
   }
   else if (log.resultEE != SYS_OK)
   {
      log.failureType = ACT_EE_COPY_BAD;
      // Write a log that the RTC was bad and restored with the EEPROM
      LogWriteSystem( SYS_ID_ACTION_PERSIST_RETRIEVE_FAIL,
                      LOG_PRIORITY_3,
                      &log,
                      sizeof(log),
                      NULL );

      // Restore the EE version
      NV_WriteNow( NV_ACT_STATUS_EE, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
   }
   else
   {
      // If both are GOOD then since RTC is saved first, it will most likely be the
      // most update to date.

      // Read from RTC version
      m_ActionData.persist = m_RTC_Copy;

      // Do we have a persistent action stored?
      if (0 != m_ActionData.persist.action.nUsedMask)
      {
         m_ActionData.bNVStored         = TRUE;
         m_ActionData.bUpdatePersistOut = TRUE;
      }

      // If not in sync make the EEPROM the same as the RTC
      if ( (m_RTC_Copy.bState != m_EE_Copy.bState) ||
           (m_RTC_Copy.bLatch != m_EE_Copy.bLatch) ||
           (m_RTC_Copy.action.nUsedMask != m_EE_Copy.action.nUsedMask) ||
           (m_RTC_Copy.action.nLSS_Mask != m_EE_Copy.action.nLSS_Mask) )
      {
         m_EE_Copy = m_RTC_Copy;

         NV_WriteNow( NV_ACT_STATUS_EE, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
      }
   }
}


/******************************************************************************
 * Function:     ActionRequest
 *
 * Description:  The Action Request function allows other functions to
 *               activate and deactivate any of the configurable actions.
 *
 * Parameters:   [in] INT8   nReqNum    - Request number of the action to update
 *                                        (-1 for init request - ACTION_NO_REQ)
 *               [in] UINT16 nAction    - Bit encoded action flags (b0 = act0, b1 = act1, etc.)
 *               [in] ACTION_TYPE state - State to place action into (ACTION_ON, ACTION_OFF)
 *               [in] BOOLEAN bACK      - TRUE if action is acknowledgable
 *               [in] BOOLEAN bLatch    - TRUE if action is latched.
 *
 * Returns:      INT8 - ID for the requested action
 *
 * Notes:        This function returns an ID when called with nReqNum = -1
 *               The ID can then be used to deactivate the action.
 *
 *****************************************************************************/
INT8 ActionRequest( INT8 nReqNum, UINT16 nAction, ACTION_TYPE state,
                    BOOLEAN bACK, BOOLEAN bLatch )
{
   // Local Data
   ACTION_REQUEST request;
   ACTION_REQUEST oldestRecord;

   // Make sure this is a valid action request before queueing
   if ((0 != nAction) && (TRUE == m_ActionData.bInitialized))
   {
      if ( ACTION_NO_REQ == nReqNum )
      {
         nReqNum = m_ActionData.nRequestCounter++;
      }
      // Build the request
      request.nID     = nReqNum;
      request.nAction = nAction;
      request.state   = state;
      request.bACK    = bACK;
      request.bLatch  = bLatch;

      // Push the request into the Queue
      while (FALSE == FIFO_PushBlock(&m_Request.recordFIFO, &request, sizeof(ACTION_REQUEST)))
      {
         // Couldn't fit anymore into the queue so set the overflow flag and pop the oldest
         m_Request.bOverFlow = TRUE;
         FIFO_PopBlock (&m_Request.recordFIFO, &oldestRecord, sizeof(ACTION_REQUEST));
         m_Request.nRecordCnt--;
      }
      m_Request.nRecordCnt++;
   }

   return nReqNum;
}

/******************************************************************************
* Function:     ActionResetNVPersist
*
* Description:  Resets any persistent data in the NV Memory
*
* Parameters:   None
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
void ActionResetNVPersist ( void )
{
   // Log the Reset of the non volatile persist data
   LogWriteETM ( SYS_ID_ACTION_PERSIST_RESET,
                 LOG_PRIORITY_3,
                 &m_ActionData.persist,
                 sizeof(m_ActionData.persist),
                 NULL );

   memset((void *)&m_RTC_Copy, 0, sizeof(m_RTC_Copy));
   memset((void *)&m_EE_Copy,  0, sizeof(m_EE_Copy ));

   m_ActionData.bNVStored = FALSE;

   NV_Write( NV_ACT_STATUS_RTC, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
   NV_Write( NV_ACT_STATUS_EE,  0, &m_EE_Copy,  sizeof(m_EE_Copy ) );

   m_ActionData.bUpdatePersistOut = TRUE;
   m_ActionData.persist.bLatch    = FALSE;
   m_ActionData.persist.bState    = OFF;
   m_ActionData.persist.actionNum = ACTION_NONE;

   ActionClearLatch ( &m_ActionData );
}

/******************************************************************************
 * Function:     ActionAcknowledgable
 *
 * Description:  The Action Acknowledgable function allows others to determine if
 *               there are any acknowledgable actions active.
 *
 * Parameters:   INT32 nAction - Not Used but needed for evaluator
 *                               This could be used in the future if a specific
 *                               Action Acknowledge needed to be checked.
 *
 * Returns:      BOOLEAN [ TRUE  = Acknowledgable Actions Active,
 *                         FALSE = No acknowledgable actions ]
 *
 * Notes:        None.
 *
 *****************************************************************************/
BOOLEAN ActionAcknowledgable (  INT32 nAction )
{
   // Local Data
   ACTION_DATA *pData;
   ACTION_FLAGS *pFlags;
   UINT8         nActionIndex;
   BITARRAY128   maskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };
   BOOLEAN       ACK;

   ACK = FALSE;
   pData = &m_ActionData;

   if (( TRUE == pData->persist.bLatch ) && ( ON == pData->persist.bState ))
   {
      ACK = TRUE;
   }
   else
   {
      // Loop through all the Actions
      for (  nActionIndex = 0;
           ((nActionIndex < MAX_ACTION_DEFINES) && (ACK == FALSE));
             nActionIndex++ )
      {
         pFlags = &pData->action[nActionIndex];

         // Check if any ACK flag is set
         if ( TRUE == TestBits( maskAll, sizeof(maskAll),
                                pFlags->flagACK, sizeof(pFlags->flagACK), FALSE ) )
         {
            // Need to check if any of the Active flags match the ACK
            if ( TRUE == TestBits( pFlags->flagACK, sizeof(pFlags->flagACK),
                                   pFlags->flagActive, sizeof(pFlags->flagActive), FALSE))
            {
               ACK = TRUE;
            }
         }
      }
   }

   return ACK;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:     ActionTask
 *
 * Description:  The Action Task runs once per MIF and is responsible
 *               converting actions into the proper, configured outputs.
 *
 * Parameters:   void *pParam
 *
 * Returns:      None.
 *
 * Notes:
 *
 *****************************************************************************/
static
void ActionTask ( void *pParam )
{
   // Local Data
   BOOLEAN                 bACK;
   ER_STATE                engineState;
   ACTION_CFG             *pCfg;
   ACTION_DATA            *pData;
   UINT8                   nEngRunFlags;
   ACTION_CLR_PERSIST_LOG  actionLog;

   // Initialize Local Data
   pData = &m_ActionData;
   pCfg  = &m_ActionCfg;

   // Check if we have received an ACK
   bACK        = TriggerGetState( (INT32)pCfg->aCKTrigger );
   // Check the engine state for ANY possible running engine
   engineState = EngRunGetState(ENGRUN_ID_ANY, &nEngRunFlags);

   // Update the Action flags
   ActionUpdateFlags(pCfg, pData);
   // Did the engine just transition to STOPPED State?
   if ((ER_STATE_STOPPED != pData->prevEngState) && (ER_STATE_STOPPED == engineState))
   {
      // Check if the persistent latch is set
      if ( TRUE == pData->persist.bLatch)
      {
         // Turn on the persistent action
         pData->persist.bState    = ON;
         pData->bUpdatePersistOut = TRUE;
         // Copy RTC to EE
         m_EE_Copy = m_RTC_Copy;
         // Engine Run ended now update the EEPROM Copy
         NV_Write( NV_ACT_STATUS_EE, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
      }
   }
   // Is the engine in starting or run state?
   else if ( ER_STATE_STOPPED != engineState )
   {
      // Check if the persistent output is on because we aren't in stopped anymore
      if ( ON == pData->persist.bState )
      {
         // Save the Action Log Data
         actionLog.persistState = pData->persist;
         actionLog.clearReason  = ACT_TRANS_TO_ENG_RUN;
         // Clear the persistent output
         pData->persist.bState  = OFF;
         pData->bUpdatePersistOut = TRUE;
         ActionClearLatch(pData);
         // Write the action log
         LogWriteETM (SYS_ID_ACTION_PERSIST_CLR,
                      LOG_PRIORITY_3,
                      &actionLog,
                      sizeof(actionLog),
                      NULL);
      }
   }
   // Guess we must be in the stopped state
   else
   {
      // Now monitor the persist state for an ACK
      if (( TRUE == bACK ) && ( TRUE == pData->persist.bLatch ) &&
          ( ON == pData->persist.bState ))
      {
         // Save the action log data
         actionLog.persistState = pData->persist;
         actionLog.clearReason  = ACT_ACKNOWLEDGED;
         // Clear the previous exceed outputs
         pData->persist.bState = OFF;
         pData->bUpdatePersistOut = TRUE;
         // Write the action log
         LogWriteETM (SYS_ID_ACTION_PERSIST_CLR,
                      LOG_PRIORITY_3,
                      &actionLog,
                      sizeof(actionLog),
                      NULL);
      }
   }

   // Have we received an ACK now check if there is an Action to Acknowledge
   if ( TRUE == bACK )
   {
      ActionCheckACK ( pData );
   }
   // Save the Engine Run State
   pData->prevEngState = engineState;
   // Check the Event Action data and set the outputs accordingly
   ActionUpdateOutputs ( pCfg, pData );
}

/******************************************************************************
 * Function:     ActionUpdateFlags
 *
 * Description:  Pops any requests from the FIFO and then sets or resets the
 *               Active, Latch and/or ACK flags. These flags are used
 *               by the output process to activate and deactivate the
 *               configured outputs.
 *
 * Parameters:  [in] ACTION_CFG  *pCfg
*               [in] ACTION_DATA *pData
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void ActionUpdateFlags ( const ACTION_CFG *pCfg, ACTION_DATA *pData )
{
   // Local Data
   ACTION_REQUEST newRequest;

   // Pop any new Action Requests from the FIFO
   while (0 < m_Request.nRecordCnt)
   {
      FIFO_PopBlock(&m_Request.recordFIFO, &newRequest, sizeof(newRequest));
      m_Request.nRecordCnt--;

      if ( ACTION_ON == newRequest.state )
      {
         ActionSetFlags ( pCfg, pData, newRequest.nID,  newRequest.nAction,
                                 newRequest.bACK, newRequest.bLatch );
      }
      else if ( ACTION_OFF == newRequest.state )
      {
         ActionResetFlags ( pData, newRequest.nID, newRequest.nAction );
      }
   }
}

/******************************************************************************
 * Function:     ActionResetFlags
 *
 * Description:  The Action Reset Flags is used to reset the appropriate
 *               flags when an Action is deactivated.
 *
 * Parameters:   [in] ACTION_DATA *pData
 *               [in] INT8         nID
 *               [in] UINT16       nAction
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void ActionResetFlags ( ACTION_DATA *pData, INT8 nID, UINT16 nAction )
{
   // Local Data
   UINT16        i;
   ACTION_FLAGS *pFlags;

   // Loop through all the possible actions
   for ( i = 0; i < MAX_ACTION_DEFINES; i++ )
   {
      pFlags = &pData->action[i];

      if ( 0 != BIT( nAction, i ) )
      {
         // Check if the action latched before resetting the event flag
         if ( FALSE == GetBit(nID, pFlags->flagLatch, sizeof(pFlags->flagLatch)))
         {
            // Reset the Active flag for the Event
            ResetBit(nID, pFlags->flagActive, sizeof(pFlags->flagActive));
            // Reset the Acknowledge for the Event
            ResetBit(nID, pFlags->flagACK, sizeof(pFlags->flagACK));
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionSetFlags
 *
 * Description:  The Action Set Flags is used to set the appropriate
 *               flags when an Action is Activated.
 *
 * Parameters:   [in] ACTION_CFG  *pCfg
*                [in] ACTION_DATA *pData
 *               [in] INT8 nID
 *               [in] UINT16 nAction
 *               [in] BOOLEAN bACK
 *               [in] BOOLEAN bLatch
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void ActionSetFlags ( const ACTION_CFG  *pCfg, ACTION_DATA *pData,
                      INT8 nID, UINT16 nAction, BOOLEAN bACK, BOOLEAN bLatch )
{
   // Local Data
   UINT16          nActionIndex;
   ACTION_FLAGS   *pFlags;

   for ( nActionIndex = 0; nActionIndex < MAX_ACTION_DEFINES; nActionIndex++ )
   {
      pFlags = &pData->action[nActionIndex];

      if ( 0 != BIT( nAction, nActionIndex ) )
      {
         // Set the bit for the Action
         SetBit(nID, pFlags->flagActive, sizeof(pFlags->flagActive));

         if ( TRUE == bLatch )
         {
            // Set the bit for the Latch
            SetBit(nID, pFlags->flagLatch, sizeof(pFlags->flagLatch));

            pData->persist.bLatch    = TRUE;

            // If the persist action is enabled then the action is based on the
            // persistent output configuration, otherwise we should save the action
            // that is latched so it can be restored after a powercycle
            if (FALSE == pCfg->persist.bEnabled)
            {
               // Check if this a higher priority action
               if ( nActionIndex < pData->persist.actionNum )
               {
                  pData->persist.actionNum = nActionIndex;
                  pData->persist.action    = pCfg->output[nActionIndex];
                  m_RTC_Copy.actionNum     = pData->persist.actionNum;
                  m_RTC_Copy.bState        = ON;
                  m_RTC_Copy.bLatch        = pData->persist.bLatch;
                  m_RTC_Copy.action        = pCfg->output[nActionIndex];
                  NV_Write( NV_ACT_STATUS_RTC, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
               }
            }
            else // There is a configured persistent action so just do that!
            {
               // Check if the persistent action is already stored to RTC RAM
               if (FALSE == pData->bNVStored)
               {
                  pData->bNVStored = TRUE;
                  // Action not stored to RTC RAM yet
                  m_RTC_Copy.actionNum  = nActionIndex;
                  m_RTC_Copy.bState     = ON;
                  m_RTC_Copy.bLatch     = pData->persist.bLatch;
                  m_RTC_Copy.action     = pCfg->persist.output;
                  pData->persist.action = pCfg->persist.output;
                  // Save the pesistent status to Non-Volatile Memory
                  NV_Write( NV_ACT_STATUS_RTC, 0, &m_RTC_Copy, sizeof(m_RTC_Copy) );
               }
            }
         }

         if ( TRUE == bACK )
         {
            // Set the bit for the ACK
            SetBit(nID, pFlags->flagACK, sizeof(pFlags->flagACK));
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionCheckACK
 *
 * Description:  The Action Check ACK function checks all the actions and
 *               acknowledges the actions that were activated with an ACK
 *               requests set to TRUE.
 *
 * Parameters:   [in]     ACTION_CFG *pCfg
 *               [in/out] ACTION_DATA *pData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void ActionCheckACK ( ACTION_DATA *pData )
{
   // Local Data
   ACTION_FLAGS *pFlags;
   UINT8         nActionIndex;
   BITARRAY128   maskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };

   // Loop through all the Action
   for ( nActionIndex = 0; nActionIndex < MAX_ACTION_DEFINES; nActionIndex++ )
   {
      pFlags = &pData->action[nActionIndex];

      // Check if any ACK flag is set
      if ( TRUE == TestBits( maskAll, sizeof(maskAll),
                             pFlags->flagACK, sizeof(pFlags->flagACK), FALSE ) )
      {
         // Need to check if any of the Active flags match the ACK
         if ( TRUE == TestBits( pFlags->flagACK, sizeof(pFlags->flagACK),
                                pFlags->flagActive, sizeof(pFlags->flagActive), FALSE))
         {
            // We know we have an Active Flag to ACK, so reset the Active bits
            ResetBits ( pFlags->flagACK, sizeof(pFlags->flagACK),
                        pFlags->flagActive, sizeof(pFlags->flagActive) );

            // Clear the ACK for all IDs
            memset((void *)pFlags->flagACK, 0, sizeof(pFlags->flagACK));
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionClearLatch
 *
 * Description:  The Action clear latch function clears all the actions that
 *               are latched. It resets the flags for latch, ack and active.
 *
 * Parameters:   [in/out] ACTION_DATA *pData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void ActionClearLatch ( ACTION_DATA *pData )
{
   // Local Data
   ACTION_FLAGS *pFlags;
   UINT8         nActionIndex;
   BITARRAY128   maskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };

   // Loop through all the Action
   for ( nActionIndex = 0; nActionIndex < MAX_ACTION_DEFINES; nActionIndex++ )
   {
      pFlags = &pData->action[nActionIndex];

      // Check if any ACK flag is set
      if ( TRUE == TestBits( maskAll, sizeof(maskAll),
                             pFlags->flagLatch, sizeof(pFlags->flagLatch), FALSE ) )
      {
         // We know which flags are latched so just clear those
         ResetBits ( pFlags->flagLatch, sizeof(pFlags->flagLatch),
                     pFlags->flagActive, sizeof(pFlags->flagActive) );
         ResetBits ( pFlags->flagLatch, sizeof(pFlags->flagLatch),
                     pFlags->flagACK, sizeof(pFlags->flagACK) );
         // Clear the Latch for all IDs
         memset((void *)pFlags->flagLatch, 0, sizeof(pFlags->flagLatch));
      }
   }
}

/******************************************************************************
 * Function:     ActionUpdateOutputs
 *
 * Description:  The Action Update Output function uses the Action flags and
 *               persistent state to determine how to set the configured outputs.
 *
 * Parameters:   [in]     ACTION_CFG  *pCfg
 *               [in/out] ACTION_DATA *pData
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static
void ActionUpdateOutputs ( ACTION_CFG *pCfg, ACTION_DATA *pData )
{
   // Local Data
   DIO_OUT_OP output;
   UINT8      i;

   // Check if persistent is ON or OFF because the Persist ON is highest
   // priority
   if ( ( ON == pData->persist.bState ) && ( TRUE == pData->bUpdatePersistOut) )
   {
      for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
      {
         if ( TRUE == BIT( pData->persist.action.nUsedMask, i ) )
         {
            pData->nLSS_Priority[i] = ACTION0;

            if ( (ON == BIT( pData->persist.action.nLSS_Mask,i )) &&
                 (ON == BIT( pCfg->activeState, i)) )
            {
               output = DIO_SetHigh;
            }
            else
            {
               output = DIO_SetLow;
            }
            ActionSetOutput ( i, output );
         }
      }
      pData->bUpdatePersistOut = FALSE;
   }
   else // Persist is OFF
   {
      if ( ( OFF == pData->persist.bState ) && ( TRUE == pData->bUpdatePersistOut) )
      {
         // Shut off persistent outputs - Reset Priority Mask
         // Update the Action Outputs in Priority Order
         // Loop through the Actions and see if any bit is set Active
         // Also check the mask because the priority is only for actions that
         // share a mask
         for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
         {
            if ( TRUE == BIT( pData->persist.action.nUsedMask, i ) )
            {
               pData->nLSS_Priority[i] = ACTION_NONE;

               if ( ( ON == BIT( pData->persist.action.nLSS_Mask, i )) &&
                    ( ON == BIT( pCfg->activeState,   i )) )
               {
                  ActionSetOutput ( i, DIO_SetLow );
               }
               else if ( (ON  == BIT( pData->persist.action.nLSS_Mask, i )) &&
                         (OFF == BIT( pCfg->activeState,  i )))
               {
                  ActionSetOutput ( i, DIO_SetHigh );
               }
            }
         }
         pData->bUpdatePersistOut = FALSE;
      }
   }

   ActionRectifyOutputs ( pCfg, pData, pData->persist.bState );
}

/******************************************************************************
 * Function:     ActionRectifyOutputs
 *
 * Description:  The Action Rectify Outputs converts the action flags
 *               into the proper LSS output and then sets that output to
 *               the proper state.
 *
 * Parameters:   [in] ACTION_CFG  *pCfg
 *               [in] ACTION_DATA *pData
 *               [in] BOOLEAN      bPersistOn
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void ActionRectifyOutputs ( ACTION_CFG *pCfg, ACTION_DATA *pData, BOOLEAN bPersistOn )
{
   // Local Data
   UINT8          i;
   UINT16         nActionIndex;
   ACTION_FLAGS  *pFlags;
   ACTION_OUTPUT *pOutCfg;
   DIO_OUT_OP     output;
   BITARRAY128    maskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };

   // Loop through all the actions
   for ( nActionIndex = 0; nActionIndex < MAX_ACTION_DEFINES; nActionIndex++ )
   {
      // Set the pointers
      pFlags  = &pData->action[nActionIndex];
      pOutCfg = &pCfg->output[nActionIndex];

      // First check if any event is active for this Action
      if ( TRUE == TestBits( maskAll, sizeof(maskAll),
                             pFlags->flagActive, sizeof(pFlags->flagActive), FALSE))
      {
         // Find the USED LSS Bits
         for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
         {
            // Check if this Action uses the LSS
            if ( TRUE == BIT( pOutCfg->nUsedMask, i ) )
            {
               if ( (FALSE == bPersistOn) ||
                     ((TRUE  == bPersistOn) &&
                      (FALSE == BIT(pData->persist.action.nUsedMask, i))) )
               {
                  // Now Make sure a higher priority Action hasn't already used this LSS
                  if ( ((nActionIndex + 1) < pData->nLSS_Priority[i]) )
                  {
                     // Set the LSS
                     if ( (ON == BIT( pOutCfg->nLSS_Mask,i )) &&
                          (ON == BIT( pCfg->activeState,  i)) )
                     {
                        output = DIO_SetHigh;
                     }
                     else
                     {
                        output = DIO_SetLow;
                     }
                     pData->nLSS_Priority[i] = (nActionIndex + 1);
                     ActionSetOutput ( i, output );
                     if (OFF == pFlags->bState)
                     {
                        GSE_DebugStr(VERBOSE,TRUE,"Action %d ON: LSS %d  State %d",
                                     nActionIndex, i, output );
                     }
                  }
               }
            }
         }
         pFlags->bState = ON;
      }
      else // If the Action was on and is now off we need to turn off
      {
         if ( ON == pFlags->bState )
         {
            for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
            {
               if ( (TRUE == BIT( pOutCfg->nUsedMask, i ) ) &&
                   ((nActionIndex + 1) <= pData->nLSS_Priority[i]) )
               {
                  pData->nLSS_Priority[i] = ACTION_NONE;
                  if ( ( ON == BIT( pOutCfg->nLSS_Mask, i )) &&
                       ( ON == BIT( pCfg->activeState,   i )) )
                  {
                     ActionSetOutput ( i, DIO_SetLow );
                     GSE_DebugStr(VERBOSE,TRUE,"Action %d OFF: LSS %d  State %d",
                                                nActionIndex, i, DIO_SetLow  );
                  }
                  else if ( (ON  == BIT( pOutCfg->nLSS_Mask, i )) &&
                            (OFF == BIT( pCfg->activeState,  i )))
                  {
                     ActionSetOutput ( i, DIO_SetHigh );
                     GSE_DebugStr(VERBOSE,TRUE,"Action %d OFF: LSS %d  State %d",
                                  nActionIndex, i, DIO_SetHigh );
                  }
               }
            }
            pFlags->bState = OFF;
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionSetOutput
 *
 * Description:  The Action Set Output converts the passed in LSS Mask
 *               into the proper LSS output and then sets that output to
 *               requested state.
 *
 * Parameters:   [in] UINT8 nLSS
 *               [in] DIO_OUT_OP state
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void ActionSetOutput ( UINT8 nLSS, DIO_OUT_OP state )
{
   switch (nLSS)
   {
      case LSS0_INDEX:
         DIO_SetPin(LSS0, state);
         break;
      case LSS1_INDEX:
         DIO_SetPin(LSS1, state);
         break;
      case LSS2_INDEX:
         DIO_SetPin(LSS2, state);
         break;
      case LSS3_INDEX:
         DIO_SetPin(LSS3, state);
         break;
      default:
         break;
   }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManager.c $
 *
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-08-29   Time: 3:23p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Cleanup
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:33a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Action Acknowledgable function for the evaluator
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 12-08-24   Time: 9:30a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - ETM Fault Action Logic
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-08-16   Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fault Action Processing
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:59p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 5  *****************
 * User: John Omalley Date: 12-08-13   Time: 4:21p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Clean up Action Priorities and Latch
 *
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12-08-09   Time: 8:36a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Disabled the Persistent ouput when cleared from a GSE
 * Command
 *
 * *****************  Version 3  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Manager Persistent Updates
 *
 * *****************  Version 2  *****************
 * User: John Omalley Date: 12-07-19   Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Cleaned up Code Review Tool findings
 *
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
  ***************************************************************************/


