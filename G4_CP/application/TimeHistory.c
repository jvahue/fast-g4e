#define TIMEHISTORY_BODY
/**********************************************************************************************
                    Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
                              Altair Engine Diagnostic Solutions
                      All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.c
    
    Description: 
 
    VERSION
    $Revision: 3 $  $Date: 5/02/12 4:53p $   
   
**********************************************************************************************/

/*********************************************************************************************/
/* Compiler Specific Includes                                                                */
/*********************************************************************************************/    
 
/*********************************************************************************************/
/* Software Specific Includes                                                                */
/*********************************************************************************************/
#include "timehistory.h"
#include "clockmgr.h"
/*********************************************************************************************/
/* Local Defines                                                                             */
/*********************************************************************************************/
 
/*********************************************************************************************/
/* Local Typedefs                                                                            */
/*********************************************************************************************/
 
/*********************************************************************************************/
/* Local Variables                                                                           */
/*********************************************************************************************/
//static UINT8                 TimeHistoryRawBuffer[MAX_TIMEHISTORY_BUFFER];
static UINT8                 numSensorsUsed;

/*********************************************************************************************/
/* Local Function Prototypes                                                                 */
/*********************************************************************************************/
 
/*********************************************************************************************/
/* Public Functions                                                                          */
/*********************************************************************************************/
 
/**********************************************************************************************
 * Function:    TimeHistoryInitialize
 *
 * Description: Initiailizes the Time History data and buffers.
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
void TimeHistoryInitialize ( void )
{
   // Local Data
   UINT8 SensorIndex;
   
   // Determine how many sensors are configured 
   // so the time history record sizes can be calculated.
   numSensorsUsed = 0;
   
   // Loop through all the sensors and count those that are used
   for ( SensorIndex = 0; SensorIndex < MAX_SENSORS; SensorIndex++ )
   {
      if ( TRUE == SensorIsUsed((SENSOR_INDEX)SensorIndex) )
      {
         numSensorsUsed++;
      }
   }
}

/**********************************************************************************************
 * Function:    TimeHistoryStart
 *
 * Description: 
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
void TimeHistoryStart(TIMEHISTORY_TYPE Type, UINT16 Time_s)
{
   
}

/**********************************************************************************************
 * Function:    TimeHistoryEnd
 *
 * Description: 
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
void TimeHistoryEnd(TIMEHISTORY_TYPE Type, UINT16 Time_s)
{
   
}

/**********************************************************************************************
 * Function:    
 * Description:
 * Parameters:
 * Returns:
 * Notes:
 *
 *********************************************************************************************/

/*********************************************************************************************/
/* Local Functions                                                                           */
/*********************************************************************************************/

/**********************************************************************************************
 * Function:     TimeHistoryUpdateTask
 *
 * Description:  
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
static
void TimeHistoryUpdateTask ( void )
{
   // Local Data 
   
}   

/**********************************************************************************************
 * Function:     TimeHistoryBuildRecord
 *
 * Description:  Store Time History logs to the RAM data buffer at the configured rate.
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
static
UINT16 TimeHistoryBuildRecord ( TIMEHISTORYRECORD *Record )
{
   // Local Data
   UINT8                      SensorIndex;
   UINT16                     BufferIndex;
   UINT16                     SummarySize;
   TIMEHISTORY_SENSOR_SUMMARY Summary;
   
   // Initialize Local Data
   SummarySize = sizeof (TIMEHISTORY_SENSOR_SUMMARY);
   BufferIndex = 0;
   
   CM_GetTimeAsTimestamp ( &Record->Time );
   Record->nNumSensors = numSensorsUsed;
   
   for ( SensorIndex = 0; SensorIndex < MAX_SENSORS; SensorIndex++ )
   {
      if ( SensorIsUsed ( (SENSOR_INDEX)SensorIndex ) )   
      {
         Summary.Index    = SensorIndex;
         Summary.Value    = SensorGetValue((SENSOR_INDEX)SensorIndex);
         Summary.Validity = SensorIsValid ((SENSOR_INDEX)SensorIndex);
      }
      
      memcpy ( &Record->RawSensorBuffer + BufferIndex, &Summary, SummarySize );
      
      BufferIndex += ( SensorIndex * SummarySize );
   }
   
   return BufferIndex;
}   


/**********************************************************************************************
 * Function:
 * Description:
 * Parameters:
 * Returns:
 * Notes:
 *
 *********************************************************************************************/
/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.c $
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 5/02/12    Time: 4:53p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 4/27/12    Time: 5:04p
 * Updated in $/software/control processor/code/application
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 4/24/12    Time: 10:59a
 * Created in $/software/control processor/code/application
 * 
 *
 *********************************************************************************************/
