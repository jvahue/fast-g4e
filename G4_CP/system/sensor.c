#define SENSOR_BODY
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        sensor.c

    Description: Sensor runtime processing

    A sensor is an object that holds the value of some measurable
      input. The sensor defines what type of input is being measured
      (analog, digital, pulse, etc.), how often the input is sampled,
      what filtering is done on the inputs, and what types of conversion
      is done on the measured values.

    For instance, Engine Temperature might be an analog input on channel
      2. It might be sampled 5 times a second and averaged to reduce
      noise, then converted from analog input units to degrees C once
      every second.

    Each sensor has a value which may be read at any time, and the
      value represents the current result of all the previously
      mentioned calculations.

    Notes:

    VERSION
      $Revision: 72 $  $Date: 12-09-11 2:13p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <math.h>
#include <float.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "adc.h"
#include "assert.h"
#include "GSE.h"
#include "sensor.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
// Sensor Live Data Task Parameters
typedef struct
{
  UINT32  StartTime;
}SENSOR_LD_PARMS;
/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static SENSOR             Sensors[MAX_SENSORS]; // Dynamic Sensor Storage
static UINT16             maxSensorUsed;   // Storage for largest indexed cfg
static SENSOR_LD_PARMS    LiveDataBlock;   // Live Data Task block
/*static SENSORLOG SensorLog;*/
static SENSOR_CONFIG      m_SensorCfg[MAX_SENSORS]; // Sensors cfg array
static SENSOR_LD_CONFIG   LiveDataDisplay; // runtime copy of LiveData config data

// include the usertable here so local vars are declared
#include "SensorUserTables.c" // Include user cmd tables & functions .c


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void          SensorsConfigure    ( void );
static void          SensorsReset        ( void );
static void          SensorReset         ( SENSOR_INDEX Sensor );
static void          SensorInitFilter    ( FILTER_CONFIG *pFilterCfg,
                                           FILTER_DATA   *pFilterData,
                                           UINT16         nMaximumSamples);
static void          SensorsUpdateTask   ( void *pParam );
static void          SensorRead          ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor );
static BOOLEAN       SensorCheck         ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                              FLOAT32       fVal,     FLOAT32 filterVal);
static SENSOR_STATUS SensorRangeTest     ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                              FLOAT32       filterVal );
static SENSOR_STATUS SensorRateTest      ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                              FLOAT32       fVal );
static SENSOR_STATUS SensorSignalTest    ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                              FLOAT32       fVal );
static SENSOR_STATUS SensorInterfaceTest ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                              FLOAT32       fVal );
static BOOLEAN       SensorNoTest        ( UINT16 nIndex );
static BOOLEAN       SensorNoInterface   ( UINT16 nIndex );
static void          SensorLogFailure    ( SENSOR_CONFIG *pConfig, SENSOR  *pSensor,
                                           FLOAT32       fVal,   SENSORFAILURE type );

static FLOAT32       SensorApplyFilter   ( FLOAT32 fVal,
                                           SENSOR_CONFIG *pConfig, SENSOR  *pSensor );

static void          SensorPersistCounter      ( BOOLEAN set, BOOLEAN reset,
                                           FILTER_CONFIG *pFilterCfg,
                                           FILTER_DATA *pFilterData);

static FLOAT32       SensorExponential   ( FLOAT32 fVal, FILTER_DATA *pFilterData );

static FLOAT32       SensorSpikeReject   ( FLOAT32 fVal,
                                           FILTER_CONFIG *pFilterCfg,
                                           FILTER_DATA   *pFilterData);

static FLOAT32       SensorMaxReject     ( FLOAT32 fVal,
                                           FILTER_CONFIG *pFilterCfg,
                                           FILTER_DATA *pFilterData);

static FLOAT32       SensorSlope         ( FLOAT32 fVal, SENSOR_CONFIG *pConfig,
                                           SENSOR *pSensor);

static void          SensorsLiveDataTask ( void *pParam );
static void          SensorDumpASCIILiveData ( void );


const CHAR *SensorFailureNames[MAX_FAILURE+1] =
{
  "No Failure",
  "BIT Failure",
  "Signal Failure",
  "Range Failure",
  "Rate Failure",
  0
};

const CHAR *SensorFilterNames[MAXFILTER+1] =
{
   "No Filter",
   "Simple Average Filter",
   "Spike Reject Filter",
   "Max Reject Filter",
   "Slope Filter",
   0
};

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     SensorsInitialize
 *
 * Description:  The sensor initialize function reads the static configuration,
 *               initializes the sensor arrays, and initializes the sensor
 *               tasks.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void SensorsInitialize( void)
{
  // Local Data
  TCB TaskInfo;

  // Add user commands for the sensors to the user command tables
  User_AddRootCmd(&RootSensorMsg);
  User_AddRootCmd(&LiveDataMsg);

  // Reset the Sensor configuration storage array
  memset(&m_SensorCfg, 0, sizeof(m_SensorCfg));

  // Load the current configuration to the sensor configuration array.
  memcpy(&m_SensorCfg,
         &(CfgMgr_RuntimeConfigPtr()->SensorConfigs),
         sizeof(m_SensorCfg));

  // init runtime copy of live data config
  memcpy(&LiveDataDisplay, &(CfgMgr_RuntimeConfigPtr()->LiveDataConfig),
             sizeof(LiveDataDisplay));

  // Reset the dynamic sensor storage array
  SensorsReset();
  // Configure the sensor interface parameters
  SensorsConfigure();

  // Create Sensor Task - DT
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Sensor Task",_TRUNCATE);
  TaskInfo.TaskID           = Sensor_Task;
  TaskInfo.Function         = SensorsUpdateTask;
  TaskInfo.Priority         = taskInfo[Sensor_Task].priority;
  TaskInfo.Type             = taskInfo[Sensor_Task].taskType;
  TaskInfo.modes            = taskInfo[Sensor_Task].modes;
  TaskInfo.MIFrames         = taskInfo[Sensor_Task].MIFframes;
  TaskInfo.Enabled          = TRUE;
  TaskInfo.Locked           = FALSE;

  TaskInfo.Rmt.InitialMif   = taskInfo[Sensor_Task].InitialMif;
  TaskInfo.Rmt.MifRate      = taskInfo[Sensor_Task].MIFrate;
  TaskInfo.pParamBlock      = NULL;

  TmTaskCreate (&TaskInfo);

  // Create Sensor Live Data Task - DT
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Sensor Live Data Task",_TRUNCATE);
  TaskInfo.TaskID           = Sensor_Live_Data_Task;
  TaskInfo.Function         = SensorsLiveDataTask;
  TaskInfo.Priority         = taskInfo[Sensor_Live_Data_Task].priority;
  TaskInfo.Type             = taskInfo[Sensor_Live_Data_Task].taskType;
  TaskInfo.modes            = taskInfo[Sensor_Live_Data_Task].modes;
  TaskInfo.MIFrames         = taskInfo[Sensor_Live_Data_Task].MIFframes;
  TaskInfo.Enabled          = TRUE;
  TaskInfo.Locked           = FALSE;

  TaskInfo.Rmt.InitialMif   = taskInfo[Sensor_Live_Data_Task].InitialMif;
  TaskInfo.Rmt.MifRate      = taskInfo[Sensor_Live_Data_Task].MIFrate;
  TaskInfo.pParamBlock      = &LiveDataBlock;

  LiveDataBlock.StartTime   = 0;
  TmTaskCreate (&TaskInfo);

}

/******************************************************************************
 * Function:     SensorIsValid
 *
 * Description:  This function provides a method to determine if a sensor
 *               is valid.
 *
 * Parameters:   [in] nSensor - Index of sensor to check
 *
 * Returns:      [out] BOOLEAN - TRUE  = Valid
 *                               FALSE = Invalid
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN SensorIsValid( SENSOR_INDEX Sensor )
{
  // returns the sensor validity
  return (Sensors[Sensor].bValueIsValid);
}

/******************************************************************************
 * Function:     SensorIsUsed
 *
 * Description:  This function provides a method to determine if a sensor is
 *               configured to be used.
 *
 * Parameters:   [in] nSensor - Index of sensor to check
 *
 * Returns:      [out] BOOLEAN - TRUE  = used
 *                               FALSE = not used
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN SensorIsUsed( SENSOR_INDEX Sensor)
{
  // return if the sensor is used
  return (BOOLEAN)(m_SensorCfg[Sensor].Type != UNUSED);
}

/******************************************************************************
 * Function:     SensorGetValue
 *
 * Description:  This function provides a method to read the value of a sensor.
 *
 * Parameters:   [in] nSensor - Index of sensor to get value
 *
 * Returns:      [out] FLOAT32 - Current value of sensor
 *
 * Notes:        None
 *
 *****************************************************************************/
FLOAT32 SensorGetValue( SENSOR_INDEX Sensor)
{
  // return the current value of the sensor
  return (Sensors[Sensor].fValue);
}

/******************************************************************************
 * Function:     SensorGetPreviousValue
 *
 * Description:  This function provides a method to read the previous value
 *               of a sensor.
 *
 * Parameters:   [in] nSensor - Index of sensor to get value
 *
 * Returns:      [out] FLOAT32 - Current value of sensor
 *
 * Notes:        None
 *
 *****************************************************************************/
FLOAT32 SensorGetPreviousValue( SENSOR_INDEX Sensor )
{
   // return the previous value of the sensor
   return (Sensors[Sensor].fPriorValue);
}

/******************************************************************************
 * Function:     SensorGetPreviousValid
 *
 * Description:  This function provides a method to read the previous value's
 *                validity of a sensor.
 *
 * Parameters:   [in] nSensor - Index of sensor to get value
 *
 * Returns:      [out] BOOLEAN - Validity of the prev sensor value(
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN SensorGetPreviousValid( SENSOR_INDEX Sensor )
{
   // return the previous validity of the sensor
   return (Sensors[Sensor].PriorValueValid);
}

/******************************************************************************
 * Function:     SensorGetSystemHdr
 *
 * Description:  Retrieves the binary system header for the sensor
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
UINT16 SensorGetSystemHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   SENSOR_HDR          SensorHdr[MAX_SENSORS];
   UINT16              SensorIndex;
   INT8                *pBuffer;
   UINT16              nRemaining;
   UINT16              nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( &SensorHdr, 0, sizeof(SensorHdr) );

   // Loop through all the sensors
   for ( SensorIndex = 0;
         ((SensorIndex < MAX_SENSORS) && (nRemaining > sizeof (SENSOR_HDR)));
         SensorIndex++ )
   {
      // Copy the name and units
      strncpy_safe( SensorHdr[SensorIndex].Name,
                    sizeof(SensorHdr[0].Name),
                    m_SensorCfg[SensorIndex].SensorName,
                    _TRUNCATE);
      strncpy_safe( SensorHdr[SensorIndex].Units,
                    sizeof(SensorHdr[0].Units),
                    m_SensorCfg[SensorIndex].OutputUnits,
                    _TRUNCATE);
      // Increment the total used and decrement the amount remaining
      nTotal     += sizeof (SENSOR_HDR);
      nRemaining -= sizeof (SENSOR_HDR);
   }

   // Copy the Header to the buffer
   memcpy ( pBuffer, &SensorHdr, nTotal );
   // Return the total bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     SensorGetETMHdr
 *
 * Description:  Retrieves the binary ETM header for the sensor
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
UINT16 SensorGetETMHdr ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   SENSOR_ETM_HDR      sensorETMHdr[MAX_SENSORS];
   UINT16              sensorIndex;
   INT8                *pBuffer;
   UINT16              nRemaining;
   UINT16              nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( &sensorETMHdr, 0, sizeof(sensorETMHdr) );

   // Loop through all the sensors
   for ( sensorIndex = 0;
         ((sensorIndex < MAX_SENSORS) && (nRemaining > sizeof (sensorETMHdr[sensorIndex])));
         sensorIndex++ )
   {
      sensorETMHdr[sensorIndex].configured = FALSE;

      if ( UNUSED != m_SensorCfg[sensorIndex].Type )
      {
         sensorETMHdr[sensorIndex].configured = TRUE;
      }

      // Copy the name and units
      strncpy_safe( sensorETMHdr[sensorIndex].sensor.Name,
                    sizeof(sensorETMHdr[sensorIndex].sensor.Name),
                    m_SensorCfg[sensorIndex].SensorName,
                    _TRUNCATE);
      strncpy_safe( sensorETMHdr[sensorIndex].sensor.Units,
                    sizeof(sensorETMHdr[sensorIndex].sensor.Units),
                    m_SensorCfg[sensorIndex].OutputUnits,
                    _TRUNCATE);
      // Increment the total used and decrement the amount remaining
      nTotal     += sizeof (SENSOR_ETM_HDR);
      nRemaining -= sizeof (SENSOR_ETM_HDR);
   }

   // Copy the Header to the buffer
   memcpy ( pBuffer, &sensorETMHdr, nTotal );
   // Return the total bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     SensorDisableLiveStream
 *
 * Description:  SensorDisableLiveStream sets the live data mode.
 *
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
void SensorDisableLiveStream( void )
{
  LiveDataDisplay.Type = LD_NONE;
}

/******************************************************************************
 * Function:     SensorSetupSummaryArray
 *
 * Description:  Utility function which uses a provided big-endian Bit-array
 *               representing which which of the available sensors (127...0)
 *               are to be added to the provided list.
 *
 * Parameters:   [in/out] summary           - SNSR_SUMMARY array structure to be populated.
 *               [in]     summarySize       - The number of entries in SNSR_SUMMARY[].
 *               [in]     snsrMask          - Array of bits denoting which sensors to add to list.
 *               [in]     snsrMaskSizeBytes - Size of snsrMask[] in bytes.

 * Returns:      INT32 the number of entries added to the summary array
 *
 * Notes:
 *****************************************************************************/
 UINT16 SensorSetupSummaryArray (SNSR_SUMMARY summary[],  INT32  summarySize,
                                 UINT32       snsrMask[], INT32 snsrMaskSizeBytes)
{
  SENSOR_INDEX snsrIdx;
  INT16        summaryCnt;
  INT16        i;
  TIMESTAMP    timeStampNow;

  // "Clear" the summary table by setting all indexes to "unused"
  for (i = 0; i < summarySize; ++i)
  {
    memset(&summary[i],0, sizeof(SNSR_SUMMARY));
    summary[i].SensorIndex = SENSOR_UNUSED;
  }

  CM_GetTimeAsTimestamp(&timeStampNow);

  // Loop thru the mask-of-sensors to be added to the Summary array
  // For each "ON" bit, initialize the next available SnsrSummary entry.

  summaryCnt = 0;
  for(snsrIdx = SENSOR_0; snsrIdx < MAX_SENSORS && summaryCnt < summarySize; ++snsrIdx)
  {
    if ( TRUE == GetBit(snsrIdx, snsrMask, snsrMaskSizeBytes ))
    {
      // Only store the first <summarySize> number of sensors.
      // Get the TOTAL COUNT of configured sensors in the mask.
      //  notify the caller if they have accidentally
      // configured more in the snsrMask than can be stored in the provided SNSR_SUMMARY
      i =  ++summaryCnt - 1;
      summary[i].SensorIndex  = snsrIdx;
      summary[i].bInitialized = FALSE;
      summary[i].bValid       = FALSE;
      summary[i].fMinValue    = FLT_MAX;
      summary[i].timeMinValue = timeStampNow;
      summary[i].fMaxValue    = 0.f;
      summary[i].timeMaxValue = timeStampNow;
      summary[i].fTotal       = 0.f;
      summary[i].fAvgValue    = 0.f;
    }
  }

  // Return the count of "ON" bits
  // in the snsrMask[]. Caller should check this to
  // ensure that the count did not exceed <summarySize>
  return summaryCnt;
}


/******************************************************************************
 * Function:     SensorUpdateSummaryItem
 *
 * Description:  Update the total, min and max for the passed SNSR_SUMMARY item.
 *
 * Parameters:   [in/out] pSummary - SNSR_SUMMARY structure to be populated.
 *
 * Returns:      None
 *
 * Notes:        None
 *****************************************************************************/
void SensorUpdateSummaryItem(SNSR_SUMMARY* pSummary)
{
  FLOAT32  NewValue = SensorGetValue(pSummary->SensorIndex);

  // Add value to total
  pSummary->fTotal += NewValue;

  if(NewValue < pSummary->fMinValue)
  {
    pSummary->fMinValue = NewValue;
    CM_GetTimeAsTimestamp(&pSummary->timeMinValue);
  }

  if( NewValue > pSummary->fMaxValue )
  {
    pSummary->fMaxValue = NewValue;
    CM_GetTimeAsTimestamp(&pSummary->timeMaxValue);
  }

  // Set the sensor validity
  pSummary->bValid = SensorIsValid((SENSOR_INDEX)pSummary->SensorIndex );
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:     SensorsConfigure
 *
 * Description:  Initializes the sensor interface indexes and function
 *               pointers for reading and testing the sensors.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void SensorsConfigure (void)
{
   // Local Data
   UINT16        i;
   SENSOR_CONFIG *pSensorCfg;

   // Loop through each sensor and setup the interface index and
   // the function pointers used to get the data and test the sensor
   // on the interface.
   for (i=0; i < MAX_SENSORS; i++)
   {
      // Set pointer to the current configuration
      pSensorCfg = &m_SensorCfg[i];

      // Configure the sensor interface based on the type
      switch (pSensorCfg->Type)
      {
         case UNUSED:
            // Nothing to do, go to next sensor
            break;
         case SAMPLE_ARINC429:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex  =
                            Arinc429MgrSensorSetup(pSensorCfg->GeneralPurposeA,
                                                 pSensorCfg->GeneralPurposeB,
                                                 pSensorCfg->nInputChannel,
                                                 i);
            // Set the function pointers for the ARINC429 interface
            Sensors[i].GetSensorData   = Arinc429MgrReadWord;
            Sensors[i].TestSensor      = Arinc429MgrSensorTest;
            Sensors[i].InterfaceActive = Arinc429MgrInterfaceValid;
            break;
         case SAMPLE_QAR:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex = QAR_SensorSetup (pSensorCfg->GeneralPurposeA,
                                                          pSensorCfg->GeneralPurposeB,
                                                          i);
            // Set the function pointers for the ARINC717 interface
            Sensors[i].GetSensorData    = QAR_ReadWord;
            Sensors[i].TestSensor       = QAR_SensorTest;
            Sensors[i].InterfaceActive  = QAR_InterfaceValid;
            break;
         case SAMPLE_ANALOG:
            // Set the interface index and the function pointers for the ADC
            Sensors[i].nInterfaceIndex  = pSensorCfg->nInputChannel;
            Sensors[i].GetSensorData    = ADC_GetValue;
            Sensors[i].TestSensor       = ADC_SensorTest;
            Sensors[i].InterfaceActive  = SensorNoInterface;
            break;
         case SAMPLE_DISCRETE:
            // Set the interface index and the function pointers for the Discretes
            Sensors[i].nInterfaceIndex  = pSensorCfg->nInputChannel;
            Sensors[i].GetSensorData    = DIO_GetValue;
            Sensors[i].TestSensor       = SensorNoTest;
            Sensors[i].InterfaceActive  = SensorNoInterface;
            break;
         case SAMPLE_UART:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex  =
                            UartMgr_SensorSetup(pSensorCfg->GeneralPurposeA,
                                                pSensorCfg->GeneralPurposeB,
                                                pSensorCfg->nInputChannel,
                                                i);
            // Set the function pointers for the UART interface
            Sensors[i].GetSensorData   = UartMgr_ReadWord;
            Sensors[i].TestSensor      = UartMgr_SensorTest;
            Sensors[i].InterfaceActive = UartMgr_InterfaceValid;
            break;
         default:
            // FATAL ERROR WITH CONFIGURATION
            // Initialize String
            FATAL("Sensor Config Fail: Sensor %d, Type %d not Valid",
                                                    i, pSensorCfg->Type);
            break;
      }
   }
}

/******************************************************************************
 * Function:     SensorsReset
 *
 * Description:  This function will go through all of the sensor configurations
 *               and reset the sensor's dynamic data if the sensor is configured.
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
static void SensorsReset (void)
{
  // Local Data
  UINT8 i;
  SENSOR_CONFIG *pSensorConfig;

  // track highest sensor number in use
  maxSensorUsed = 0;

  // Loop through the sensor
  for (i = 0; i < MAX_SENSORS; i++)
  {
    // Set a pointer the current sensor configuration
    pSensorConfig = &m_SensorCfg[i];

    // Check if the sensor is configured to be used
    if (pSensorConfig->Type != UNUSED)
    {
      // Reset the sensor's dynamic data
      SensorReset((SENSOR_INDEX)i);
      // Set the max sensor index to the current index
      maxSensorUsed = i;
    }
  }
}

/******************************************************************************
* Function:     SensorReset
*
* Description:  Reset one sensor's variables and values. This function is
*               called during initialization to clear the sensor's storage
*               object. This routine initializes the sensor data so that
*               valid readings may subsequently be taken.
*
* Parameters:   [in] nSensor - Sensor index to reset
*
* Returns:      None
*
* Notes:        None
*
*****************************************************************************/
static void SensorReset( SENSOR_INDEX Sensor)
{
    // Local Data
    UINT8          i;
    SENSOR*        pSensor;
    SENSOR_CONFIG  *pSensorConfig;

    // Initialize Local Data
    pSensor       = &Sensors[Sensor];
    pSensorConfig = &m_SensorCfg[Sensor];

    // Clear out the samples array, and reset max of samples
    for (i = 0; i < MAX_SAMPLES; i++)
    {
        // Initialize the sample array
        pSensor->fSamples[i] = 0;
    }

    // Initialize all the sensor storage elements
    pSensor->SensorIndex      = Sensor;
    //pSensor->bValueIsValid    = TRUE;   // Valid by default
    pSensor->bValueIsValid    = FALSE;
    pSensor->bWasValidOnce    = FALSE;
    pSensor->fValue           = 0.0F;
    pSensor->fPriorValue      = 0.0F;
    pSensor->PriorValueValid  = FALSE;  // not yet computed
    pSensor->nNextSample      = 0;
    pSensor->nCurrentSamples  = 0;
    pSensor->nSampleCounts    = (INT16)(MIFs_PER_SECOND / pSensorConfig->SampleRate);
    pSensor->nSampleCountdown = (pSensorConfig->nSampleOffset_ms / MIF_PERIOD_mS)+1;
    pSensor->bOutofRange      = FALSE;
    pSensor->bRangeFail       = FALSE;
    pSensor->nRangeTimeout_ms = 0;
    pSensor->bRateExceeded    = FALSE;
    pSensor->bRateFail        = FALSE;
    pSensor->nRateTimeout_ms  = 0;
    pSensor->fRateValue       = 0;
    pSensor->fRatePrevVal     = 0;
    pSensor->bRatePrevDetected= FALSE;
    pSensor->bSignalBad       = FALSE;
    pSensor->bSignalFail      = FALSE;
    pSensor->nSignalTimeout_ms= 0;
    pSensor->bBITFail         = FALSE;
    pSensor->nInterfaceIndex  = 0;
    pSensor->GetSensorData    = NULL;
    pSensor->TestSensor       = NULL;
    pSensor->InterfaceActive  = NULL;

    // Initialize any dynamic filter elements
    SensorInitFilter(&pSensorConfig->FilterCfg, &pSensor->FilterData,
                     pSensorConfig->nMaximumSamples);
}

/******************************************************************************
 * Function:     SensorsUpdateTask
 *
 * Description:  This is the sensor task that will count down the sample
 *               rate for each sensor and the read the sensor at the appropriate
 *               rate.
 *
 * Parameters:   [in/out] *pParam - Storage container for task variables
 *
 * Returns:      None
 *
 * Notes:        The sensor will continue to be sampled even after it goes
 *               invalid.
 *
 *****************************************************************************/
static void SensorsUpdateTask( void *pParam )
{
   // Local Data
   UINT8           nSensor;
   SENSOR_CONFIG   *pConfig;
   SENSOR          *pSensor;

   // Loop through all the sensors
   for (nSensor = 0; nSensor <= maxSensorUsed; nSensor++)
   {
      // Set pointers to the configuration and the Sensor object
      pConfig = &m_SensorCfg[nSensor];
      pSensor = Sensors + nSensor;

      // Check if the sensor is configured
      if (UNUSED != pConfig->Type)
      {
         // Countdown the number of samples until next read
         if (--pSensor->nSampleCountdown <= 0)
         {
            // Reload the sample countdown
            pSensor->nSampleCountdown = pSensor->nSampleCounts;
            // Read the sensor
            SensorRead( pConfig, pSensor );
         }
      }
   }
}

/******************************************************************************
 * Function:     SensorRead
 *
 * Description:  The SensorRead function reads the raw sensor value, perform
 *               the calibration and conversion, manages the samples, applies
 *               any configured filter and checks the sensor validity.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *
 * Returns:      None.
 *
 * Notes:        None.
 *****************************************************************************/
static void SensorRead( SENSOR_CONFIG *pConfig, SENSOR *pSensor )
{
   // Local Data
   FLOAT32  fVal;
   FLOAT32  filterVal;

   ASSERT( NULL != pSensor->GetSensorData );
   ASSERT( NULL != pSensor->InterfaceActive );
   ASSERT( NULL != pSensor->TestSensor );

   // Make sure the sensors interface is active
   if ( TRUE == pSensor->InterfaceActive(pSensor->nInterfaceIndex) )
   {
      // Read the Sensor data from the configured interface
      fVal      = pSensor->GetSensorData(pSensor->nInterfaceIndex);

      // Check if calibration is configured for sensor
      if (pConfig->Calibration.Type != NONE)
      {
         // Convert the sensor by the calibration parameters
         fVal = (fVal * pConfig->Calibration.fParams[LINEAR_SLOPE]) +
                        pConfig->Calibration.fParams[LINEAR_OFFSET];
      }

      // Check if conversion is configured for sensor
      if (pConfig->Conversion.Type != NONE)
      {
         // Convert the sensor by the conversion parameters
         fVal = (fVal * pConfig->Conversion.fParams[LINEAR_SLOPE]) +
                        pConfig->Conversion.fParams[LINEAR_OFFSET];
      }

      // Filter the latest sample value based on Cfg
      filterVal = SensorApplyFilter(fVal, pConfig, pSensor);

      // Perform a sensor check, (Rate, Range, BIT, Signal)
      if (SensorCheck( pConfig, pSensor, fVal, filterVal ))
      {
         // Save the previous value and current good value
         pSensor->fPriorValue = pSensor->fValue;
         pSensor->fValue      = filterVal;
      }
   }
   else // The interface isn't valid so the sensor cannot be valid
   {
      pSensor->bValueIsValid = FALSE;
   }
}

/******************************************************************************
 * Function:     SensorCheck
 *
 * Description:  The SensorCheck function will run any configured validity
 *               checks on the sensor. This checks include rate test, range
 *               test, interface test and BIT.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *
 * Returns:      bOK
 *
 * Notes:        None.
 *****************************************************************************/
static BOOLEAN SensorCheck( SENSOR_CONFIG *pConfig, SENSOR *pSensor,
                            FLOAT32 fVal, FLOAT32 filterVal)
{
   // Local Data
   BOOLEAN       bOK;
   SENSOR_STATUS RangeStatus;
   SENSOR_STATUS RateStatus;
   SENSOR_STATUS SignalStatus;
   SENSOR_STATUS InterfaceStatus;

   // Initialize Local Data
   bOK             = FALSE;

   // Retrieve the status of each test
   RangeStatus     = SensorRangeTest     (pConfig, pSensor, filterVal);
   RateStatus      = SensorRateTest      (pConfig, pSensor, fVal);
   SignalStatus    = SensorSignalTest    (pConfig, pSensor, fVal);
   InterfaceStatus = SensorInterfaceTest (pConfig, pSensor, fVal);

   // Check if the sensor is already valid or not
   if ( pSensor->bValueIsValid )
   {
      pSensor->bWasValidOnce = TRUE;

      if ( SENSOR_OK == RangeStatus  && SENSOR_OK == RateStatus &&
           SENSOR_OK == SignalStatus && SENSOR_OK == InterfaceStatus )
      {
         // Report the sensor is OK
         bOK = TRUE;
      }
      // Determine if any test has exceeded persistence and FAILED
      else if ( SENSOR_FAILED == RangeStatus  || SENSOR_FAILED == RateStatus ||
                SENSOR_FAILED == SignalStatus || SENSOR_FAILED == InterfaceStatus )
      {
         // Sensor is no longer valid
         pSensor->bValueIsValid = FALSE;
      }
   }
   // The sensor went invalid already, is it configured to heal itself OR
   // The sensor was never valid is it valid now.
   else if ((TRUE == pConfig->bSelfHeal) || (FALSE == pSensor->bWasValidOnce))
   {
      // Check that both the rate AND the range are OK
      if ( SENSOR_OK == RangeStatus  && SENSOR_OK == RateStatus &&
           SENSOR_OK == SignalStatus && SENSOR_OK == InterfaceStatus )
      {
         // Rate and Range Tests pass now verify the interface test is OK
         pSensor->bValueIsValid = TRUE;

         // Report the sensor is OK
         bOK = TRUE;
      }
   }
   return bOK;
}

/******************************************************************************
 * Function:     SensorRangeTest
 *
 * Description:  The Sensor Range Test function will verify that a sensor
 *               is within its configured range. This function will always
 *               return in range if the test is not configured to be
 *               enabled.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] filterVal   - Value of the sensor after the filter
 *
 * Returns:      SENSOR_FAIL_STATUS -
 *                        [SENSOR_OK]      - Test Passes
 *                        [SENSOR_FAILING] - Out of Range but duration not met
 *                        [SENSOR_FAILED]  - Out of Range and duration met
 *
 * Notes:        Returns SESNOR_OK if the sensor range test is not
 *               enabled.
 *
 *****************************************************************************/
static SENSOR_STATUS SensorRangeTest( SENSOR_CONFIG *pConfig, SENSOR *pSensor,
                                      FLOAT32 filterVal )
{
  // Local Data
  SENSOR_STATUS  Status;

  // Initialize Local Data
  Status = SENSOR_OK;

  // Check if the range test is enabled
  if ( TRUE == pConfig->RangeTest)
  {
     // check for range
     if ((filterVal < pConfig->fMinValue) || (filterVal > pConfig->fMaxValue))
     {
        Status = SENSOR_FAILING;

        // initialize the persistence timer
        if (FALSE == pSensor->bOutofRange)
        {
           pSensor->bOutofRange      = TRUE;
           pSensor->nRangeTimeout_ms = CM_GetTickCount() + pConfig->RangeDuration_ms;
        }

        // Sensor is out of range, check if it has met its persistent
        if ((CM_GetTickCount() >= pSensor->nRangeTimeout_ms) &&
            (FALSE == pSensor->bRangeFail))
        {
           // Sensor is bad
           pSensor->bRangeFail = TRUE;
           // Make a note in the log about the bad sensor.
           SensorLogFailure( pConfig, pSensor, filterVal, RANGE_FAILURE);
           Status              = SENSOR_FAILED;
        }
        else if (TRUE == pSensor->bRangeFail)
        {
           Status              = SENSOR_FAILED;
        }
     }
     else // Sensor is in range reset the countdown
     {
        // If Range Test had failed, reset sysCond
        if ( pSensor->bRangeFail == TRUE )
        {
          Flt_ClrStatus( pConfig->SystemCondition );
        }

        pSensor->bOutofRange = FALSE;
        pSensor->bRangeFail  = FALSE;
     }
  }

  return(Status);
}

/******************************************************************************
 * Function:     SensorRateTest
 *
 * Description:  The Sensor Rate Test function will verify that a sensor
 *               is not changing faster than a configured rate. This function
 *               will always return OK if the test is not configured to be
 *               enabled.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] fVal        - Current Sample Value
 *
 * Returns:      SENSOR_FAIL_STATUS -
 *                        [SENSOR_OK]      - Test Passes
 *                        [SENSOR_FAILING] - Rate Exceeded but duration not met
 *                        [SENSOR_FAILED]  - Rate Exceeded and duration met
 *
 * Notes:        Returns SENSOR_OK if the sensor rate test is not
 *               enabled.
 *
 *****************************************************************************/
static SENSOR_STATUS SensorRateTest( SENSOR_CONFIG *pConfig, SENSOR *pSensor, FLOAT32 fVal )
{
  // Local Data
  SENSOR_STATUS Status;
  FLOAT32            slope;

  // Initialize Local Data
  Status = SENSOR_OK;

  // Check if the rate test is enabled
  if ( TRUE == pConfig->RateTest)
  {
     // if the prior value has not been computed, it will be now.
     // Indicate so for next time
     if (pSensor->bRatePrevDetected == FALSE)
     {
        pSensor->bRatePrevDetected = TRUE;
        pSensor->fRatePrevVal      = fVal;
     }
     else /* the prior value has been computed, so compute the slope and compare */
     {
        /* compute the slope: slope = (current value - prior value) / interval
         * interval = sample period */
        slope  = (fVal - pSensor->fRatePrevVal) * pConfig->SampleRate;
        pSensor->fRatePrevVal = fVal;

        // Store the slope for debugging
        pSensor->fRateValue = slope;

        // Make sure the slope isn't negative
        slope = (FLOAT32)fabs(slope);

        /* check for rate */
        if (slope > pConfig->RateThreshold)
        {
           Status = SENSOR_FAILING;

           // Sensor rate exceeded
           if (FALSE == pSensor->bRateExceeded)
           {
              // The Rate has been exceeded record the time and wait for a timeout
              pSensor->bRateExceeded   = TRUE;
              pSensor->nRateTimeout_ms = CM_GetTickCount() + pConfig->RateDuration_ms;
           }

           // Check if the rate has persisted through the configured time
           if ((CM_GetTickCount() >= pSensor->nRateTimeout_ms) &&
               (FALSE == pSensor->bRateFail))
           {
              // Sensor is bad
              pSensor->bRateFail = TRUE;
              Status             = SENSOR_FAILED;
              // Make a note in the log about the bad sensor.
              SensorLogFailure( pConfig, pSensor, fVal, RATE_FAILURE);
           }
           // The Rate already timed out just keep return fail until
           // it fixes itself. Note this will only happen for
           // sensors configured to be self healing.
           else if (TRUE == pSensor->bRateFail)
           {
              Status = SENSOR_FAILED;
           }
        }
        else // Rate is OK Reset the flags
        {
           // If Rate Test had failed, reset sys cond
           if ( pSensor->bRateFail == TRUE )
           {
                Flt_ClrStatus( pConfig->SystemCondition );
           }

           pSensor->bRateExceeded = FALSE;
           pSensor->bRateFail     = FALSE;
           Status                 = SENSOR_OK;
        }
     }
  }

  return Status;
}

/******************************************************************************
 * Function:     SensorSignalTest
 *
 * Description:
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] fVal        - Current Value of sensor
 *
 * Returns:      SENSOR_FAIL_STATUS -
 *                        [SENSOR_OK]      - Test Passes
 *                        [SENSOR_FAILING] - Signal Test Failed but duration not met
 *                        [SENSOR_FAILED]  - Signal Test Failed and duration met
 *
 * Notes:        Returns SENSOR_OK if the sensor signal test is not
 *               enabled.
 *
 *****************************************************************************/
static SENSOR_STATUS SensorSignalTest( SENSOR_CONFIG *pConfig,
                                       SENSOR *pSensor, FLOAT32 fVal )
{
   // Local Data
   SENSOR_STATUS Status;

   // Initialize Local Data
   Status = SENSOR_OK;

   // Check if the signal test is enabled
   if ( TRUE == pConfig->SignalTest)
   {
      // Perform the fault processing and check for fault
      if (FaultCompareValues(pConfig->SignalTestIndex))
      {
         Status = SENSOR_FAILING;

         // Signal has failed, check if it is persistent
         if (FALSE == pSensor->bSignalBad)
         {
            // At this point the sensor is failing but hasn't failed
            pSensor->bSignalBad        = TRUE;
            pSensor->nSignalTimeout_ms = CM_GetTickCount() + pConfig->SignalDuration_ms;
         }

         // Determine if the duration has been met
         if ((CM_GetTickCount() >= pSensor->nSignalTimeout_ms) &&
             (FALSE == pSensor->bSignalFail))
         {
            // Sensor is now considered FAILED
            pSensor->bSignalFail = TRUE;
            Status               = SENSOR_FAILED;
            // Make a note in the log about the bad sensor.
            SensorLogFailure( pConfig, pSensor, fVal, SIGNAL_FAILURE);
         }
         // Was the Signal Test already failed?
         else if (TRUE == pSensor->bSignalFail)
         {
            // Sensor remains failed
            Status = SENSOR_FAILED;
         }
      }
      else // Signal is OK reset the countdown
      {
         // If Signal Test had failed, reset sys cond
         if ( pSensor->bSignalFail == TRUE )
         {
           Flt_ClrStatus( pConfig->SystemCondition );
         }

         pSensor->bSignalBad  = FALSE;
         pSensor->bSignalFail = FALSE;
      }
   }
   return Status;
}

/******************************************************************************
 * Function:     SensorInterfaceTest
 *
 * Description:  The Sensor Interface Test function will verify that the
 *               interface that is providing the sensor hasn't failed.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] fVal        - Current value of sensor
 *
 * Returns:      SENSOR_FAIL_STATUS - SENSOR_OK     - If interface passes test
 *                                    SENSOR_FAILED - If interface fails test
 *
 * Notes:
 *
 *****************************************************************************/
static SENSOR_STATUS SensorInterfaceTest(SENSOR_CONFIG *pConfig,
                                         SENSOR *pSensor, FLOAT32 fVal)
{
   // Local Data
   SENSOR_STATUS Status;
   BOOLEAN       bOK;

   ASSERT( NULL != pSensor->TestSensor );

   // Initialize Local Data
   Status = SENSOR_FAILED;

   // Check the Interface Test
   bOK = pSensor->TestSensor(pSensor->nInterfaceIndex);

   // Is the interface test OK
   if (TRUE == bOK)
   {
      // If BIT Test had failed, reset sys cond
      if ( pSensor->bBITFail == TRUE )
      {
        Flt_ClrStatus( pConfig->SystemCondition );
      }

      // No problem with the interface detected
      Status = SENSOR_OK;
      pSensor->bBITFail      = FALSE;
   }
   else if (FALSE == pSensor->bBITFail) // Problem on the interface detected
   {
      // The sensor has failed log the failure
      pSensor->bBITFail = TRUE;
      SensorLogFailure( pConfig, pSensor, fVal, BIT_FAILURE);
   }

   return Status;
}

/******************************************************************************
 * Function:     SensorNoTest
 *
 * Description:  This function is a generic function that always returns
 *               a passing result. This function is used for interfaces that
 *               do not have a specific test for the sensor.
 *
 * Parameters:   UINT16 nIndex - Sensor Index
 *
 * Returns:      TRUE
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN SensorNoTest ( UINT16 nIndex )
{
   return (TRUE);
}

/******************************************************************************
 * Function:     SensorNoInterface
 *
 * Description:  This function is a generic function that always returns
 *               a passing result. This function is used for interfaces that
 *               do not have a specific activity check for the sensor.
 *
 * Parameters:   UINT16 nIndex - Sensor Index
 *
 * Returns:      TRUE
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN SensorNoInterface ( UINT16 nIndex )
{
   return (TRUE);
}

/******************************************************************************
 * Function:     SensorLogFailure
 *
 * Description:  The SensorLogFailure function is used to record and take
 *               any action when a sensor fails.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] type        - Type of sensor failure
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void SensorLogFailure( SENSOR_CONFIG *pConfig, SENSOR *pSensor,
                              FLOAT32 fVal, SENSORFAILURE type)
{
   // Local Data
   SYS_APP_ID LogID;
   SENSORLOG  FaultLog;
   CHAR       Str[128];

   // Build the fault log
   FaultLog.failureType        = type;
   FaultLog.Index              = pSensor->SensorIndex;
   strncpy_safe(FaultLog.Name, sizeof(FaultLog.Name), pConfig->SensorName,_TRUNCATE);
   FaultLog.fCurrentValue      = fVal;
   FaultLog.fValue             = pSensor->fValue;
   FaultLog.fPreviousValue     = pSensor->fPriorValue;
   FaultLog.fRateValue         = pSensor->fRateValue;
   FaultLog.fExpectedMin       = pConfig->fMinValue;
   FaultLog.fExpectedMax       = pConfig->fMaxValue;
   FaultLog.fExpectedThreshold = pConfig->RateThreshold;
   FaultLog.nDuration_ms       = 0;

   // Display a debug message about the failure
   memset(Str,0,sizeof(Str));
   sprintf(Str,"Sensor (%d) - %s",
            pSensor->SensorIndex, SensorFailureNames[type] );
   GSE_DebugStr(NORMAL,FALSE, Str);

   // Determine the correct log ID to store
   switch (type)
   {
      case BIT_FAILURE:
         LogID                 = SYS_ID_SENSOR_CBIT_FAIL;
         break;
      case SIGNAL_FAILURE:
         LogID                 = SYS_ID_SENSOR_SIGNAL_FAIL;
         break;
      case RANGE_FAILURE:
         FaultLog.nDuration_ms = pConfig->RangeDuration_ms;
         LogID                 = SYS_ID_SENSOR_RANGE_FAIL;
         break;
      case RATE_FAILURE:
         FaultLog.nDuration_ms = pConfig->RateDuration_ms;
         LogID                 = SYS_ID_SENSOR_RATE_FAIL;
         break;
      case NO_FAILURE:
      case MAX_FAILURE:
      default:
         // FATAL ERROR WITH FAILURE TYPE
          FATAL("Invalid sensor failure type: %d", type);
         break;
   }

   // Store the fault log data
   Flt_SetStatus(pConfig->SystemCondition, LogID, &FaultLog, sizeof(FaultLog));
}

/******************************************************************************
* Function:     SensorInitFilter
*
* Description:  This function will initialize any necessary filter constants
*               if a filter is configured for a sensor.
*
* Parameters:   [in]  FILTER_CONFIG *pFilterCfg  - Pointer to the Sensor's
*                                                  Filter cfg
*               [out] FILTER_DATA   *pFilterData - Pointer to location to store
*                                                  filter data
*               [in]  UINT16 MaximumSamples      - Total number of configured
*                                                  samples for sensor
* Returns:     None
*
* Notes:       None
*
*****************************************************************************/
static void SensorInitFilter(FILTER_CONFIG *pFilterCfg, FILTER_DATA *pFilterData,
                             UINT16 nMaximumSamples)
{
    // Local Data
    FLOAT32 i;
    FLOAT32 nFloat;
    FLOAT32 temp;

    /* initialize for spike and max rejection */
    pFilterData->fLastValidValue = 0.0;
    nFloat = (FLOAT32)nMaximumSamples;

    // Check what type of filter
    switch(pFilterCfg->Type)
    {
    case FILTERNONE:
        break;

    case EXPO_AVERAGE:
        pFilterData->K = 2.0f /(1.0f + nFloat);
        break;

    case SPIKEREJECTION:
        pFilterData->K = 2.0f / (1.0f + nFloat);
        pFilterData->fullScaleInv = 1.0f/pFilterCfg->fFullScale;
        pFilterData->maxPercent = (FLOAT32)pFilterCfg->nMaxPct/100.0f;
        pFilterData->spikePercent = (FLOAT32)pFilterCfg->nSpikeRejectPct/100.0f;
        break;

    case MAXREJECT:
        pFilterData->K = 2.0f / (1.0f + nFloat);
        break;

    case SLOPEFILTER:
        /* compute the K constant */
        pFilterData->K = 0.0;

        for (i = 0.0f; i < nFloat; i = i+1.0f)
        {
            temp = (i - ((nFloat - 1.0f) * 0.5f));
            pFilterData->K += temp * temp;
        }
        pFilterData->K = 1.0f/pFilterData->K;

        /* compute (n-1)/2 */
        pFilterData->nMinusOneOverTwo = ((nFloat - 1.0f) * 0.5f);
        break;

    default:
        // FATAL ERROR WITH FILTER TYPE
        FATAL("Sensor Filter Init Fail: Type = %d", pFilterCfg->Type);
        break;
    }
}

/******************************************************************************
 * Function:     SensorManageSamples
 *
 * Description:  The SensorManageSamples function is responsible for placing
 *               each sample into the storage array in the proper order.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *               [in] fVal        - The float value of the current sample
 *
 * Returns:      BOOLEAN          - TRUE  = Sample Array is full
 *                                  FALSE = Array not full
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN SensorManageSamples (SENSOR_CONFIG *pConfig, SENSOR *pSensor, FLOAT32 fVal)
{
   // Store the latest read value
   pSensor->fSamples[pSensor->nNextSample] = fVal;

   // Increment the next sample storage location
   pSensor->nNextSample  += 1;

   // Check if the total number of samples is less than maximum
   // Filters cannot be applied until all the samples are stored
   if (pSensor->nCurrentSamples < pConfig->nMaximumSamples)
   {
      // Increment the current sample
      pSensor->nCurrentSamples++;
   }

   // if we have gone beyond the end of the number of samples, wrap around
   if (pSensor->nNextSample >= pConfig->nMaximumSamples)
   {
      pSensor->nNextSample = 0;
   }

   // Return the full status of the sample array
   return (pSensor->nCurrentSamples == pConfig->nMaximumSamples);
}

/******************************************************************************
 * Function:     SensorApplyFilter
 *
 * Description:  The SensorApplyFilter will apply any configured filter to
 *               the array of stored sensor values. This function only
 *               dispatches the filter algorithms.
 *
 * Parameters:   [in] pConfig     - Pointer to the sensor configuration
 *               [in/out] pSensor - Pointer to the dynamic sensor data
 *
 * Returns:      FLOAT32 Value    - Sensor value after the filter has been
 *                                  applied.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static FLOAT32 SensorApplyFilter ( FLOAT32 fVal, SENSOR_CONFIG *pConfig, SENSOR *pSensor)
{
   // Run the appropriate filter on the sensor
   switch (pConfig->FilterCfg.Type)
   {
      case FILTERNONE:
         break;
      case EXPO_AVERAGE:
         fVal = SensorExponential( fVal, &pSensor->FilterData);
         break;
      case SPIKEREJECTION:
         fVal = SensorSpikeReject(fVal, &pConfig->FilterCfg, &pSensor->FilterData);
         break;
      case MAXREJECT:
         fVal = SensorMaxReject(fVal, &pConfig->FilterCfg, &pSensor->FilterData);
         break;
      case SLOPEFILTER:
         fVal = SensorSlope ( fVal, pConfig, pSensor);
         break;
      default:
         // FATAL ERROR WITH FILTER TYPE
         FATAL("Invalid Sensor Filter Type: %s(%d) - FilterType(%d)",
                 pConfig->SensorName,
                 pSensor->SensorIndex,
                 pConfig->FilterCfg.Type);
         break;
   }

   return fVal;
}

/******************************************************************************
* Function:     SensorPersistCounter
*
* Description:  Perform persistence counting for filters.
*
* Parameters:   [in]     set - Set input
*               [in]     reset - reset input
*               [in]     pFilterCfg - holds max count value
*               [in/out] pFilterData - holds filter state info
*
* Returns:      None.
*
* Notes:        None.
*
*****************************************************************************/
static void SensorPersistCounter( BOOLEAN set, BOOLEAN reset,
                            FILTER_CONFIG *pFilterCfg, FILTER_DATA *pFilterData)
{
    if (TRUE == reset)
    {
        if (pFilterData->persistState)
        {
            GSE_DebugStr(VERBOSE,TRUE, "UnPersist");
        }
        pFilterData->nRejectCount = 0;
        pFilterData->persistState = FALSE;
    }

    if (TRUE == set)
    {
        ++pFilterData->nRejectCount;
        if (pFilterData->nRejectCount >= pFilterCfg->nMaxRejectCount)
        {
            pFilterData->persistState = TRUE;
            GSE_DebugStr(VERBOSE,TRUE, "Persist");
        }
    }
}

/******************************************************************************
 * Function:     SensorExponential
 *
 * Description:  The sensor average function will perform a simple average
 *               on the stored sensor values.
 *
 * Parameters:   [in] pSamples - Pointer to samples to average
 *               [in] nSamples - Total number of samples for sensor
 *
 * Returns:      FLOAT32 - Average of the samples
 *
 * Notes:        None.
 *
 *****************************************************************************/
static FLOAT32 SensorExponential (FLOAT32 fVal, FILTER_DATA *pFilterData)
{
    pFilterData->fLastAvgValue +=
        (pFilterData->K * ( fVal - pFilterData->fLastAvgValue));
    return pFilterData->fLastAvgValue;
}

/******************************************************************************
 * Function:     SensorSpikeReject
 *
 * Description:  The SensorSpikeReject function will filter out any spikes
 *               in the sensor value until that sensor exceeds the number
 *               of reject counts.
 *
 * Parameters:   [in] fVal      - Pointer to the filter configuration
 *               [in] pSamples        - Pointer to samples to average
 *               [in/out] pFilterData - Pointer to the filter dynamic data
 *               [in] nSamples        - Total number of samples for sensor
 *
 * Returns:      FLOAT32 - Filtered value of the samples
 *
 * Notes:        None.
 *
 *****************************************************************************/
static FLOAT32 SensorSpikeReject(FLOAT32 fVal,
                                 FILTER_CONFIG *pFilterCfg, FILTER_DATA *pFilterData)
{
    BOOLEAN changeErr;
    BOOLEAN rangeErr;
    BOOLEAN validSample;
    BOOLEAN hold;
    FLOAT32 pctChange;
    FLOAT32 pctFullScale;

    pctChange =
      (FLOAT32)(fabs(pFilterData->fLastValidValue - fVal)) * pFilterData->fullScaleInv;
    changeErr = (BOOLEAN)(pctChange > pFilterData->spikePercent);

    pctFullScale = fVal * pFilterData->fullScaleInv;
    rangeErr = (BOOLEAN)(pctFullScale > pFilterData->maxPercent);

    validSample = (BOOLEAN)!(changeErr || rangeErr);

    SensorPersistCounter( changeErr, validSample || pFilterData->persistStateZ1,
                    pFilterCfg, pFilterData);

    pFilterData->persistStateZ1 = pFilterData->persistState;

    // compute lastValid
    hold = pFilterData->persistState || validSample;
    pFilterData->fLastValidValue = hold ? fVal : pFilterData->fLastValidValue;

    // do a simple average
    pFilterData->fLastAvgValue = pFilterData->fLastAvgValue +
        (pFilterData->fLastValidValue - pFilterData->fLastAvgValue) * pFilterData->K;

    return pFilterData->fLastAvgValue;
}

/******************************************************************************
 * Function:     SensorMaxReject
 *
 * Description:  The SensorMaxReject function will
 *
 * Parameters:   [in] pFilterCfg      - Pointer to the filter configuration
 *               [in/out] pFilterData - Pointer to the filter dynamic data
 *               [in] pSamples        - Pointer to samples to average
 *               [in] nSamples        - Total number of samples for sensor
 *
 * Returns:      FLOAT32 - Filtered value
 *
 * Notes:        None.
 *
 ******************************************************************************/
static FLOAT32 SensorMaxReject(FLOAT32 sample,
                               FILTER_CONFIG *pFilterCfg, FILTER_DATA *pFilterData)
{
    BOOLEAN valid;
    BOOLEAN hold;
    FLOAT32 useValue;

    valid = (BOOLEAN)(sample < pFilterCfg->fRejectValue);

    SensorPersistCounter( !valid,
        valid || pFilterData->persistStateZ1, pFilterCfg, pFilterData);
    pFilterData->persistStateZ1 = pFilterData->persistState;

    hold = valid || pFilterData->persistStateZ1;

    pFilterData->fLastValidValue = hold ? sample : pFilterData->fLastValidValue;

    useValue = valid ? sample : pFilterData->fLastValidValue;

    pFilterData->fLastAvgValue = pFilterData->fLastAvgValue +
        ((useValue - pFilterData->fLastAvgValue) * pFilterData->K);

    return pFilterData->fLastAvgValue;
}

/******************************************************************************
 * Function:     SensorSlope
 *
 * Description:  Slope - Return the slope of the measured data
 *
 *               (Note: This description is for sensors with more than 2
 *                samples.  If the number of samples is 2, the slope
 *                computation is reduced to: Yi - Y(i-1) *
 *                TICKS_PER_SECOND / nSampleTicks)
 *
 *               The slope is measured by the standard linear regression
 *               formula as follows:
 *
 *                        i=n-1
 *                          Sum (Xi - Xbar)(Yi - Ybar)
 *                        i=0
 *               Slope = --------------------------
 *                        i=n-1
 *                          Sum (Xi - Xbar)^2
 *                        i=0
 *
 *               which in our case reduces to
 *
 *                          i=n-1
 *                            Sum (i - (n-1)/2)(Yi - Ybar)
 *                          i=0
 *               S=Slope = --------------------------
 *                          i=n-1
 *                            Sum (i - (n-1)/2)^2
 *                          i=0
 *                                                      i=n-1
 *               where Ybar is the average of the y's =  Sum Yi/n
 *                                                      i=0
 *
 *               To reduce the computation,
 *               we use the following logic:
 *
 *                      i=n-1
 *                let     Sum (i - (n-1)/2)^2 = 1/K
 *                      i-0
 *
 *               We look at the problem in two parts.
 *
 *                    i=n-1                   i=n-1
 *                     Sum (i - (n-1)/2)(Yi)   Sum (i - (n-1)/2)(-Ybar)
 *                    i=0                     i=0
 *                 S= --------------------- + -------------------------
 *                   i=n-1                    i=n-1
 *                    Sum (i - (n-1)/2)^2      Sum (i - (n-1)/2)^2
 *                   i=0                      i=0
 *
 *                  = SF + SBar
 *
 *               The second term, SBar can be re-expressed as
 *
 *                   i=n-1
 *                    Sum (i - (n-1)/2)
 *                   i=0                            i=n-1
 *                   --------------------- * -1/n *  Sum Yi
 *                   i=n-1                          i=0
 *                    Sum (i - (n-1)/2)^2
 *                   i=0
 *                = 0 for all n > 1.
 *
 *               So, to compute the new slope, we need to
 *               precompute the constant K, and and only compute
 *               the first term SF.
 *
 * Parameters:   [in] pFilterCfg      - Pointer to the filter configuration
 *               [in/out] pFilterData - Pointer to the filter dynamic data
 *               [in] pSamples        - Pointer to samples to average
 *               [in] nSamples        - Total number of samples for sensor
 *
 * Returns:      FLOAT32 - Filtered value of the samples
 *
 * Notes:        None.
 *
 ******************************************************************************/
static FLOAT32 SensorSlope( FLOAT32 fVal, SENSOR_CONFIG *pConfig, SENSOR *pSensor)
{
   // Local Data
   UINT16  i;
   FLOAT32 returnSlope;
   FLOAT32 iFloat;

   // Initialize Local Data
   i           = 0;
   returnSlope = 0.0;
   iFloat      = 0.0;


   // insert the current sample into the buffer
   if ( SensorManageSamples (pConfig, pSensor, fVal))
   {
       UINT8 ix;
       UINT8 nSamples = pConfig->nMaximumSamples;
       FILTER_DATA*   pFilterData = &pSensor->FilterData;

       for (i = 0; i < nSamples; i++)
       {
          iFloat       = i;
          ix = (UINT8)((i + pSensor->nNextSample) % nSamples);
          returnSlope += (iFloat - pFilterData->nMinusOneOverTwo) *
                         pSensor->fSamples[ix];
       }
       returnSlope *= pFilterData->K;
       /*
        * return the new slope value
        * The slope is in units/tick.  To convert to seconds, we
        * must multiply by the number ticks/second, and we must
        * take into account the interval of the sampling, so the
        * resulting slope  must be multiplied by TICKS_PER_SECOND,
        * and also divided by the number of ticks used to take
        * each sample, = pSensor->nSampleTicks.
        */
       //returnSlope *= TTMR_TASK_FRAMES_PER_SECOND;
       //returnSlope  = returnSlope / (TTMR_TASK_FRAMES_PER_SECOND / pConfig->SampleRate);
       // TBD - I think this can be reduced to the following
       returnSlope *= pConfig->SampleRate;
   }
   else
   {
      returnSlope = fVal;
   }

   return (returnSlope);
}

/******************************************************************************
 * Function:     SensorsLiveDataTask
 *
 * Description:  The SensorLiveData task is used to output the live data on
 *               the user interface.
 *
 * Parameters:   void *pParam - Task block parameters
 *
 * Returns:      None.
 *
 * Notes:        None.
 *****************************************************************************/
static void SensorsLiveDataTask( void *pParam )
{
   // Local Data
   SENSOR_LD_PARMS  *pTCB;
   UINT32           nRate;

   // Initialize Local Data
   pTCB   = (SENSOR_LD_PARMS *)pParam;

   // Is live data enabled?
   if (LD_NONE != LiveDataDisplay.Type)
   {
      // Make sure the rate is not too fast and fix if it is
      nRate = LiveDataDisplay.Rate_ms < SENSOR_LD_MAX_RATE ?
              SENSOR_LD_MAX_RATE : LiveDataDisplay.Rate_ms;

      // Check if end of period has been reached
      if ((CM_GetTickCount() - pTCB->StartTime) >= nRate)
      {
         // Save new start time
         pTCB->StartTime = CM_GetTickCount();
         SensorDumpASCIILiveData();
      }
   }
}

/******************************************************************************
 * Function:     SensorDumpASCIILiveData
 *
 * Description:  SesnorDumpASCIILiveData formats the sensor live data and
 *               outputs it on the user interface.
 *
 *               The format is:
 *               #77|<SensorIndex>:<SensorValue>:<ValidFlag>....
 *               |<SensorIndex>:<SensorValue>:<ValidFlag>
                 |t:<Hour>:<Minute>:<Second>:<Millisecond>|c:<Checksum>
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
static void SensorDumpASCIILiveData(void)
{
  // Local Data
  INT8          Str[2048];
  INT8          TempStr[64];
  TIMESTRUCT    TimeStruct;
  UINT32        SensorIndex;
  UINT16        Checksum;

  // Initialize local data
  memset(Str,0,sizeof(Str));

  // Build the live data header
  sprintf(Str, "\r\n");
  GSE_PutLine(Str);

  sprintf(Str, "#77");

  // Loop through all sensors
  for (SensorIndex = 0; SensorIndex < MAX_SENSORS; SensorIndex++)
  {
     // Check if inspection is enabled and the sensor is in use.
     if (TRUE == m_SensorCfg[SensorIndex].bInspectInclude &&
         SensorIsUsed(( SENSOR_INDEX)SensorIndex) )
     {
        // Add the sensor index and value to the stream
        sprintf(TempStr, "|%02d:%.4f", SensorIndex, Sensors[SensorIndex].fValue);
        strcat(Str,TempStr);

        // Check if the sensor is valid and add 1 if valid 0 not valid
        if (Sensors[SensorIndex].bValueIsValid)
        {
           strcat (Str,":1");
        }
        else
        {
           strcat (Str,":0");
        }
     }
  }
  // Read the current system time
  CM_GetSystemClock (&TimeStruct);
  // Add time to the live data stream
  sprintf (TempStr, "|t:%02d:%02d:%02d.%02d", TimeStruct.Hour,
                                              TimeStruct.Minute,
                                              TimeStruct.Second,
                                             (TimeStruct.MilliSecond/10) );
  strcat(Str,TempStr);

  // Checksum the data
  Checksum = (UINT16)ChecksumBuffer(Str, strlen(Str), 0xFFFF);
  // Add checksum to end of live data
  sprintf (TempStr, "|c:%04X", Checksum);
  strcat(Str,TempStr);

  // Output the live data stream on the GSE port
  GSE_PutLine(Str);
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: sensor.c $
 *
 * *****************  Version 72  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:13p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Binary ETM Header
 * 
 * *****************  Version 71  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 70  *****************
 * User: John Omalley Date: 12-08-28   Time: 8:35a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Updated the sensor summary to include the time of min and
 * time of max.
 *
 * *****************  Version 69  *****************
 * User: Contractor V&v Date: 8/22/12    Time: 5:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Issue #19 SensorSummaryArray ASSERTS
 *
 * *****************  Version 68  *****************
 * User: John Omalley Date: 12-07-19   Time: 10:49a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fixed Sensor summary processing
 *
 * *****************  Version 67  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Refactor for common Sensor Summary
 *
 * *****************  Version 66  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:19p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 *
 * *****************  Version 65  *****************
 * User: Contractor V&v Date: 4/11/12    Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST2  Accessor API for PriorValidity & 128sensors
 *
 * *****************  Version 64  *****************
 * User: John Omalley Date: 9/29/11    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Interface Validity Fix
 *
 * *****************  Version 63  *****************
 * User: John Omalley Date: 8/31/11    Time: 4:03p
 * Updated in $/software/control processor/code/system
 * SCR 1015 - Fixed BIT Fail Logic
 *
 * *****************  Version 62  *****************
 * User: Contractor2  Date: 5/26/11    Time: 1:18p
 * Updated in $/software/control processor/code/system
 * SCR #767 Enhancement - BIT for Analog Sensors
 *
 * *****************  Version 61  *****************
 * User: Contractor2  Date: 10/05/10   Time: 10:58a
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 60  *****************
 * User: Contractor2  Date: 10/04/10   Time: 1:27p
 * Updated in $/software/control processor/code/system
 * SCR #916 Code Review Updates
 *
 * *****************  Version 59  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed unused functions
 *
 * *****************  Version 58  *****************
 * User: John Omalley Date: 8/05/10    Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR 748 - Optimized Debug Str output to stop DT overruns on error
 *
 * *****************  Version 57  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 *
 * *****************  Version 56  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 *
 * *****************  Version 55  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 *
 * *****************  Version 54  *****************
 * User: Jeff Vahue   Date: 6/09/10    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR# 486 - mod the fix to use the 'active' version of the livedata to
 * control the task execution.  Also modify tables to use the live data
 * structure directly.
 *
 * *****************  Version 53  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:14p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 *
 * *****************  Version 52  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR #486 Livedata enhancement
 *
 * *****************  Version 52  *****************
 * User: Contractor2  Date: 6/07/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR #486 Livedata enhancement
 *
 * *****************  Version 51  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 5/20/10    Time: 5:22p
 * Updated in $/software/control processor/code/system
 * SCR# 602 - Error in logic to hold the previous sensor value while
 * sensor is failing or failed
 *
 * *****************  Version 49  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 47  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 45  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 3/02/10    Time: 10:43a
 * Updated in $/software/control processor/code/system
 * Fix float cast to FLOAT32 cast
 *
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:29p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - Coding Std fixes for line too long
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 1/26/10    Time: 1:04p
 * Updated in $/software/control processor/code/system
 * Typo: Spaces vs. Tab Fix
 *
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 1/19/10    Time: 3:52p
 * Updated in $/software/control processor/code/system
 * SCR# 399
 *
 * *****************  Version 39  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:29p
 * Updated in $/software/control processor/code/system
 * SCR 296
 * * Updated the sensor read procesing to check if the interface is alive.
 *
 * *****************  Version 38  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:13p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 37  *****************
 * User: Jeff Vahue   Date: 1/11/10    Time: 12:23p
 * Updated in $/software/control processor/code/system
 * SCR# 388
 *
 * *****************  Version 36  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * SCR 18, SCR 371
 *
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR# 326 cleanup
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 12/21/09   Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #380 Implement Real Time Sys Condition Logic
 *
 * *****************  Version 32  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:38p
 * Updated in $/software/control processor/code/system
 * SCR 374
 * - Deleted the FaultUpdate call and added the FaultCompare call
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:23p
 * Updated in $/software/control processor/code/system
 * SCR 370
 * * Added logic to only report the interface failure once.
 *
 * *****************  Version 30  *****************
 * User: Jeff Vahue   Date: 12/14/09   Time: 2:43p
 * Updated in $/software/control processor/code/system
 * SCR# 364
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 106  XXXUserTable.c consistency
 *
 * *****************  Version 28  *****************
 * User: John Omalley Date: 11/24/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 349
 * - Added saving the previous in the rate test.
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 11/24/09   Time: 2:14p
 * Updated in $/software/control processor/code/system
 * SCR 349
 * * Fixed self healing for the rate test.
 * * Set the rate value before the absolute is calculated.
 * * Updated the rate calculation
 * * Corrected the current and previous value lock mechanism.
 *
 * *****************  Version 26  *****************
 * User: John Omalley Date: 11/19/09   Time: 2:46p
 * Updated in $/software/control processor/code/system
 * SCR 339
 * Corrected timeout logic for the Rate, Range and Signal Test
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:21p
 * Updated in $/software/control processor/code/system
 * Implement SCR 196 ESC' sequence when outputting continuous data to GSE
 *
 * *****************  Version 24  *****************
 * User: John Omalley Date: 10/26/09   Time: 1:54p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Updated the rate countdown logic because the 1st calculated countdonw
 * was incorrect.
 *
 * *****************  Version 23  *****************
 * User: John Omalley Date: 10/20/09   Time: 3:35p
 * Updated in $/software/control processor/code/system
 * SCR 310
 * - Added last valid value logic
 * - Added countdown precalc
 * - Added time based checks >=
 *
 * *****************  Version 22  *****************
 * User: John Omalley Date: 10/14/09   Time: 5:18p
 * Updated in $/software/control processor/code/system
 * SCR 292
 * - Updated the sensor and trigger rate processing.
 * - Added the BOOLEAN to the user configuration function.
 *
 * *****************  Version 21  *****************
 * User: John Omalley Date: 10/07/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 283 - Added FATAL Assert logic to default switch cases.
 *
 * *****************  Version 20  *****************
 * User: John Omalley Date: 10/07/09   Time: 11:59a
 * Updated in $/software/control processor/code/system
 * Updated per SCR 292
 *
 * *****************  Version 19  *****************
 * User: John Omalley Date: 10/06/09   Time: 5:30p
 * Updated in $/software/control processor/code/system
 * Updated to remove multiple system condition configuration items
 *
 * *****************  Version 18  *****************
 * User: John Omalley Date: 10/06/09   Time: 5:05p
 * Updated in $/software/control processor/code/system
 * Updates for requirments implementation
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR #287 QAR_SensorTest() updates
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #287 QAR_SensorTest() record log and display debug msg.
 *
 * *****************  Version 15  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Implemented missing requirements.
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 4/07/09    Time: 10:13a
 * Updated in $/control processor/code/system
 * SCR# 161
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 1/30/09    Time: 4:45p
 * Updated in $/control processor/code/system
 * SCR #131 Arinc429 Updates
 * Add SSM Failure Detection to SensorTest()
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 1/23/09    Time: 10:03a
 * Updated in $/control processor/code/system
 * Uncommented and made public the "SensorGetValueandVerify" function for
 * the AircraftConfigMgr.c module.
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 9:48a
 * Updated in $/control processor/code/system
 * SCR #87
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 4:04p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *****************************************************************************/
                              
