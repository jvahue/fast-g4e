#define CYCLE_BODY
/******************************************************************************
            Copyright (C) 2012 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    FILE:        cycle.c

    DESCRIPTION
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
  $Revision: 14 $  $Date: 7/18/12 6:26p $
   
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
//#include "logmanager.h"
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

/* DEBUG_PL::CYCLE::PL_Debug */
struct {
  UINT16 InitComparison;
  UINT16 debug1;
  UINT16 debug2;
  UINT16 debug3;
} PL_Debug;

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


/*********************************************************************************************/
/* Public Functions                                                                          */
/*********************************************************************************************/

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
    if ( m_Cfg[cycIndex].Type != CYC_TYPE_NONE_CNT &&
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
    if ( m_Cfg[i].Type != CYC_TYPE_NONE_CNT)
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
    pCycleCfg->Type = CYC_TYPE_NONE_CNT;

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
          (m_Cfg[nCycle].Type == CYC_TYPE_PERSIST_DURATION_CNT )
           || (m_Cfg[nCycle].Type == CYC_TYPE_PERSIST_SIMPLE_CNT)
#ifdef PEAK_CUM_PROCESSING
           || (m_Cfg[nCycle].Type == CYC_TYPE_PEAK_VAL_CNT) 
           || (m_Cfg[nCycle].Type == CYC_TYPE_CUM_CNT)
#endif
       );
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
  
  /* DEBUG_PL */
  PL_Debug.InitComparison = 0x0000;
  PL_Debug.debug1 = 0;
  PL_Debug.debug2 = 0;
  PL_Debug.debug3 = 0;

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
          if ( CYC_TYPE_PERSIST_DURATION_CNT == pCycCfg->Type )
          {
            if ( pCycCntsEE->count.n > pCycCntsRTC->count.n )
            {
              //PL_Debug.InitComparison |= INITPCYCLES_FLASHGREATER;
              pCycCntsRTC->count.n = pCycCntsEE->count.n;
              bUpdateRTCRAM = TRUE;
            }
            else
            {
              //PL_Debug.InitComparison |= INITPCYCLES_RTCGREATER;
              pCycCntsEE->count.n = pCycCntsRTC->count.n;
              bUpdateEEPROM = TRUE;
            }
          }
          else
          {
            if ( pCycCntsEE->count.f > pCycCntsRTC->count.f )
            {
             // PL_Debug.InitComparison |= INITPCYCLES_FLASHGREATER;
              pCycCntsRTC->count.f = pCycCntsEE->count.f;
              bUpdateRTCRAM = TRUE;
            }
            else
            {
              PL_Debug.InitComparison |= INITPCYCLES_RTCGREATER;
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
        //PL_Debug.InitComparison |= INITPCYCLES_CHECKIDFAIL;
      }
    }
  }

  if (bUpdateRTCRAM | bUpdateEEPROM)
  {
    CycleSyncPersistFiles(CYC_COUNT_UPDATE_NOW);
  }

 /* if ( ( ( PL_Debug.InitComparison & INITPCYCLES_RTCGREATER) != 0x00) ||
    ( ( PL_Debug.InitComparison & INITPCYCLES_FLASHGREATER) != 0x00)  )
    LogPCycleFail(PL_Debug.InitComparison);
 */

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
  if ( pCycleCfg->Type == CYC_TYPE_NONE_CNT ||
       pCycleCfg->nTriggerId >= MAX_TRIGGERS ||
       !TriggerIsConfigured( pCycleCfg->nTriggerId ) )       
  {
    pCycleCfg->Type         = CYC_TYPE_NONE_CNT;
    pCycleCfg->nEngineRunId = ENGRUN_UNUSED;
  }
  else
  {
    //pCycle->nCheckTime = pCycle->nCPUOffset;  /* Start checking here */
    pCycleData->CycleActive = FALSE;            /* Count not yet active */
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
  switch (pCycleCfg->Type)
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
      FATAL ("Cycle[%d] %s has Unrecognized Cycle Type: %d", cycIndex, pCycleCfg->Name, pCycleCfg->Type);
      pCycleCfg->Type = CYC_TYPE_NONE_CNT;
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
  //#ifdef CYCLES_UNDER_CONSTRUCTION
  BOOLEAN      CycleStart = FALSE;
  BITARRAY128  trigMask;
  UINT32*      pErDataCycles;

  CLR_TRIGGER_FLAGS(trigMask);
  SetBit(pCycleCfg->nTriggerId, trigMask, sizeof(BITARRAY128 ));

  pErDataCycles = EngRunGetPtrToCycleCounts(pCycleCfg->nEngineRunId);  
  
  if ( (pCycleData->CycleActive) )
  {
    // Cycle was already active from a previous pass...

    // If a Duration Count, update duration every time through.
    if ( (pCycleCfg->Type == CYC_TYPE_DURATION_CNT) || 
         (pCycleCfg->Type == CYC_TYPE_PERSIST_DURATION_CNT) )
    {
      // Increment time every time through when active, use casts to FLOAT32
      // so we do not do integer math and get truncation on the division

      // Update the count in the enginerun object.
      pErDataCycles[cycIndex] +=
                (CM_GetTickCount() - EngRunGetStartingTime(pCycleCfg->nEngineRunId));
      
      if (pCycleCfg->Type == CYC_TYPE_PERSIST_DURATION_CNT)
      {
        CycleBackupPersistentCounts(cycIndex, CYC_BKUP_RTC);
      }
    }

    // Update the trigger active status.
    pCycleData->CycleActive = ( TriggerIsActive(&trigMask ) )? TRUE : FALSE;

    #ifdef DEBUG_CYCLE
    if(!pCycleData->CycleActive)
    {
      GSE_DebugStr(NORMAL, TRUE,"Cycle: Cycle[%d] Ended", cycIndex);
    }
    #endif 

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

      pCycleData->CycleActive = TRUE;
      
      if (( pCycleCfg->Type == CYC_TYPE_SIMPLE_CNT ) ||
          ( pCycleCfg->Type == CYC_TYPE_PERSIST_SIMPLE_CNT))
      {
        pErDataCycles[cycIndex] += pCycleCfg->nCount;

        if (pCycleCfg->Type == CYC_TYPE_PERSIST_SIMPLE_CNT)
        {
          CycleBackupPersistentCounts(cycIndex, CYC_BKUP_RTC);
        }
      }
    } // End of if cycle has Started    
  } //End of else prev cycle was inactive

//#endif
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

  CYCLE_CFG_PTR pCycle = &m_Cfg[nCycle];
 

  // Get ptr to this cycle's enginerun collection area.
  UINT32* pErDataCycles = EngRunGetPtrToCycleCounts(pCycle->nEngineRunId);

  // Update persistent count 
  // The copy in EEPROM acts as the baseline since 'start of ER' against which the
  // incrementation are calculated. The updated total is stored in RTC memory.
  // When the engrun ends, this function will be called with updateFlag == CYC_WRITE_EE and
  // the EEPROM memory will be synced to match RTC.

  switch (pCycle->Type)
  {
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
    case CYC_TYPE_PERSIST_DURATION_CNT:
      temp_value.n = m_CountsEEProm.data[nCycle].count.n + pErDataCycles[nCycle];      
      break;

      /* Add additional cycle types here so cycle types may be cast to the required data-types as needed */
    
    default:
      FATAL ("Unrecognized Cycle Type Value: %d", pCycle->Type);
      break;

  }  /* End of switch (pCycle->Type) */

  switch (mode)
  {
    case CYC_BKUP_RTC:
      /* Update RTC_RAMRunTime memory */
      m_CountsRTC.data[nCycle].count.n = temp_value.n;

      /* Send data to RTC RAM */

      #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      NV_Write( NV_PCYCLE_CNTS_RTC, N_OFFSET(nCycle), &m_CountsRTC.data[nCycle].count.n,
        sizeof(m_CountsRTC.data[nCycle].count.n) );
      //PL_Debug.debug2++; //todo DaveB
      #pragma ghs endnowarning

      #ifdef DEBUG_CYCLE
      GSE_DebugStr(NORMAL,TRUE, "Cycle: RTC Update Cycle[%d] P-Count: %d",
                                  nCycle,
                                  m_CountsRTC.data[nCycle].count.n );
      #endif
      break;

    case CYC_BKUP_EE:
      // Update EEPROM ( EngRun has ended, update the EEProm so it will match the
      // the totals in RTC during this EngRun
      memcpy(&m_CountsEEProm, &m_CountsRTC, sizeof(CYCLE_COUNTS));

      #pragma ghs nowarning 1545 //Suppress packed structure alignment warning
      NV_Write( NV_PCYCLE_CNTS_EE, 0, &m_CountsEEProm, sizeof(sizeof(CYCLE_COUNTS) ) );   
      //PL_Debug.debug2++; //todo DaveB
      #pragma ghs endnowarning

      #ifdef DEBUG_CYCLE
      if ( 0 != memcmp(&m_CountsEEProm, &m_CountsRTC, sizeof(CYCLE_COUNTS)) )
      {
        INT16 i;
        for (i = 0; i < MAX_CYCLES; ++i )
        {
          GSE_DebugStr(NORMAL,TRUE,"m_CountsEEProm[%d] = %d, %d  |  m_CountsRTC[%d] = %d, %d",
                                    i,
                                    m_CountsEEProm.data[i].count.n,
                                    m_CountsEEProm.data[i].checkID,
                                    i,
                                    m_CountsRTC.data[i].count.n,
                                    m_CountsRTC.data[i].checkID);
        }
      }

    //ASSERT_MESSAGE(0 == memcmp(&m_CountsEEProm, &m_CountsRTC,sizeof(CYCLE_COUNTS)),"PERSIST MEMORY NOT EQUAL",NULL );    
    #endif
    GSE_DebugStr(NORMAL,TRUE, "Cycle: EEPROM Updated Cycle[%d] P-Count: %d", nCycle,
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

    if ( m_Cfg[i].Type != CYC_TYPE_NONE_CNT &&
         m_Cfg[i].nEngineRunId == erID)
    {
      // Persist cycle info to both EEPROM and RTC at end of cycle.
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
 * Notes:        This function should be preceded by a call to CycleBackupPersistentCounts
 *               to ensure m_CountsEEProm has been properly updated.
 *
 *****************************************************************************/
static void CycleFinish( UINT16 nCycle )
{
  CYCLE_DATA*  pCycle    = &m_Data[nCycle];
  CYCLE_CFG*   pCycleCfg = &m_Cfg[nCycle];

  UINT32* pErDataCycles  = NULL;

  ENGRUN_INDEX erID = pCycleCfg->nEngineRunId;

  // Get ptr to the Log collection area of the owning EngineRun
  pErDataCycles = EngRunGetPtrToCycleCounts(erID);

  // Act on the different types of cycles
  switch (pCycleCfg->Type)
  {
    case CYC_TYPE_SIMPLE_CNT:
    case CYC_TYPE_PERSIST_SIMPLE_CNT:
      // Mark the cycle as completed.

      pCycle->CycleActive = FALSE;

      // If the 
      if (pErDataCycles != NULL)
      {
        if ( pCycleCfg->Type == CYC_TYPE_SIMPLE_CNT )
        {
          pErDataCycles[nCycle] = m_CountsEEProm.data[nCycle].count.n;
        }
      }
      break;

    case CYC_TYPE_DURATION_CNT:
    case CYC_TYPE_PERSIST_DURATION_CNT:
      if (pErDataCycles != NULL)
      {
        if (pCycleCfg->Type == CYC_TYPE_PERSIST_DURATION_CNT)
        {
          pErDataCycles[nCycle] = m_CountsEEProm.data[nCycle].count.n;
        }
        else
        {
          /* convert ticks to seconds correcting for CLK time error */
// todo DaveB          pLog->CycleCounts[nCycle] = CLK_ERR( EngineRunData[erID].pLog->CycleCounts[nCycle]);
// todo DaveB           pLog->CycleCounts[nCycle] *= 1.0f/((FLOAT32)TICKS_PER_SECOND);
        }
      }

      pCycle->CycleActive = FALSE;
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
      FATAL ("Unrecognized Cycle Type Value: %d" ,pCycleCfg->Type);
      break;
  }
}
#ifdef CYCLES_UNDER_CONSTRUCTION
/******************************************************************************
 * Function:     CycleLogPersistFail
 *
 * Description:  Logs a Fault Entry for a PCycleData failure
 *               
 * Parameters:   [in] cycle table index.
 *
 * Returns:      None.
 *
 * Notes:        None.
 *
 *****************************************************************************/
static void CycleLogPersistFail ( UINT16 nCode )
{
  FAULTLOG pcycleLog;
  LOG_HANDLE logHandle;
  FAULTLOG *pLog = &pcycleLog;
  char msg[MAX_FAULTNAME];

  (void)StartLog (&logHandle,
    sizeof (FAULTLOG), FAULTLOGENTRY, pLog,
    NULL);

  if ( LOG_IS_ALLOCATED (logHandle))
  {
    sprintf ( msg, "Xcptn: PCycle %04x",nCode);
    strncpy ( pLog->FaultName, msg, sizeof(pLog->FaultName));
    FinishLog ( &logHandle );
  }


}
#endif

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
//#ifdef CYCLES_UNDER_CONSTRUCTION
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
    LogWriteETM( SYS_ID_CYCLES_PERSIST_FILES_INVALID, LOG_PRIORITY_LOW, NULL, 0, NULL );
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
    

    /*PL_Debug.InitComparison = INITPCYCLES_BOTH_LOCATION_BAD;*/   
   
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
                   m_Cfg[nCycle].Name,  MAX_CYCLENAME);
      sprintf( changeLog.PrevValue, "0x%04x", m_CountsEEProm.data[nCycle].checkID);
      sprintf( changeLog.NewValue, "0x%04x",  iTempID);      
      LogWriteETM( SYS_ID_CYCLES_CHECKID_CHANGED, LOG_PRIORITY_LOW, NULL, 0, NULL );     
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

  cksum = (UINT16) pCycleCfg->Type         +
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
    if ( m_Cfg[i].Type != CYC_TYPE_NONE_CNT && 
         m_Cfg[i].nEngineRunId == erID )
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
 * Parameters:   [in] boolen write mode flag              
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
   NV_WriteNow( NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC, size);
   NV_WriteNow( NV_PCYCLE_CNTS_EE,  0, &m_CountsRTC, size);
  }
  else
  {
   NV_Write( NV_PCYCLE_CNTS_RTC, 0, &m_CountsRTC, size);
   NV_Write( NV_PCYCLE_CNTS_EE,  0, &m_CountsRTC, size);
  }
 
#ifdef DEBUG_CYCLE
  GSE_DebugStr(NORMAL,TRUE, "Cycle: Updated Persisted Cnts to EEPROM and RTC");
#endif

}


/*************************************************************************
 *  MODIFICATIONS
 *    $History: Cycle.c $
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
 *************************************************************************/
