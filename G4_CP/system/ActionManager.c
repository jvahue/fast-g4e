#define ACTION_BODY
/******************************************************************************
           Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
              All Rights Reserved. Proprietary and Confidential.

   File:        ActionManager.c

   Description:

   Note:

 VERSION
 $Revision: 1 $  $Date: 12-07-17 11:32a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "actionmanager.h"
#include "alt_basic.h"
#include "assert.h"
#include "cfgmanager.h"
#include "gse.h"
#include "logmanager.h"
#include "NVMgr.h"
#include "taskmanager.h"
#include "systemlog.h"
#include "utility.h"
#include "user.h"

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

/* FIFO Definition for requested Actions                                              */
static ACTION_REQUEST_FIFO m_Request;
static ACTION_REQUEST      m_RequestStorage [MAX_ACTION_REQUESTS];

#include "ActionManagerUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void ActionTask          ( void *pParam );
static void ActionUpdateFlags   ( ACTION_DATA *pData );
static void ActionCheckACK      ( ACTION_CFG  *pCfg, ACTION_DATA *pData );
static void ActionUpdateOutputs ( ACTION_CFG  *pCfg, ACTION_DATA *pData );
static void ActionSetOuptut     ( UINT8 LSS,   DIO_OUT_OP State );
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
   TCB TaskInfo;

   // Add Event Action to the User Manager
   User_AddRootCmd(&RootActionMsg);

   // Reset the Event Action cfg and storage objects
   memset(&m_ActionCfg,  0, sizeof(m_ActionCfg));
   memset(&m_ActionData, 0, sizeof(m_ActionData));

   // Load the current configuration to the configuration array.
   memcpy(&m_ActionCfg,
          &(CfgMgr_RuntimeConfigPtr()->ActionConfig),
          sizeof(m_ActionCfg));

   // Event Action Initialize FIFO
   memset ( (void *) &m_Request, 0, sizeof( m_Request ) );
   FIFO_Init( &m_Request.RecordFIFO, (INT8*)&m_RequestStorage, sizeof(m_RequestStorage) );

   // Create Action Task - DT
   memset(&TaskInfo, 0, sizeof(TaskInfo));
   strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Action",_TRUNCATE);
   TaskInfo.TaskID          = Action_Task;
   TaskInfo.Function        = ActionTask;
   TaskInfo.Priority        = taskInfo[Action_Task].priority;
   TaskInfo.Type            = taskInfo[Action_Task].taskType;
   TaskInfo.modes           = taskInfo[Action_Task].modes;
   TaskInfo.MIFrames        = taskInfo[Action_Task].MIFframes;
   TaskInfo.Enabled         = TRUE;
   TaskInfo.Locked          = FALSE;
   TaskInfo.Rmt.InitialMif  = taskInfo[Action_Task].InitialMif;
   TaskInfo.Rmt.MifRate     = taskInfo[Action_Task].MIFrate;
   TaskInfo.pParamBlock     = NULL;

   TmTaskCreate (&TaskInfo);
}

/******************************************************************************
 * Function:     ActionRequest
 *
 * Description:
 *
 * Parameters:   [in] UINT32  Action - Flags
 *               [in] BOOLEAN State
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
INT8 ActionRequest ( INT8 ReqNum, UINT32 Action, ACTION_TYPE State )
{
   // Local Data
   ACTION_REQUEST Request;

   // Make sure this is a valid action request before queueing
   if (0 != Action)
   {
      if ( ACTION_NO_REQ == ReqNum )
      {
         ReqNum = m_ActionData.RequestCounter++;
      }
      // Build the request
      Request.Index  = ReqNum;
      Request.Action = Action;
      Request.State  = State;

//      GSE_DebugStr(NORMAL,TRUE,"Action Req: EI: %d A: 0x%08x S: %d", EventIndex, Action, State );

      // Push the request into the Queue
      // TBD: What to do on a return value of FALSE --- No room in FIFO?
      FIFO_PushBlock(&m_Request.RecordFIFO, &Request, sizeof(Request));
      m_Request.RecordCnt++;
   }

   return ReqNum;
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
   BOOLEAN       bACK;
   ER_STATE      EngineState;
   ACTION_CFG   *pCfg;
   ACTION_DATA  *pData;
   UINT8         EngRunFlags;

   // Initialize Local Data
   pData = &m_ActionData;
   pCfg  = &m_ActionCfg;

   // Check if we have received an ACK
   bACK        = TriggerGetState( pCfg->ACKTrigger );
   // TBD - Should the event have a configurable engine run index, similar to trends
   EngineState = EngRunGetState(ENGRUN_ID_ANY, &EngRunFlags);

   // Update the Action flags
   ActionUpdateFlags(pData);

   if (TRUE == pCfg->Persist.bEnabled)
   {
      // Need to determine if the engine run ended
      if ((ER_STATE_STOPPED != pData->PrevEngState) && (ER_STATE_STOPPED == EngineState))
      {
         // Check if the persistent latch is set
         if ( TRUE == pData->Persist.bLatch)
         {
            // Turn on the persistent action
            pData->Persist.bState = ON;
         }

      }
      else if ( ER_STATE_STOPPED != EngineState ) // We are in the starting or running state
      {
         // Check if the previous exceed is on because we aren't in stopped anymore
         if ( TRUE == pData->Persist.bLatch )
         {
            pData->Persist.bState = OFF;
            // TODO: Write a Log
         }

         // We have received an ACK now check if there is an Action to Acknowledge
         if ( TRUE == bACK )
         {
            ActionCheckACK ( pCfg, pData );
         }
      }
      else // We are in the stopped state
      {
         if ( (( TRUE == bACK ) && ( TRUE == pData->Persist.bLatch )) ||
               ((ER_STATE_STOPPED == pData->PrevEngState) && (ER_STATE_STOPPED != EngineState)) )
         {
            // Clear the previous exceed outputs
            pData->Persist.bState = OFF;
            // TODO: Write A Log
         }
      }
   }
   else
   {
      // Have we received an ACK now check if there is an Action to Acknowledge
      if ( TRUE == bACK )
      {
         ActionCheckACK ( pCfg, pData );
      }
   }
   // Save the Engine Run State
   pData->PrevEngState = EngineState;

   // Check the Event Action data and set the outputs accordingly
   ActionUpdateOutputs ( pCfg, pData );

}

/******************************************************************************
 * Function:     ActionUpdateFlags
 *
 * Description:
 *
 * Parameters:
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void ActionUpdateFlags ( ACTION_DATA *pData )
{
   // Local Data
   ACTION_REQUEST NewRequest;
   ACTION_FLAGS   *pFlags;
   UINT8          i;

   // Pop any new Action Requests from the FIFO
   while (0 < m_Request.RecordCnt)
   {
      FIFO_PopBlock(&m_Request.RecordFIFO, &NewRequest, sizeof(NewRequest));
      m_Request.RecordCnt--;

//      GSE_DebugStr(NORMAL,TRUE,"Update Flags: EI: %d A: 0x%08x S: %d", NewRequest.Index, NewRequest.Action, NewRequest.State );

      // Loop through the possible action bits
      for ( i = 0; i < MAX_ACTION_DEFINES; i++ )
      {
         // Check if an action flag is set
         if ( 0 != BIT( (NewRequest.Action & ACTION_MASK), i ) )
         {
            pFlags = &pData->Action[i];

            // Initiate the Action
            if (((ON_MET == NewRequest.State) && (0 != (ACTION_WHEN_MET & NewRequest.Action))) ||
                  ((ON_DURATION == NewRequest.State) &&
                   (0 != (ACTION_ON_DURATION & NewRequest.Action))))
            {
               // Set the bit for the Action
               SetBit(NewRequest.Index, pFlags->Active, sizeof(pFlags->Active));

               if (0 != (NewRequest.Action & ACTION_LATCH))
               {
                  // Set the bit for the Latch
                  SetBit(NewRequest.Index, pFlags->Latch, sizeof(pFlags->Latch));
               }

               if ( 0 != (NewRequest.Action & ACTION_ACK))
               {
                  // Set the bit for the ACK
                  SetBit(NewRequest.Index, pFlags->ACK, sizeof(pFlags->ACK));
               }
            }
            else if ( ACTION_OFF == NewRequest.State )
            {
               // Check if the action latched before resetting the event flag
               if ( FALSE == GetBit(NewRequest.Index, pFlags->Latch, sizeof(pFlags->Latch)))
               {
                  // Reset the Active flag for the Event
                  ResetBit(NewRequest.Index, pFlags->Active, sizeof(pFlags->Active));
                  // Reset the Acknowledge for the Event
                  ResetBit(NewRequest.Index, pFlags->ACK, sizeof(pFlags->ACK));
               }
            }
         }
      }
   }
}


/******************************************************************************
 * Function:     ActionCheckACK
 *
 * Description:
 *
 * Parameters:
 *
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void ActionCheckACK ( ACTION_CFG  *pCfg, ACTION_DATA *pData )
{
   // Local Data
   ACTION_FLAGS *pFlags;
   UINT8         ActionIndex;
   BITARRAY128   MaskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };

   // Loop through all the Action
   for ( ActionIndex = 0; ActionIndex < MAX_ACTION_DEFINES; ActionIndex++ )
   {
      pFlags = &pData->Action[ActionIndex];

      // Check if any ACK flag is set
      if ( TRUE == TestBits( MaskAll, sizeof(MaskAll),
                             pFlags->ACK, sizeof(pFlags->ACK), FALSE ) )
      {
         // Need to check if any of the Active flags match the ACK
         if ( TRUE == TestBits( pFlags->ACK, sizeof(pFlags->ACK),
                                pFlags->Active, sizeof(pFlags->Active), FALSE))
         {
            // We know we have an Active Flag to ACK, so reset the Active bits
            ResetBits ( pFlags->ACK, sizeof(pFlags->ACK),
                        pFlags->Active, sizeof(pFlags->Active) );

            // Now we need to figure out if we Acknowledged a latched event
            // If it is latched and the Persistent Logic is enabled
            // save a flag to let the software know we should perform the persistent
            // action
            if (TRUE == TestBits (pFlags->ACK, sizeof(pFlags->ACK),
                                  pFlags->Latch, sizeof(pFlags->Latch), FALSE))
            {
               pData->Persist.bLatch = TRUE;
               // TODO: This should be stored to non-volatile memory ASAP!!

               // If the persist action is enabled then the action is based on the
               // persistent output configuration, otherwise we should save the action
               // that is latched so it can be restored after a powercycle
               if (FALSE == pCfg->Persist.bEnabled)
               {
                  pData->Persist.Action = pCfg->Output[ActionIndex];
                  // TODO: This also needs to go into non-volatile memory ASAP
               }

               // Since we have cleared with an ACK and then set the persistent latch
               // we can now clear the latch for this event.
               ResetBits ( pFlags->ACK, sizeof(pFlags->ACK),
                           pFlags->Latch, sizeof(pFlags->Latch) );
            }
            // Clear the ACK for all events
            memset(&pFlags->ACK, 0, sizeof(pFlags->ACK));
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionUpdateOutputs
 *
 * Description:
 *
 * Parameters:
 *
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
static
void ActionUpdateOutputs ( ACTION_CFG *pCfg, ACTION_DATA *pData )
{
   // Local Data
   UINT8          LSS;
   DIO_OUT_OP     Output;
   ACTION_FLAGS  *pFlags;
   ACTION_OUTPUT *pOutCfg;
   UINT8          i;
   UINT16         ActionIndex;
   BITARRAY128    MaskAll = { 0xFFFF,0xFFFF,0xFFFF,0xFFFF };

   // Check if persistent is ON or OFF because the Persist ON is highest
   // priority
   if ( ON == pData->Persist.bState )
   {
      for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
      {
         LSS = 0;
         if ( TRUE == BIT( pData->Persist.Action.UsedMask, i ) )
         {
            pData->PriorityMask = SET_BIT(pData->PriorityMask,i);
            LSS    = SET_BIT(LSS,i);
            Output = (ON == BIT ( pData->Persist.Action.LSS_Mask, i )) ?
                     DIO_SetHigh : DIO_SetLow;
         }
         ActionSetOuptut ( LSS, Output );
      }

      // Now loop through the Actions and set any outputs that don't overlap
      // the persistent outputs
      for ( ActionIndex = 0; ActionIndex < MAX_ACTION_DEFINES; ActionIndex++ )
      {
         pFlags  = &pData->Action[ActionIndex];
         pOutCfg = &pCfg->Output[ActionIndex];

         // First check if any event is active for this Action
         if ( TRUE == TestBits( MaskAll, sizeof(MaskAll),
                                pFlags->Active, sizeof(pFlags->Active), FALSE))
         {
            // Find the USED LSS Bits
            for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
            {
               LSS = 0;
               // Check if this Action uses the LSS
               if ( TRUE == BIT( pOutCfg->UsedMask, i ) )
               {
                  // Now make sure the persist action isn't using this LSS
                  if ( FALSE == BIT( pData->Persist.Action.UsedMask, i )  )
                  {
                     // Now Make sure a higher priority Action hasn't already used this LSS
                     if ( FALSE == BIT( pData->PriorityMask, i ) )
                     {
                        LSS = 0;
                        // Set the LSS
                        LSS    = SET_BIT ( LSS, i );
                        Output = ( ON == BIT( pOutCfg->LSS_Mask,i ) ) ?
                                 DIO_SetHigh : DIO_SetLow;
                        pData->PriorityMask = SET_BIT( pData->PriorityMask, i );
                        pFlags->bState = ON;
                     }
                  }
               }
               ActionSetOuptut ( LSS, Output );
            }
         }
         else // If the Action was on and is now off we need to turn off
         {
            if ( ON == pFlags->bState )
            {
               LSS    = SET_BIT ( LSS, i );
               Output = (ON == BIT( pOutCfg->LSS_Mask, i)) ? DIO_SetLow : DIO_SetHigh;
               pFlags->bState = OFF;
               ActionSetOuptut ( LSS, Output );
            }
         }
      }
   }
   else // Persist is OFF
   {
      // Shut off persistent outputs - Reset Priority Mask
      // Update the Action Outputs in Priority Order
      // Loop through the Actions and see if any bit is set Active
      // Also check the mask because the priority is only for actions that
      // share a mask
      for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
      {
         LSS = 0;
         if ( TRUE == BIT( pData->Persist.Action.UsedMask, i ) )
         {
            pData->PriorityMask = RESET_BIT ( pData->PriorityMask, i );
            LSS    = RESET_BIT ( LSS, i );
            Output = ( ON == BIT( pData->Persist.Action.LSS_Mask,i) ) ?
                     DIO_SetLow : DIO_SetHigh ;
            ActionSetOuptut ( LSS, Output );
         }

      }

      // Now loop through the Actions and set any outputs
      for ( ActionIndex = 0; ActionIndex < MAX_ACTION_DEFINES; ActionIndex++ )
      {
         pFlags  = &pData->Action[ActionIndex];
         pOutCfg = &pCfg->Output[ActionIndex];

         // First check if any event is active for this Action
         if ( TRUE == TestBits( MaskAll, sizeof(MaskAll),
                                pFlags->Active, sizeof(pFlags->Active), FALSE))
         {
            // Find the USED LSS Bits
            for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
            {
               LSS = 0;
               // Check if this Action uses the LSS
               if ( TRUE == BIT( pOutCfg->UsedMask, i ) )
               {
                  // Now Make sure a higher priority Action hasn't already used this LSS
                  if ( FALSE == BIT( pData->PriorityMask, i ) )
                  {
                     if (OFF == pFlags->bState)
                     {
                        // Set the LSS
                        LSS    = SET_BIT ( LSS, i );
                        Output = (ON == BIT( pOutCfg->LSS_Mask,i)) ?
                                 DIO_SetHigh : DIO_SetLow;
                        pData->PriorityMask = SET_BIT( pData->PriorityMask, i );
                        ActionSetOuptut ( LSS, Output );
                        GSE_DebugStr(NORMAL,TRUE,"Output ON: LSS %d  State %d", LSS, Output  );
                     }
                  }
                  pFlags->bState = ON;

               }
            }
         }
         else // If the Action was on and is now off we need to turn off
         {
            if ( ON == pFlags->bState )
            {
               for ( i = 0; i < MAX_OUTPUT_LSS; i++ )
               {
                  LSS = 0;
                  if ( TRUE == BIT( pOutCfg->UsedMask, i ) )
                  {
                     LSS    = SET_BIT ( LSS, i );
                     Output = (ON == BIT( pOutCfg->LSS_Mask, i)) ?
                              DIO_SetLow : DIO_SetHigh;
                     ActionSetOuptut ( LSS, Output );
                     GSE_DebugStr(NORMAL,TRUE,"Output OFF: LSS %d  State %d", LSS, Output  );
                     pData->PriorityMask = RESET_BIT( pData->PriorityMask, i );
                  }
               }
               pFlags->bState = OFF;
            }
         }
      }
   }
}

/******************************************************************************
 * Function:     ActionSetOutput
 *
 * Description:
 *
 *
 * Parameters:
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static
void ActionSetOuptut ( UINT8 LSS, DIO_OUT_OP State )
{
   switch (LSS)
   {
      case LSS0MASK:
         DIO_SetPin(LSS0, State);
         break;
      case LSS1MASK:
         DIO_SetPin(LSS1, State);
         break;
      case LSS2MASK:
         DIO_SetPin(LSS2, State);
         break;
      case LSS3MASK:
         DIO_SetPin(LSS3, State);
         break;
      default:
         break;
   }
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ActionManager.c $
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 12-07-17   Time: 11:32a
 * Created in $/software/control processor/code/system
 * SCR 1107
 *
  ***************************************************************************/


