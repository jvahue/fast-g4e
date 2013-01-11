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
  $Revision: 34 $  $Date: 13-01-08 11:04a $

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
static CYCLE_CFG    m_Cfg [MAX_CYCLES];      // Cycle Cfg Array
static CYCLE_DATA   m_Data[MAX_CYCLES];      // Runtime data related to the cycles

static CYCLE_COUNTS m_CountsEEProm;  // master count list, Persisted to EE at end of ER
                                     // otherwise is used as the compare base for
                                     // counts during this ER


static CYCLE_COUNTS m_CountsRTC;    // buffer for counts written to RTC during the ER
                                    // during init acts as the working buffer to cross-check
                                    // RTC/EEPROM.

static UINT32 m_CountsCurrent[MAX_CYCLES]; // Current totals reflecting persist when applicable

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void    CycleReset ( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData );
static void    CycleUpdate( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData, UINT16 cycIndex );

static void    CycleUpdateSimpleAndDuration (CYCLE_CFG*  pCycleCfg,
                                             CYCLE_DATA* pCycleData,
                                             UINT16      cycIndex );

static void    CycleInitPersistent(void);
static BOOLEAN CycleIsPersistentType ( UINT8 nCycle );

static void    CycleUpdateCount( UINT16 nCycle,  CYC_BKUP_TYPE mode );
static void    CycleRestoreCntsFromPersistFiles(void);
static BOOLEAN CycleUpdateCheckId( UINT16 nCycle, BOOLEAN bLogUpdate);
static UINT16  CycleCalcCheckID ( UINT16 nCycle );
static void    CycleSyncPersistFiles(BOOLEAN bNow);

// Include cmd tables and functions here after local dependencies are declared.
#include "CycleUserTables.c"


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     CycleInitialize
 *
 * Description:  Initialize cycle processing
 *
 * Parameters:   None.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleInitialize(void)
{
  UINT16 i;
  // Add user commands for Cycles to the user command tables.
  User_AddRootCmd(&rootCycleMsg);

  // Load the current cfg info.
  memcpy(m_Cfg,
         CfgMgr_RuntimeConfigPtr()->CycleConfigs,
        sizeof(m_Cfg));

  // Reload the persistent cycle count  info from EEPROM & RTCNVRAM
  CycleInitPersistent();

  // Initialize/CycleResetAll storage.
  for (i = 0; i < MAX_CYCLES; i++)
  {
    if ( m_Cfg[i].type != CYC_TYPE_NONE_CNT)
    {
     CycleReset( &m_Cfg[i], &m_Data[i] );
    }
  }

}
/******************************************************************************
 * Function:     CycleUpdateAll
 *
 * Description:  Updates all cycles for EngineRun 'erIndex'
 *
 * Parameters:   erIndex - The index into the CYCLE_CFG and CYCLE_DATA object arrays.
 *               erState - The current ER state.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleUpdateAll(ENGRUN_INDEX erIndex, ER_STATE erState)
{
  UINT16 cycIndex;

  // Update all active cycles associated with this engine run
  for (cycIndex = 0; cycIndex < MAX_CYCLES; cycIndex++)
  {
    if ( m_Cfg[cycIndex].type != CYC_TYPE_NONE_CNT &&
         m_Cfg[cycIndex].nEngineRunId == erIndex )
    {
      if (ER_STATE_STOPPED == erState )
      {
        m_Data[cycIndex].cycleActive = FALSE;
      }
      else
      {
        CycleUpdate(&m_Cfg[cycIndex], &m_Data[cycIndex], cycIndex);
      }
    }
  }
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
   memset ( cycleHdr, 0, sizeof(cycleHdr) );

   // Loop through all the cycles
   for ( cycleIndex = 0;
         ((cycleIndex < MAX_CYCLES) && (nRemaining > sizeof (CYCLE_HDR)));
         cycleIndex++ )
   {
      // Copy the Cycle names
      strncpy_safe( cycleHdr[cycleIndex].name,
                    sizeof(cycleHdr[cycleIndex].name),
                    m_Cfg[cycleIndex].name,
                    _TRUNCATE);

      cycleHdr[cycleIndex].type         = m_Cfg[cycleIndex].type;
      cycleHdr[cycleIndex].nCount       = m_Cfg[cycleIndex].nCount;
      cycleHdr[cycleIndex].nTriggerId   = m_Cfg[cycleIndex].nTriggerId;
      cycleHdr[cycleIndex].nEngineRunId = m_Cfg[cycleIndex].nEngineRunId;

      // Increment the total number of bytes and decrement the remaining
      nTotal     += sizeof (CYCLE_HDR);
      nRemaining -= sizeof (CYCLE_HDR);
   }
   // Copy the Cycle header to the buffer
   memcpy ( pBuffer, cycleHdr, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}


/******************************************************************************
 * Function:     CycleGetCount
 *
 * Description:  Retrieves the count for cycle 'nCycle'
 *
 *
 * Parameters:   UINT8 nCycle - Cycle Index of the value to return
 *
 *
 * Returns:      UINT32 - The current value of the Cycle
 *
 * Notes:        None
 *
 *****************************************************************************/
UINT32  CycleGetCount( CYCLE_INDEX nCycle )
{
  return m_CountsCurrent[nCycle];
}

/******************************************************************************
 * Function:     CycleCollectCounts
 *
 * Description:  Returns the values of all cycles associated with the erIdx,
 *               in the provided array 'counts'
 *
 * Parameters:   [out] counts - array for returning cycle counts
 *               [in]  erIdx  - EngineRun whose cycle counts are to be returned
 *
 * Returns:      None
 *
 * Notes:        This function should be preceded by a call to
 *               CycleBackupPersistentCounts() to ensure m_CountsEEProm has
 *               been properly updated. The updated info in m_CountsEEProm is
 *               used to bring the cycle totals up to current persisted values.
 *
 *****************************************************************************/
void CycleCollectCounts( UINT32 counts[], ENGRUN_INDEX erIdx )
{
  INT16 cyc;
  //CYCLE_DATA*    pData;
  //CYCLE_CFG*     pCfg;

  ASSERT(erIdx < MAX_ENGINES);

  for(cyc = 0; cyc < MAX_CYCLES; ++cyc)
  {
    if (m_Cfg[cyc].nEngineRunId == erIdx &&
        m_Cfg[cyc].type != CYC_TYPE_NONE_CNT )
    {
      counts[cyc] = m_CountsCurrent[cyc];
    }
  }
}

/******************************************************************************
 * Function:    CycleEEFileInit
 *
 * Description: Clears the EEPROM storage location
 *
 * Parameters:  None
 *
 * Returns:     TRUE
 *
 * Notes:       Standard Initialization format to be compatible with
 *              NVMgr Interface.
 *
 *****************************************************************************/
BOOLEAN CycleEEFileInit(void)
{
   memset((void *)&m_CountsEEProm,  0, sizeof(m_CountsEEProm ));

   NV_Write( NV_PCYCLE_CNTS_EE,  0, &m_CountsEEProm,  sizeof(m_CountsEEProm) );

   return TRUE;
}

/******************************************************************************
 * Function:    CycleRTCFileInit
 *
 * Description: Clears the RTC storage location
 *
 * Parameters:  None
 *
 * Returns:     TRUE
 *
 * Notes:       Standard Initialization format to be compatible with
 *              NVMgr Interface.
 *
 *****************************************************************************/
BOOLEAN CycleRTCFileInit(void)
{
   memset((void *)&m_CountsRTC,  0, sizeof(m_CountsRTC ));

   NV_Write( NV_PCYCLE_CNTS_RTC,  0, &m_CountsRTC,  sizeof(m_CountsRTC) );

   return TRUE;
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/


/******************************************************************************

 * Function:     CycleInitPersistent
 *
 * Description:  Copies persistent cycle from RTC RAM or EEPROM and
 *               copies to run time memory (ies).
 *
 * Parameters:   None
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleInitPersistent(void)
{
  BOOLEAN bUpdatePersist = FALSE;
  UINT8   i;
  CYCLE_PERSIST_COUNTS_DIFF_LOG persistDiffLog;
  CYCLE_ENTRY*   pCycCntsEE;
  CYCLE_ENTRY*   pCycCntsRTC;

  CycleRestoreCntsFromPersistFiles();

  // Init the array used to display current values of cycles, persist and simple.
  memset( m_CountsCurrent, 0, sizeof(m_CountsCurrent) );


  /* If both sets of persistent data is corrupted reset to 0 */

  // RTC and EEPROM copies of persist cycles have valid but different CRC's.
  // Determine which values are valid based on checkID and create a good list.

  for ( i = 0; i < MAX_CYCLES; i++)
  {
    if ( CycleIsPersistentType( i) )
    {
      // Set ptrs to areas containing files read from NVRAM
      pCycCntsEE  = &m_CountsEEProm.data[i];
      pCycCntsRTC = &m_CountsRTC.data[i];

      // Compare the counts. Higher count is considered the
      // valid one. Make the lesser side equivalent.

      if ( pCycCntsEE->count.n != pCycCntsRTC->count.n )
      {
        // create sys log and flag that persist will need updating.
        bUpdatePersist = TRUE;

        persistDiffLog.cycleId     = i;
        persistDiffLog.rtcCount    = pCycCntsRTC->count.n;
        persistDiffLog.eepromCount = pCycCntsEE->count.n;

        LogWriteSystem( SYS_ID_CYCLES_PERSIST_COUNTS_DIFF,
                        LOG_PRIORITY_3,
                        &persistDiffLog,
                        sizeof(CYCLE_PERSIST_COUNTS_DIFF_LOG),
                        NULL );

        GSE_DebugStr(NORMAL, TRUE,
                    "Cycle[%d] - Different Persist values RTC: %d, EEPROM: %d",
                    persistDiffLog.cycleId,
                    persistDiffLog.rtcCount,
                    persistDiffLog.eepromCount);

        if ( pCycCntsEE->count.n > pCycCntsRTC->count.n )
        {
          pCycCntsRTC->count.n = pCycCntsEE->count.n;
        }
        else
        {
          pCycCntsEE->count.n = pCycCntsRTC->count.n;
        }
      }

      // Calc new checkId... a "TRUE" return signals that persist
      // needs to be re-written.
      if ( CycleUpdateCheckId( i, LOGUPDATE_YES) )
      {
        bUpdatePersist = TRUE;  /* for simplicity update both */
      }

      // At this point the EEPROM and RTC have been synchronized.
      // EEPROM values are used as the baseline so use them to
      // set the current-counts
      m_CountsCurrent[i] = m_CountsEEProm.data[i].count.n;

    } /* End of if cycle->type is a persistent */
  } /* End of for MAX_CYCLES loop             */


  if (bUpdatePersist)
  {
    CycleSyncPersistFiles(CYC_COUNT_UPDATE_NOW);
  }

}

/******************************************************************************
 * Function:     CycleIsPersistentType
 *
 * Description:  checks cycle type for persistent.
 *
 * Parameters:   [in] nCycle - cycle table index.
 *
 * Returns:      TRUE if Cycle is persistent otherwise FALSE.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static BOOLEAN CycleIsPersistentType( UINT8 nCycle )
{
  return ((m_Cfg[nCycle].type     == CYC_TYPE_PERSIST_DURATION_CNT ) ||
          (m_Cfg[nCycle].type == CYC_TYPE_PERSIST_SIMPLE_CNT) );
}

/******************************************************************************
* Function:     CycleReset
 *
 * Description:   Reset one Cycle object
 *                Called when we transition to RUN mode, we initialize the Cycle
 *                data so that valid checks may subsequently be done
 *
 * Parameters:   [in]     pCycleCfg  Pointer to cycle config data
 *               [in/out] pCycleData Pointer to cycle runtime data
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
    pCycleData->cycleActive      = FALSE;  /* Count not yet active      */
    pCycleData->cycleLastTime_ms = 0;      /* Clear start time          */
    pCycleData->currentCount     = 0;      /* Clear working cycle count */

#ifdef DEBUG_CYCLE
/*vcast_dont_instrument_start*/
    GSE_DebugStr(NORMAL,TRUE,"Cycle: %s CycleReset",pCycleCfg->Name);
/*vcast_dont_instrument_end*/
#endif
  }
}


/******************************************************************************
 * Function:     CycleUpdate
 *
 * Description:  Check to see if a specific cycle needs counting.
 *
 * Parameters:   [in]     pCycleCfg  Pointer to cycle config data
 *               [in/out] pCycleData Pointer to cycle runtime data
 *               [in]     cycIndex   index value of this cycle for updating
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleUpdate( CYCLE_CFG* pCycleCfg, CYCLE_DATA* pCycleData, UINT16 cycIndex )
{
  // At present simple are duration ( w/wo persist) are only supported types.

  if( ((pCycleCfg->type == CYC_TYPE_SIMPLE_CNT)          ||
       (pCycleCfg->type == CYC_TYPE_PERSIST_SIMPLE_CNT)  ||
       (pCycleCfg->type == CYC_TYPE_DURATION_CNT)        ||
       (pCycleCfg->type == CYC_TYPE_PERSIST_DURATION_CNT) ) )
  {
    CycleUpdateSimpleAndDuration( pCycleCfg, pCycleData, cycIndex );
  }
}

/******************************************************************************
 * Function:     CycleUpdateSimpleAndDuration
 *
 * Description:  Processes simple and duration count cycle object.
 *
 * Parameters:   [in]     pCycleCfg  Pointer to cycle config data
 *               [in/out] pCycleData Pointer to cycle runtime data
 *               [in]     cycIndex   index value of this cycle for updating
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
  BOOLEAN      cycleStart = FALSE;
  UINT32       nowTimeMs;

  if ( (pCycleData->cycleActive) )
  {
    // Cycle is already active from a previous pass...

    // If a Duration Count, update duration every time through.
    if ( (pCycleCfg->type == CYC_TYPE_DURATION_CNT) ||
         (pCycleCfg->type == CYC_TYPE_PERSIST_DURATION_CNT) )
    {
      // Increment duration every time through when active.

      // Update the millisec duration count
      nowTimeMs = CM_GetTickCount();
      pCycleData->currentCount     += nowTimeMs - pCycleData->cycleLastTime_ms;
      pCycleData->cycleLastTime_ms =  nowTimeMs;
    }

    // Update the cycle active status.
    pCycleData->cycleActive = TriggerGetState(pCycleCfg->nTriggerId);

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
    // Cycle is inactive - see if the start criteria are met
    cycleStart = TriggerGetState(pCycleCfg->nTriggerId);

    // Cycle has started
    if (cycleStart)
    {
      // Previous run was "inactive" and trigger is now active
      // value - count the cycle in the flight log and set the flag
      // that indicates that the cycle is active.
      #ifdef DEBUG_CYCLE
      /*vcast_dont_instrument_start*/
       GSE_DebugStr(NORMAL, TRUE,"Cycle: Cycle[%d] Started", cycIndex);
       /*vcast_dont_instrument_end*/
      #endif

      pCycleData->cycleActive      = TRUE;
      pCycleData->cycleLastTime_ms = CM_GetTickCount();

      if (( pCycleCfg->type == CYC_TYPE_SIMPLE_CNT ) ||
          ( pCycleCfg->type == CYC_TYPE_PERSIST_SIMPLE_CNT))
      {
        pCycleData->currentCount += pCycleCfg->nCount;
      }
    } // End of if cycle has Started
  } //End of else prev cycle was inactive

  // Update the count for this cycle and write it to RTC if persistent type
  CycleUpdateCount(cycIndex, CYC_BKUP_RTC);
}


/******************************************************************************
 * Function:     CycleUpdateCount
 *
 * Description:  Update current count for persistent and non-persistent
 *
 * Parameters:   [in] nCycle index
 *               [in] mode   flag to indicate RTC RAM or EEPROM should be updated
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleUpdateCount( UINT16 nCycle,  CYC_BKUP_TYPE mode )
{
  static union
  {
    UINT32  n;
    FLOAT32 f;
  } temp_value;

  BOOLEAN bDataChanged  = FALSE;
  BOOLEAN bIsPersistent = FALSE;

  CYCLE_CFG_PTR  pCycleCfg  = &m_Cfg[nCycle];
  CYCLE_DATA_PTR pCycleData = &m_Data[nCycle];
  temp_value.n = 0;

  // Update count and persist if configured.
  // The copy in EEPROM acts as the baseline since the start of ER, against which the
  // incrementations are calculated. For persisted cycles, the updated total is stored in
  // RTC memory. When the engrun ends, this function will be called with
  // updateFlag == CYC_WRITE_EE and EEPROM memory will be synced to match RTC.

  switch (pCycleCfg->type)
  {
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
      bIsPersistent = TRUE;
      bDataChanged = TRUE;
      m_CountsCurrent[nCycle] = m_CountsEEProm.data[nCycle].count.n +
                                             pCycleData->currentCount;
      break;

    case CYC_TYPE_SIMPLE_CNT:
      m_CountsCurrent[nCycle] = pCycleData->currentCount;
      break;

    case CYC_TYPE_PERSIST_DURATION_CNT:
      bIsPersistent = TRUE;
      // Get the current duration (in millisecs) for this cycle-run.
      // add 0.5 seconds for int truncation assuming
      // 50% of values > 0.5 and 50% of values < 0.5 over time
      // then convert the duration to secs and add to the persisted total
      temp_value.f  = 500 + (FLOAT32)pCycleData->currentCount;
      temp_value.f *= 1.0f /(FLOAT32)MILLISECONDS_PER_SECOND;
      m_CountsCurrent[nCycle] = m_CountsEEProm.data[nCycle].count.n + (UINT32)temp_value.f;

      // If the updated total is more than one second greater than last persisted value,
      // flag the RTC to be updated.
      bDataChanged = (m_CountsCurrent[nCycle] - m_CountsRTC.data[nCycle].count.n)
                       >= 1 ? TRUE : FALSE;
      break;

    case CYC_TYPE_DURATION_CNT:
      // Get the current duration (in millisecs) for this cycle-run.
      // add 0.5 seconds for int truncation assuming
      // 50% of values > 0.5 and 50% of values < 0.5 over time
      // then convert the duration to secs and add to the persisted total
      temp_value.f = 500 + (FLOAT32)pCycleData->currentCount;
      temp_value.f *= 1.0f /(FLOAT32)MILLISECONDS_PER_SECOND;
      m_CountsCurrent[nCycle] = (UINT32)temp_value.f;
      break;

    // Never should get here
	  //lint -fallthrough
	  case CYC_TYPE_NONE_CNT:
	  case MAX_CYC_TYPE:
    default:
      FATAL ("Unrecognized Cycle Type Value: %d", pCycleCfg->type);
      break;
  }

  if (bIsPersistent)
  {
    switch (mode)
    {
    case CYC_BKUP_EE:
      // EngRun has ended, update the EEProm so it will match the
      // the value updated in RTC during this EngRun.

      m_CountsEEProm.data[nCycle].count.n = m_CountsCurrent[nCycle];

      #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      NV_Write( NV_PCYCLE_CNTS_EE, N_OFFSET(nCycle), &m_CountsEEProm.data[nCycle].count.n,
        sizeof(m_CountsEEProm.data[nCycle].count.n) );
      #pragma ghs endnowarning

      #ifdef DEBUG_CYCLE
      /*vcast_dont_instrument_start*/
      if ( 0 != memcmp(&m_CountsEEProm, &m_CountsRTC, sizeof(CYCLE_COUNTS)) )
      {
        INT16 i;
        for (i = 0; i < MAX_CYCLES; ++i )
        {
          if (m_CountsEEProm.data[nCycle].count.n !=  m_CountsRTC.data[nCycle].count.n &&
            m_CountsEEProm.data[nCycle].checkID !=  m_CountsRTC.data[nCycle].checkID)
          {
            GSE_DebugStr(NORMAL,TRUE,"m_CountsEEProm[%d] = %d, %d  | m_CountsRTC[%d] = %d, %d",
              i, m_CountsEEProm.data[i].count.n, m_CountsEEProm.data[i].checkID,
              i, m_CountsRTC.data[i].count.n,    m_CountsRTC.data[i].checkID);
          }
        }
      }
      /*vcast_dont_instrument_end*/
      #endif
      GSE_DebugStr(NORMAL,TRUE, "Cycle: Cycle[%d] EEPROM Updated P-Count: %d", nCycle,
                                  m_CountsEEProm.data[nCycle].count.n);

      // Flag the data as changed and fallthru so RTC will be updated with the same data
      bDataChanged = TRUE;

      //lint -fallthrough
    case CYC_BKUP_RTC:
      /* Update RTC_RAMRunTime memory ONLY IF the value is changing and is persistent type */
      if ( bDataChanged )
      {
        m_CountsRTC.data[nCycle].count.n = m_CountsCurrent[nCycle];

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

    default:
      FATAL("Unrecognized Persist backup mode requested: %d",mode);
    }
  }
}

/******************************************************************************
 * Function:     CycleFinishEngineRun
 *
 * Description:  Called by EngineRun to finish up any currently running
 *               cycles for the indicated Engine Run
 *
 * Parameters:   [in] erID - id of owning-EngineRun.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
void CycleFinishEngineRun( ENGRUN_INDEX erID )
{
  UINT8 i;
  for (i = 0; i < MAX_CYCLES; i++)
  {
    // If the cycle entry is configured, and belongs to the EngineRun,
    // save the values and close it out.

    if ( m_Cfg[i].type != CYC_TYPE_NONE_CNT  && m_Cfg[i].nEngineRunId == erID)
    {
      CycleUpdateCount( i, CYC_BKUP_EE );
    }
  }
}

/******************************************************************************
 * Function:     CycleRestoreCntsFromPersistFiles
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
static void CycleRestoreCntsFromPersistFiles(void)
{
  RESULT resultEE;
  RESULT resultRTC;
  UINT16   i;
  UINT32  size = sizeof(CYCLE_COUNTS);

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
      m_CountsEEProm.data[i].count.n = 0;
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

  }
  // else if RTC was bad, read from EE version.
  else if (resultRTC != SYS_OK)
  {
    NV_Read(NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, size);
    memcpy(&m_CountsRTC, &m_CountsEEProm, size);
    resultRTC = NV_WriteNow(NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Persist Counts from EEPROM copied to RTCNVRAM...%s",
                               SYS_OK == resultRTC ? "SUCCESS":"FAILED");
  }
  // EEPROM bad, read from RTC version
  else if (resultEE != SYS_OK)
  {
    NV_Read( NV_PCYCLE_CNTS_RTC, 0, &m_CountsEEProm, size );
    memcpy(&m_CountsEEProm, &m_CountsRTC, size);
    resultEE = NV_WriteNow(NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, size);
    GSE_DebugStr(NORMAL,TRUE, "Cycle - Persist Counts from RTCNVRAM copied to EEPROM... %s",
                 SYS_OK == resultEE ? "SUCCESS":"FAILED");
  }
  // Both RTC and EEPROM are good
  else
  {
    NV_Read(NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC,    size);
    NV_Read(NV_PCYCLE_CNTS_EE,  0, &m_CountsEEProm, size);
  }
}

/******************************************************************************
 * Function:     CycleUpdateCheckId
 *
 * Description:  Compares CheckId and Updates as necessary
 *
 * Parameters:   [in] nCycle the persist cycle to be updated.
 *               [in] bLogUpdate
 *
 * Returns:      TRUE if persist needs to be updated, otherwise FALSE.
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
      changeLog.cycleId     = nCycle;
      changeLog.prevCount   = m_CountsEEProm.data[nCycle].count.n;
      changeLog.prevCheckID = m_CountsEEProm.data[nCycle].checkID;
      changeLog.newCheckID  = iTempID;

      LogWriteETM( SYS_ID_CYCLES_CHECKID_CHANGED,
                   LOG_PRIORITY_3,
                   &changeLog,
                   sizeof(CYCLE_COUNT_CHANGE_ETM_LOG),
                   NULL );
    }

    // Reset the counter to zero and update the checkId;

    m_CountsEEProm.data[nCycle].count.n = 0;
    m_CountsEEProm.data[nCycle].checkID = iTempID;

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
 *
 * Returns:      The checksum value.
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
 * Parameters:   [in] erID - Engine Run Index
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
    // Reset all cycles for erID
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
 * Parameters:   [in] bNow - commit update to NV memory immediately
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleSyncPersistFiles(BOOLEAN bNow)
{
  UINT32 size = sizeof(CYCLE_COUNTS);

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
  /*vcast_dont_instrument_start*/
  GSE_DebugStr(NORMAL,TRUE, "Cycle: Updated Persisted Cnts to EEPROM and RTC");
  /*vcast_dont_instrument_end*/
#endif

}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Cycle.c $
 *
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 13-01-08   Time: 11:04a
 * Updated in $/software/control processor/code/system
 * SCR #1213 FAST II Performance Enh GetTrigger, remove commented code for
 * RHL & CUM PEAK
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 1/02/13    Time: 5:54p
 * Updated in $/software/control processor/code/system
 * SCR #1213 Cycle needs to also check for difference on Init when
 * RTC-EEPROM Open as valid
 *
 * *****************  Version 32  *****************
 * User: Contractor V&v Date: 12/28/12   Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR #1197 Pass eng state to cycle.
 *
 * *****************  Version 31  *****************
 * User: Melanie Jutras Date: 12-12-27   Time: 3:57p
 * Updated in $/software/control processor/code/system
 * SCR #1172. Code Review Fixes: PCLint 545 and 539.
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 12/18/12   Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 cycle count code cleanup
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 12/14/12   Time: 5:02p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review
 *
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 12/12/12   Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review  reset cydlesat begiin
 *
 * *****************  Version 27  *****************
 * User: John Omalley Date: 12-12-08   Time: 11:44a
 * Updated in $/software/control processor/code/system
 * SCR 1162 - NV MGR File Init function
 *
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 12/03/12   Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Code Review
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 11/09/12   Time: 5:16p
 * Updated in $/software/control processor/code/system
 * Code Review/ function reduction
 *
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 11/08/12   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * SCR # 1183
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 10/23/12   Time: 3:54p
 * Updated in $/software/control processor/code/system
 * SCR# 1107 - dev work
 *
 * *****************  Version 22  *****************
 * User: Melanie Jutras Date: 12-10-23   Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 10/12/12   Time: 6:29p
 * Updated in $/software/control processor/code/system
 * FAST 2 Review Findings
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
