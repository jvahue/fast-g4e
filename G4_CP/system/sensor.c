#define SENSOR_BODY
/******************************************************************************
            Copyright (C) 2009-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

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
      $Revision: 109 $  $Date: 5/19/16 1:03p $

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
  UINT32  startTime;
}SENSOR_LD_PARMS;

// Sensor LiveData collection buffer.
#pragma pack(1)
typedef struct
{
  UINT16   nSensorIndex;
  FLOAT32  fValue;
  BOOLEAN  bValueIsValid;
}SENSOR_LD_ENTRY;
#pragma pack()

#pragma pack(1)
typedef struct
{
  TIMESTAMP       timeStamp;           // Timestamp for this collection
  UINT16          count;               // number of entries used in sensor field
  SENSOR_LD_ENTRY sensor[MAX_SENSORS]; // SENSOR_LD_ENTRY for sensors with inspect=TRUE
}SENSOR_LD_COLLECT;
#pragma pack()

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static SENSOR             Sensors[MAX_SENSORS]; // Dynamic Sensor Storage
static UINT16             maxSensorUsed;   // Storage for largest indexed cfg
static SENSOR_LD_PARMS    liveDataBlock;   // Live Data Task block
/*static SENSORLOG SensorLog;*/
static SENSOR_CONFIG      m_SensorCfg[MAX_SENSORS]; // Sensors cfg array
static SENSOR_LD_CONFIG   m_liveDataDisplay; // runtime copy of LiveData config data
static SENSOR_LD_ENUM     m_msType;          // runtime copy of MS display type
static SENSOR_LD_COLLECT  m_liveDataBuffer;  // collection of all sensor values.
static UINT16             m_liveDataMap[MAX_SENSORS]; // Maps Sensors -> m_liveDataBuffer

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

static void          SensorsLiveDataTask     ( void *pParam );
static void          SensorDumpASCIILiveData ( LD_DEST_ENUM dest );
static void          SensorDumpBinaryLiveData( LD_DEST_ENUM dest );
static void          SensorInitializeLiveData( void );

//---- Function implementations for Virtual sensors to play nice with physical sensors
static FLOAT32 SensorVirtualGetValue      ( UINT16 nIndex, UINT32 *null );
static BOOLEAN SensorVirtualInterfaceValid( UINT16 nIndex );
static BOOLEAN SensorVirtualSensorTest    ( UINT16 nIndex );


static const CHAR *sensorFailureNames[MAX_FAILURE+1] =
{
  "No Failure",
  "BIT Failure",
  "Signal Failure",
  "Range Failure",
  "Rate Failure",
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
  TCB tcbTaskInfo;
  
  // Add user commands for the sensors to the user command tables
  User_AddRootCmd(&RootSensorMsg);
  User_AddRootCmd(&LiveDataMsg);

  // Reset the Sensor configuration storage array
  memset(m_SensorCfg, 0, sizeof(m_SensorCfg));

  // Reset the buffer used to collect sensor data for stream
  memset( &m_liveDataBuffer, 0, sizeof(m_liveDataBuffer ));

  // Load the current configuration to the sensor configuration array.
  memcpy(m_SensorCfg,
         CfgMgr_RuntimeConfigPtr()->SensorConfigs,
         sizeof(m_SensorCfg));

  // init runtime copy of live data config, MS output is disabled on startup
  memcpy(&m_liveDataDisplay, &(CfgMgr_RuntimeConfigPtr()->LiveDataConfig),
             sizeof(m_liveDataDisplay));
  m_msType = LD_NONE;

  // Reset the dynamic sensor storage array
  SensorsReset();
  // Configure the sensor interface parameters
  SensorsConfigure();

  // Setup the storage and sensor-index mapping table for live-data monitoring.
  SensorInitializeLiveData();

  // Create Sensor Task - DT
  memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
  strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Sensor Task",_TRUNCATE);
  tcbTaskInfo.TaskID           = (TASK_INDEX)Sensor_Task;
  tcbTaskInfo.Function         = SensorsUpdateTask;
  tcbTaskInfo.Priority         = taskInfo[Sensor_Task].priority;
  tcbTaskInfo.Type             = taskInfo[Sensor_Task].taskType;
  tcbTaskInfo.modes            = taskInfo[Sensor_Task].modes;
  tcbTaskInfo.MIFrames         = taskInfo[Sensor_Task].MIFframes;
  tcbTaskInfo.Enabled          = TRUE;
  tcbTaskInfo.Locked           = FALSE;

  tcbTaskInfo.Rmt.InitialMif   = taskInfo[Sensor_Task].InitialMif;
  tcbTaskInfo.Rmt.MifRate      = taskInfo[Sensor_Task].MIFrate;
  tcbTaskInfo.pParamBlock      = NULL;

  TmTaskCreate (&tcbTaskInfo);

  // Create Sensor Live Data Task - DT
  memset(&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
  strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"Sensor Live Data Task",_TRUNCATE);
  tcbTaskInfo.TaskID           = (TASK_INDEX)Sensor_Live_Data_Task;
  tcbTaskInfo.Function         = SensorsLiveDataTask;
  tcbTaskInfo.Priority         = taskInfo[Sensor_Live_Data_Task].priority;
  tcbTaskInfo.Type             = taskInfo[Sensor_Live_Data_Task].taskType;
  tcbTaskInfo.modes            = taskInfo[Sensor_Live_Data_Task].modes;
  tcbTaskInfo.MIFrames         = taskInfo[Sensor_Live_Data_Task].MIFframes;
  tcbTaskInfo.Enabled          = TRUE;
  tcbTaskInfo.Locked           = FALSE;

  tcbTaskInfo.Rmt.InitialMif   = taskInfo[Sensor_Live_Data_Task].InitialMif;
  tcbTaskInfo.Rmt.MifRate      = taskInfo[Sensor_Live_Data_Task].MIFrate;
  tcbTaskInfo.pParamBlock      = &liveDataBlock;

  liveDataBlock.startTime   = 0;
  TmTaskCreate (&tcbTaskInfo);
  
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
  BOOLEAN bValid;

  bValid = FALSE;

  if ( (Sensor < MAX_SENSORS) && Sensors[Sensor].bValueIsValid )
  {
      bValid = TRUE;
  }

  // returns the sensor validity
  return (bValid);
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
  BOOLEAN bUsed;

  bUsed = FALSE;

  if ((Sensor < MAX_SENSORS) && (m_SensorCfg[Sensor].type != UNUSED))
  {
     bUsed = TRUE;
  }
  // return if the sensor is used
  return bUsed;
}

/******************************************************************************
 * Function:     SensorIsUsedEx
 *
 * Description:  This function combines the behavior of SensorIsUsed with
 *               IsSensorValid into a single call for improved performance
 *               over when those pair of functions are called sequentially.
 *
 * Parameters:   [in] sensorIdx - Index of sensor to check
 *
 *               [out] bIsUsed - TRUE  = used
 *                               FALSE = not used
 * Returns:      The current validity of the sensor if TRUE, or FASLE when unused.
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN SensorIsUsedEx( SENSOR_INDEX sensorIdx, BOOLEAN* bIsUsed)
{
  BOOLEAN bValid;

  if ((sensorIdx < MAX_SENSORS) && (m_SensorCfg[sensorIdx].type != UNUSED))
  {
    *bIsUsed = TRUE;
     bValid  = Sensors[sensorIdx].bValueIsValid;
  }
  else
  {
    *bIsUsed = FALSE;
    bValid   = FALSE;
  }
  return bValid;
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
  FLOAT32 fValue;

  fValue = 0;

  if (Sensor < MAX_SENSORS)
  {
     fValue = Sensors[Sensor].fValue;
  }
  // return the current value of the sensor
  return (fValue);
}

/******************************************************************************
 * Function:     SensorGetValueEx ( SENSOR_INDEX sensorIdx, FLOAT32* fValue )
 *
 * Description:  This function combines the behavior of SensorIsValid and
 *               SensorGetValue into a single call. Since these two functions
 *               are often called in sequence, this provides significant
 *               performance improvement(30%->40%) where the IsValid/GetValue
 *               are called in a loop.
 *
 * Parameters:   [in]  sensorIdx - Index of sensor to get value
 *               [out] fValue    - pointer to a FLOAT32 to return the sensor value.
 *
 * Returns:      [out] BOOLEAN - validity state of the sensor.
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN SensorGetValueEx( SENSOR_INDEX sensorIdx, FLOAT32* fValue)
{
  BOOLEAN bValid  = FALSE;
  *fValue = 0.f;
  if (sensorIdx < MAX_SENSORS)
  {
    bValid  = Sensors[sensorIdx].bValueIsValid;
    *fValue = Sensors[sensorIdx].fValue;
  }
  return bValid;
}

/******************************************************************************
 * Function:     SensorGetLastUpdateTime ( SENSOR_INDEX Sensor )
 *
 * Description:  This function provides a method to return the last rx time of a
 *               sensor.
 *
 * Parameters:   [in] Sensor - Index of sensor to get value
 *
 * Returns:      [out] UINT32 - Tick time of last sensor update
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT32 SensorGetLastUpdateTime( SENSOR_INDEX Sensor)
{
  UINT32 nTickTime;

  nTickTime = 0;

  if (Sensor < MAX_SENSORS)
  {
    nTickTime = Sensors[Sensor].lastUpdateTick;
  }
  // return the current value of the sensor
  return (nTickTime);
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
   FLOAT32 fPriorValue;

   fPriorValue = 0;

   if ( Sensor < MAX_SENSORS )
   {
     fPriorValue = Sensors[Sensor].fPriorValue;
   }
   // return the previous value of the sensor
   return (fPriorValue);
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
   SENSOR_HDR          sensorHdr[MAX_SENSORS];
   UINT16              nSensorIndex;
   INT8                *pBuffer;
   UINT16              nRemaining;
   UINT16              nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( sensorHdr, 0, sizeof(sensorHdr) );

   // Loop through all the sensors
   for ( nSensorIndex = 0;
         ((nSensorIndex < MAX_SENSORS) && (nRemaining > sizeof (SENSOR_HDR)));
         nSensorIndex++ )
   {
      // Copy the name and units
      strncpy_safe( sensorHdr[nSensorIndex].sName,
                    sizeof(sensorHdr[0].sName),
                    m_SensorCfg[nSensorIndex].sSensorName,
                    _TRUNCATE);
      strncpy_safe( sensorHdr[nSensorIndex].sUnits,
                    sizeof(sensorHdr[0].sUnits),
                    m_SensorCfg[nSensorIndex].sOutputUnits,
                    _TRUNCATE);
      // Increment the total used and decrement the amount remaining
      nTotal     += sizeof (SENSOR_HDR);
      nRemaining -= sizeof (SENSOR_HDR);
   }

   // Copy the Header to the buffer
   memcpy ( pBuffer, sensorHdr, nTotal );
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
   memset ( sensorETMHdr, 0, sizeof(sensorETMHdr) );

   // Loop through all the sensors
   for ( sensorIndex = 0;
         ((sensorIndex < MAX_SENSORS) && (nRemaining > sizeof (SENSOR_ETM_HDR)));
         sensorIndex++ )
   {
      sensorETMHdr[sensorIndex].configured = FALSE;

      if ( UNUSED != m_SensorCfg[sensorIndex].type )
      {
         sensorETMHdr[sensorIndex].configured = TRUE;
      }

      // Copy the name and units
      strncpy_safe( sensorETMHdr[sensorIndex].sensor.sName,
                    sizeof(sensorETMHdr[sensorIndex].sensor.sName),
                    m_SensorCfg[sensorIndex].sSensorName,
                    _TRUNCATE);
      strncpy_safe( sensorETMHdr[sensorIndex].sensor.sUnits,
                    sizeof(sensorETMHdr[sensorIndex].sensor.sUnits),
                    m_SensorCfg[sensorIndex].sOutputUnits,
                    _TRUNCATE);
      // Increment the total used and decrement the amount remaining
      nTotal     += sizeof (SENSOR_ETM_HDR);
      nRemaining -= sizeof (SENSOR_ETM_HDR);
   }

   // Copy the Header to the buffer
   memcpy ( pBuffer, sensorETMHdr, nTotal );
   // Return the total bytes written
   return ( nTotal );
}

/******************************************************************************
 * Function:     SensorDisableLiveStream
 *
 * Description:  SensorDisableLiveStream disables the live data mode for
 *               the GSE stream.
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
void SensorDisableLiveStream( void )
{
  m_liveDataDisplay.gseType = LD_NONE;
}

/******************************************************************************
 * Function:     SensorInitSummaryArray
 *
 * Description:  Utility function which uses a provided big-endian Bit-array
 *               representing which of the available sensors (127...0)
 *               are to be added to the provided list.
 *
 * Parameters:   [in/out] summary           - SNSR_SUMMARY array structure to be populated.
 *               [in]     summarySize       - The max allowable entries in SNSR_SUMMARY[].
 *               [in]     snsrMask          - Array of bits denoting sensors to add to list.
 *               [in]     snsrMaskSizeBytes - Size of snsrMask[] in bytes.
 *
 * Returns:      UINT16 the number of entries added to the summary array
 *
 * Notes:        This just sets up the array during initialization. To reset
 *               the values at run time use SensorResetSummaryArray.
 *****************************************************************************/
UINT16 SensorInitSummaryArray ( SNSR_SUMMARY summary[],  INT32 summarySize,
                                UINT32 snsrMask[],       INT32 snsrMaskSizeBytes )
{
  SENSOR_INDEX snsrIdx;
  UINT16       summaryCnt;
  INT16        i;

  // "Clear" the summary table by setting all indexes to "unused"
  for (i = 0; i < summarySize; ++i)
  {
    memset(&summary[i],0, sizeof(SNSR_SUMMARY));
    summary[i].SensorIndex = SENSOR_UNUSED;
  }

  summaryCnt = 0;
  for( snsrIdx = SENSOR_0;
       snsrIdx < (SENSOR_INDEX)MAX_SENSORS && summaryCnt < summarySize;
       ++snsrIdx )
  {
    if ( TRUE == GetBit((INT32)snsrIdx, snsrMask, snsrMaskSizeBytes ))
    {
      summary[summaryCnt].SensorIndex  = snsrIdx;
      ++summaryCnt;
    }
  }

  return (summaryCnt);
}


/******************************************************************************
 * Function:     SensorResetSummaryArray
 *
 * Description:  The SensorResetSummaryArray resets the statistics of the
 *               sensors that were initilialized to be monitored by independent
 *               system objects.
 *
 * Parameters:   [in/out] summary      - SNSR_SUMMARY array structure to be reset.
 *               [in]     nEntries     - number of used entries in SNSR_SUMMARY[].
 *
 * Returns:      None
 *
 * Notes:        Must run SensorInitSummaryArray to setup the summary array
 *               and get the nEntries. This should be done at initialization
 *               and then use the Reset Summary Array at run time.
 *****************************************************************************/
void SensorResetSummaryArray (SNSR_SUMMARY summary[], UINT16 nEntries)
{
  UINT32    arrayIdx;
  TIMESTAMP timeStampNow;

  CM_GetTimeAsTimestamp(&timeStampNow);

  // Loop thru the mask-of-sensors to be added to the Summary array
  // For each "ON" bit, initialize the next available SnsrSummary entry.
  for( arrayIdx = 0; arrayIdx < nEntries; arrayIdx++ )
  {
     summary[arrayIdx].nSampleCount = 0;
     summary[arrayIdx].bValid       = FALSE;
     summary[arrayIdx].fMinValue    = FLT_MAX;
     summary[arrayIdx].timeMinValue = timeStampNow;
     summary[arrayIdx].fMaxValue    = -FLT_MAX;
     summary[arrayIdx].timeMaxValue = timeStampNow;
     summary[arrayIdx].fTotal       = 0.f;
     summary[arrayIdx].fAvgValue    = 0.f;
  }
}

/******************************************************************************
 * Function:     SensorUpdateSummaries
 *
 * Description:  Update all the configured values in passed SNSR_SUMMARY array.
 *               Un-configured sensor-entries are ignored.
 *               Configured sensors which have become invalid are latched at
 *               their late-valid values time and averages.
 *
 * Parameters:   [in/out] summaryArray - SNSR_SUMMARY structure to be updated.
 *               [in]     nEntries     - Number of active entries in SNSR_SUMMARY
 *
 * Returns:      None
 *
 * Notes:        None
 *****************************************************************************/
 void SensorUpdateSummaries( SNSR_SUMMARY summaryArray[], UINT16 nEntries )
 {
  SNSR_SUMMARY* pSummary;
  FLOAT32       newValue = 0.f;
  BOOLEAN       bInitialized;
  INT32         i = 0;

  // Process each CONFIGURED entry in the array
  for (i = 0; i < nEntries; ++i)
  {
    pSummary = &summaryArray[i];

    // If the min or max values have been updated from default, it indicates
    // this sensor has been "initialized" (i.e. transitioned from unknown -> valid)
    bInitialized = (pSummary->fMinValue < FLT_MAX) || (pSummary->fMaxValue > -FLT_MAX);

    // INVALIDITY LATCHING LOGIC and totaling.

    // If sensor was initialized and is flagged as invalid.
    // Skip processing until this summary obj is reset.

    if( !pSummary->bValid && bInitialized )
    {
      continue;
    }

    // If the sensor is valid, update its total and sample count
    // If not, we stop updating the sensor summary for this sensor, calc it's average
    // and latch it until the this SNSR_SUMMARY is reset.

    // Get the validity and value of the sensor at 'pSummary'
    pSummary->bValid = SensorGetValueEx((SENSOR_INDEX)pSummary->SensorIndex, &newValue );

    if ( pSummary->bValid )
    {
      ++pSummary->nSampleCount;

      // Add value to total

      pSummary->fTotal += newValue;

      // Update min/max values and timestamp as needed.
      if(newValue < pSummary->fMinValue)
      {
        pSummary->fMinValue = newValue;
        CM_GetTimeAsTimestamp(&pSummary->timeMinValue);
      }

      if( newValue > pSummary->fMaxValue )
      {
        pSummary->fMaxValue = newValue;
        CM_GetTimeAsTimestamp(&pSummary->timeMaxValue);
      }
    }
    else
    {
      // If sensor WAS valid/initialized and NOW is invalid...
      // calculate the avg based on current count and total and never update this
      // sensor until reset.
      if( bInitialized && pSummary->nSampleCount != 0)
      {
        pSummary->fAvgValue = pSummary->fTotal * (1.0f / (FLOAT32) pSummary->nSampleCount);
      }
    }
  }
}


/******************************************************************************
 * Function:     SensorCalculateSummaryAvgs
 *
 * Description:  Calculate the average for configured values in passed SNSR_SUMMARY array.
 *               Un-configured sensor-entries are ignored.
 *               Configured sensors which have become invalid had their averages
 *               calculated at the time they went invalid and are not updated.
 *
 * Parameters:   [in/out] summaryArray - SNSR_SUMMARY structure to be updated.
 *               [in]     nEntries
 *
 * Returns:      None
 *
 * Notes:        None
 *****************************************************************************/
 void SensorCalculateSummaryAvgs( SNSR_SUMMARY summaryArray[], UINT16 nEntries )
 {
  SNSR_SUMMARY* pSummary;
  INT32 i;

  // Process each CONFIGURED entry
  for(i = 0; i < nEntries; ++i)
  {
    pSummary = &summaryArray[i];

    // If the sensor is still valid, and the sample count is not zero,
    // calculate the average. If it went invalid the average was already
    // calculated when the entry was latched to invalid.
    if(pSummary->bValid && pSummary->nSampleCount != 0)
    {
      pSummary->fAvgValue = pSummary->fTotal * (1.0f / (FLOAT32) pSummary->nSampleCount);
    }
  }
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
      switch (pSensorCfg->type)
      {
         case UNUSED:
            // Nothing to do, go to next sensor
            break;
         case SAMPLE_ARINC429:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex  =
                            Arinc429MgrSensorSetup(pSensorCfg->generalPurposeA,
                                                   pSensorCfg->generalPurposeB,
                                                   (UINT8)pSensorCfg->nInputChannel,
                                                   i);
            // Set the function pointers for the ARINC429 interface
            Sensors[i].pGetSensorData   = Arinc429MgrReadWord;
            Sensors[i].pTestSensor      = Arinc429MgrSensorTest;
            Sensors[i].pInterfaceActive = Arinc429MgrInterfaceValid;
            break;
         case SAMPLE_QAR:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex = QAR_SensorSetup (pSensorCfg->generalPurposeA,
                                                          pSensorCfg->generalPurposeB,
                                                          i);
            // Set the function pointers for the ARINC717 interface
            Sensors[i].pGetSensorData    = QAR_ReadWord;
            Sensors[i].pTestSensor       = QAR_SensorTest;
            Sensors[i].pInterfaceActive  = QAR_InterfaceValid;
            break;
         case SAMPLE_ANALOG:
            // Set the interface index and the function pointers for the ADC
            Sensors[i].nInterfaceIndex   = pSensorCfg->nInputChannel;
            Sensors[i].pGetSensorData    = ADC_GetValue;
            Sensors[i].pTestSensor       = ADC_SensorTest;
            Sensors[i].pInterfaceActive  = SensorNoInterface;
            break;
         case SAMPLE_DISCRETE:
            // Set the interface index and the function pointers for the Discretes
            Sensors[i].nInterfaceIndex   = pSensorCfg->nInputChannel;
            Sensors[i].pGetSensorData    = DIO_GetValue;
            Sensors[i].pTestSensor       = DIO_SensorTest;
            Sensors[i].pInterfaceActive  = SensorNoInterface;
            break;
         case SAMPLE_UART:
            // Supply the necessary data to the interface and record the
            // interface index to retrieve the data later.
            Sensors[i].nInterfaceIndex  =
                            UartMgr_SensorSetup(pSensorCfg->generalPurposeA,
                                                pSensorCfg->generalPurposeB,
                                                pSensorCfg->nInputChannel,
                                                i);
            // Set the function pointers for the UART interface
            Sensors[i].pGetSensorData   = UartMgr_ReadWord;
            Sensors[i].pTestSensor      = UartMgr_SensorTest;
            Sensors[i].pInterfaceActive = UartMgr_InterfaceValid;
            break;

         case SAMPLE_VIRTUAL:
         // GPA word contains the packed sensor indexes(A and B) and
         // the type(operation)
            Sensors[i].vSnsrA  =
           (SENSOR_INDEX)UNPACK_VIRTUAL_SNRA( pSensorCfg->generalPurposeA);
            Sensors[i].vSnsrB  =
           (SENSOR_INDEX)UNPACK_VIRTUAL_SNRB( pSensorCfg->generalPurposeA);
            Sensors[i].vOpType =
           (VIRTUALTYPE) UNPACK_VIRTUAL_TYPE( pSensorCfg->generalPurposeA);
            Sensors[i].vRawCombineAsize = 
           (UINT16) UNPACK_VIRTUAL_RAW_A_SIZE(pSensorCfg->generalPurposeA);
            Sensors[i].vRawCombineBsize = 
           (UINT16) UNPACK_VIRTUAL_RAW_B_SIZE(pSensorCfg->generalPurposeA);
            Sensors[i].vRawCombineNeg = 
           (BOOLEAN) UNPACK_VIRTUAL_RAW_NEGATIVE(pSensorCfg->generalPurposeA);
              

            //Validate the config of the Virtual sensor
            ASSERT_MESSAGE( (TRUE == SensorIsUsed(Sensors[i].vSnsrA)),
                            "Virtual Sensor[%d] - Sensor A Not Configured", i);
            ASSERT_MESSAGE( (TRUE == SensorIsUsed(Sensors[i].vSnsrB)),
                            "Virtual Sensor[%d] - Sensor B Not Configured", i);
            ASSERT_MESSAGE( Sensors[i].vSnsrA != i,
                            "Virtual Sensor[%d] - Sensor A Self Ref Error", i);
            ASSERT_MESSAGE( Sensors[i].vSnsrB != i,
                            "Virtual Sensor[%d] - Sensor B Self Ref Error", i);
            ASSERT_MESSAGE( (Sensors[i].vOpType > VIRTUAL_UNUSED &&
                            Sensors[i].vOpType < VIRTUAL_MAX),
                            "Virtual Sensor[%d] - Type Error: %d",
                            i, Sensors[i].vOpType );

            // Setup info unique to Virtual sensors.
            // Use our sensor index as the interface so the virtual sensor info can be
            // retrieved from SensorCfg[nSensorIndex] at run time.
            Sensors[i].nInterfaceIndex  = Sensors[i].nSensorIndex;
            Sensors[i].pGetSensorData   = SensorVirtualGetValue;
            Sensors[i].pTestSensor      = SensorVirtualSensorTest;
            Sensors[i].pInterfaceActive = SensorVirtualInterfaceValid;
            break;

        case MAX_SAMPLETYPE:
        default:
            // FATAL ERROR WITH CONFIGURATION
            // Initialize String
            FATAL("Sensor Config Fail: Sensor %d, Type %d not Valid",
                                                    i, pSensorCfg->type);
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
    if (pSensorConfig->type != UNUSED)
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
    pSensor->nSensorIndex     = Sensor;
    //pSensor->bValueIsValid    = TRUE;   // Valid by default
    pSensor->bValueIsValid    = FALSE;
    pSensor->bWasValidOnce    = FALSE;
    pSensor->fValue           = 0.0F;
    pSensor->fPriorValue      = 0.0F;
    pSensor->bPriorValueValid = FALSE;  // not yet computed
    pSensor->nNextSample      = 0;
    pSensor->nCurrentSamples  = 0;
    pSensor->nSampleCounts    = (INT16)(MIFs_PER_SECOND / pSensorConfig->sampleRate);
    pSensor->nSampleCountdown = (INT16)((pSensorConfig->nSampleOffset_ms / MIF_PERIOD_mS)+1);
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
    pSensor->pGetSensorData   = NULL;
    pSensor->pTestSensor      = NULL;
    pSensor->pInterfaceActive = NULL;

    // Initialize any dynamic filter elements
    SensorInitFilter(&pSensorConfig->filterCfg, &pSensor->filterData,
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
   UINT16          snsrMapIdx;

   // Loop through all the sensors
   for (nSensor = 0; nSensor <= maxSensorUsed; nSensor++)
   {
      // Set pointers to the configuration and the Sensor object
      pConfig = &m_SensorCfg[nSensor];
      pSensor = Sensors + nSensor;

      // Check if the sensor is configured
      if (UNUSED != pConfig->type)
      {
         // Countdown the number of samples until next read
         if (--pSensor->nSampleCountdown <= 0)
         {
            // Reload the sample countdown
            pSensor->nSampleCountdown = pSensor->nSampleCounts;
            // Read the sensor
            SensorRead( pConfig, pSensor );

            // If this sensor is in the livedata map, update it
            snsrMapIdx = m_liveDataMap[nSensor];
            if ( SENSOR_UNUSED != snsrMapIdx)
            {
              m_liveDataBuffer.sensor[snsrMapIdx].fValue        = pSensor->fValue;
              m_liveDataBuffer.sensor[snsrMapIdx].bValueIsValid = pSensor->bValueIsValid;
            }

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
   UINT32   lastUpdateTick;


    ASSERT( NULL != pSensor->pGetSensorData );
    ASSERT( NULL != pSensor->pInterfaceActive );
    ASSERT( NULL != pSensor->pTestSensor );

   // Make sure the sensors interface was not disabled on a previous call and is active
   if ( TRUE == pSensor->pInterfaceActive(pSensor->nInterfaceIndex) )
   {
      // Preset TickCount to current time.  For bus i/f type, TickCount will be
      //  updated to actual param rx time.  For others (ANALOG,DISC) it will be
      //  this time as param rx time.
      lastUpdateTick = CM_GetTickCount();

      // Read the Sensor data from the configured interface
      fVal = pSensor->pGetSensorData(pSensor->nInterfaceIndex,
                                     &lastUpdateTick);

      // Check if calibration is configured for sensor
      if (pConfig->calibration.type != NONE)
      {
         // Convert the sensor by the calibration parameters
         fVal = (fVal * pConfig->calibration.fParams[LINEAR_SLOPE]) +
                        pConfig->calibration.fParams[LINEAR_OFFSET];
      }

      // Check if conversion is configured for sensor
      if (pConfig->conversion.type != NONE)
      {
         // Convert the sensor by the conversion parameters
         fVal = (fVal * pConfig->conversion.fParams[LINEAR_SLOPE]) +
                        pConfig->conversion.fParams[LINEAR_OFFSET];
      }

      // Filter the latest sample value based on Cfg
      filterVal = SensorApplyFilter(fVal, pConfig, pSensor);

      // Perform a sensor check, (Rate, Range, BIT, Signal)
      if (SensorCheck( pConfig, pSensor, fVal, filterVal ))
      {
         // Save the previous value and current good value
         pSensor->fPriorValue = pSensor->fValue;
         pSensor->fValue      = filterVal;

         // Only if the sensor fValue has been updated (and passed SensorCheck())
         //    will this be considered a new updated value
         pSensor->lastUpdateTick = lastUpdateTick;
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
 *               [in] fVal        - Sensor value
 *               [in] filterVal   - Filter Value
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
   SENSOR_STATUS rangeStatus;
   SENSOR_STATUS rateStatus;
   SENSOR_STATUS signalStatus;
   SENSOR_STATUS interfaceStatus;

   // Initialize Local Data
   bOK             = FALSE;

   // Retrieve the status of each test
   rangeStatus     = SensorRangeTest     (pConfig, pSensor, filterVal);
   rateStatus      = SensorRateTest      (pConfig, pSensor, fVal);
   signalStatus    = SensorSignalTest    (pConfig, pSensor, fVal);
   interfaceStatus = SensorInterfaceTest (pConfig, pSensor, fVal);

   // Check if the sensor is already valid or not
   if ( pSensor->bValueIsValid )
   {
      pSensor->bWasValidOnce = TRUE;

      if ( SENSOR_OK == rangeStatus  && SENSOR_OK == rateStatus &&
           SENSOR_OK == signalStatus && SENSOR_OK == interfaceStatus )
      {
         // Report the sensor is OK
         bOK = TRUE;
      }
      // Determine if any test has exceeded persistence and FAILED
      else if ( SENSOR_FAILED == rangeStatus  || SENSOR_FAILED == rateStatus ||
                SENSOR_FAILED == signalStatus || SENSOR_FAILED == interfaceStatus )
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
      if ( SENSOR_OK == rangeStatus  && SENSOR_OK == rateStatus &&
           SENSOR_OK == signalStatus && SENSOR_OK == interfaceStatus )
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
  SENSOR_STATUS  status;

  // Initialize Local Data
  status = SENSOR_OK;

  // Check if the range test is enabled
  if ( TRUE == pConfig->bRangeTest)
  {
     // check for range
     if ((filterVal < pConfig->fMinValue) || (filterVal > pConfig->fMaxValue))
     {
        status = SENSOR_FAILING;

        // initialize the persistence timer
        if (FALSE == pSensor->bOutofRange)
        {
           pSensor->bOutofRange      = TRUE;
           pSensor->nRangeTimeout_ms = CM_GetTickCount() + pConfig->nRangeDuration_ms;
        }

        // Sensor is out of range, check if it has met its persistent
        if ((CM_GetTickCount() >= pSensor->nRangeTimeout_ms) &&
            (FALSE == pSensor->bRangeFail))
        {
           // Sensor is bad
           pSensor->bRangeFail = TRUE;
           // Make a note in the log about the bad sensor.
           SensorLogFailure( pConfig, pSensor, filterVal, RANGE_FAILURE);
           status              = SENSOR_FAILED;
        }
        else if (TRUE == pSensor->bRangeFail)
        {
           status              = SENSOR_FAILED;
        }
     }
     else // Sensor is in range reset the countdown
     {
        // If Range Test had failed, reset sysCond
        if ( pSensor->bRangeFail == TRUE )
        {
          Flt_ClrStatus( pConfig->systemCondition );
        }

        pSensor->bOutofRange = FALSE;
        pSensor->bRangeFail  = FALSE;
     }
  }

  return(status);
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
  SENSOR_STATUS status;
  FLOAT32       slope;

  // Initialize Local Data
  status = SENSOR_OK;

  // Check if the rate test is enabled
  if ( TRUE == pConfig->bRateTest)
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
        slope  = (fVal - pSensor->fRatePrevVal) * pConfig->sampleRate;
        pSensor->fRatePrevVal = fVal;

        // Store the slope for debugging
        pSensor->fRateValue = slope;

        // Make sure the slope isn't negative
        slope = (FLOAT32)fabs(slope);

        /* check for rate */
        if (slope > pConfig->fRateThreshold)
        {
           status = SENSOR_FAILING;

           // Sensor rate exceeded
           if (FALSE == pSensor->bRateExceeded)
           {
              // The Rate has been exceeded record the time and wait for a timeout
              pSensor->bRateExceeded   = TRUE;
              pSensor->nRateTimeout_ms = CM_GetTickCount() + pConfig->nRateDuration_ms;
           }

           // Check if the rate has persisted through the configured time
           if ((CM_GetTickCount() >= pSensor->nRateTimeout_ms) &&
               (FALSE == pSensor->bRateFail))
           {
              // Sensor is bad
              pSensor->bRateFail = TRUE;
              status             = SENSOR_FAILED;
              // Make a note in the log about the bad sensor.
              SensorLogFailure( pConfig, pSensor, fVal, RATE_FAILURE);
           }
           // The Rate already timed out just keep return fail until
           // it fixes itself. Note this will only happen for
           // sensors configured to be self healing.
           else if (TRUE == pSensor->bRateFail)
           {
              status = SENSOR_FAILED;
           }
        }
        else // Rate is OK Reset the flags
        {
           // If Rate Test had failed, reset sys cond
           if ( pSensor->bRateFail == TRUE )
           {
                Flt_ClrStatus( pConfig->systemCondition );
           }

           pSensor->bRateExceeded = FALSE;
           pSensor->bRateFail     = FALSE;
           status                 = SENSOR_OK;
        }
     }
  }

  return status;
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
   SENSOR_STATUS status;

   // Initialize Local Data
   status = SENSOR_OK;

   // Check if the signal test is enabled
   if ( TRUE == pConfig->bSignalTest)
   {
      // Perform the fault processing and check for fault
      if (FaultCompareValues(pConfig->signalTestIndex))
      {
         status = SENSOR_FAILING;

         // Signal has failed, check if it is persistent
         if (FALSE == pSensor->bSignalBad)
         {
            // At this point the sensor is failing but hasn't failed
            pSensor->bSignalBad        = TRUE;
            pSensor->nSignalTimeout_ms = CM_GetTickCount() + pConfig->nSignalDuration_ms;
         }

         // Determine if the duration has been met
         if ((CM_GetTickCount() >= pSensor->nSignalTimeout_ms) &&
             (FALSE == pSensor->bSignalFail))
         {
            // Sensor is now considered FAILED
            pSensor->bSignalFail = TRUE;
            status               = SENSOR_FAILED;
            // Make a note in the log about the bad sensor.
            SensorLogFailure( pConfig, pSensor, fVal, SIGNAL_FAILURE);
         }
         // Was the Signal Test already failed?
         else if (TRUE == pSensor->bSignalFail)
         {
            // Sensor remains failed
            status = SENSOR_FAILED;
         }
      }
      else // Signal is OK reset the countdown
      {
         // If Signal Test had failed, reset sys cond
         if ( pSensor->bSignalFail == TRUE )
         {
           Flt_ClrStatus( pConfig->systemCondition );
         }

         pSensor->bSignalBad  = FALSE;
         pSensor->bSignalFail = FALSE;
      }
   }
   return status;
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
   SENSOR_STATUS status;
   BOOLEAN       bOK;

   ASSERT( NULL != pSensor->pTestSensor );

   // Initialize Local Data
   status = SENSOR_FAILED;

   // Check the Interface Test
   bOK = pSensor->pTestSensor(pSensor->nInterfaceIndex);

   // Is the interface test OK
   if (TRUE == bOK)
   {
      // If BIT Test had failed, reset sys cond
      if ( pSensor->bBITFail == TRUE )
      {
        Flt_ClrStatus( pConfig->systemCondition );
      }

      // No problem with the interface detected
      status = SENSOR_OK;
      pSensor->bBITFail      = FALSE;
   }
   else if (FALSE == pSensor->bBITFail) // Problem on the interface detected
   {
      // The sensor has failed log the failure
      pSensor->bBITFail = TRUE;
      SensorLogFailure( pConfig, pSensor, fVal, BIT_FAILURE);
   }

   return status;
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
 *               [in] fVal        - Sensor value
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
   SYS_APP_ID logID;
   SENSORLOG  faultLog;


   // Build the fault log
   faultLog.failureType        = type;
   faultLog.nIndex             = pSensor->nSensorIndex;
   strncpy_safe(faultLog.sName, sizeof(faultLog.sName), pConfig->sSensorName,_TRUNCATE);
   faultLog.fCurrentValue      = fVal;
   faultLog.fValue             = pSensor->fValue;
   faultLog.fPreviousValue     = pSensor->fPriorValue;
   faultLog.fRateValue         = pSensor->fRateValue;
   faultLog.fExpectedMin       = pConfig->fMinValue;
   faultLog.fExpectedMax       = pConfig->fMaxValue;
   faultLog.fExpectedThreshold = pConfig->fRateThreshold;
   faultLog.nDuration_ms       = 0;

   // Display a debug message about the failure
   GSE_DebugStr(NORMAL, TRUE, "Sensor (%d) - %s",
                pSensor->nSensorIndex, sensorFailureNames[type]);

   // Determine the correct log ID to store
   switch (type)
   {
      case BIT_FAILURE:
         logID                 = SYS_ID_SENSOR_CBIT_FAIL;
         break;
      case SIGNAL_FAILURE:
         logID                 = SYS_ID_SENSOR_SIGNAL_FAIL;
         break;
      case RANGE_FAILURE:
         faultLog.nDuration_ms = pConfig->nRangeDuration_ms;
         logID                 = SYS_ID_SENSOR_RANGE_FAIL;
         break;
      case RATE_FAILURE:
         faultLog.nDuration_ms = pConfig->nRateDuration_ms;
         logID                 = SYS_ID_SENSOR_RATE_FAIL;
         break;
      case NO_FAILURE:
      case MAX_FAILURE:
      default:
         // FATAL ERROR WITH FAILURE TYPE
          FATAL("Invalid sensor failure type: %d", type);
         break;
   }

   // Store the fault log data
   Flt_SetStatus(pConfig->systemCondition, logID, &faultLog, sizeof(faultLog));
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
*               [in]  UINT16 nMaximumSamples     - Total number of configured
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
    switch(pFilterCfg->type)
    {
    case FILTERNONE:
        break;

    case EXPO_AVERAGE:
        pFilterData->fK = 2.0f /(1.0f + nFloat);
        break;

    case SPIKEREJECTION:
        pFilterData->fK = 2.0f / (1.0f + nFloat);
        pFilterData->fullScaleInv = 1.0f/pFilterCfg->fFullScale;
        pFilterData->maxPercent = (FLOAT32)pFilterCfg->nMaxPct/100.0f;
        pFilterData->spikePercent = (FLOAT32)pFilterCfg->nSpikeRejectPct/100.0f;
        break;

    case MAXREJECT:
        pFilterData->fK = 2.0f / (1.0f + nFloat);
        break;

    case SLOPEFILTER:
        /* compute the K constant */
        pFilterData->fK = 0.0;

        for (i = 0.0f; i < nFloat; i = i+1.0f)
        {
            temp = (i - ((nFloat - 1.0f) * 0.5f));
            pFilterData->fK += temp * temp;
        }
        pFilterData->fK = 1.0f/pFilterData->fK;

        /* compute (n-1)/2 */
        pFilterData->nMinusOneOverTwo = ((nFloat - 1.0f) * 0.5f);
        break;

    default:
        // FATAL ERROR WITH FILTER TYPE
        FATAL("Sensor Filter Init Fail: Type = %d", pFilterCfg->type);
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
 * Parameters:   [in] fVal - Sensor Value
 *               [in] pConfig     - Pointer to the sensor configuration
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
   switch (pConfig->filterCfg.type)
   {
      case FILTERNONE:
         break;
      case EXPO_AVERAGE:
         fVal = SensorExponential( fVal, &pSensor->filterData);
         break;
      case SPIKEREJECTION:
         fVal = SensorSpikeReject(fVal, &pConfig->filterCfg, &pSensor->filterData);
         break;
      case MAXREJECT:
         fVal = SensorMaxReject(fVal, &pConfig->filterCfg, &pSensor->filterData);
         break;
      case SLOPEFILTER:
         fVal = SensorSlope ( fVal, pConfig, pSensor);
         break;
      default:
         // FATAL ERROR WITH FILTER TYPE
         FATAL("Invalid Sensor Filter Type: %s(%d) - FilterType(%d)",
                 pConfig->sSensorName,
                 pSensor->nSensorIndex,
                 pConfig->filterCfg.type);
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
 * Parameters:   [in] fVal            - Sensor value
 *               [in/out] pFilterData - pointer to filter data
 *
 * Returns:      FLOAT32 - Average of the samples
 *
 * Notes:        None.
 *
 *****************************************************************************/
static FLOAT32 SensorExponential (FLOAT32 fVal, FILTER_DATA *pFilterData)
{
    pFilterData->fLastAvgValue +=
        (pFilterData->fK * ( fVal - pFilterData->fLastAvgValue));
    return pFilterData->fLastAvgValue;
}

/******************************************************************************
 * Function:     SensorSpikeReject
 *
 * Description:  The SensorSpikeReject function will filter out any spikes
 *               in the sensor value until that sensor exceeds the number
 *               of reject counts.
 *
 * Parameters:   [in] fVal            - Value of raw sensor
 *               [in] pFilterCfg      - Pointer to filter config
 *               [in/out] pFilterData - pointer to the filter data
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
        (pFilterData->fLastValidValue - pFilterData->fLastAvgValue) * pFilterData->fK;

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
        ((useValue - pFilterData->fLastAvgValue) * pFilterData->fK);

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
 * Parameters:   [in] fVal         - Value of raw sensor
 *               [in] pConfig      - Pointer to sensor config
 *               [in/out] pSensor  - pointer to the sensor data
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
       FILTER_DATA*   pFilterData = &pSensor->filterData;

       for (i = 0; i < nSamples; i++)
       {
          iFloat       = i;
          ix = (UINT8)((i + pSensor->nNextSample) % nSamples);
          returnSlope += (iFloat - pFilterData->nMinusOneOverTwo) *
                         pSensor->fSamples[ix];
       }
       returnSlope *= pFilterData->fK;
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
       // Reduced Code above to the statement below
       returnSlope *= pConfig->sampleRate;
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
  INT16            i;
  SENSOR_LD_ENUM   liveDataDest[LD_DEST_MAX];

  // Initialize Local Data
  pTCB = (SENSOR_LD_PARMS *)pParam;

  // Is live data enabled?
  if (LD_NONE != m_liveDataDisplay.gseType ||
      LD_NONE != m_msType)
  {
    // Make sure the rate is not too fast and fix if it is
    nRate = m_liveDataDisplay.nRate_ms < SENSOR_LD_MAX_RATE ?
              SENSOR_LD_MAX_RATE : m_liveDataDisplay.nRate_ms;

    // Check if end of period has been reached
    if ((CM_GetTickCount() - pTCB->startTime) >= nRate)
    {
      // Save new start time
      pTCB->startTime = CM_GetTickCount();

      // Use an array to process the streams dest, prevent if..if..if
      liveDataDest[LD_DEST_GSE] = m_liveDataDisplay.gseType;
      liveDataDest[LD_DEST_MS]  = m_msType;

      // The livedata structure is updated in real-time when the sensors are read...
      // Get the current time and perform the outputs.
      CM_GetTimeAsTimestamp(&m_liveDataBuffer.timeStamp);

      for (i = 0; i < LD_DEST_MAX; ++i)
      {
        switch( liveDataDest[i] )
        {
          case LD_NONE:
            // Nothing to do here
            break;

          case LD_ASCII:
            SensorDumpASCIILiveData( (LD_DEST_ENUM)i );
            break;

          case LD_BINARY:
            SensorDumpBinaryLiveData( (LD_DEST_ENUM)i );
            break;

          default:
            FATAL("Unrecognized LiveData Stream output %d", liveDataDest[i] );
            break;
        } // switch output fmts
      } // for dest types
    } // time to process?
  } //Is live data enabled?
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
 * Parameters:   dest - SENSOR_LD_ENUM value indicating the dest (GSE or MS for
 *                      stream output.
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
static void SensorDumpASCIILiveData(LD_DEST_ENUM dest)
{
  // Local Data
  TIMESTRUCT    ts;
  INT8          str[3072]; // 3k buffer is enough to handle max 128-sensor ascii stream
  INT8          tempStr[64];
  UINT32        idx;
  UINT16        checksum;
  const CHAR*   startDelims = "\r\n\0";

  // Initialize local data
  memset(str,0, sizeof(str));

  snprintf(str, sizeof(str), "#77");

  // Loop through all sensors
  for (idx = 0; idx < m_liveDataBuffer.count; idx++)
  {
    // Add the sensor index, value and validity indicator to the stream
    snprintf(tempStr, sizeof(tempStr), "|%03d:%.4f:%c",
             m_liveDataBuffer.sensor[idx].nSensorIndex,
             m_liveDataBuffer.sensor[idx].fValue,
             m_liveDataBuffer.sensor[idx].bValueIsValid ? '1' : '0');
    strcat(str,tempStr);
  }

  // Add the collection time to the live data stream
  CM_ConvertTimeStamptoTimeStruct(&m_liveDataBuffer.timeStamp, &ts);

  snprintf (tempStr, sizeof(tempStr), "|t:%02d:%02d:%02d.%02d", ts.Hour,
             ts.Minute,
             ts.Second,
             (ts.MilliSecond/10) );
  strcat(str,tempStr);

  // Checksum the data
  checksum = (UINT16)ChecksumBuffer(str, strlen(str), 0xFFFF);
  // Add checksum to end of live data
  snprintf (tempStr, sizeof(tempStr), "|c:%04X", checksum);
  strcat(str,tempStr);

  // Output the live data stream on the GSE port or to MS
  // #77 msg should be preceded by \r\n
  if (LD_DEST_GSE == dest)
  {
    GSE_PutLine(startDelims);
    GSE_PutLine(str);
  }
  else if (LD_DEST_MS == dest)
  {
    GSE_MsWrite(startDelims, strlen(startDelims) );
    GSE_MsWrite( str, strlen(str) );
  }
  else
  {
    FATAL("Unrecognized Output dest for ASCII Stream: %d", dest);
  }
}

/******************************************************************************
 * Function:     SensorDumpBinaryLiveData
 *
 * Description:  SensorDumpBinaryLiveData formats the sensor live data and
 *               outputs it on the 'dest' interface.
 *
 *               The format is:
 *               #88
 *               TIMESTAMP Time;
 *               UINT16 NumSensors;
 *               struct sensor [NumSensors]
 *               {
 *                 UINT16 Index;
 *                 FLOAT32 Value;
 *                 BOOLEAN Validity;
 *               }
 *              UINT32 Checksum;
 *
 * Parameters:   dest - SENSOR_LD_ENUM value indicating the dest (GSE or MS for
 *                      stream output.
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
static void SensorDumpBinaryLiveData(LD_DEST_ENUM dest)
{
  // Local Data
  INT8    str[2048]; // 2K buffer is enough to handle max 128-sensor binary stream
  UINT16  checksum;
  UINT32  dataBlockSize;
  INT8*   pStr = str;

  // Initialize local data
  memset(str,0, sizeof(str));

  // Msg type
  snprintf(str, sizeof(str), "#88");
  pStr += 3;

  // Timestamp, num sensors, the sensor sets(idx, value, validity)
  dataBlockSize = sizeof(m_liveDataBuffer.timeStamp) +
                  sizeof(m_liveDataBuffer.count)     +
                  (m_liveDataBuffer.count * sizeof(SENSOR_LD_ENTRY));

  memcpy(pStr, &m_liveDataBuffer.timeStamp, dataBlockSize);
  pStr += dataBlockSize;

  // Checksum the data
  checksum = (UINT16)ChecksumBuffer(str, strlen(str), 0xFFFF);

  // Add checksum to end of live data
  memcpy(pStr, &checksum, sizeof(checksum));
  pStr += sizeof(checksum);


  if (LD_DEST_GSE == dest)
  {
    GSE_write(str, (UINT16)(pStr - str));
  }
  else if (LD_DEST_MS == dest)
  {
    GSE_MsWrite( str, (UINT16)(pStr - str));
  }
  else
  {
    FATAL("Unrecognized Output dest for BINARY Stream= %d", dest);
  }

}
/******************************************************************************
 * Function:     SensorInitializeLiveData
 *
 * Description:  SensorInitializeLiveData inits the structure used for ASCII
 *               and BINARY streaming and the mapping from Sensors -> livedata
 *               structure.
 *
 * Parameters:   None
 *
 * Returns:      None.
 *
 * Notes:        None
 *****************************************************************************/
static void SensorInitializeLiveData(void)
{
  UINT32 sensorIndex;
  UINT16 nCount = 0;

  // Clear buffer
  memset(&m_liveDataBuffer, 0, sizeof(m_liveDataBuffer));

  // Initialize the map
  for (sensorIndex = 0; sensorIndex < MAX_SENSORS; sensorIndex++)
  {
    m_liveDataMap[sensorIndex] = SENSOR_UNUSED;
  }

  // Loop through all configured sensors.
  // For each sensor enabled for livedata streaming, init the next avail entry in collection
  // structure and create a map entry so it can be quickly updated during sensor processing
  for (sensorIndex = 0; sensorIndex < MAX_SENSORS; sensorIndex++)
  {
    // Check if inspection is enabled and the sensor is in use.
    if (TRUE == m_SensorCfg[sensorIndex].bInspectInclude &&
        SensorIsUsed(( SENSOR_INDEX)sensorIndex) )
    {
      m_liveDataMap[sensorIndex] = nCount;
      m_liveDataBuffer.sensor[nCount].nSensorIndex = (UINT16)sensorIndex;
      nCount++;
    }
  }
  // Save the count of sensors in the list and the time this collection was stored.
  m_liveDataBuffer.count = nCount;
}

/******************************************************************************
 * Function:     SensorVirtualSensorTest
 *
 * Description:  Verifies the virtual sensor's input are/still valid
 *
 * Parameters:   nIndex - index into Sensors[] data array containing the
 *                        the sensor to be checked.
 *
 * Returns:      FALSE - the sensor has invalid input
 *               TRUE  - the sensor inputs are valid
 *
 * Notes:        This test is used to determine if inputs continue to be
 *               valid/invalid during self-heal testing.
 *
 *****************************************************************************/
static BOOLEAN SensorVirtualSensorTest (UINT16 nIndex)
{
  BOOLEAN bResult = TRUE;
  SENSOR* pData    = &Sensors[nIndex];
  //SENSOR* pSensorA = &Sensors[pData->vSnsrA];
  SENSOR* pSensorB = &Sensors[pData->vSnsrB];

  if(VIRTUAL_DIV == pData->vOpType && 0.f == pSensorB->fValue)
  {
    bResult = FALSE;
  }

  // Add additional tests here..

  return bResult;
}

/******************************************************************************
 * Function:     SensorVirtualGetValue
 *
 * Description:  Calculates and returns the current value of the sensor using
 *               the configured virtualType(i.e. arithmetic operation)
 *
 * Parameters:   nIndex - index into the SensorConfig structure for the sensor
 *                        configuration. This contains the sensors ( A and B) and the
 *                        operation to be performed
 *               *null -  Not used - definition required by common interface.
 *
 * Returns:      FLOAT32 the requested value as the result of the virtual type operation
 *
 * Notes:
 *
 *****************************************************************************/
static FLOAT32 SensorVirtualGetValue ( UINT16 nIndex, UINT32 *null )
{
  FLOAT32 fVal = 0.f;
  SENSOR* pData    = &Sensors[nIndex];
  SENSOR* pSensorA = &Sensors[pData->vSnsrA];
  SENSOR* pSensorB = &Sensors[pData->vSnsrB];
  UINT32 rawA;
  UINT32 rawB;
  UINT32 rawCombined; 
  SINT32 *signedPtr; 

  switch( pData->vOpType )
  {
    case VIRTUAL_ADD:
      fVal = pSensorA->fValue + pSensorB->fValue;
      break;

    case VIRTUAL_DIFF:
      fVal = pSensorA->fValue - pSensorB->fValue;
      break;

    case VIRTUAL_MULT:
      fVal = pSensorA->fValue * pSensorB->fValue;
      break;

    case VIRTUAL_DIV:
      // Check for potential divide by zero, flag sensor as invalid
      if(pSensorB->fValue != 0.f)
      {
        fVal = pSensorA->fValue / pSensorB->fValue;
      }
      else
      {
        fVal = pData->fPriorValue;
        pData->bValueIsValid = FALSE;
      }
      break;

    case VIRTUAL_RAW_COMBINE:
      // Combine A and B, where A is MSW and B is LSW.  Handle sign if configured
      rawA = (UINT32) pSensorA->fValue; 
      rawB = (UINT32) pSensorB->fValue; 
      rawCombined = 0; 
      rawCombined = (rawA << (32 - pData->vRawCombineAsize));
      rawCombined = rawCombined | (rawB << (32 - (pData->vRawCombineBsize +
                                                  pData->vRawCombineAsize)));
      // Shift it back into place, also include sign if configured as signed value
      if (pData->vRawCombineNeg == TRUE) {
        signedPtr = (SINT32 *) &rawCombined; 
        fVal = ((FLOAT32)(*signedPtr)) / (1U << (32 - (pData->vRawCombineBsize + 
                                                       pData->vRawCombineAsize)));
      }
      else {
        fVal = (FLOAT32)rawCombined / (1U << (32 - (pData->vRawCombineBsize + 
                                                    pData->vRawCombineAsize)));
      }
      break; 
      
    case VIRTUAL_MAX:
    case VIRTUAL_UNUSED:
    default:
      FATAL("Unsupported VIRTUALTYPE = %d", pData->vOpType );
      break;
  }
  return fVal;
}


/******************************************************************************
 * Function:     SensorVirtualInterfaceValid
 *
 * Description:  Returns the validity of the virtual sensor's interface
 *               as the logical AND of the validity of the two referenced sensors A and B
 *
 * Parameters:   nIndex - index into the SensorConfig structure for the sensor
 *                        configuration. This contains the sensors ( A and B)
 *
 *
 * Returns:      TRUE if both Sensor A and Sensor B are valid, otherwise FALSE
 *
 * Notes:        None
 *
 *****************************************************************************/
static BOOLEAN SensorVirtualInterfaceValid(UINT16 nIndex)
{
  BOOLEAN bValid = TRUE;

  ASSERT(nIndex < MAX_SENSORS);

  // The virtual sensor is valid IFF both of it's referenced sensors are valid.
  // Use the value validity of the component sensors  as this sensors validity
    bValid = Sensors[ Sensors[nIndex].vSnsrA].bValueIsValid &&
             Sensors[ Sensors[nIndex].vSnsrB].bValueIsValid;
  return bValid;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: sensor.c $
 * 
 * *****************  Version 109  *****************
 * User: John Omalley Date: 5/19/16    Time: 1:03p
 * Updated in $/software/control processor/code/system
 * SCR 1328 - Code Review Findings
 * 
 * *****************  Version 108  *****************
 * User: Contractor V&v Date: 4/19/16    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #1328 Add support for vSensor Raw Combine
 * 
 * *****************  Version 107  *****************
 * User: Contractor V&v Date: 2/02/16    Time: 5:19p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Perf Enhancment improvements removed SensorNoTest
 * 
 * *****************  Version 106  *****************
 * User: Contractor V&v Date: 2/01/16    Time: 7:07p
 * Updated in $/software/control processor/code/system
 * Perf Enhancment improvements CR fixes and bug
 * 
 * *****************  Version 105  *****************
 * User: Contractor V&v Date: 2/01/16    Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR #1192 - Perf Enhancment improvements 
 * 
 * *****************  Version 104  *****************
 * User: John Omalley Date: 1/21/16    Time: 4:33p
 * Updated in $/software/control processor/code/system
 * SCR 1302 - Discrete processing from display updates
 * 
 * *****************  Version 103  *****************
 * User: Contractor V&v Date: 12/18/15   Time: 11:30a
 * Updated in $/software/control processor/code/system
 * SCR #1299 - Code Review update for SensorVirtualTestSensor func.
 * 
 * *****************  Version 102  *****************
 * User: Contractor V&v Date: 12/17/15   Time: 6:38p
 * Updated in $/software/control processor/code/system
 * SCR #1299 bug fix - added SensorTest impl for vSensor keep it invalid
 * 
 * *****************  Version 101  *****************
 * User: Contractor V&v Date: 12/10/15   Time: 3:48p
 * Updated in $/software/control processor/code/system
 * SCR #1299 CR compliance changes.
 * 
 * *****************  Version 100  *****************
 * User: Contractor V&v Date: 12/03/15   Time: 5:59p
 * Updated in $/software/control processor/code/system
 * SCR #1299 CR compliance.
 * 
 * *****************  Version 99  *****************
 * User: Contractor V&v Date: 12/03/15   Time: 11:50a
 * Updated in $/software/control processor/code/system
 * SCR #1300 - Code Review compliance update
 *
 * *****************  Version 98  *****************
 * User: Contractor V&v Date: 11/05/15   Time: 6:31p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Switch to ENUM types vs UINT8
 *
 * *****************  Version 97  *****************
 * User: Contractor V&v Date: 11/02/15   Time: 6:22p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Use ASSERT instead of log msg and invalidated
 * sensors
 *
 * *****************  Version 96  *****************
 * User: Contractor V&v Date: 9/23/15    Time: 1:21p
 * Updated in $/software/control processor/code/system
 * P100 Update for virtual sensor validation and error logging
 *
 * *****************  Version 95  *****************
 * User: Contractor V&v Date: 9/09/15    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * P100 Update for virtual sensor validation and error logging.
 *
 * *****************  Version 94  *****************
 * User: Contractor V&v Date: 8/26/15    Time: 7:19p
 * Updated in $/software/control processor/code/system
 * SCR #1299 - P100 Implement the Deferred Virtual Sensor Requirements
 *
 * *****************  Version 93  *****************
 * User: John Omalley Date: 1/19/15    Time: 12:03p
 * Updated in $/software/control processor/code/system
 * SCR 1276 - Code Review Updates
 *
 * *****************  Version 92  *****************
 * User: John Omalley Date: 12/11/14   Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR 1276 - Wrapped sensor utilities with a check for vaild sensor
 * index.
 *
 * *****************  Version 91  *****************
 * User: Contractor V&v Date: 11/20/14   Time: 7:25p
 * Updated in $/software/control processor/code/system
 * SCR #1262 -  LiveData CP to MS  Code Review changes
 *
 * *****************  Version 90  *****************
 * User: Contractor V&v Date: 11/03/14   Time: 5:21p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS Modfix for ms_type
 *
 * *****************  Version 89  *****************
 * User: Contractor V&v Date: 9/23/14    Time: 6:04p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS Fix fornatting.
 *
 * *****************  Version 88  *****************
 * User: Contractor V&v Date: 9/22/14    Time: 6:46p
 * Updated in $/software/control processor/code/system
 * SCR #1262 - LiveData CP to MS
 *
 * *****************  Version 87  *****************
 * User: John Omalley Date: 4/16/14    Time: 11:50a
 * Updated in $/software/control processor/code/system
 * SCR 1168 - Fixed specific complaint for debug message not having a
 * timestamp on sensor failure.
 *
 * *****************  Version 86  *****************
 * User: John Omalley Date: 12-12-08   Time: 1:32p
 * Updated in $/software/control processor/code/system
 * SCR 1167 - Sensor Summary Init optimization
 *
 * *****************  Version 85  *****************
 * User: Contractor V&v Date: 12/05/12   Time: 4:17p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Bitbucket #80
 *
 * *****************  Version 84  *****************
 * User: John Omalley Date: 12-12-02   Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 83  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 82  *****************
 * User: Contractor V&v Date: 11/26/12   Time: 12:42p
 * Updated in $/software/control processor/code/system
 * SCR #1167 SensorSummary
 *
 * *****************  Version 81  *****************
 * User: Peter Lee    Date: 12-11-19   Time: 7:35p
 * Updated in $/software/control processor/code/system
 * SCR #1195 had to update GetSensorData()  to pGetSensorData()
 *
 * *****************  Version 80  *****************
 * User: Peter Lee    Date: 12-11-18   Time: 3:26p
 * Updated in $/software/control processor/code/system
 * SCR #1195 Creep coding errors.  Incorrect update of
 * sensor.lastUpdateTick.
 *
 * *****************  Version 79  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 78  *****************
 * User: John Omalley Date: 12-11-12   Time: 11:36a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 77  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:04p
 * Updated in $/software/control processor/code/system
 * SCR #1191 Returns update time of param
 *
 * *****************  Version 76  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:42p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 75  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 6:49p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Init max sensor to -FLT_MAX
 *
 * *****************  Version 74  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:24p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Fix AutoTrends
 *
 * *****************  Version 73  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:48p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 fixed SensorSummary handling
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

