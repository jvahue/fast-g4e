#define CREEP_BODY

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        Creep.c

    Description: Contains all functions and data related to the Creep

    VERSION
      $Revision: 3 $  $Date: 12-11-10 4:37p $

******************************************************************************/


/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include <float.h>
#include <math.h>
#include <stdio.h>

#include "Creep.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"
#include "TaskManager.h"

#include "Sensor.h"
#include "EngineRun.h"

#include "Assert.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
CREEP_STATUS m_Creep_Status[CREEP_MAX_OBJ];
CREEP_APP_DATA m_Creep_AppData;
CREEP_CFG m_Creep_Cfg;
CREEP_SENSOR_VAL m_Creep_Sensors[CREEP_MAX_OBJ];
CREEP_APP_DATA_RTC m_Creep_AppDataRTC;
CREEP_DEBUG m_Creep_Debug;

FLOAT64 m_Creep_100_pcnt;
UINT32 m_Creep_SensorStartTimeout_ms;

static CHAR gse_OutLine[128];

CREEP_FAULT_HISTORY m_creepHistory;

// Test Timing
#ifdef CREEP_TIMING_TEST
  #define CREEP_TIMING_BUFF_MAX 200
  UINT32 m_CreepTiming_Buff[CREEP_TIMING_BUFF_MAX];
  UINT32 m_CreepTiming_Start;
  UINT32 m_CreepTiming_End;
  UINT16 m_CreepTiming_Index;
#endif
// Test Timing



/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void Creep_Task (void *pParam);
static void Creep_SensorProcessing ( CREEP_OBJECT_PTR pCreepObj,
                                     CREEP_SENSOR_VAL_PTR pCreepSensorVal );
static void Creep_ProcessSensor ( const CREEP_SENSOR *pSensor,
                                  CREEP_SENSOR_DATA_PTR pSensorData );
static FLOAT64 Creep_Calculate( CREEP_SENSOR_VAL_PTR pCreepSensorVal,
                                UINT16 CreepTblId );
static void Creep_FindEntries( UINT16 *p1, UINT16 *p0, const FLOAT64 *pEntries,
                               FLOAT64 val, UINT16 entriesMax );
static FLOAT64 Creep_InterpolateVal( FLOAT64 y1, FLOAT64 x1, FLOAT64 y0, FLOAT64 x0,
                                   FLOAT64 val);
static BOOLEAN Creep_RestoreAppData( void );
static BOOLEAN Creep_RestoreAppDataRTC( void );

//static void Creep_UpdateCreepXAppData (UINT16 index_obj);

static void Creep_CreateSummaryLog ( UINT16 index_obj );
static void Creep_CreateSensorFailLog ( UINT16 index_obj );
static void Creep_CreateFaultLog ( CREEP_FAULT_TYPE type );
static void Creep_CreatePBITLog ( PBIT_FAIL_TYPE type, UINT16 expCRC, UINT16 calcCRC );

static BOOLEAN Creep_CheckCfgAndData ( void );
static void Creep_RestoreCfg ( void );

static void Creep_RestoreFaultHistory( void );
static void Creep_AddToFaultBuff ( SYS_APP_ID id );

static void Creep_FailAllObjects ( UINT16 index );
static void Creep_InitPersistentPcnt ( void ); 


#include "CreepUserTables.c"


#ifdef CREEP_TEST
  // Debug Functions
  static ER_STATE EngRunGetState_Sim (ENGRUN_INDEX idx, UINT8* EngRunFlags);
  static UINT32 SensorGetLastUpdateTime_Sim ( SENSOR_INDEX id );
  static FLOAT32 SensorGetValue_Sim( SENSOR_INDEX id);
  // Debug Functions
#endif

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/

// Test data
#ifdef CREEP_TEST
  #define SIM_SENSOR_MAX_VALUES 20
  #define SIM_SENSORS 2

  const FLOAT32 SIM_SENSOR [SIM_SENSORS][SIM_SENSOR_MAX_VALUES] =
  {
  //{ 1.0f, 2.0f, 805.118765, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f,
  //11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },

  //{ 101.0f, 102.0f, 103.0f, 37200.0, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f,
  // 111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f }

  { 1.0f, 2.0f, 910.0, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f,
   11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, 17.0f, 18.0f, 19.0f, 20.0f },

  { 101.0f, 102.0f, 103.0f, 41000, 105.0f, 106.0f, 107.0f, 108.0f, 109.0f, 110.0f,
   111.0f, 112.0f, 113.0f, 114.0f, 115.0f, 116.0f, 117.0f, 118.0f, 119.0f, 120.0f }
  };

  UINT16 sim_sensor_cnt[SIM_SENSORS];

  ER_STATE m_er_state;
  UINT16 m_sensor_rate_sim[SIM_SENSORS];

  UINT32 m_er_state_timer;
  #define ER_STATE_STARTUP_DELAY  10000 // Delay 10 sec on startup
  //#define ER_STATE_STARTUP_DELAY  60000 // Delay 60 sec on startup
  //#define ER_STATE_ER_DELAY       600000 // ER is 10 minutes
  //#define ER_STATE_ER_DELAY       6000     // ER is 6 sec
  #define ER_STATE_ER_DELAY       15000     // ER is 15 sec
  #define ER_STATE_STOP_DELAY     0
  UINT32 m_er_test_state;

  // Test Debug
  UINT32 m_Test_CntERInterval;
  // Test Debug
#endif


typedef struct {
  SYS_APP_ID id;
} CREEP_TYPE_TO_SYSLOG_ID;

// NOTE: Changes to PBIT_FAIL_TYPE enum list will affect the item below.
const CREEP_TYPE_TO_SYSLOG_ID creep_PBIT_TypeToSysLog[PBIT_FAIL_MAX] =
{
  SYS_ID_NULL_LOG,
  APP_ID_CREEP_PBIT_CFG_CRC,
  APP_ID_CREEP_PBIT_EE_APP,
  APP_ID_CREEP_PBIT_CRC_CHANGED
};

// NOTE: Changes to CREEP_FAULT_TYPE enum list will affect the item below.
const CREEP_TYPE_TO_SYSLOG_ID creep_CBIT_TypeToSysLog[CREEP_FAULT_TYPE_MAX] =
{
  SYS_ID_NULL_LOG,
  APP_ID_CREEP_CBIT_FAULT_ER,
  APP_ID_CREEP_CBIT_FAULT_RESET
};


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/
/******************************************************************************
 * Function:    Creep_Initialize
 *
 * Description: Initializes the Creep functionality
 *
 * Parameters:  BOOLEAN degradedMode TRUE or FALSE
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void Creep_Initialize ( BOOLEAN degradedMode )
{
  TCB    task;
  UINT16 i,j;
  BOOLEAN bCheckDsbOk;


  // Initialize Local Variables
  memset ( m_Creep_Status, 0, sizeof(CREEP_STATUS) * CREEP_MAX_OBJ);
  memset ( &m_Creep_AppData, 0, sizeof(CREEP_APP_DATA) );
  memset ( &m_Creep_Cfg, 0, sizeof(CREEP_CFG) );
  memset ( m_Creep_Sensors, 0, sizeof(CREEP_SENSOR_VAL) * CREEP_MAX_OBJ);
  memset ( &m_Creep_AppDataRTC, 0, sizeof(CREEP_APP_DATA_RTC) );
  memset ( &m_creepHistory, 0, sizeof(CREEP_FAULT_HISTORY) );

  // Retrieve Creep History
  Creep_RestoreFaultHistory();


  // Initialize Rate Cnt to 1 Hz
  for (j=0;j<CREEP_MAX_OBJ;j++)
  {
    for (i=0;i<CREEP_SENSOR_RATE_BUFF_SIZE;i++)
    {
      m_Creep_Sensors[j].row.rateTimeBuff[i] = CREEP_SENSOR_RATE_MINIMUM;
      m_Creep_Sensors[j].col.rateTimeBuff[i] = CREEP_SENSOR_RATE_MINIMUM;
    }
    m_Creep_Sensors[j].row.rateTime = CREEP_SENSOR_RATE_MINIMUM;
    m_Creep_Sensors[j].col.rateTime = CREEP_SENSOR_RATE_MINIMUM;

    m_Creep_Sensors[j].row.timeout_ms = CREEP_SENSOR_RATE_MINIMUM;
    m_Creep_Sensors[j].col.timeout_ms = CREEP_SENSOR_RATE_MINIMUM;
  }

  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    // Initially set status to FAULTED for better code coverage below
    m_Creep_Status[i].status = CREEP_STATUS_FAULTED_PBIT;
    m_Creep_Status[i].state = CREEP_STATE_IDLE;
    // Setup Creep Interval
    m_Creep_Status[i].interval.nRateCounts = MIFs_PER_SECOND;  // 1 Hz
    m_Creep_Status[i].interval.nRateCountDown =
             (INT16) (m_Creep_Cfg.creepObj[i].cpuOffset_ms / MIF_PERIOD_mS) + 1;
             // Why "+1" ? To be consistent with other modules.
  }

  // Did we get a PBIT error ? If no, then update status to OK.
  //   Note: this logic help code coverage
  Creep_RestoreCfg();
  bCheckDsbOk = TRUE;
  if (degradedMode != TRUE)
  {
    bCheckDsbOk = Creep_CheckCfgAndData();
  }

  // if (Creep_RestoreAndCheckCfgAndData() == TRUE)
  if (bCheckDsbOk == TRUE )
  {
    for (i=0;i<CREEP_MAX_OBJ;i++)
    {
      m_Creep_Status[i].status = CREEP_STATUS_OK;
      // If any RowSensor/ColSensor/TableId/ERId is not defined or Creep Not enabled
      //   DISABLE this Obj
      if ( (m_Creep_Cfg.creepObj[i].sensorRow.id == (INT16) SENSOR_UNUSED) ||
           (m_Creep_Cfg.creepObj[i].sensorCol.id == (INT16) SENSOR_UNUSED) ||
           (m_Creep_Cfg.creepObj[i].creepTblId == CREEP_TABLE_UNUSED) ||
           (m_Creep_Cfg.creepObj[i].engId == ENGINERUN_UNUSED)||
           ( m_Creep_Cfg.bEnabled == FALSE ) )
      {
        m_Creep_Status[i].status = CREEP_STATUS_OK_DISABLED;
      }
    }
  }


  // Add an entry in the user message handler table for F7X Cfg Items
  User_AddRootCmd(&creepRootTblPtr);

  // Setup Creep Processing Task
  memset(&task, 0, sizeof(task));
  strncpy_safe(task.Name, sizeof(task.Name), "Creep Task", _TRUNCATE);
  task.TaskID         = (INT16) Creep;
  task.Function       = Creep_Task;
  task.Priority       = taskInfo[Creep].priority;
  task.Type           = taskInfo[Creep].taskType;
  task.modes          = taskInfo[Creep].modes;
  task.MIFrames       = taskInfo[Creep].MIFframes;
  task.Rmt.InitialMif = taskInfo[Creep].InitialMif;
  task.Rmt.MifRate    = taskInfo[Creep].MIFrate;
  task.Enabled        = TRUE;
  task.Locked         = FALSE;
  task.pParamBlock    = m_Creep_Status;
  TmTaskCreate (&task);

  m_Creep_SensorStartTimeout_ms = CM_GetTickCount() + m_Creep_Cfg.sensorStartTime_ms;

  m_Creep_Debug.nSensorFailedCnt = 0;

  // Test Debug
#ifdef CREEP_TEST
  m_er_state = ER_STATE_STOPPED;
  for (i=0;i<SIM_SENSORS;i++)
  {
    m_sensor_rate_sim[i] = 50;  // 50 msec or 20 Hz
  }
  // Test Debug

  m_er_state_timer = CM_GetTickCount() + ER_STATE_STARTUP_DELAY;
  m_er_test_state = ER_STATE_STARTUP_DELAY;
#endif


#ifdef CREEP_TIMING_TEST
  for (i=0;i<CREEP_TIMING_BUFF_MAX;i++)
  {
    m_CreepTiming_Buff[i] = 0;
  }
  m_CreepTiming_Index = 0;
  m_CreepTiming_Start = 0;
  m_CreepTiming_End = 0;
#endif

}


/******************************************************************************
 * Function:    Creep_Task
 *
 * Description: Main Creep Task Process
 *
 * Parameters:  pParam - ptr to m_Creep_Status[]
 *
 * Returns:     None
 *
 * Notes:       This task must exe at 10 msec (or every MIF frame), due to
 *              timing dependencies.
 *
 *****************************************************************************/
static void Creep_Task ( void *pParam )
{
  CREEP_CFG_PTR pCfg;
  ER_STATE er_state;
  CREEP_DATA_PTR pCreepData;
  CREEP_STATUS_PTR pStatus;
  CREEP_INTERVAL_PTR pCreepInterval;
  FLOAT64 creepIncrement;
  BOOLEAN bCPUOffset;
  UINT16 i;
  BOOLEAN bUpdateNV;
  BOOLEAN bLogSummary;


  pCfg = (CREEP_CFG_PTR) &m_Creep_Cfg;
  pStatus = ((CREEP_STATUS_PTR) pParam);

#ifdef CREEP_TIMING_TEST
   m_CreepTiming_Start = TTMR_GetHSTickCount();
#endif


  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    pStatus = pStatus + i;
    bUpdateNV = FALSE;
    bLogSummary = FALSE;

    // If Creep Object is defined and not PBIT faulted,
    //   then process CREEP else move to next Creep Object
    if ( (pStatus->status != CREEP_STATUS_OK_DISABLED) &&
         (pStatus->status != CREEP_STATUS_FAULTED_PBIT) &&
         (pCfg->bEnabled == TRUE) )
    {
      pCreepData = (CREEP_DATA_PTR) &pStatus->data;
      pCreepInterval = (CREEP_INTERVAL_PTR) &pStatus->interval;
      creepIncrement = 0;

      // Count Down Creep Interval.  Note: Only when ER is ACTIVE will
      //   Creep Count be processed.  However, we want the CPU offset interval
      //   to line up with other modules, thus it needs to run here.
      bCPUOffset = FALSE;
      if (--pCreepInterval->nRateCountDown <= 0)
      {
        pCreepInterval->nRateCountDown = pCreepInterval->nRateCounts;
        bCPUOffset = TRUE;
      }

      // Process Row/Col Sensor for this Object on every MIF Frame
      Creep_SensorProcessing( &pCfg->creepObj[i], &m_Creep_Sensors[i]);

      // Get current ER status
      er_state = EngRunGetState( (ENGRUN_INDEX) pCfg->creepObj[i].engId, NULL );
      // er_state = EngRunGetState_Sim ( (ENGRUN_INDEX) pCfg->creepObj[i].EngId, NULL );

      if ( (er_state == ER_STATE_STARTING) || (er_state == ER_STATE_RUNNING) )
      {
        // ER Active.
        if ((pStatus->state == CREEP_STATE_IDLE) ||
            (pStatus->state == CREEP_STATE_FAULT_IDLE) )
        {
          // We just trans from IDLE or FAULT_IDLE to ER
          pStatus->state = CREEP_STATE_ENGRUN;
          pCreepData->creepMissionCnt = 0;  // Clear current creep count if not already cleared
          pStatus->erTime_ms = 0;
        }

        if (m_Creep_Sensors[i].bFailed == FALSE)
        {
          // Process current Creep Cnt, on processing interval
          if (bCPUOffset == TRUE)
          {
            creepIncrement = Creep_Calculate(&m_Creep_Sensors[i],
                                               pCfg->creepObj[i].creepTblId);
            // Update erTime count
            pStatus->erTime_ms += (pCreepInterval->nRateCounts * 10);
          }
        }
        else
        {
          // Sensor failed
          if (pStatus->state != CREEP_STATE_FAULT_ER)
          {
            // Force other object to also fail per SRS-4601
            Creep_FailAllObjects(i);
          }
        } // End of else Creep Sensors have failed

        // Update Creep Count for this flight.
        // When ER ends, the count will be the new mission count or thrown away
        //   due to sensor failure detected.
        pCreepData->creepMissionCnt += creepIncrement;

        // Update App Data RTC if new count.  Note: _Calculate occurs every 1 sec
        //   Thus fastest is  1 Hz write (or for two Creep Obj, this will be 2 writes
        //   per sec or 2 Hz).  Do we need to limit ?
        if (creepIncrement != 0)
        {
          m_Creep_AppDataRTC.creepMissionCnt[i] = pCreepData->creepMissionCnt;
          NV_Write(NV_CREEP_CNTS_RTC, 0, (void *) &m_Creep_AppDataRTC,
                   sizeof(CREEP_APP_DATA_RTC) );
        }
      }
      else
      {
        // No ER.
        // We just trans from ER to IDLE.
        if (pStatus->state == CREEP_STATE_ENGRUN)
        {
          // Save current Creep Cnt to current Mission Cnt.
          // Clear current Creep Cnt.
          // Store current Mission Cnt to accum Mission Cnt.
          pCreepData->creepLastMissionCnt = pCreepData->creepMissionCnt;
          pCreepData->creepMissionCnt = 0;
          pCreepData->creepAccumCnt += pCreepData->creepLastMissionCnt;

          // Set to IDLE
          pStatus->state = CREEP_STATE_IDLE;

          bUpdateNV = TRUE;
          bLogSummary = TRUE;
        } // End of if trans from ER to IDLE

        // We just trans from ER_FAULT to IDLE
        if (pStatus->state == CREEP_STATE_FAULT_ER)
        {
          // Sensor failed while ER was active.  Trans to IDLE.
          //   Save current Creep Cnt to accum Thrown Aways Creep Cnt
          //   Clear current Creep Cnt.
          pCreepData->creepAccumCntTrashed += pCreepData->creepMissionCnt;
          pCreepData->creepLastMissionCnt = pCreepData->creepMissionCnt;
          pCreepData->creepMissionCnt = 0;

          bUpdateNV = TRUE;
        } // End of if

        // Detect if Sensor have failed.
        if ( m_Creep_Sensors[i].bFailed == TRUE )
        {
          // If trans to sensor faulted then inc ->creepFailures
          if (pStatus->state != CREEP_STATE_FAULT_IDLE)
          {
            pCreepData->sensorFailures++;
            // If we trans from ER FAULT, then we don't want to record
            //   Sensor Failure log as Creep Fault Log would have been recorded.
            if (pStatus->state != CREEP_STATE_FAULT_ER)
            {
              if (m_Creep_Debug.nSensorFailedCnt < m_Creep_Cfg.maxSensorLossRec)
              {
                Creep_CreateSensorFailLog(i);
              }
              snprintf (gse_OutLine, sizeof(gse_OutLine),
                        "Creep Task: Sensor Failed (Obj=%d) ! \r\n ", i );
              GSE_DebugStr(NORMAL,TRUE,gse_OutLine);
            }
            pStatus->state = CREEP_STATE_FAULT_IDLE;
            if (m_Creep_Debug.nSensorFailedCnt < m_Creep_Cfg.maxSensorLossRec)
            {
              bUpdateNV = TRUE;  // Note: RTC App Data also cleared.
            }
          }
        }
        else
        {
          pStatus->state = CREEP_STATE_IDLE;
        }

        // Update NV EEPROM Data
        if (bUpdateNV == TRUE)
        {
          // Update EEPROM App Data
          m_Creep_AppData.data[i] = *pCreepData;
          NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));

          // Clear RTC App Data
          m_Creep_AppDataRTC.creepMissionCnt[i] = 0;
          NV_Write(NV_CREEP_CNTS_RTC, 0, (void *) &m_Creep_AppDataRTC,
                   sizeof(CREEP_APP_DATA_RTC) );

          // Update percentages
          m_Creep_Status[i].data_percent.accumCnt =
                      m_Creep_Status[i].data.creepAccumCnt / m_Creep_100_pcnt;
          m_Creep_Status[i].data_percent.accumCntTrashed =
                      m_Creep_Status[i].data.creepAccumCntTrashed / m_Creep_100_pcnt;
          m_Creep_Status[i].data_percent.lastMissionCnt =
                      m_Creep_Status[i].data.creepLastMissionCnt / m_Creep_100_pcnt;

          // Write Summary Log
          if (bLogSummary == TRUE)
          {
            Creep_CreateSummaryLog(i);
          }
        }
      } // End of ELSE if current ER State is IDLE

      // Clear buffer after interval.  This is done always.
      if ( bCPUOffset == TRUE )
      {
        m_Creep_Sensors[i].row.cnt = 0;
        m_Creep_Sensors[i].col.cnt = 0;
      }

    } // End of if Creep Object is defined

  } // End for i Loop thru CREEP_MAX_OBJ


  // Testing
#ifdef CREEP_TEST
  switch ( m_er_test_state )
  {
    case ER_STATE_STARTUP_DELAY:  // Wait about 10 sec
      if (m_er_state_timer <= CM_GetTickCount())
      {
        m_er_state_timer = CM_GetTickCount() + ER_STATE_ER_DELAY;
        m_er_test_state = ER_STATE_ER_DELAY;
        m_er_state = ER_STATE_STARTING;
      }
      break;

    case ER_STATE_ER_DELAY:  // Stay in ER for 15 sec
      if (m_er_state_timer <= CM_GetTickCount())
      {
        m_er_test_state = ER_STATE_STOP_DELAY;
        m_er_state = ER_STATE_STOPPED;
      }
      break;

    case ER_STATE_STOP_DELAY:  // Do nothing
    default:
      break;
  }
  // Testing
#endif

#ifdef CREEP_TIMING_TEST
   m_CreepTiming_End = TTMR_GetHSTickCount();
   m_CreepTiming_Buff[m_CreepTiming_Index] = (m_CreepTiming_End - m_CreepTiming_Start);
   m_CreepTiming_Index = (++m_CreepTiming_Index) % CREEP_TIMING_BUFF_MAX;
#endif

}


/******************************************************************************
 * Function:    Creep_GetStatus
 *
 * Description: Utility function to request current Creep[index] status
 *
 * Parameters:  index_obj of Creep Object
 *
 * Returns:     Ptr to Creep Status data
 *
 * Notes:       None
 *
 *****************************************************************************/
CREEP_STATUS_PTR Creep_GetStatus (UINT8 index_obj)
{
  return ( (CREEP_STATUS_PTR) &m_Creep_Status[index_obj] );
}


/******************************************************************************
 * Function:    Creep_GetSensor
 *
 * Description: Utility function to request current Creep[index] Sensor
 *
 * Parameters:  index_obj of Sensor Object
 *
 * Returns:     Ptr to Creep Sensor data
 *
 * Notes:       None
 *
 *****************************************************************************/
CREEP_SENSOR_VAL_PTR Creep_GetSensor (UINT8 index_obj)
{
  return ( (CREEP_SENSOR_VAL_PTR) &m_Creep_Sensors[index_obj] );
}


/******************************************************************************
 * Function:    Creep_GetCfg
 *
 * Description: Utility function to request current Creep Cfg
 *
 * Parameters:  None
 *
 * Returns:     Ptr to Creep Cfg
 *
 * Notes:       None
 *
 *****************************************************************************/
CREEP_CFG_PTR Creep_GetCfg ( void )
{
  return ( (CREEP_CFG_PTR) &m_Creep_Cfg );
}


/******************************************************************************
 * Function:    Creep_GetDebug
 *
 * Description: Utility function to request current Creep Debug
 *
 * Parameters:  None
 *
 * Returns:     Ptr to Creep Debug
 *
 * Notes:       None
 *
 *****************************************************************************/
CREEP_DEBUG_PTR Creep_GetDebug ( void )
{
  return ( (CREEP_DEBUG_PTR) &m_Creep_Debug );
}


/******************************************************************************
 * Function:    Creep_GetBinaryHdr()
 *
 * Description: Function to return the Creep file hdr structure
 *
 * Parameters:  pDest - ptr to destination bufffer
 *              nMaxByteSize - max size of dest buffer
 *
 * Returns:     byte count of data returned
 *
 * Notes:       none
 *
 *****************************************************************************/
UINT16 Creep_GetBinaryHdr ( void *pDest, UINT16 nMaxByteSize )
{
  CREEP_BINHDR creepBinHdr;
  UINT16    i;
  CREEP_OBJ_BINHDR_PTR pCreepObjHdr;
  CREEP_OBJECT_PTR     pCreepObjCfg;


  // If buffer is smaller than what we need ASSERT as this is
  //  a development / design error.  If this get's into field
  //  something is fundamentally wrong !!! You deserve an ASSERT !!
  ASSERT ( nMaxByteSize > sizeof(CREEP_BINHDR) );

  creepBinHdr.bEnabled = m_Creep_Cfg.bEnabled;
  creepBinHdr.crc16 = m_Creep_Cfg.crc16;
  creepBinHdr.baseUnits = m_Creep_Cfg.baseUnits;

  pCreepObjCfg = (CREEP_OBJECT_PTR) &m_Creep_Cfg.creepObj[0];
  pCreepObjHdr = (CREEP_OBJ_BINHDR_PTR) &creepBinHdr.creepObj[0];

  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    memcpy ( (void *) pCreepObjHdr->name, (void *) pCreepObjCfg->name,
             CREEP_MAX_NAME );
    pCreepObjHdr->rowSensorId = pCreepObjCfg->sensorRow.id;
    pCreepObjHdr->colSensorId = pCreepObjCfg->sensorCol.id;

    ++pCreepObjHdr;
    ++pCreepObjCfg;
  }

  memcpy ( pDest, (void *) &creepBinHdr, sizeof(CREEP_BINHDR) );

  return (sizeof(CREEP_BINHDR));
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
 * Function:    Creep_SensorProcessing
 *
 * Description: Processes the Row and Col Sensors associated with a Creep Obj /
 *              Creep Table.
 *
 * Parameters:  pCreepObj  - Ptr to Creep Cfg Obj containing Row and Col Sensor Info
 *              pCreepSensorVal - Ptr to Run Time Creep Obj Sensor Data (Row and Col)
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void Creep_SensorProcessing ( CREEP_OBJECT_PTR pCreepObj,
                                     CREEP_SENSOR_VAL_PTR pCreepSensorVal )
{
  CREEP_SENSOR_PTR pSensor;
  CREEP_SENSOR_DATA_PTR pSensorData;


  // Update Row Sensor Value
  pSensor = (CREEP_SENSOR_PTR) &pCreepObj->sensorRow;
  pSensorData = (CREEP_SENSOR_DATA_PTR) &pCreepSensorVal->row;
  Creep_ProcessSensor ( pSensor, pSensorData );

  // Get Update Col Sensor Value
  pSensor = (CREEP_SENSOR_PTR) &pCreepObj->sensorCol;
  pSensorData = (CREEP_SENSOR_DATA_PTR) &pCreepSensorVal->col;
  Creep_ProcessSensor ( pSensor, pSensorData );

  // Update bFailed flag from Row and Col Sensor Status
  pCreepSensorVal->bFailed = pCreepSensorVal->row.bFailed || pCreepSensorVal->col.bFailed;

}


/******************************************************************************
 * Function:    Creep_ProcessSensor
 *
 * Description: Reads the current updated value of a creep sensor and determine
 *              if the sensor has failed (i.e. data loss timeout).
 *
 * Parameters:  pSensor - Ptr to Creep Sensor Cfg data
 *              pSensorData - Ptr to Sensor Buffer to store/buff updated values
 *                                (also contains sensor statistics)
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static void Creep_ProcessSensor ( const CREEP_SENSOR *pSensor,
                                  CREEP_SENSOR_DATA_PTR pSensorData )
{
  UINT32 lastTime, rateTime;
  UINT16 id, i;
  BOOLEAN bNewVal;
  UINT32 timeout_ms;
  FLOAT32 fval;


  id = pSensor->id;
  bNewVal = FALSE;

  // lastTime = SensorGetLastUpdateTime ((SENSOR_INDEX) id);
  // lastTime = SensorGetLastUpdateTime_Sim ( (SENSOR_INDEX) id );
  lastTime = SensorGetLastUpdateTime( (SENSOR_INDEX) id );

  if (lastTime != pSensorData->lastUpdate)
  {
    // We have updated value.  Get sensor value.
    // Add to current value list.  Note: As of 09/21/12 we don't need slope/offset, but
    //   will support it.
    // pSensorData->val[pSensorData->cnt++] = SensorGetValue( (SENSOR_INDEX) id);
    // pSensorData->val[pSensorData->cnt++] = SensorGetValue_Sim( (SENSOR_INDEX) id);
    fval = SensorGetValue( (SENSOR_INDEX) id);

    // Convert value mx + b
    fval = (pSensor->slope * fval) + pSensor->offset;

    // metal = hot - E * (hot - cool) ?

    // Buffer updated value
    pSensorData->val[pSensorData->cnt++] = fval;
    pSensorData->currVal = fval;

    // House keeping
    // ASSERT if cnt >= CREEP_MAX_SENSOR_VAL, when # vals > buffer size

    // Determine approx rate of input
    rateTime = lastTime - pSensorData->lastUpdate;

    // If rate time > 1 sec, then something must be wrong, throw this value away
    if (rateTime < CREEP_SENSOR_RATE_MINIMUM)
    {
      // Add this one to the buffer
      pSensorData->rateTimeBuff[pSensorData->rateIndex++] = rateTime;
      if (pSensorData->rateIndex >= CREEP_SENSOR_RATE_BUFF_SIZE)
      {
        pSensorData->rateIndex = 0; // Handle Wrap Case
      }

      // Calc the ave est rate
      pSensorData->rateTime = 0.0f;
      for (i=0;i<CREEP_SENSOR_RATE_BUFF_SIZE;i++)
      {
        pSensorData->rateTime += pSensorData->rateTimeBuff[i];
      }
      pSensorData->rateTime /= CREEP_SENSOR_RATE_BUFF_SIZE;
      // If rate has not changed over past 10 sec, then its probably a solid
      //   value, don't need to continue calc rate.  Need logic?  Also might
      //   want to seperate out.
    }

    // Update LastUpdate Time
    pSensorData->lastUpdate = lastTime;
    bNewVal = TRUE;
  }

  // If startup and no data yet (as indicated by lastTime == 0), use
  //   Startup Time to determine sensor failure
  if ( lastTime == 0)
  {
    // Startup Timeout to determine Creep Sensor Fault
    if (CM_GetTickCount() > m_Creep_SensorStartTimeout_ms)
    {
      pSensorData->bFailed = TRUE;
    }
  }
  else
  {
    if (bNewVal == TRUE)
    {
      // If sensor failure is indicated, un-sensor failure
      if (pSensorData->bFailed == TRUE)
      {
        pSensorData->bFailed = FALSE;
      }
    }
    else
    {
      if (pSensorData->bFailed == FALSE)
      {
        // Note: Can't calc timeout until we actually know what the input rate is
        //       thus must determine timeout real time.  Note: If input rate
        //       is determined on startup (i.e. wait 1-2 min), then timeout
        //       could be determined in that logic, thus not necessary to compute
        //       continuously here.

        // Use ->sampleRate_ms as default for now.  Line add here for code coverage, rather
        //   then creating 'else' case.
        pSensorData->timeout_ms = pSensor->sampleRate_ms;

        if (pSensor->sampleCnt != 0)
        {
          // Use minimum sampleRate as default for timeout
          pSensorData->timeout_ms = (pSensor->sampleRate_ms * pSensor->sampleCnt);
          // Calc timeout based on actual data input rate
          timeout_ms = (UINT32) (pSensorData->rateTime * pSensor->sampleCnt);
          if ( timeout_ms < pSensorData->timeout_ms )
          {
            // Use faster rate timeout
            pSensorData->timeout_ms = timeout_ms;
          }
        }

        if ((CM_GetTickCount() - pSensorData->lastUpdate) > (pSensorData->timeout_ms))
        {
          pSensorData->bFailed = TRUE;
        }
      } // End of if (pSensorData->bFailed == FALSE)

    } // End of else no new data
  } // End of lastTime == 0

}


/******************************************************************************
 * Function:    Creep_Calculate
 *
 * Description: Determines the "Creep Increment" count (1 sec) based on
 *              a) the highest Row and Col Value in buffer (1 sec)
 *              b) extrapolates the damage count from a Creep Table based
 *                 on the found highest Row and Col Value.
 *
 * Parameters:  pCreepSensorVal - Ptr to the Creep Sensors (Row and Col) associated
 *                                with this Creep Object
 *              CreepTblId - Id of Creep Table to be used to determine current
 *                                "Creep Increment" count
 *
 * Returns:     INT64 Creep Increment Count - Extrapolated damage count based on
 *                                            current highest Row and Col value
 *                                            that have been buffered.
 *
 * Notes:       None
 *
 *****************************************************************************/
static
FLOAT64 Creep_Calculate ( CREEP_SENSOR_VAL_PTR pCreepSensorVal, UINT16 CreepTblId )
{
  FLOAT64 rowVal, colVal;  // Highest Row and Col Value for this period
  CREEP_SENSOR_DATA_PTR pSensorData;
  FLOAT64 creepIncrement;
  UINT16 c1,c0,r1,r0;   // Indeces into Row Col of Table
  FLOAT64 d1, d0;         // Intermediate Damage Cnt Value used during interploation
                        //    of creepIncrement value
  CREEP_TBL_PTR pCreepTbl;
  UINT16 i;


  // Find the highest Row Sensor Value
  pSensorData = (CREEP_SENSOR_DATA_PTR) &pCreepSensorVal->row;
  rowVal = -FLT_MAX;
  for (i=0;i<pSensorData->cnt;i++)
  {
    if (rowVal < pSensorData->val[i])
    {
      rowVal = pSensorData->val[i];
    }
  }
  pSensorData->peakVal = rowVal;

  // Find the highest Col Sensor Value
  pSensorData = (CREEP_SENSOR_DATA_PTR) &pCreepSensorVal->col;
  colVal = -FLT_MAX;
  for (i=0;i<pSensorData->cnt;i++)
  {
    if (colVal < pSensorData->val[i])
    {
      colVal = pSensorData->val[i];
    }
  }
  pSensorData->peakVal = colVal;

  c1 = 0;
  c0 = 0;
  r1 = 0;
  r0 = 0;
  d1 = 0;
  d0 = 0;
  creepIncrement = 0;
  pCreepTbl = (CREEP_TBL_PTR) &m_Creep_Cfg.creepTbl[CreepTblId];

  // Is Row Val or Col Val below the table mins ?  Set creepIncrement to 0 if Yes
  if (!((rowVal < pCreepTbl->rowVal[0]) || (colVal < pCreepTbl->colVal[0])))
  {
    // Interplolate the creepIncrement Damage from the table
    // Find if we are outside the maximum of the table
    if ( ( rowVal >= pCreepTbl->rowVal[CREEP_MAX_ROWS-1] ) &&
         ( colVal >= pCreepTbl->colVal[CREEP_MAX_COLS-1] ) )
    {
      // Apply max damage count value
      creepIncrement = pCreepTbl->dCount[CREEP_MAX_ROWS-1][CREEP_MAX_COLS-1];
    }
    else
    {
      // Interpolate between Row and Col value
      if ( (rowVal < pCreepTbl->rowVal[CREEP_MAX_ROWS-1]) &&
           (colVal < pCreepTbl->colVal[CREEP_MAX_COLS-1]) )
      {
        // Find Col1 and Col0 where ColVal falls between
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
        Creep_FindEntries( &c1, &c0, &pCreepTbl->colVal[0], colVal, CREEP_MAX_COLS);
        // Find Row1 and Row0 where RowVal falls between
        Creep_FindEntries( &r1, &r0, &pCreepTbl->rowVal[0], rowVal, CREEP_MAX_ROWS);
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
        // Extrapolate the d1 val for RowVal between Row1 and Row0 at Col1
        d1 = Creep_InterpolateVal( pCreepTbl->dCount[r1][c1], pCreepTbl->rowVal[r1],
                                   pCreepTbl->dCount[r0][c1], pCreepTbl->rowVal[r0], rowVal );
        // Extrapolate the d0 val for RowVal between Row1 and Row0 at Col0
        d0 = Creep_InterpolateVal( pCreepTbl->dCount[r1][c0], pCreepTbl->rowVal[r1],
                                   pCreepTbl->dCount[r0][c0], pCreepTbl->rowVal[r0], rowVal );
        // Extrapolate the creepIncrement for the ColVal betwen Col1 and Col0 at RowVal
        creepIncrement = Creep_InterpolateVal( d1, pCreepTbl->colVal[c1],
                                               d0, pCreepTbl->colVal[c0], colVal );
      }
      else  // Either Row or Col max encountered
      {
        if ( rowVal >= pCreepTbl->rowVal[CREEP_MAX_ROWS-1])
        {
          // RowVal beyond RowMax
          // Find Col1 and Col0 where ColVal falls between
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
          Creep_FindEntries( &c1, &c0, &pCreepTbl->colVal[0], colVal, CREEP_MAX_COLS );
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
          // Interpolate the creepIncrement for the ColVal between
          //                                    Col1 and Col0 for RowMax
          creepIncrement = Creep_InterpolateVal( pCreepTbl->dCount[CREEP_MAX_ROWS-1][c1],
                                                 pCreepTbl->colVal[c1],
                                                 pCreepTbl->dCount[CREEP_MAX_ROWS-1][c0],
                                                 pCreepTbl->colVal[c0],colVal );
        }
        else
        {
          // ColVal beyond ColMax
          // Find Row1 and Row0 where RowVal falls between
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
          Creep_FindEntries( &r1, &r0, &pCreepTbl->rowVal[0], rowVal, CREEP_MAX_ROWS );
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
          // Interpolate the creepIncrement for the RowVal between
          //                                    Row1 and Row0 for ColMax
          creepIncrement = Creep_InterpolateVal( pCreepTbl->dCount[r1][CREEP_MAX_COLS - 1],
                                                 pCreepTbl->rowVal[r1],
                                                 pCreepTbl->dCount[r0][CREEP_MAX_COLS - 1],
                                                 pCreepTbl->rowVal[r0],rowVal );
        }
      } // End else, either Row or Col Max Val encountered
    } // Else, need to interpolate creepIncrement from the table
  } // End of if neither Row or Col Value is below table minimum
  // Else creepIncrement = 0 if either Row or Col Val is below the minimum

  return (creepIncrement);

}


/******************************************************************************
 * Function:    Creep_FindEntries
 *
 * Description: Determine the two entries (either row or col) where the 'val'
 *              falls between.  Used later to interpolate damage count for this
 *              value.
 *
 * Parameters:  UINT16  *p1 - ptr to return index of lower entry (found)
 *              UINT16  *p0 - ptr to return index of upper entry (found)
 *              FLOAT64 *pEntries - ptr to either the row or col entries in table to
 *                                  search for lower and upper entries that 'val'
 *                                  falls between
 *              FLOAT64 val - value to search for lower and upper entries
 *              UINT16  entriesMax - Max col or row entries in table ptr to by
 *                                   pEntries
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void Creep_FindEntries( UINT16 *p1, UINT16 *p0, const FLOAT64 *pEntries,
                               FLOAT64 val, UINT16 entriesMax)
{
  UINT16 i;


  for (i=0;i<(entriesMax-1);i++)  // Less one for entriesMax as we should not
  {                               //    go beyond MaxEntry in table (calling app restricts)
    if ( (val >= *pEntries) && (val < *(pEntries + 1)) )
    {
      *p0 = i;
      *p1 = i + 1;
      break;
    }
    pEntries++;  // Advance to the next entry for comparison
  }

  // Assert if i==entriesMax-1 !!! ????
}


/******************************************************************************
 * Function:    Creep_InterpolateVal
 *
 * Description: Extrapolate a value 'z' for 'val' given y_1,x_1,y_0,x_0
 *
 * Parameters:  FLOAT64   y_1 - y_1 value (i.e. damage cnt)
 *              FLOAT64   x_1 - x_1 value associated with y1 value
 *              FLOAT64   y_0 - y_0 value (i.e. damage cnt)
 *              FLOAT64   x_0 - x_0 value associated with y0 value
 *              FLOAT64   val - value to determine damage cnt based on y_1,x_1,y_0,x_0
 *
 * Returns:     Extrapolated Value 'z' (i.e. damage cnt)
 *
 * Notes:       Ex Value = [((y_1 - y_0)/(x_1 - x_0)) * (val - x_0)] + y_0
 *
 *****************************************************************************/
static
FLOAT64 Creep_InterpolateVal( FLOAT64 y_1, FLOAT64 x_1, FLOAT64 y_0, FLOAT64 x_0, FLOAT64 val)
{
  FLOAT64 dVal;
  FLOAT64 z;


  dVal = (y_1 - y_0)/( (FLOAT64) x_1 - (FLOAT64) x_0);
  z = ( (( (FLOAT64) val - (FLOAT64) x_0) * dVal) + y_0);

  return (z);
}


/******************************************************************************
 * Function:    Creep_RestoreAppData
 *
 * Description: Restores the Creep App Data from EEPROM
 *
 * Parameters:  None
 *
 * Returns:     TRUE if data retrieved successfully
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN Creep_RestoreAppData( void )
{
  RESULT result;
  BOOLEAN bOk;

  bOk = TRUE;

  result = NV_Open(NV_CREEP_DATA);
  if ( result == SYS_OK )
  {
    NV_Read(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));
  }

  // If open failed or eeprom reset to 0xFFFF, re-init app data
  if ( ( result != SYS_OK ) || (m_Creep_AppData.crc16 == 0xFFFF) )
  {
    Creep_FileInit();
    bOk = FALSE;  // Always return fail if we have to re-init File
  }

  return (bOk);

}


/******************************************************************************
 * Function:    Creep_RestoreFaultHistory
 *
 * Description: Restores the Creep Fault History Buffer in EE RAM
 *
 * Parameters:  None
 *
 * Returns:     TRUE if data retrieved successfully
 *
 * Notes:       None
 *
 *****************************************************************************/
static
void Creep_RestoreFaultHistory( void )
{
  RESULT result;


  result = NV_Open(NV_CREEP_HISTORY);
  if ( result == SYS_OK )
  {
    NV_Read(NV_CREEP_HISTORY, 0, (void *) &m_creepHistory, sizeof(CREEP_FAULT_HISTORY));
  }

  // If open failed, re-init app data
  if (result != SYS_OK)
  {
    Creep_FaultFileInit();
  }

  m_Creep_Debug.nCreepFailBuffCnt = m_creepHistory.cnt;

}

/******************************************************************************
 * Function:    Creep_RestoreAppDataRTC
 *
 * Description: Restores the Creep App Data from RTC
 *
 * Parameters:  None
 *
 * Returns:     TRUE - Restore of App Data RTC successful
 *
 * Notes:       None
 *
 *****************************************************************************/
static
BOOLEAN Creep_RestoreAppDataRTC( void )
{
  RESULT result;
  BOOLEAN bOk;

  bOk = TRUE;

  result = NV_Open(NV_CREEP_CNTS_RTC);
  if ( result == SYS_OK )
  {
    NV_Read(NV_CREEP_CNTS_RTC, 0, (void *) &m_Creep_AppDataRTC, sizeof(CREEP_APP_DATA_RTC));
  }
  else  // If open failed, just clear data
  {
    // Clear data. Note m_Creep_AppDataRTC should be cleared at this point
    NV_Write(NV_CREEP_CNTS_RTC, 0, (void *) &m_Creep_AppDataRTC, sizeof(CREEP_APP_DATA_RTC));
    bOk = FALSE;
  }

  return (bOk);
}


/******************************************************************************
 * Function:     Creep_FileInit
 *
 * Description:  Reset the file to the initial state.
 *
 * Parameters:   none
 *
 * Returns:      BOOLEAN TRUE  The file reset was successful.
 *                       FALSE The file reset failed.
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN Creep_FileInit(void)
{

  // Init App data
  memset ( (void *) &m_Creep_AppData, 0, sizeof(CREEP_APP_DATA) );

  // Update App data
  NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));

  return TRUE;
}


/******************************************************************************
 * Function:     Creep_FaultFileInit
 *
 * Description:  Reset the file to the initial state.
 *
 * Parameters:   none
 *
 * Returns:      BOOLEAN TRUE  The file reset was successful.
 *                       FALSE The file reset failed.
 *
 * Notes:        none
 *
 *****************************************************************************/
BOOLEAN Creep_FaultFileInit(void)
{

  // Init App data
  memset ( (void *) &m_creepHistory, 0, sizeof(m_creepHistory) );

  // Update App data
  NV_Write(NV_CREEP_HISTORY, 0, (void *) &m_creepHistory, sizeof(m_creepHistory));

  return TRUE;
}


/******************************************************************************
 * Function:     Creep_UpdateCreepXAppData
 *
 * Description:  Updates a specific Creep Obj App Data.
 *
 * Parameters:   index_obj Index of Creep Obj to update
 *
 * Returns:      none
 *
 * Notes:        Support CreepUserTable.c to update App Data
 *
 *****************************************************************************/
//static
void Creep_UpdateCreepXAppData (UINT16 index_obj)
{
  // Copy from m_Creep_Status[] into m_Creep_AppData[creep_index]
  memcpy ( (void *) &m_Creep_AppData.data[index_obj], (void *) &m_Creep_Status[index_obj].data,
           sizeof(CREEP_DATA) );

  // Update NV EEPROM
  NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));
}


/******************************************************************************
 * Function:     Creep_FailAllObjects
 *
 * Description:  Fails all other creep Object per SRS-4601
 *
 * Parameters:   Index - index_obj of Obj which detected fault
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_FailAllObjects ( UINT16 index_obj )
{
  UINT16 i;
  CREEP_STATUS_PTR pStatus;
  CREEP_DATA_PTR pCreepData;


  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    pStatus = (CREEP_STATUS_PTR) &m_Creep_Status[i];
    pCreepData = (CREEP_DATA_PTR) &pStatus->data;

    pStatus->state = CREEP_STATE_FAULT_ER;
    pCreepData->creepFailures++;

    // Copy all update data cnt values (incl creepFailures) back to EE APP Obj
    m_Creep_AppData.data[i] = *pCreepData;
  }

  // Create Creep Failure Log here
  Creep_CreateFaultLog(CREEP_FAULT_TYPE_SENSOR_LOSS);

  NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));

  snprintf (gse_OutLine, sizeof(gse_OutLine), 
            "Creep Task: Creep Fault (Obj=%d) ! \r\n", index_obj);
  GSE_DebugStr(NORMAL,TRUE,gse_OutLine);

}


/******************************************************************************
 * Function:     Creep_CreateSummaryLog
 *
 * Description:  Creates the Creep Summary Log when
 *
 * Parameters:   UINT16 index_obj of Creep Obj
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_CreateSummaryLog ( UINT16 index_obj )
{
  CREEP_SUMMARY_LOG alog;


  alog.index = index_obj;

  alog.accumCntPcnt = m_Creep_Status[index_obj].data_percent.accumCnt;
  alog.accumCnt = m_Creep_Status[index_obj].data.creepAccumCnt;

  alog.lastMissionCntPcnt = m_Creep_Status[index_obj].data_percent.lastMissionCnt;
  alog.lastMissionCnt = m_Creep_Status[index_obj].data.creepLastMissionCnt;

  alog.diag.rowInRate = m_Creep_Sensors[index_obj].row.rateTime;
  alog.diag.colInRate = m_Creep_Sensors[index_obj].col.rateTime;

#pragma ghs nowarning 1545  // Ignore alignment in log.
  memcpy ( (void *) &alog.diag.data, (void *) &m_Creep_Status[index_obj].data,
           sizeof(CREEP_DATA) );
#pragma ghs endnowarning // Ignore alignment in log.

  LogWriteETM (APP_ID_CREEP_SUMMARY, LOG_PRIORITY_LOW, &alog, sizeof(CREEP_SUMMARY_LOG),
               NULL);
}


/******************************************************************************
 * Function:     Creep_CreateFaultLog
 *
 * Description:  Creates the Creep Fault Log
 *
 * Parameters:   type of fault (CREEP_FAULT_TYPE_RESET, CREEP_FAULT_TYPE_SENSOR_LOSS)
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_CreateFaultLog ( CREEP_FAULT_TYPE type )
{
  CREEP_FAULT_OBJ_PTR obj_ptr;
  CREEP_FAULT_LOG_SUMMARY alog;
  UINT16 i;


  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    obj_ptr = (CREEP_FAULT_OBJ_PTR) &alog.obj[i];

    obj_ptr->index = i;
    obj_ptr->type = type;

    obj_ptr->missionCnt = m_Creep_Status[i].data.creepMissionCnt;
    obj_ptr->creepAccumCntTrashed = m_Creep_Status[i].data.creepAccumCntTrashed;
    obj_ptr->creepFailures = m_Creep_Status[i].data.creepFailures;

    obj_ptr->bRowSensorFailed = m_Creep_Sensors[i].row.bFailed;
    obj_ptr->bColSensorFailed = m_Creep_Sensors[i].col.bFailed;

    obj_ptr->accumCntPcnt = m_Creep_Status[i].data_percent.accumCnt;
    obj_ptr->accumCnt = m_Creep_Status[i].data.creepAccumCnt;

    obj_ptr->lastMissionCntPcnt = m_Creep_Status[i].data_percent.lastMissionCnt;
    obj_ptr->lastMissionCnt = m_Creep_Status[i].data.creepLastMissionCnt;
  }
  LogWriteETM (creep_CBIT_TypeToSysLog[type].id, LOG_PRIORITY_LOW, &alog,
               sizeof(CREEP_FAULT_LOG_SUMMARY), NULL);

  // Add to Creep Fault History buffer in EE Memory
  Creep_AddToFaultBuff( creep_CBIT_TypeToSysLog[type].id );

  // Set CBIT Sys Cond
  if (m_Creep_Cfg.sysCondCBIT != STA_NORMAL)
  {
    Flt_SetStatus(m_Creep_Cfg.sysCondCBIT, creep_CBIT_TypeToSysLog[type].id, NULL, 0);
  }

}


/******************************************************************************
 * Function:     Creep_CreateSensorFailLog
 *
 * Description:  Creates the Creep Sensor Fail Log
 *
 * Parameters:   UINT16 index_obj of Creep Obj
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_CreateSensorFailLog ( UINT16 index_obj )
{
  CREEP_SENSOR_FAIL_LOG alog;

  m_Creep_Debug.nSensorFailedCnt++;

  alog.index = index_obj;

  alog.bRowSensorFailed = m_Creep_Sensors[index_obj].row.bFailed;
  alog.bColSensorFailed = m_Creep_Sensors[index_obj].col.bFailed;

  LogWriteETM (APP_ID_CREEP_SENSOR_FAILURE, LOG_PRIORITY_LOW, &alog,
               sizeof(CREEP_SENSOR_FAIL_LOG), NULL);
}


/******************************************************************************
 * Function:     Creep_CreatePBITLog
 *
 * Description:  Creates the Creep PBIT Failed Log
 *
 * Parameters:   PBIT_FAIL_TYPE type of PBIT failure
 *               UINT16         expCRC  this is CRC in cfg
 *               UINT16         calcCRC for _CFG_CRC this is calc cfg crc
 *                              calcCRC for _CRC_MISSMATCH this is EE APP crc
 *
 * Returns:      none
 *
 * Notes:        m_Creep_Cfg must be initialized before call to this function
 *
 *****************************************************************************/
static
void Creep_CreatePBITLog ( PBIT_FAIL_TYPE type, UINT16 expCRC, UINT16 calcCRC )
{
  CREEP_PBIT_LOG alog;
  CREEP_SUMMARY_LOG_PTR pdata;
  CREEP_DIAG_PTR pdiag;
  UINT16 i;


  alog.type = type;
  alog.expCRC = expCRC;
  alog.calcCRC = calcCRC;

  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    pdata = (CREEP_SUMMARY_LOG_PTR) &alog.data[i];
    pdata->index = i;
    pdata->accumCntPcnt = m_Creep_Status[i].data_percent.accumCnt;
    pdata->accumCnt = m_Creep_Status[i].data.creepAccumCnt;

    pdata->lastMissionCntPcnt = m_Creep_Status[i].data_percent.lastMissionCnt;
    pdata->lastMissionCnt = m_Creep_Status[i].data.creepLastMissionCnt;

    pdiag = (CREEP_DIAG_PTR) &pdata->diag;

    pdiag->rowInRate = 0;
    pdiag->colInRate = 0;

#pragma ghs nowarning 1545  // Ignore alignment in Log.
    memcpy ( (void *) &pdiag->data, (void *) &m_Creep_Status[i].data,
             sizeof(CREEP_DATA) );
#pragma ghs endnowarning // Ignore alignment in Log.
  }

  LogWriteETM (creep_PBIT_TypeToSysLog[type].id, LOG_PRIORITY_LOW, &alog,
               sizeof(CREEP_PBIT_LOG), NULL);

  // Add to Creep Fault History buffer in EE Memory
  Creep_AddToFaultBuff( creep_PBIT_TypeToSysLog[type].id );

  // Set PBIT Sys Cond
  if (m_Creep_Cfg.sysCondPBIT != STA_NORMAL)
  {
    Flt_SetStatus(m_Creep_Cfg.sysCondPBIT, creep_PBIT_TypeToSysLog[type].id, NULL, 0);
  }

}


/******************************************************************************
 * Function:     Creep_RestoreCfg
 *
 * Description:  Restore Creep Cfg.
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_RestoreCfg ( void )
{
  // Restore Creep Cfg
  memcpy(&m_Creep_Cfg, &(CfgMgr_RuntimeConfigPtr()->CreepConfig), sizeof(m_Creep_Cfg));
}


/******************************************************************************
 * Function:     Creep_CheckCfgAndData
 *
 * Description:  Checks Creep Cfg and App Data.  Check to ensure data is
 *               correct and consistent.
 *
 * Parameters:   none
 *
 * Returns:      TRUE - If restore and check is OK.
 *
 * Notes:        none
 *
 *****************************************************************************/
static
BOOLEAN Creep_CheckCfgAndData ( void )
{
  BOOLEAN bOk;
  BOOLEAN bOkCfgCrc;
  UINT16 calcCRC;
  BOOLEAN bUpdateNV;
  UINT16 i;

  PBIT_FAIL_TYPE pbit_status;


  // Restore Creep Cfg
  // memcpy(&m_Creep_Cfg, &(CfgMgr_RuntimeConfigPtr()->CreepConfig), sizeof(m_Creep_Cfg));

  // Note: If def cfg restored due to corrupted cfg, then enb == FALSE, but we should still
  //   calc crc as it will be the default and should be OK when comparison is made.
  calcCRC = CRC16 ( &m_Creep_Cfg, sizeof(m_Creep_Cfg) - sizeof(m_Creep_Cfg.crc16) );
  m_Creep_Debug.exp_crc = calcCRC;
  pbit_status = PBIT_OK;

  // Determine if creep cfg crc16 is Ok.  Don't qualify with enable == TRUE.
  //   If default restored (due to 'bad' overall cfg), crc16 should match.
  //   If overall cfg is good, but creep cfg is bad (crc16 calc fails), indicate so.
  bOkCfgCrc = TRUE;
  if (calcCRC != m_Creep_Cfg.crc16)
  {
    bOkCfgCrc = FALSE;

    // Record Sys log to indicate bad crc16
    pbit_status = PBIT_FAIL_CFG_CRC;

    // Do not clear creep cfg.  Leave it and on every power up we will get into this path,
    //   until user takes action to fix.
    //   For corrupted overall cfg, we should not enter this path, as default reloaded
    //   and crc OK.
  }

  // Always Restore App Data
  if ( Creep_RestoreAppData() == TRUE )
  {
    // Always compare EE APP DATA crc16 with Cfg crc16, as long as cfg crc is Ok.
    //   If def cfg was retrieved, and prev had a diff creep crc16 in EE APP DATA,
    //   then we will record log and indicate mismatch.   User should then
    //   update creep cnt.  NOTE: if def cfg was created thru "fast.reset=really" then
    //   EE APP DATA crc16 and cfg crc16 will be forced to ==.
    if (bOkCfgCrc == TRUE)
    {
      // Compare Cfg crc16 and EE APP Data crc16 to determine if cfg has changed.
      if (m_Creep_Cfg.crc16 != m_Creep_AppData.crc16)
      {
        // We've had change in creep cfg (could also be overall cfg restored to
        //    default and prev creep processing was enable/active with non default crc16
        //    in EE APP DATA).
        // Record Sys Log crc16 mismatch
        pbit_status = PBIT_FAIL_CRC_MISMATCH;
        calcCRC = m_Creep_AppData.crc16;  // Used to create PBIT log below

        // Clear EE APP Data and update its crc16 to cfg crc16
        memset ( (void *) &m_Creep_AppData, 0, sizeof(m_Creep_AppData) );
        // m_Creep_AppData.bInUse = TRUE;
        m_Creep_AppData.crc16 = m_Creep_Cfg.crc16;
        // Update App data in EEPROM
        NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(m_Creep_AppData));
      }
      // else, we are GOOD, No action required.
    }
    else
    {
      // else cfg crc16 is bad, so don't make comparison with EE APP Data.  Creep processing
      //   will be disabled.
      m_Creep_AppData.crc16 = 0x0000;  // Clear the crc, so when cfg get's reloaded mismatch
                                       //  will be declared and counts updated.
      // Update App data in EEPROM
      NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(m_Creep_AppData));
    }
  }
  else
  {
    // Record Sys log to indicate EE APP Data retrieval failed, only if we don't have
    //   a prev failed pbit.
    if (pbit_status == PBIT_OK)
    {
      pbit_status = PBIT_FAIL_EE_APP_DATA;
    }

    // Clear EE APP Data.  Update crc16 to cfg crc16, if cfg is good, else use
    //   default crc16 (in this case we have double fault where EE APP DATA was bad and
    //   cfg crc16 was also bad).  Thus when, real cfg reloaded to fix creep cfg,
    //   another log will be recorded above.  Good.
    // Clear EE APP Data and update its crc16 to cfg crc16
    memset ( (void *) &m_Creep_AppData, 0, sizeof(m_Creep_AppData) );
    // m_Creep_AppData.bInUse = TRUE;

    // Update EE APP Data crc16 to default crc16
    m_Creep_AppData.crc16 = CREEP_DEFAULT_CRC16;
    // If cfg crc16 is good then update EE APP Data to it
    if ( bOkCfgCrc == TRUE )
    {
      m_Creep_AppData.crc16 = m_Creep_Cfg.crc16;
    }

    // Update App data in EEPROM
    NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(CREEP_APP_DATA));
  }


  // Determine if we were interrupt by previous power cycle, this check is independent
  //   of checking the restore of EE APP DATA.
  bUpdateNV = FALSE;
  if ( Creep_RestoreAppDataRTC() == TRUE )
  {
    for (i=0;i<CREEP_MAX_OBJ;i++)
    {
      if ( m_Creep_AppDataRTC.creepMissionCnt[i] != 0 )
      {
        m_Creep_AppData.data[i].creepAccumCntTrashed += m_Creep_AppDataRTC.creepMissionCnt[i];
        m_Creep_AppData.data[i].creepFailures += 1;
        m_Creep_AppDataRTC.creepMissionCnt[i] = 0;  // Clear data
        // Sys Log to indicate interrupted counting
        m_Creep_Status[i].bCntInterrupted = TRUE;
        bUpdateNV = TRUE;
      }
    } // Loop thru CREEP_MAX_OBJ

    // Update NV_Write() with interrupted counts and cleared App Data RTC
    if (bUpdateNV == TRUE)
    {
      NV_Write(NV_CREEP_DATA, 0, (void *) &m_Creep_AppData, sizeof(m_Creep_AppData));
      NV_Write(NV_CREEP_CNTS_RTC, 0, (void *) &m_Creep_AppDataRTC,
               sizeof(m_Creep_AppDataRTC) );
    }
  } // End if restore _AppDataRTC() was successful
  // else retrieve of RTC APP DATA failed.
  //   This could or could not be an issue.  If creep processing was interrupted,
  //   then this will be a problem.  This is similar to sensor failure.
  //   Do nothing for now.  TBD


  /*
  // Update Creep Status Object with what was in EE APP DATA
  m_Creep_100_pcnt = 1;
  for (i=0;i<(m_Creep_Cfg.baseUnits-2);i++)
  {
    // Could use powerof() math func but will require coverage. This is easier.
    //  "-2" to produce percent (or * 100)
    m_Creep_100_pcnt *= 10;
  }

  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    m_Creep_Status[i].data = m_Creep_AppData.data[i];
    // Need to calculate the percent fields
    m_Creep_Status[i].data_percent.accumCnt =
                m_Creep_Status[i].data.creepAccumCnt / m_Creep_100_pcnt;
    m_Creep_Status[i].data_percent.accumCntTrashed =
                m_Creep_Status[i].data.creepAccumCntTrashed / m_Creep_100_pcnt;
    m_Creep_Status[i].data_percent.lastMissionCnt =
                m_Creep_Status[i].data.creepLastMissionCnt / m_Creep_100_pcnt;

    m_Creep_Status[i].interval.nRateCounts = MIFs_PER_SECOND /
                                             (INT16) m_Creep_Cfg.creepObj[i].intervalRate;
  }
  */
  Creep_InitPersistentPcnt(); 
  

  // If PBIT failed, record log here
  if ( pbit_status != PBIT_OK)
  {
    // Create Creep PBIT Log
    Creep_CreatePBITLog ( pbit_status, m_Creep_Cfg.crc16, calcCRC );
  }

  // If RTC APP DATA indicates interrupted counts then record Creep Fault
  //   Note: Need to put logic here as Creep_CreateFaultLog() uses m_Creep_Status data
  //   which needs to be updated (above) first.
  for (i=0;i<CREEP_MAX_OBJ;i++)
  {
    if ( (m_Creep_Status[i].bCntInterrupted == TRUE) )
    {
      // Record Creep Failure Log
      Creep_CreateFaultLog (CREEP_FAULT_TYPE_RESET);
      // Record log just once as per SRS-4601, when a fault is detected,
      //  both Ct and Pt values shall be rejected.  Thus this single log
      //  will force operator to adjust creep count for both Ct and Pt.
      //  The single creep fault log will flag both Ct and Pt creep count failure.
      break;
    }
  } // End loop thru all creep obj

  bOk = FALSE;
  if (pbit_status != PBIT_FAIL_CFG_CRC)
  {
    bOk = TRUE; // Note: JV will like this code for coverage.  Need to do more.
  }

  return (bOk);

}


/******************************************************************************
 * Function:     Creep_InitPersistentPcnt
 *
 * Description:  Initializes the Creep Persistent Percentage display values
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:        Creep Configuration must have been restored before calling 
 *                  calling this func.
 *
 *****************************************************************************/
static
void Creep_InitPersistentPcnt ( void )
{
  UINT16 i; 
	
  // Update Creep Status Object with what was in EE APP DATA
	m_Creep_100_pcnt = 1;
	for (i=0;i<(m_Creep_Cfg.baseUnits-2);i++)
	{
		// Could use powerof() math func but will require coverage. This is easier.
		//  "-2" to produce percent (or * 100)
		m_Creep_100_pcnt *= 10;
	}
	
	for (i=0;i<CREEP_MAX_OBJ;i++)
	{
		m_Creep_Status[i].data = m_Creep_AppData.data[i];
		// Need to calculate the percent fields
		m_Creep_Status[i].data_percent.accumCnt =
			m_Creep_Status[i].data.creepAccumCnt / m_Creep_100_pcnt;
		m_Creep_Status[i].data_percent.accumCntTrashed =
			m_Creep_Status[i].data.creepAccumCntTrashed / m_Creep_100_pcnt;
		m_Creep_Status[i].data_percent.lastMissionCnt =
			m_Creep_Status[i].data.creepLastMissionCnt / m_Creep_100_pcnt;
	
		m_Creep_Status[i].interval.nRateCounts = MIFs_PER_SECOND /
				(INT16) m_Creep_Cfg.creepObj[i].intervalRate;
	}
	
}


/******************************************************************************
 * Function:     Creep_AddToFaultBuff
 *
 * Description:  Add Creep Fault to History Buffer in EEPROM
 *
 * Parameters:   id - SYS_APP_ID
 *
 * Returns:      TRUE - If restore and check is OK.
 *
 * Notes:        none
 *
 *****************************************************************************/
static
void Creep_AddToFaultBuff ( SYS_APP_ID id )
{
  CREEP_FAULT_BUFFER_PTR pbuff;


  if (m_creepHistory.cnt < CREEP_HISTORY_MAX)
  {
    pbuff = (CREEP_FAULT_BUFFER_PTR) &m_creepHistory.buff[m_creepHistory.cnt];
    pbuff->id = id;
    CM_GetTimeAsTimestamp(&pbuff->ts);
  }
  m_creepHistory.cnt++;
  NV_Write( NV_CREEP_HISTORY, 0, (void *) &m_creepHistory.cnt, sizeof(CREEP_FAULT_HISTORY) );
  m_Creep_Debug.nCreepFailBuffCnt = m_creepHistory.cnt;
}







// Debug Functions
#ifdef CREEP_TEST
static
ER_STATE EngRunGetState_Sim (ENGRUN_INDEX idx, UINT8* EngRunFlags)
{
  volatile ER_STATE er_state;

  er_state = m_er_state;

  return (er_state);
}

UINT32 SensorGetLastUpdateTime_Sim ( SENSOR_INDEX id )
{
  static UINT32 tickTime[SIM_SENSORS];


  // % 150 is every 150 msec or 6.667 Hz
  if ( (CM_GetTickCount() % m_sensor_rate_sim[id]) == 0)
  {
    tickTime[id] = CM_GetTickCount();
  }

  return (tickTime[id]);
}

FLOAT32 SensorGetValue_Sim( SENSOR_INDEX id)
{
  sim_sensor_cnt[id] = ++sim_sensor_cnt[id] % SIM_SENSOR_MAX_VALUES;
  return (SIM_SENSOR[id][sim_sensor_cnt[id]]);
}
#endif


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Creep.c $
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 12-11-10   Time: 4:37p
 * Updated in $/software/control processor/code/application
 * Code Review Updates
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 12-11-02   Time: 6:14p
 * Updated in $/software/control processor/code/application
 * SCR #1195 V&V findings items #1 and #2
 * SCR #1190 Additional Items to complete
 * Code Review Tool Updates
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:09p
 * Created in $/software/control processor/code/application
 * SCR #1190 Creep Requirements
 *
 *
 *****************************************************************************/
