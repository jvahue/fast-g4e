#define FAULT_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        fault.c
    
    Description: Fault runtime processing
                
                 The fault processing object is currently used to determine the 
                 validity of a specified sensor based on the criteria of other 
                 sensors. (Sensor consistency check). 
       
    Notes: 
  
    VERSION
      $Revision: 17 $  $Date: 12-10-19 10:55a $     
  
******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/ 
#include "Cfgmanager.h"
#include "fault.h"
#include "sensor.h"
#include "user.h"

/*****************************************************************************/
/* Local Defines                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static FAULT_CONFIG ConfigTemp;
static FAULT_CONFIG m_FaultCfg[MAX_FAULTS];
/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static BOOLEAN FaultCheckSensorStates  ( FAULT_CONFIG *pFaultCfg);
static BOOLEAN FaultCheckSensorTriggers( FAULT_CONFIG *pFaultCfg);

// Include tables and command functions here
#include "FaultUserTables.c"

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/******************************************************************************
 * Function:     FaultInitialize
 *
 * Description:  The fault initialize function reads the static configuration,
 *               initializes the fault arrays.
 *               
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void FaultInitialize( void)
{    
  
   // Add user commands for the faults to the user command tables
   User_AddRootCmd(&RootFaultMsg);
 
   // Reset the Fault configuration storage array  
   memset(m_FaultCfg, 0, sizeof(m_FaultCfg));
  
   // Load the current configuration to the fault configuration array.
   memcpy(m_FaultCfg,
          CfgMgr_RuntimeConfigPtr()->FaultConfig,
          sizeof(m_FaultCfg));
}

/******************************************************************************
 * Function:    FaultCompareValues 
 *
 * Description: Check to see if fault has triggered/faulted. 
 *               
 *               
 * Parameters:  Pointer to Fault object. 
 *
 * Returns:     Boolean indicating if all relevant triggers & devices
 *              have triggered/faulted. 
 *
 * Notes:        
 *
 *****************************************************************************/
BOOLEAN FaultCompareValues( FAULT_INDEX Fault ) 
{
  // Local Data
  BOOLEAN FaultMet;
  BOOLEAN f1;
  BOOLEAN f2;
  FAULT_CONFIG *pFaultCfg;
  
  // Initialize Local Data
  pFaultCfg = &m_FaultCfg[Fault];  
  FaultMet = FALSE;

  // Check the states, triggers, of the Fault
  f1 = FaultCheckSensorStates( pFaultCfg);
  f2 = FaultCheckSensorTriggers( pFaultCfg);
  if ( f1 && f2)
  {
    FaultMet = TRUE;
  }

  return FaultMet;
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    FaultCheckSensorStates 
 *
 * Description: This function determines if all sensor states are as specified
 *              by the fault.
 *               
 * Parameters:  pFault (i): pointer to the fault being processed 
 *
 * Returns:     TRUE if sensor states are consistent with their criteria, 
 *              FALSE otherwise 
 *
 * Notes:       This code short circuits the computation as much as possible.  
 *              Once a sensor state does not match or a sensor ID is invalid, 
 *              or a sensor is unused the processing is complete. 
 *
 *****************************************************************************/
static BOOLEAN FaultCheckSensorStates( FAULT_CONFIG *pFaultCfg )
{
  // Local Data
  BOOLEAN        sensorStateMatch;
  FAULT_TRIGGER *pFltTrig;
  SENSOR_INDEX   SensorIndex;
  UINT16         FltTrigIndex;
  
  // Initialize Local Data
  sensorStateMatch = TRUE;
  
  // Initialize the Fault Trigger to analyze
  pFltTrig    = &pFaultCfg->faultTrigger[0];

  // Loop though all the sensors to test
  for ( FltTrigIndex = 0; 
        ((FltTrigIndex < MAX_FAULT_TRIGGERS) && pFltTrig->SensorIndex != SENSOR_UNUSED && 
         sensorStateMatch); 
        FltTrigIndex++ )
  {
     // Initialize the sensor index to use
     SensorIndex = pFltTrig->SensorIndex;
    
     // Check if fault is met and the sensor is used
     if ( SensorIsUsed(SensorIndex) )
     {
        // check for the sensorStateMatch  
        sensorStateMatch &= ((FALSE == SensorIsValid(SensorIndex)) ^ 
                             (HAS_NO_FAULT == pFltTrig->SenseOfSignal));
     }

     // next sensors
     pFltTrig++;
  }

  return sensorStateMatch;
}
/******************************************************************************
 * Function:    FaultCheckSensorTriggers 
 *
 * Description: This function determines if all sensor triggers are as 
 *              specified by the fault.
 *               
 * Parameters:  pFault (i): pointer to the fault being processed 
 *
 * Returns:     TRUE if sensor states are consistent with their criteria, 
 *              FALSE otherwise 
 *
 * Notes:       This code short circuits the computation as much as possible.  
 *              Once a sensor state does not match or a sensor ID is invalid,
 *              or a sensor is unused the processing is complete. 
 *
 *              This Code is always going to return FaultMet = TRUE if no 
 *              comparisons are defined.
 *
 *****************************************************************************/
static BOOLEAN FaultCheckSensorTriggers( FAULT_CONFIG *pFaultCfg)
{
   // Local Data
   SENSOR_INDEX  SensorIndex;
   UINT16        TriggerIndex;
   BOOLEAN       FaultMet;
   FAULT_TRIGGER *pFltTrig;
   
   // Initialize Local Data
   FaultMet = TRUE;
   
   // Initialize the Fault Trigger to analyze
   pFltTrig = &pFaultCfg->faultTrigger[0];

   // Loop through all the triggers for this Fault
   for (TriggerIndex = 0; 
        ((TriggerIndex < MAX_FAULT_TRIGGERS) &&
         (pFltTrig->SensorIndex != SENSOR_UNUSED) && FaultMet);
        TriggerIndex++)
   {
      // Initialize the sensor index to use
      SensorIndex = pFltTrig->SensorIndex;
 
      if (SensorIsUsed(SensorIndex) && (NO_COMPARE != pFltTrig->SensorTrigger.Compare)) 
      {
         FaultMet = SensorIsValid(SensorIndex);

         FaultMet = FaultMet &&
             TriggerCompareValues(SensorGetValue(SensorIndex),
                                  pFltTrig->SensorTrigger.Compare,
                                  pFltTrig->SensorTrigger.fValue,
                                  SensorGetPreviousValue(SensorIndex));
      }

      // next sensor check
      pFltTrig++;
   }
   
   return FaultMet;
}
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Fault.c $
 * 
 * *****************  Version 17  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 10:55a
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 11:13a
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:40p
 * Updated in $/software/control processor/code/system
 * SCR# 649 - add NO_COMPARE back in for use in Signal Tests.
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 * 
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 5/03/10    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 530: Error: Trigger NO_COMPARE infinite logs
 * Removed NO_COMPARE from code.
 * 
 * *****************  Version 11  *****************
 * User: Jeff Vahue   Date: 4/23/10    Time: 6:55p
 * Updated in $/software/control processor/code/system
 * SCR #565 - refactor code while trying to determine failure issue.
 * Failure not present in refactored code.
 * 
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:10p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - typos in headers unsued -> unused
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR 374
 * - Removed the Fault check type and deleted the Fault Update function.
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:45p
 * Updated in $/software/control processor/code/system
 * SCR 42 and SCR 106
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 12/02/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #350 Misc Code Review Items
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 10/21/09   Time: 4:09p
 * Updated in $/software/control processor/code/system
 * SCR 311
 * - Removed the comparison to previous for everything except NOT_EQUAL.
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 10/20/09   Time: 3:05p
 * Updated in $/software/control processor/code/system
 * SCR 308
 * - Added check for NO_COMPARE before performing comparison.
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 10/07/09   Time: 11:59a
 * Updated in $/software/control processor/code/system
 * Updated per SCR 292
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 10/06/09   Time: 5:05p
 * Updated in $/software/control processor/code/system
 * Updates for requirments implementation
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Updated user tables
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 9/30/09    Time: 11:15a
 * Created in $/software/control processor/code/system
 * 
 *
 ***************************************************************************/

/*EOF*/
