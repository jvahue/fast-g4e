#define CYCLE_BODY
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:         Cycle.c

    Description:
        Cycle runtime processing

        An Cycle is an indication that some measured input has exceeded
          a specified value. As implemented, the Cycle is based on one
          sensor, and is "activated" when that sensor exceeds a specified
          comparison value (see compare.c). When activated, the
          Cycle will increment a counter in the flight log, and then wait
          until the sensor no longer exceeds the specified value.

        For example, an Cycle may become active when airspeed exceeds
          a specified value. At that point, the counter in the flight log
          is incremented. Later, the airspeed goes below the specified
          value, and the cycle resets looking for another cycle crossing.


  VERSION
  $Revision: 20 $  $Date: 12-10-02 1:23p $

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
#include "cycle.h"
#include "EngineRun.h"
#include "assert.h"
#include "nvmgr.h"
#include "CfgManager.h"
#include "GSE.h"
#include "user.h"
#include "systemlog.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

// macros for calculating the size and offset into the RTC_RAM_PERSISTCYCLE structure.
#define N_OFFSET(n)  (UINT8) ((n)*(sizeof(UINT32) + sizeof(UINT16)))

#define INITPCYCLES_BOTH_LOCATION_BAD 0x01
#define INITPCYCLES_FLASHBAD          0x04
#define INITPCYCLES_RTCBAD            0x02
#define INITPCYCLES_RTCGREATER        0x08
#define INITPCYCLES_FLASHGREATER      0x20
#define INITPCYCLES_CHECKIDFAIL       0x10

#define CYC_COUNT_UPDATE_NOW        TRUE
#define CYC_COUNT_UPDATE_BACKGROUND FALSE

//#define DEBUG_CYCLE

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

typedef enum
{
  A_VALUE = 1,
  B_VALUE
} CYCLEVALUETYPE;

typedef enum
{
  CYC_BKUP_RTC,   // Persist the RTC counts to RTCRAM
  CYC_BKUP_EE     // Sync the EE baseline counts to match RTC and persist to EEPROM
}CYC_BKUP_TYPE;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static CYCLE_CFG    m_Cfg [MAX_CYCLES];  // Cycle Cfg Array
static CYCLE_DATA   m_Data[MAX_CYCLES];  // Runtime data related to the cycles

static CYCLE_COUNTS m_CountsEEProm;  // master count list, Persisted to EE at end of ER
                                     // otherwise is used as the compare base for
                                     // counts during this ER


static CYCLE_COUNTS m_CountsRTC; // buffer for counts written to RTC during the ER
                                   // during init acts as the working buffer to cross-check
                                   // RTC/EEPROM.

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    CycleReset ( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData );
static void    CycleUpdate( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData, UINT16 cycIndex );

static void    CycleUpdateSimpleAndDuration (CYCLE_CFG*  pCycleCfg,
                                             CYCLE_DATA* pCycleData,
                                             UINT16      cycIndex );

static void    CycleInitPersistent(void);

static void    CycleBackupPersistentCounts( UINT16 nCycle, CYC_BKUP_TYPE mode);
static void    CycleFinish( UINT16 nCycle);
static BOOLEAN CycleRestoreCountsFromPersistFiles(void);
static BOOLEAN CycleUpdateCheckId( UINT16 nCycle, BOOLEAN bLogUpdate);
static UINT16  CycleCalcCheckID ( UINT16 nCycle );
static void    CycleSyncPersistFiles(BOOLEAN bNow);

// Include cmd tables and functions here after local dependencies are declared.
#include "CycleUserTables.c"


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     CycleUpdateAll
 *
 * Description:  Updates all cycles for EngineRun 'erIndex'
 *
 * Parameters:   The index into the CYCLE_CFG and CYCLE_DATA object arrays.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
EXPORT void CycleInitialize(void)
{
  // Add user commands for Cycles to the user command tables.
  User_AddRootCmd(&RootCycleMsg);

  // Load the current cfg info.
  memcpy(&m_Cfg,
         &(CfgMgr_RuntimeConfigPtr()->CycleConfigs),
        sizeof(m_Cfg));

  // Reload the persistent cycle count  info from EEPROM & RTCNVRAM
  CycleInitPersistent();

  // Initialize Engine Runs storage.
  CycleResetAll();

}
/******************************************************************************
 * Function:     CycleUpdateAll
 *
 * Description:  Updates all cycles for EngineRun 'erIndex'
 *
 * Parameters:   The index into the CYCLE_CFG and CYCLE_DATA object arrays.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleUpdateAll(ENGRUN_INDEX erIndex)
{
  UINT16 cycIndex;

  // Update all active cycles associated with this enginerun
  for (cycIndex = 0; cycIndex < MAX_CYCLES; cycIndex++)
  {
    if ( m_Cfg[cycIndex].type != CYC_TYPE_NONE_CNT &&
         m_Cfg[cycIndex].nEngineRunId == erIndex )
    {
      CycleUpdate(&m_Cfg[cycIndex], &m_Data[cycIndex], cycIndex);
    }
  }
}

/******************************************************************************
 * Function:     CycleResetAll
 *
 * Description:  Reset all cycle not configured as type "NONE'
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:        None
 *
 *****************************************************************************/
void CycleResetAll(void)
{
  UINT16 i;

  for (i = 0; i < MAX_CYCLES; i++)
  {
    if ( m_Cfg[i].type != CYC_TYPE_NONE_CNT)
    {
     CycleReset( &m_Cfg[i], &m_Data[i] );
    }
  }
}

/******************************************************************************
 * Function:     CycleClearAll
 *
 * Description:  Sets Type->NONE_COUNT, trigger -> TRIGGER_UNUSED
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleClearAll( void )
{
  UINT8 i;
  CYCLE_CFG_PTR pCycleCfg = m_Cfg;

  for (i = 0; i < MAX_CYCLES; i++)
  {
    // Set cycle type to none
    pCycleCfg->type = CYC_TYPE_NONE_CNT;

    // Set cycle's trigger index to unused.
    pCycleCfg->nTriggerId = TRIGGER_UNUSED;

    // Move next cycle config
    pCycleCfg++;
  }
}


/******************************************************************************
 * Function:     CycleIsPersistentType
 *
 * Description:  checks cycle type for persistent.
 *
 * Parameters:   [in] cycle table index.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
BOOLEAN CycleIsPersistentType( UINT8 nCycle )
{
  return (
          (m_Cfg[nCycle].type     == CYC_TYPE_PERSIST_DURATION_CNT )
           || (m_Cfg[nCycle].type == CYC_TYPE_PERSIST_SIMPLE_CNT)
#ifdef PEAK_CUM_PROCESSING
           || (m_Cfg[nCycle].type == CYC_TYPE_PEAK_VAL_CNT)
           || (m_Cfg[nCycle].type == CYC_TYPE_CUM_CNT)
#endif
       );
}

/******************************************************************************
 * Function:     CycleGetBinaryHeader
 *
 * Description:  Retrieves the binary header for the Cycle
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
UINT16 CycleGetBinaryHeader ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   CYCLE_HDR cycleHdr[MAX_CYCLES];
   UINT16    cycleIndex;
   INT8      *pBuffer;
   UINT16    nRemaining;
   UINT16    nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;
   memset ( &cycleHdr, 0, sizeof(cycleHdr) );

   // Loop through all the cycles
   for ( cycleIndex = 0;
         ((cycleIndex < MAX_CYCLES) && (nRemaining > sizeof (cycleHdr[cycleIndex])));
         cycleIndex++ )
   {
      // Copy the Cycle names
      strncpy_safe( cycleHdr[cycleIndex].Name,
                    sizeof(cycleHdr[cycleIndex].Name),
                    m_Cfg[cycleIndex].name,
                    _TRUNCATE);

      cycleHdr[cycleIndex].Type         = m_Cfg[cycleIndex].type;
      cycleHdr[cycleIndex].nCount       = m_Cfg[cycleIndex].nCount;
      cycleHdr[cycleIndex].nTriggerId   = m_Cfg[cycleIndex].nTriggerId;
      cycleHdr[cycleIndex].nEngineRunId = m_Cfg[cycleIndex].nEngineRunId;

      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (cycleHdr[cycleIndex]);
      nRemaining -= sizeof (cycleHdr[cycleIndex]);
   }
   // Copy the Cycle header to the buffer
   memcpy ( pBuffer, &cycleHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}


/******************************************************************************
 * Function:     CycleGetPersistentCount
 *
 * Description:  Retrieves the count for cycle 'nCycle'
 *               
 *
 * Parameters:   UINT8 nCycle         - Cycle Index of the value to return
 *              
 *
 * Returns:      UINT32 - Value of cycle as persisted to RTCRam
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT32  CycleGetPersistentCount( CYCLE_INDEX nCycle )
{  
  return m_CountsRTC.data[nCycle].count.n;
}

/*********************************************************************************************/
/* Local Functions                                                                           */
/*********************************************************************************************/

/******************************************************************************
 * Function:     CycleInitPersistent
 *
 * Description:  Copies persistent cycle from RTC RAM or EEPROM and
 *               copies to run time memory (ies).
 *
 * Parameters:   [in] bStateCorrupted - Flag indicating whether persistent data
 *                                      is corrupted.
 *
 * Returns:      Boolean indicating update was successful.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleInitPersistent(void)
{

  BOOLEAN bUpdateEEPROM = FALSE;
  BOOLEAN bUpdateRTCRAM = FALSE;
  UINT8   i;

  CYCLE_CFG*     pCycCfg;

  CYCLE_ENTRY*   pCycCntsEE;
  CYCLE_ENTRY*   pCycCntsRTC;


  /* If both sets of persistent data is corrupted reset to 0 */
  if ( !CycleRestoreCountsFromPersistFiles() )
  {
    // RTC and EEPROM copies of persist cycles have valid but different CRC's.
    // Determine which values are valid based on checkID and create a good list.

    for ( i = 0; i < MAX_CYCLES; i++)
    {
      if ( CycleIsPersistentType( i) )
      {
        pCycCfg  = &m_Cfg[i];

        // Set ptrs to areas containing files read from NVRAM
        pCycCntsEE  = &m_CountsEEProm.data[i];
        pCycCntsRTC = &m_CountsRTC.data[i];

        // Compare the counts. Higher count is considered the
        // valid one. Make the lesser side equivalent.

        if ( pCycCntsEE->count.n != pCycCntsRTC->count.n )
        {
          if ( CYC_TYPE_PERSIST_DURATION_CNT == pCycCfg->type )
          {
            if ( pCycCntsEE->count.n > pCycCntsRTC->count.n )
            {
              pCycCntsRTC->count.n = pCycCntsEE->count.n;
              bUpdateRTCRAM = TRUE;
            }
            else
            {
              pCycCntsEE->count.n = pCycCntsRTC->count.n;
              bUpdateEEPROM = TRUE;
            }
          }
          else
          {
            if ( pCycCntsEE->count.f > pCycCntsRTC->count.f )
            {
              pCycCntsRTC->count.f = pCycCntsEE->count.f;
              bUpdateRTCRAM = TRUE;
            }
            else
            {
              pCycCntsEE->count.f = pCycCntsRTC->count.f;
              bUpdateEEPROM = TRUE;
            }
          } /* End of else != P_DURATION_COUNT   */
        } /* End of if ( != )                  */
      } /* End of if cycle->type is a persistent */
    } /* End of for MAX_CYCLES loop             */

  }


  /* Both PCycleData and RTC_RAMRunTime are good, and count == */
  /* compare .checkId  */
  for ( i=0; i < MAX_CYCLES; i++)
  {
    if ( CycleIsPersistentType(i) )
    {
      if ( CycleUpdateCheckId( i, LOGUPDATE_YES) )
      {
        bUpdateEEPROM = TRUE;  /* for simplicity update both */
        bUpdateRTCRAM = TRUE;  /* for simplicity update both */
      }
    }
  }

  if (bUpdateRTCRAM | bUpdateEEPROM)
  {
    CycleSyncPersistFiles(CYC_COUNT_UPDATE_NOW);
  }
}


/******************************************************************************
* Function:     CycleReset
 *
 * Description:   Reset one Cycle object
 *                Called when we transition to RUN mode, we initialize the Cycle
 *                data so that valid checks may subsequently be done
 *
 * Parameters:   [in] nCycle - Index of Cycle to update
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleReset( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData )
{
  if ( pCycleCfg->type       == CYC_TYPE_NONE_CNT ||
       pCycleCfg->nTriggerId >= MAX_TRIGGERS ||
       !TriggerIsConfigured( pCycleCfg->nTriggerId ) )
  {
    pCycleCfg->type         = CYC_TYPE_NONE_CNT;
    pCycleCfg->nEngineRunId = ENGRUN_UNUSED;
  }
  else
  {
    pCycleData->cycleActive      = FALSE;  /* Count not yet active */
    pCycleData->cycleLastTime_ms = 0;      /* Clear start time     */
#ifdef DEBUG_CYCLE
/*vcast_dont_instrument_start*/
    GSE_DebugStr(NORMAL,TRUE,"Cycle: %s CycleReset",pCycleCfg->Name);
/*vcast_dont_instrument_end*/
#endif

#ifdef PEAK_CUM_PROCESSING
    pCycle->ACrossed = FALSE;         /* not crossed threshold from above yet */
    pCycle->BCrossed = FALSE;         /* not crossed threshold from above yet */

    pCycle->MaxValueA = -FLT_MAX;
    pCycle->MaxValueB = -FLT_MAX;
    pCycle->MinValue = FLT_MAX;
    pCycle->MaxSensorValueA = -FLT_MAX;
    pCycle->MaxSensorValueB = -FLT_MAX;
    pCycle->MinSensorValue = FLT_MAX;

    /* check to see if cycle values are correclty ordered for those that need it */

    if ( (pCycle->Type == PEAK_VALUE_COUNT)         ||
      (pCycle->Type == CUMULATIVE_VALLEY_COUNT)  ||
      (pCycle->Type == P_PEAK_COUNT)             ||
      (pCycle->Type == P_CUMULATIVE_COUNT) )
    {
      pCycle->bTableOrdered = CheckCycleValues(&(pCycle->CycleValue[0]));
    }
#endif
#ifdef RHL_PROCESSING
    pCycle->RHLstate = RHL_ENABLED;
    pCycle->RHLstableTicks = 0;
#endif
  }
}


/******************************************************************************
 * Function:     CycleUpdate
 *
 * Description:  Check to see if a specific cycle needs counting.
 *
 * Parameters:   [in] nCycle - Index of Cycle to update
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleUpdate( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData, UINT16 cycIndex )
{
  // Act on the different types of cycles
  switch (pCycleCfg->type)
  {
    case CYC_TYPE_SIMPLE_CNT:
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
    case CYC_TYPE_DURATION_CNT:
    case CYC_TYPE_PERSIST_DURATION_CNT:
      CycleUpdateSimpleAndDuration( pCycleCfg, pCycleData, cycIndex );
      break;

#ifdef PEAK_CUM_PROCESSING
    case PEAK_VALUE_COUNT:
    case P_PEAK_COUNT:
      UpdateCyclePeak ( pCycleCfg );
      break;

    case CUMULATIVE_VALLEY_COUNT:
    case P_CUMULATIVE_COUNT:
      UpdateCycleCummulative ( pCycleCfg );
      break;

    case REPETITIVE_HEAVY_LIFT:
      RHL( pCycleCfg);
      break;
#endif
    case CYC_TYPE_NONE_CNT:
      break;

    default:
      FATAL ("Cycle[%d] %s has Unrecognized Cycle Type: %d",
             cycIndex, pCycleCfg->name,
             pCycleCfg->type);
      pCycleCfg->type = CYC_TYPE_NONE_CNT;
      break;
   }
}


/******************************************************************************
 * Function:     CycleUpdateSimpleAndDuration
 *
 * Description:  Processes simple and duration count cycle object.
 *
 * Parameters:   [in] nCycle the cycle object being processed.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleUpdateSimpleAndDuration ( CYCLE_CFG*  pCycleCfg,
                                           CYCLE_DATA* pCycleData,
                                           UINT16      cycIndex )
{
  BOOLEAN      CycleStart = FALSE;
  BITARRAY128  trigMask;
  UINT32*      pErDataCycles;
  UINT32       nowTimeMs;

  CLR_TRIGGER_FLAGS(trigMask);
  SetBit(pCycleCfg->nTriggerId, trigMask, sizeof(BITARRAY128 ));

  pErDataCycles = EngRunGetPtrToCycleCounts(pCycleCfg->nEngineRunId);

  if ( (pCycleData->cycleActive) )
  {
    // Cycle was already active from a previous pass...

    // If a Duration Count, update duration every time through.
    if ( (pCycleCfg->type == CYC_TYPE_DURATION_CNT) ||
         (pCycleCfg->type == CYC_TYPE_PERSIST_DURATION_CNT) )
    {
      // Increment duration every time through when active.

      // Update the millisec duration in the enginerun count object.
      nowTimeMs = CM_GetTickCount();
      pErDataCycles[cycIndex] += (nowTimeMs - pCycleData->cycleLastTime_ms);
      pCycleData->cycleLastTime_ms = nowTimeMs;

      if (pCycleCfg->type == CYC_TYPE_PERSIST_DURATION_CNT)
      {
        CycleBackupPersistentCounts(cycIndex, CYC_BKUP_RTC);
      }
    }
    // Update the cycle active status.
    pCycleData->cycleActive = ( TriggerIsActive(&trigMask ) )? TRUE : FALSE;

    // If cycle has ended, reset the start-time for the next duration.
    if(!pCycleData->cycleActive)
    {
      pCycleData->cycleLastTime_ms = 0;
#ifdef DEBUG_CYCLE
/*vcast_dont_instrument_start*/
       GSE_DebugStr(NORMAL, TRUE,"Cycle: Cycle[%d] Ended", cycIndex);
/*vcast_dont_instrument_end*/
#endif
    }

  }
  else
  {
    // Previous cycle was inactive - see if the start criteria are met
    // Set up a bit-mask for querying the state of the trigger for this cycle

    CycleStart = ( TriggerIsActive(&trigMask ) )? TRUE : FALSE;

    // Cycle has started
    if (CycleStart)
    {
      // Previous run was "inactive" and trigger is now active
      // value - count the cycle in the flight log and set the flag
      // that indicates that the cycle is active.
      #ifdef DEBUG_CYCLE
       GSE_DebugStr(NORMAL, TRUE,"Cycle: Cycle[%d] Started", cycIndex);
      #endif

      pCycleData->cycleActive    = TRUE;
      pCycleData->cycleLastTime_ms = CM_GetTickCount();

      if (( pCycleCfg->type == CYC_TYPE_SIMPLE_CNT ) ||
          ( pCycleCfg->type == CYC_TYPE_PERSIST_SIMPLE_CNT))
      {
        pErDataCycles[cycIndex] += pCycleCfg->nCount;

        if (pCycleCfg->type == CYC_TYPE_PERSIST_SIMPLE_CNT)
        {
          CycleBackupPersistentCounts(cycIndex, CYC_BKUP_RTC);
        }
      }
    } // End of if cycle has Started
  } //End of else prev cycle was inactive

}


/******************************************************************************
 * Function:     CycleBackupPersistentCounts
 *
 * Description:  Backs up current persistent counts to RTC RAM or FLASH
 *
 * Parameters:   [in] cycle index
 *               [in] bUpdateRtcRam flag to indicate RTC RAM should be updated
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleBackupPersistentCounts( UINT16 nCycle,  CYC_BKUP_TYPE mode )
{
  union
  {
    UINT32  n;
    FLOAT32 f;
  } temp_value;
  BOOLEAN bDataChanged = FALSE;

  CYCLE_CFG_PTR pCycle = &m_Cfg[nCycle];

  // Get ptr to this cycle's enginerun collection area.
  UINT32* pErDataCycles = EngRunGetPtrToCycleCounts(pCycle->nEngineRunId);

  // Update persistent count
  // The copy in EEPROM acts as the baseline since 'start of ER' against which the
  // incrementation are calculated. The updated total is stored in RTC memory.
  // When the engrun ends, this function will be called with updateFlag == CYC_WRITE_EE and
  // the EEPROM memory will be synced to match RTC.

  switch (pCycle->type)
  {
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
      temp_value.n = m_CountsEEProm.data[nCycle].count.n + pErDataCycles[nCycle];
      bDataChanged = TRUE;
      break;

    case CYC_TYPE_PERSIST_DURATION_CNT:
      // Get the current duration (in millisecs) for this cycle-run.
      // add 0.5 seconds for int truncation assuming
      // 50% of values > 0.5 and 50% of values < 0.5 over time
      temp_value.f = 500 + (FLOAT32)pErDataCycles[nCycle];

      // Convert the duration to secs and add to the persisted total
      temp_value.f *= 1.0f /(FLOAT32)MILLISECONDS_PER_SECOND;
      temp_value.n = (UINT32)temp_value.f + m_CountsEEProm.data[nCycle].count.n;

      // If the updated total is more than one second greater than last persisted value,
      // flag the RTC to be updated.
      bDataChanged = (temp_value.n - m_CountsRTC.data[nCycle].count.n) >= 1 ? TRUE : FALSE;
      break;

    default:
      FATAL ("Unrecognized Cycle Type Value: %d", pCycle->type);
      break;
  }



  switch (mode)
  {
    case CYC_BKUP_RTC:
      /* Update RTC_RAMRunTime memory ONLY IF the value is changing. */
      if ( bDataChanged )
      {
        m_CountsRTC.data[nCycle].count.n = temp_value.n;

        #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
        NV_Write( NV_PCYCLE_CNTS_RTC, N_OFFSET(nCycle), &m_CountsRTC.data[nCycle].count.n,
          sizeof(m_CountsRTC.data[nCycle].count.n) );
        #pragma ghs endnowarning

        #ifdef DEBUG_CYCLE
        /*vcast_dont_instrument_start*/
        GSE_DebugStr(NORMAL,TRUE, "Cycle: Cycle[%d] RTC Update Dur-Count: %d | P-Count: %d",
                    nCycle,
                    pErDataCycles[nCycle],
                    m_CountsRTC.data[nCycle].count.n );
        /*vcast_dont_instrument_end*/
        #endif
      }
      break;

    case CYC_BKUP_EE:
      // EngRun has ended, update the EEProm so it will match the
      // the value updated in RTC during this EngRun.

      m_CountsEEProm.data[nCycle].count.n = m_CountsRTC.data[nCycle].count.n;

      #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      NV_Write( NV_PCYCLE_CNTS_EE, N_OFFSET(nCycle), &m_CountsEEProm.data[nCycle].count.n,
                                            sizeof(m_CountsEEProm.data[nCycle].count.n) );
      #pragma ghs endnowarning

      #ifdef DEBUG_CYCLE
      if ( 0 != memcmp(&m_CountsEEProm, &m_CountsRTC, sizeof(CYCLE_COUNTS)) )
      {
        INT16 i;
        for (i = 0; i < MAX_CYCLES; ++i )
        {
          if (m_CountsEEProm.data[nCycle].count.n !=  m_CountsRTC.data[nCycle].count.n &&
              m_CountsEEProm.data[nCycle].checkID !=  m_CountsRTC.data[nCycle].checkID)
          {
            GSE_DebugStr(NORMAL,TRUE,"m_CountsEEProm[%d] = %d, %d  |  m_CountsRTC[%d] = %d, %d",
                          i, m_CountsEEProm.data[i].count.n, m_CountsEEProm.data[i].checkID,
                          i, m_CountsRTC.data[i].count.n,    m_CountsRTC.data[i].checkID);
          }
        }
      }
      #endif
    GSE_DebugStr(NORMAL,TRUE, "Cycle: Cycle[%d] EEPROM Updated P-Count: %d", nCycle,
                 m_CountsEEProm.data[nCycle].count.n);
    break;

    default:
    FATAL("Unrecognized Persist backup mode requested: %d",mode);
  }
}

/******************************************************************************
 * Function:     CyclesFinishEngineRun
 *
 * Description:  Called by EngineRun to finish up any currently running
 *               cycles for the indicated Engine Run
 *
 * Parameters:   [in] id of owning-EngineRun.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleFinishEngineRun( ENGRUN_INDEX erID )
{
  UINT16 i;
  for (i = 0; i < MAX_CYCLES; i++)
  {
    // If the cycle entry is configured, and belongs to the EngineRun,
    // save the values and close it out.

    if ( m_Cfg[i].type != CYC_TYPE_NONE_CNT &&
         m_Cfg[i].nEngineRunId == erID)
    {
      // Persist cycle info to  EEPROM from RTC at end of cycle.
      CycleBackupPersistentCounts( i, CYC_BKUP_EE );
      CycleFinish( i);
    }
  }
}


/******************************************************************************
 * Function:     CycleFinish
 *
 * Description:  Finish a cycle if it is currently active
 *               Cycle data is copied to the EngineRun's cycle counts.
 *
 * Parameters:   [in] cycle table index.
 *
 * Returns:      None.
 *
 * Notes:        This function should be preceded by a call to
 *               CycleBackupPersistentCounts() to ensure m_CountsEEProm has
 *               been properly updated. The updated info in m_CountsEEProm is
 *               used to bring the cycle totals up to current persisted values.
 *
 *****************************************************************************/
static void CycleFinish( UINT16 nCycle )
{
  CYCLE_DATA*    pCycle    = &m_Data[nCycle];
  CYCLE_CFG*     pCycleCfg = &m_Cfg[nCycle];
  ENGRUN_INDEX   erID      = pCycleCfg->nEngineRunId;
  ENGRUN_RUNLOG* pLog      = EngRunGetPtrToLog(erID);
  UINT32* pErDataCycles    = EngRunGetPtrToCycleCounts(erID);



  // Act on the different types of cycles
  switch (pCycleCfg->type)
  {
    case CYC_TYPE_SIMPLE_CNT:
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
      // Mark the cycle as completed.
      pCycle->cycleActive    = FALSE;
      pCycle->cycleLastTime_ms = 0;

      // Update the log entry with the aggregate persisted count from EEPROM/RTCNvRAM
      if ( pCycleCfg->type == CYC_TYPE_PERSIST_SIMPLE_CNT )
      {
        pLog->cycleCounts[nCycle] = m_CountsEEProm.data[nCycle].count.n;
      }
      break;

    case CYC_TYPE_DURATION_CNT:
    case CYC_TYPE_PERSIST_DURATION_CNT:
      pCycle->cycleActive    = FALSE;
      pCycle->cycleLastTime_ms = 0;

      // Update the log entries with the count.
      // Note: counts are in secs.

      if (pCycleCfg->type == CYC_TYPE_PERSIST_DURATION_CNT)
      {
        pLog->cycleCounts[nCycle] = m_CountsEEProm.data[nCycle].count.n;
      }
      else
      {
        /* convert ticks to seconds correcting for CLK time error */
        pLog->cycleCounts[nCycle] *= 1.0f /(FLOAT32)MILLISECONDS_PER_SECOND;
      }

      break;
#ifdef PEAK_CUM_PROCESSING
    case PEAK_VALUE_COUNT:
    case P_PEAK_COUNT:
      // put the max value (if there is one) into the EngineRunLog (if it is still active)
      if (EngineRunData[erID].pLog != NULL) {

        if ( pCycle->Type == P_PEAK_COUNT ) {
          EngineRunData[erID].pLog->CycleCounts[nCycle] = m_CountsEEProm[nCycle].count.f;
        }
        else {
          if ( (pCycle->MaxValueA > -FLT_MAX) || (pCycle->MaxValueB > -FLT_MAX) ) {
            EngineRunData[erID].pLog->CycleCounts[nCycle] =
                (pCycle->MaxValueA > pCycle->MaxValueB ?
                 pCycle->MaxValueA : pCycle->MaxValueB);
          }
          else {
            EngineRunData[erID].pLog->CycleCounts[nCycle] = 0.0f;
          }
        }
      }
      pCycle->ActiveA = FALSE;
      pCycle->ActiveB = FALSE;
      pCycle->ACrossed = FALSE;
      pCycle->BCrossed = FALSE;
      pCycle->MaxValueA = -FLT_MAX;
      pCycle->MaxValueB = -FLT_MAX;
      break;

    case CUMULATIVE_VALLEY_COUNT:
    case P_CUMULATIVE_COUNT:
      // no value is added to the engine run log since it is only added if the sensor
      // threshold value is crossed from below, which would have already happened.
      pCycle->ActiveA = FALSE;
      pCycle->ACrossed = FALSE;
      pCycle->MinValue = FLT_MAX;
      pCycle->MinSensorValue = FLT_MAX;

      if (EngineRunData[erID].pLog != NULL) {
        if ( pCycle->Type == P_CUMULATIVE_COUNT ) {
          EngineRunData[erID].pLog->CycleCounts[nCycle] =
              m_CountsEEProm[nCycle].count.f;
        }
      }
      break;
#endif
#ifdef RHL_PROCESSING
    case REPETITIVE_HEAVY_LIFT:
    case NONE_COUNT:
      break;
#endif

    default:
      FATAL ("Unrecognized Cycle Type Value: %d" ,pCycleCfg->type);
      break;
  }

#ifdef DEBUG_CYCLE
/*vcast_dont_instrument_start*/
  GSE_DebugStr(NORMAL,TRUE,"Cycle: EngRun[%d], Cycle[%d] CycleFinished",erID,nCycle);
/*vcast_dont_instrument_end*/
#endif
}


/******************************************************************************
 * Function:     CycleRestoreCountsFromPersistFiles
 *
 * Description:  Opens the Persist Count files RTC and EEPROM
 *
 * Parameters:   [in] None
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN CycleRestoreCountsFromPersistFiles(void)
{
  RESULT resultEE;
  RESULT resultRTC;
  BOOLEAN status = FALSE;
  INT16   i;
  INT16  size = sizeof(CYCLE_COUNTS);

  // Retrieves the stored counters from non-volatile
  // Note: NV_Open() performs checksum and creates sys log if checksum fails !

  resultEE  = NV_Open(NV_PCYCLE_CNTS_EE);
  resultRTC = NV_Open(NV_PCYCLE_CNTS_RTC);

  memset( (void *) &m_CountsEEProm, 0, size);
  memset( (void *) &m_CountsRTC,      0, size);

  // Upon startup if the RTC and EEPROM are both invalid, a system log shall be logged
  // and both files shall be initialized to zero counts
  if ((resultEE != SYS_OK) && (resultRTC != SYS_OK))
  {
    // Both locations are corrupt ! Create ETM log ? Will this be a duplicate ?
    LogWriteETM( SYS_ID_CYCLES_PERSIST_FILES_INVALID, LOG_PRIORITY_3, NULL, 0, NULL );
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Both Persisted Cycle Files Bad - Resetting");

    for ( i = 0; i < MAX_CYCLES; i++)
    {
      /* Re-Initialize count to 0 */
      m_CountsEEProm.data[i].count.n      = 0;
      CycleUpdateCheckId( i, LOGUPDATE_NO);
    }

    // Sync the RTC  copy to match the EE
    memcpy(&m_CountsRTC, &m_CountsEEProm, sizeof(m_CountsRTC) );

    resultRTC = NV_WriteNow(NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Initialized counts copied to RTCNVRAM...%s",
                               SYS_OK == resultRTC ? "SUCCESS":"FAILED");

    resultEE = NV_WriteNow(NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Initialized counts copied to EEPROM...%s",
                              SYS_OK == resultRTC ? "SUCCESS":"FAILED");

    status = ((resultRTC == SYS_OK) || (resultEE == SYS_OK)) ? TRUE : FALSE;
  }
  // else if RTC was bad, read from EE version.
  else if (resultRTC != SYS_OK)
  {
    NV_Read(NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, size);
    memcpy(&m_CountsRTC, &m_CountsEEProm, size);
    resultRTC = NV_WriteNow(NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Persist Counts from EEPROM copied to RTCNVRAM...%s",
                               SYS_OK == resultRTC ? "SUCCESS":"FAILED");
    status = TRUE;
  }
  // EEPROM bad, read from RTC version
  else if (resultEE != SYS_OK)
  {
    NV_Read( NV_PCYCLE_CNTS_RTC, 0, &m_CountsEEProm, size );
    memcpy(&m_CountsEEProm, &m_CountsRTC, size);
    resultEE = NV_WriteNow(NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Persist Counts from RTCNVRAM copied to EEPROM... %s",
                 SYS_OK == resultEE ? "SUCCESS":"FAILED");
    status = TRUE;
  }
  // Both RTC and EEPROM are good
  else
  {
    NV_Read(NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC,    size);
    NV_Read(NV_PCYCLE_CNTS_EE,  0, &m_CountsEEProm, size);
    status = TRUE;
  }
  return status;
}

/******************************************************************************
 * Function:     CycleUpdateCheckId
 *
 * Description:  Compares CheckId and Updates as necessary
 *
 * Parameters:   [in] nCycle the persist cycle to be updated.
 *               [in] bLogUpdate
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN CycleUpdateCheckId( UINT16 nCycle, BOOLEAN bLogUpdate )
{
  BOOLEAN bNeedUpdate = FALSE;
  UINT16  iTempID;
  CYCLE_COUNT_CHANGE_ETM_LOG changeLog;

  // Calculate the actual checkId
  iTempID = CycleCalcCheckID (nCycle);

  // If the actual is different than current log it if bLogUpdate.
  if ( m_CountsEEProm.data[nCycle].checkID != iTempID )
  {
    if ( bLogUpdate )
    {
      strncpy_safe(changeLog.CycleName, sizeof(changeLog.CycleName),
                   m_Cfg[nCycle].name,  MAX_CYCLENAME);
      sprintf( changeLog.PrevValue, "0x%04x", m_CountsEEProm.data[nCycle].checkID);
      sprintf( changeLog.NewValue, "0x%04x",  iTempID);
      LogWriteETM( SYS_ID_CYCLES_CHECKID_CHANGED, LOG_PRIORITY_3, NULL, 0, NULL );
    }

    m_CountsEEProm.data[nCycle].checkID = iTempID;
    /* Don't Clear count to 0 in this case*/
    bNeedUpdate = TRUE;

  }
  return bNeedUpdate;
}

/******************************************************************************
 * Function:     CycleCalcCheckID
 *
 * Description:  Calculates the CheckID for the PCYCLEDATA
 *               Currently this includes the following fields:
 *               Type, nCount, nSensorAId, nSensorBId,
 *               nEngineRunId
 *
 * Parameters:   [in] nCycle the persist cycle to be updated.
 *               [in] bLogUpdate
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static UINT16 CycleCalcCheckID( UINT16 nCycle )
{
  UINT16 cksum;
  CYCLE_CFG* pCycleCfg = &m_Cfg[nCycle];

  cksum = (UINT16) pCycleCfg->type         +
          (UINT16) pCycleCfg->nCount       +
          (UINT16) pCycleCfg->nTriggerId   +
          (UINT16) pCycleCfg->nEngineRunId;
  return(cksum);
}


/******************************************************************************
 * Function:     CycleResetEngineRun
 *
 * Description:  Reset Cycle calculations for a given
 *               Engine Run
 *
 * Parameters:   [in] Engine Run Index
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleResetEngineRun( ENGRUN_INDEX erID)
{
  UINT16 i;

  for (i = 0; i < MAX_CYCLES; i++)
  {
    if ( m_Cfg[i].type != CYC_TYPE_NONE_CNT && m_Cfg[i].nEngineRunId == erID )
    {
      CycleReset( &m_Cfg[i], &m_Data[i]);
    }
  }
}


/******************************************************************************
 * Function:     CycleSyncPersistFiles
 *
 * Description:  Write back the persist counts to both EE and RTC NvRam
 *               Engine Run
 *
 * Parameters:   [in] boolean write mode flag
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleSyncPersistFiles(BOOLEAN bNow)
{
  INT16 size = sizeof(CYCLE_COUNTS);

  memcpy ( &m_CountsRTC, &m_CountsEEProm,  size );
  if (bNow)
  {
   NV_WriteNow( NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC,    size);
   NV_WriteNow( NV_PCYCLE_CNTS_EE,  0, &m_CountsEEProm, size);
  }
  else
  {
   NV_Write( NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC,    size);
   NV_Write( NV_PCYCLE_CNTS_EE,  0, &m_CountsEEProm, size);
  }

#ifdef DEBUG_CYCLE
  GSE_DebugStr(NORMAL,TRUE, "Cycle: Updated Persisted Cnts to EEPROM and RTC");
#endif

}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Cycle.c $
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 12-10-02   Time: 1:23p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Coding standard compliance
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 12-09-19   Time: 3:23p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Coding standard 
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * FAST 2 Refactor Cycle for externing variable to Trend
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 12-09-11   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Binary ETM Header
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 15  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Fixed bug in persist count writing
 *
 * *****************  Version 14  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:26p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Change data type for counts
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:35p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle impl and persistence
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:32p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 refactored cycle in engrun as array not struct
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 6/18/12    Time: 4:27p
 * Updated in $/software/control processor/code/system
 * fixed bug at end of CycleUpdateSimpleAndDuration CYC_TYPE_SIMPL
 *
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 6/07/12    Time: 7:14p
 * Updated in $/software/control processor/code/system
 * Bug fix
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 6/05/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 5/24/12    Time: 3:05p
 * Updated in $/software/control processor/code/system
 * FAST2 Refactoring
 *
 * *****************  Version 7  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:38p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Fixed function prototype
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-05-22   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Check in for Dave
 *
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle
 ***************************************************************************/
