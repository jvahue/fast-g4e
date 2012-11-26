#define SPI_MANAGER_BODY
/******************************************************************************
            Copyright (C) 2010-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:       SPIManager.c

    Description: System level SPI Manager functions which centralizes access
                 to all SPI devices to the application

    VERSION
      $Revision: 30 $  $Date: 12-11-13 5:46p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <math.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "spimanager.h"
#include "stddefs.h"
#include "user.h"
#include "GSE.h"

#include "clockmgr.h"
#include "RTC.h"
#include "logmanager.h"

#include "ADC.h"
#include "Utility.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define SPI_MANAGER_RATE_MS  10
#define SPI_TO_LOGS ((SYS_ID_SPIMGR_BOARD_TEMP_READ_FAIL-SYS_ID_SPIMGR_EEPROM_WRITE_FAIL)+1)


#define EEPROM_QUEUE_SIZE   2
#define RTC_QUEUE_SIZE      2
#define RTCNVRAM_QUEUE_SIZE 4

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

// DefaultIoResultAddress is a default variable for callers not providing a
// IO_RESULT* var. (i.e. a valid "bit-bucket")
// It is assigned by SPIManager as needed so code does not have to always
// check-for-null.
static IO_RESULT DefaultIoResultAddress;

static SPI_RUNTIME_INFO SPI_RuntimeInfo[SPI_MAX_DEV];

// SpiManager State Variables
static BOOLEAN m_bDirectToDev;
static BOOLEAN m_bTaskStarted;
//static BOOLEAN EEPromDetected;

// Variable containing the value of Real Time Clock.
static TIMESTRUCT m_rtcTimeStruct;
RESULT m_resultGetRTC;

// Circular queues for devices using buffered I/O.
CIRCULAR_QUEUE CQ_EEPromOutput;
CIRCULAR_QUEUE CQ_RtcOutput;
CIRCULAR_QUEUE CQ_RtcNvRamOutput;

// only record SPI timeout error once per POR
BOOLEAN m_spiErrorRecorded[SPI_TO_LOGS];

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

// SPI Handler functions
void SPIMgr_UpdateRTC         (SPIMGR_DEV_ID Dev);
void SPIMgr_UpdateEEProm      (SPIMGR_DEV_ID Dev);
void SPIMgr_UpdateAnalogDevice(SPIMGR_DEV_ID Dev);
void SPIMgr_UpdateRTCNVRam    (SPIMGR_DEV_ID Dev);

// Helper function to setup data passing method

void SPIMgr_SetupDataPassing(DATA_PASSING method, SPIMGR_ENTRY* entry,
                                void* buffer, size_t size);

void SPIMgr_LogError(SYS_APP_ID Dev, RESULT result, UINT32 Address);

// Queue Mgmt functions
void    SPIMgr_CircQueue_SetName(CIRCULAR_QUEUE* cq, const CHAR* aName, UINT8 size );
BOOLEAN SPIMgr_CircQueue_Add    (CIRCULAR_QUEUE* cq, const SPIMGR_ENTRY* newEntry);
void    SPIMgr_CircQueue_Remove (CIRCULAR_QUEUE* cq, SPIMGR_ENTRY* data );

void    SPIMgr_CircQueue_GetHead( CIRCULAR_QUEUE* cq, SPIMGR_ENTRY** head );
void    SPIMgr_CircQueue_Reset  ( CIRCULAR_QUEUE* cq );

//      SPI_DevInfo[] allocation and initialization
#undef SPI_DEV
#define SPI_DEV(Id, Rate, HndlrFunc) {Rate, HndlrFunc}
static const SPI_MGR_INFO  SPI_DevInfo[SPI_MAX_DEV]  = {SPI_DEV_LIST};
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     SPIMgr_Initialize
 *
 * Description:  Initializes SPI Manager Processing
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *
 *****************************************************************************/
void SPIMgr_Initialize (void)
{
  TCB    tcbTaskInfo;
  UINT32 i;

  // Creates the SPI Manager Task
  memset( (void*)&tcbTaskInfo, 0, sizeof(tcbTaskInfo));
  strncpy_safe(tcbTaskInfo.Name, sizeof(tcbTaskInfo.Name),"SPI Manager",_TRUNCATE);
  tcbTaskInfo.TaskID         = (TASK_INDEX)SPI_Manager;
  tcbTaskInfo.Function       = SPIMgr_Task;
  tcbTaskInfo.Priority       = taskInfo[SPI_Manager].priority;
  tcbTaskInfo.Type           = taskInfo[SPI_Manager].taskType;
  tcbTaskInfo.modes          = taskInfo[SPI_Manager].modes;
  tcbTaskInfo.MIFrames       = taskInfo[SPI_Manager].MIFframes;
  tcbTaskInfo.Rmt.InitialMif = taskInfo[SPI_Manager].InitialMif;
  tcbTaskInfo.Rmt.MifRate    = taskInfo[SPI_Manager].MIFrate;
  tcbTaskInfo.Enabled        = TRUE;
  tcbTaskInfo.Locked         = FALSE;
  tcbTaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&tcbTaskInfo);

  // Setup runtime structure
  for(i = 0; i < (UINT32)SPI_MAX_DEV; ++i)
  {
    //Copy file size from the file table.
    SPI_RuntimeInfo[i].rate    = SPI_DevInfo[i].rate;
    SPI_RuntimeInfo[i].counterMs = 0;
    SPI_RuntimeInfo[i].pHandlerFunc = SPI_DevInfo[i].pHandlerFunc;

    // ----- fields below are used solely for storing/serving-up ADC values.
    SPI_RuntimeInfo[i].adcValue    = 0.0f;
    SPI_RuntimeInfo[i].adcResult   = DRV_OK;

    // Init function pointer to the applicable ADC_ReadXXX analog functions.
    switch (i)
    {
      case SPI_AC_BUS_VOLT:
        SPI_RuntimeInfo[i].pAdcFunc = ADC_ReadACBusVoltage;
        break;

      case SPI_AC_BAT_VOLT:
        SPI_RuntimeInfo[i].pAdcFunc = ADC_ReadACBattVoltage;
        break;

      case SPI_LIT_BAT_VOLT:
        SPI_RuntimeInfo[i].pAdcFunc = ADC_ReadLiBattVoltage;
        break;

      case SPI_BOARD_TEMP:
        SPI_RuntimeInfo[i].pAdcFunc = ADC_ReadBoardTemp;
        break;

      // For non-analog devices, no ADC function used.
      case SPI_RTC:
      case SPI_EEPROM:
      case SPI_RTC_NVRAM:
      default:
        SPI_RuntimeInfo[i].pAdcFunc = NULL;
        break;
    }
  }

  // init the SPI timeout error recorded flags
  for ( i = 0; i < (UINT32)SPI_TO_LOGS; i++)
  {
    m_spiErrorRecorded[i] = FALSE;
  }

  m_bDirectToDev = TRUE;
  m_bTaskStarted = FALSE;

  // Setup output-buffering queues for EEPROM and RTC
  // Specify name and max-queue depth for each.
  SPIMgr_CircQueue_Reset  (&CQ_EEPromOutput);
  SPIMgr_CircQueue_SetName(&CQ_EEPromOutput, "EE-Write-Q",  EEPROM_QUEUE_SIZE);

  SPIMgr_CircQueue_Reset  (&CQ_RtcOutput);
  SPIMgr_CircQueue_SetName(&CQ_RtcOutput, "RTC-Time-Q",     RTC_QUEUE_SIZE);

  SPIMgr_CircQueue_Reset  (&CQ_RtcNvRamOutput);
  SPIMgr_CircQueue_SetName(&CQ_RtcNvRamOutput, "RTC-RAM-Q", RTCNVRAM_QUEUE_SIZE);

  // Do an initial read of time so we can correctly service requests-for-time before
  // this task becomes scheduled.
  SPIMgr_UpdateRTC(SPI_RTC);
}


/******************************************************************************
* Function:     SPIMgr_SetModeDirectToDevice
*
* Description:  Sets the internal write mode flag to true.
*               This causes writes to be performed directly to devices.
*
* Parameters:   None
*
* Returns:      None
*
* Notes:        This function is used to allow tasks to write directly to devices
*               instead of queues. If this function is invoked during Initialization
*               it has no effect as the SpiManager starts-up with queuing disabled.
*               If called during run time, it allows shutdown activities to be
*               performed directly to NVRAM
*****************************************************************************/
void SPIMgr_SetModeDirectToDevice(void)
{
  m_bDirectToDev = TRUE;
}

/******************************************************************************
 * Function:     SPIMgr_Task
 *
 * Description:  SPI task function
 *
 * Parameters:   [in] pParam pointer to a void containing parameter block for
 *                    this task
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
void SPIMgr_Task(void *pParam )
{
  INT16 i;
  SPI_RUNTIME_INFO* SpiDev;


  if (!m_bTaskStarted)
  {
    // Enable queuing on startup.
    m_bDirectToDev = FALSE;
    m_bTaskStarted = TRUE;
  }

  // Set a pointer into the struct to de-clutter code
  SpiDev = &SPI_RuntimeInfo[0];
  for (i = 0; i < SPI_MAX_DEV; ++i)
  {
    SpiDev->counterMs += SPI_MANAGER_RATE_MS;

    // If counter limit has been reached...
    // Perform processing for this device
    // by calling the appropriate SPI_DEV_FUNC
    // The device id param is only used by SPIMgr_UpdateAnalogDevice

    if(SpiDev->counterMs >= SpiDev->rate)
    {
      SpiDev->pHandlerFunc( (SPIMGR_DEV_ID)i );
      SpiDev->counterMs = 0;
    }

    // go to the next device
    ++SpiDev;
  }
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/******************************************************************************
* Function:    SPIMgr_UpdateRTC
*
* Description: Periodic function used to update a local copy of the
*              RTC
*
* Parameters:  SPIMGR_DEV_ID Dev - not used by this function, required by interface.
*
* Returns:     void
*
* Notes:
*  1) CM_SYS_CLOCK should be initialized before exec this func.  _UpdateRTC
*     is called periodically during normal operation. CM_SYS_CLOCK should
*     be initialized at this point. (SCR #699 Item #2)
*
*****************************************************************************/
void SPIMgr_UpdateRTC(SPIMGR_DEV_ID Dev)
{
  UINT32 TempVar;
  SPIMGR_ENTRY* pEntry  = (SPIMGR_ENTRY*)&TempVar;
  SPIMGR_ENTRY  Entry;
  TIMESTRUCT ts;
  RESULT result;

  // Check output queue for pending RTC writes

  // Get the head(oldest) entry
  SPIMgr_CircQueue_GetHead(&CQ_RtcOutput, &pEntry);

  // Handle the top/head entry, if any
  // RTC returns values immediately so there is no need to enter
  // a waiting state.
  while (pEntry != NULL && !m_bDirectToDev)
  {
    // RTC should always pass by value.
    ASSERT(pEntry->dataMethod == DATA_PASSING_VALUE);

    // copy set-time data from queue entry to TIMESTRUCT variable and write to device.
    memcpy( (void*)&ts, pEntry->data, pEntry->size );

    result = RTC_WriteTime(&ts);
    if(DRV_SPI_TIMEOUT == result)
    {
      SPIMgr_LogError(SYS_ID_SPIMGR_RTC_WRITE_FAIL, result, NULL);
    }

    *pEntry->pResult = IO_RESULT_READY;

#ifdef DEBUG_SPIMGR_TASK
/*vcast_dont_instrument_start*/
      GSE_DebugStr(NORMAL, TRUE, "RTC WRITE COMPLETED: %d", pEntry->addr, *pEntry->pResult);
/*vcast_dont_instrument_end*/
#endif

    // Delete the entry from the queue
    SPIMgr_CircQueue_Remove(&CQ_RtcOutput, &Entry );

    // Get the next entry( in any) and loop again.
    SPIMgr_CircQueue_GetHead(&CQ_RtcOutput, &pEntry );
  }

  // Read the RTC. This will reflect the latest set time.
  m_resultGetRTC = RTC_ReadTime(&m_rtcTimeStruct);
  if(DRV_SPI_TIMEOUT == m_resultGetRTC)
  {
    SPIMgr_LogError(SYS_ID_SPIMGR_RTC_READ_FAIL, m_resultGetRTC, NULL);

    // Set the m_rtcTimeStruct to current CM_SYS_CLOCK on failure rather
    //   then leave it to 1/1/1997 00:00:00
    // Note: During init CM_GetSystemClock() will return val from RTC, but if
    //   RTC is data is 'bad' 1/1/1997 will be returned, otherwise CM_SYS_CLOCK
    //   will be returned.
    CM_GetSystemClock( (TIMESTRUCT *) &m_rtcTimeStruct );
  }
}

/******************************************************************************
* Function:    SPIMgr_UpdateRTCNVRam
*
* Description: Function used to send writes to the RTC NvRam
*
* Parameters:  SPIMGR_DEV_ID Dev - not used by this function, required by interface.
*
* Returns:     void
*
* Notes:       Applications should call   to get current value.
*
*****************************************************************************/
void SPIMgr_UpdateRTCNVRam(SPIMGR_DEV_ID Dev)
{
  UINT32 tempVar;
  SPIMGR_ENTRY* pEntry  = (SPIMGR_ENTRY*)&tempVar;
  SPIMGR_ENTRY  entry;
  RESULT result;
  void*  ptr;

  // Check output queue for pending RTC NVRam writes

  // Get the head(oldest) entry
  SPIMgr_CircQueue_GetHead(&CQ_RtcNvRamOutput, &pEntry);

  // Handle the top/head entry if any.
  while (pEntry != NULL && !m_bDirectToDev)
  {
    // Set the write pointer local data or pointer-to-data as defined.
    ptr = (pEntry->dataMethod == DATA_PASSING_PTR) ?
                             (void*) pEntry->pData :
                             (void*) pEntry->data;

    result = RTC_WriteNVRam((UINT8)pEntry->addr, ptr, pEntry->size);
    if(DRV_SPI_TIMEOUT == result)
    {
      SPIMgr_LogError(SYS_ID_SPIMGR_RTC_NVRAM_WRITE_FAIL, result, NULL);
    }

    pEntry->opState = OP_STATUS_IN_PROGRESS;
    *pEntry->pResult = IO_RESULT_READY;

    SPIMgr_CircQueue_Remove(&CQ_RtcNvRamOutput, &entry );

#ifdef DEBUG_SPIMGR_TASK
/*vcast_dont_instrument_start*/
    GSE_DebugStr(NORMAL, TRUE, "RTC NvRam WRITE COMPLETED: %d", entry.addr, *entry.pResult);
/*vcast_dont_instrument_end*/
#endif

    // Get the next entry( in any) and loop again.
    SPIMgr_CircQueue_GetHead(&CQ_RtcNvRamOutput, &pEntry );
  }
}



/******************************************************************************
 * Function:    SPIMgr_UpdateAnalogDevice
 *
 * Description: Function used to update a local copy of the
 *              value for the passed device.
 *
 * Parameters:  SPIMGR_DEV_ID Dev: analog device to be read
 *
 * Returns:     void
 *
 * Notes:       Applications should call SPIMgr_GetXXXXX to get current value.
 *
 *****************************************************************************/
void SPIMgr_UpdateAnalogDevice(SPIMGR_DEV_ID Dev)
{
  SYS_APP_ID appId;
  // Get the current value and result for the analog
  SPI_RuntimeInfo[Dev].adcResult =
       SPI_RuntimeInfo[Dev].pAdcFunc(&SPI_RuntimeInfo[Dev].adcValue);

  if(DRV_SPI_TIMEOUT == SPI_RuntimeInfo[Dev].adcResult)
  {
    appId = SYS_ID_NULL_LOG;

    switch(Dev)
    {
      case SPI_AC_BUS_VOLT:
        appId = SYS_ID_SPIMGR_BUS_VOLT_READ_FAIL;
        break;
      case SPI_AC_BAT_VOLT:
        appId = SYS_ID_SPIMGR_BAT_VOLT_READ_FAIL;
        break;
      case SPI_LIT_BAT_VOLT:
        appId = SYS_ID_SPIMGR_LITBAT_VOLT_READ_FAIL;
        break;
      case SPI_BOARD_TEMP:
        appId = SYS_ID_SPIMGR_BOARD_TEMP_READ_FAIL;
        break;
      default:
        FATAL("Invalid Analog device: %d", Dev);
        break;
    }
    SPIMgr_LogError(appId, SPI_RuntimeInfo[Dev].adcResult, NULL);
  }
}

/******************************************************************************
 * Function:    SPIMgr_UpdateEEProm
 *
 * Description: function used to perform buffered writes to the EEPROM
 *
 * Parameters:  SPIMGR_DEV_ID Dev - not used by this function, required by interface.
 *
 * Returns:     void
 *
 * Notes:       Applications should call SPIMgr_WriteBlock/SPIMgr_ReadBlock
 *
 *****************************************************************************/
void SPIMgr_UpdateEEProm(SPIMGR_DEV_ID Dev)
{
  UINT32 tempVar;
  SPIMGR_ENTRY* pEntry = (SPIMGR_ENTRY*)&tempVar;
  SPIMGR_ENTRY  entry;
  BOOLEAN bIsWriteInProgress;
  BOOLEAN bDone = FALSE;
  SPI_DEVS device;
  RESULT   result;
  BOOLEAN  timeOutDetected = FALSE;
  void* ptr;

  // Check output queue for pending EEPROM writes

  // Get (peek) at the head(oldest) entry
  SPIMgr_CircQueue_GetHead(&CQ_EEPromOutput, &pEntry);

  // Handle the top/head entry as long as not done and not in -direct-to-device mode.
  while (pEntry != NULL && !bDone && !m_bDirectToDev)
  {
    result = EEPROM_GetDeviceId(pEntry->addr, &device);

    if (result != DRV_OK)
    {
      // EEPROM failure, signal caller, remove entry
      //EEPromDetected = FALSE;
      *(pEntry->pResult) = IO_RESULT_READY;
      SPIMgr_CircQueue_Remove(&CQ_EEPromOutput, &entry );
    }
    else
    {
      // Determine if write is in progress.
      // NOTE: If the call to check write-in-progress times-out,
      // the value of IsWriteInProgress is unchanged.
      result = EEPROM_IsWriteInProgress(device, &bIsWriteInProgress);

      // Set/clear timeOutDetected flag.
      timeOutDetected = (DRV_SPI_TIMEOUT == result) ? TRUE : FALSE;

      switch (pEntry->opState)
      {
        case OP_STATUS_PENDING:
          // The item at the head of the of the queue is waiting to write.

          // If the call to check for write-in-progress completed ok AND
          // there is no write in progress, Write the block.
          if( !timeOutDetected)
          {
            if ( !bIsWriteInProgress )
            {
              // Set the write pointer to local-databuffer or pointer-to-data as defined.
              ptr = (pEntry->dataMethod == DATA_PASSING_PTR) ?
                                        (void*) pEntry->pData :
                                        (void*) pEntry->data;
              // Write the block to memory.
              // Change the operation state and let the caller know the
              // the write is being committed to the device.
              result = EEPROM_WriteBlock(device, pEntry->addr, ptr, pEntry->size);
              if( DRV_OK == result)
              {
                pEntry->opState  = OP_STATUS_IN_PROGRESS;
                *pEntry->pResult = IO_RESULT_TRANSMITTED;

                #ifdef DEBUG_SPIMGR_TASK
                /*vcast_dont_instrument_start*/
                GSE_DebugStr(NORMAL, TRUE, "0x%08X -> 0x%08X, SENT",ptr,pEntry->addr);
                /*vcast_dont_instrument_end*/
                #endif

              }
              else if (DRV_SPI_TIMEOUT == result)
              {
                // Timeout during WriteBlock
                timeOutDetected = TRUE;
              }
            }
            else
            {
              // The SPI bus busy (how?), 
              // we can't do anything so leave and check again next time
              bDone = TRUE;
            }
          }
          break;

        case OP_STATUS_IN_PROGRESS:
          if(!timeOutDetected)
          {
            if( !bIsWriteInProgress )
            {
              // Write completed.
              #ifdef DEBUG_SPIMGR_TASK
              /*vcast_dont_instrument_start*/
                ptr = (pEntry->dataMethod == DATA_PASSING_PTR) ?
                                          (void*) pEntry->pData :
                                          (void*)&(pEntry->data);
                GSE_DebugStr(NORMAL,TRUE,"0x%08X -> 0x%08X, COMPLETED: %d, Requester: %s (%s)",
                                        ptr, pEntry->addr, *pEntry->pResult,
                                        pEntry->pResult == &DefaultIoResultAddress ?
                                                                  "Direct-to-Dev" : "NVMGR",
                                        pEntry->dataMethod == DATA_PASSING_PTR ? "PTR":"VAL");
              /*vcast_dont_instrument_end*/
              #endif

              // Flag the sector write as completed.
              *(pEntry->pResult) = IO_RESULT_READY;

              // Delete top of queue.
              SPIMgr_CircQueue_Remove(&CQ_EEPromOutput, &entry );

              // Get the next entry off the queue(if any) and loop again
              SPIMgr_CircQueue_GetHead(&CQ_EEPromOutput, &pEntry);
            }
            else
            {
              // Write is in progress...
              // Break out and check it in the next frame.
              bDone = TRUE;
            }
          }
          break;

        default:
          FATAL("Unknown OP_STATE: %d", pEntry->opState);
          break;
      } //switch opState

      if (timeOutDetected)
      {
        // Write timed-out. Make system log entry and flag the result as finished.
        SPIMgr_LogError(SYS_ID_SPIMGR_EEPROM_WRITE_FAIL,result,pEntry->addr);
        *(pEntry->pResult) = IO_RESULT_READY;
        SPIMgr_CircQueue_Remove(&CQ_EEPromOutput, pEntry);
        // Get the next entry off the queue(if any) and loop again
        SPIMgr_CircQueue_GetHead(&CQ_EEPromOutput, &pEntry);
      }
    }
  }  // while queue not empty and not
}


/******************************************************************************
 * Function:    SPIMgr_GetTime
 *
 * Description: Function for accessing the current value of the RTC
 *              RTC
 *
 * Parameters:  [in][out] Ts: Pointer to a TIMESTRUCT for returning the current
 *                            "buffered" value of the RTC.
 *
 * Returns:     DRV_OK
 *
 * Notes:       All application should use this function instead
 *              of calling RTC_GetTime directly.
 *
 *****************************************************************************/
RESULT SPIMgr_GetTime(TIMESTRUCT *Ts)
{

  // If using direct-read mode get time from RTC, otherwise return the time
  // from or DT update storage.
  if (m_bDirectToDev)
  {
    m_resultGetRTC = RTC_ReadTime(&m_rtcTimeStruct);
    if(DRV_SPI_TIMEOUT == m_resultGetRTC)
    {
      SPIMgr_LogError(SYS_ID_SPIMGR_RTC_READ_FAIL, m_resultGetRTC, NULL);
    }
  }
  memcpy( Ts, &m_rtcTimeStruct, sizeof(TIMESTRUCT) );
  return m_resultGetRTC;
}

/*****************************************************************************
* Function:    SPIMgr_SetRTC
*
* Description: En-queue a buffer of time-data for writing to RTC.
*              This function buffers the operation to a circular queue and
*              performs the actual write using SPIMgr_UpdateEEPROM.
*
* Parameters:  [in]     Ts:        Time data to copy to RTC device
*              [in/out] ioResult:  Pointer to caller's IORESULT value to
*                                  communicate write outcome.
*
* Returns:     DRV_OK:                   Data sent to RTC
*              !DRV_OK:                  See ResultCodes.h
*
* Notes:
*
****************************************************************************/
RESULT SPIMgr_SetRTC(TIMESTRUCT *Ts, IO_RESULT* ioResult)
{
  SPIMGR_ENTRY entry;
  RESULT result;


  if (m_bDirectToDev)
  {
    // If queuing disabled as during startup or if an application has
    // disabled it, write directly to dev
    result = RTC_WriteTime(Ts);
    if (result == DRV_OK)
    {
      // Write to RTC successful, update our copy to service callers with the latest time.
      m_resultGetRTC = SPIMgr_GetTime(&m_rtcTimeStruct);
    }
    else if(DRV_SPI_TIMEOUT == result)
    {
      SPIMgr_LogError(SYS_ID_SPIMGR_RTC_WRITE_FAIL, result, NULL);
    }
  }
  else
  {
    // Attempt to en-queue msg onto circular queue.
    entry.opType   = OP_TYPE_WRITE;
    entry.opState  = OP_STATUS_PENDING;
    entry.addr     = NULL;
    entry.pResult  = (ioResult != NULL) ? ioResult : &DefaultIoResultAddress;
    *entry.pResult = IO_RESULT_PENDING;

    SPIMgr_SetupDataPassing( DATA_PASSING_VALUE, &entry, Ts, sizeof(TIMESTRUCT) );

    // Write the entry to the circular queue.
    // If queue is full, tell the caller "a write is in progress" so write-count
    // will be zero, and caller will retry.
    result = ( SPIMgr_CircQueue_Add(&CQ_RtcOutput, &entry) ) ?
               DRV_OK : DRV_RTC_ERR_CANNOT_SET_TIME;
  }
  return result;
}


/*****************************************************************************
 * Function:    SPIMgr_WriteEEPROM
 *
 * Description: En-queue a buffer of data for writing to EEPROM.
 *              This function buffers the operation to a circular queue and
 *              performs the actual write using SPIMgr_UpdateEEPROM.
 *
 * Parameters:  [in] DestAddr: Starting byte address to read the data from
 *                    EEPROM ON CS3 is from address 0x00000000 to EEPROM_SIZE
 *                    EEPROM ON CS5 is from address 0x10000000 to
 *                                 0x10000000+EEPROM_SIZE
 *              [in] Buf:      Buffer to store the outgoing bytes
 *                             (must be at least "Size" elements)
 *              [in] Size:     Number of bytes to get
 *         [in/out] ioResult:  Pointer to caller's IORESULT value to communicate write
 *                             outcome.
 *
 * Returns:     DRV_OK:                   Data sent to EEPROM
 *              DRV_EEPROM_NOT_DETECTED:  The EEPROM device was not detected
 *                                        at startup
 *
 * Notes:
 *
 ****************************************************************************/
RESULT SPIMgr_WriteEEPROM(UINT32 DestAddr,void* Buf,size_t Size,
                         IO_RESULT* ioResult)
{
  SPIMGR_ENTRY entry;
  RESULT result;
  SPI_DEVS device;
  BOOLEAN IsWriteInProgress;

  if (m_bDirectToDev)
  {
    // If not enabled, write directly to device
    result = EEPROM_GetDeviceId(DestAddr, &device);
    if(result == DRV_OK)
    {
      result = EEPROM_IsWriteInProgress(device, &IsWriteInProgress);
      if(result == DRV_OK)
      {
        if (!IsWriteInProgress)
        {
          result = EEPROM_WriteBlock(device, DestAddr, Buf, Size);
        }
        else
        {
          result = DRV_EEPROM_ERR_WRITE_IN_PROGRESS;
        }
      }

      // If an EEPROM call timed-out, log it.
      if(DRV_SPI_TIMEOUT == result)
      {
        SPIMgr_LogError(SYS_ID_SPIMGR_EEPROM_WRITE_FAIL, result, DestAddr);
      }
    }
  }
  else
  {
    // Set up SPIMGR_ENTRY to add operation onto circular queue.
    entry.opType   = OP_TYPE_WRITE;
    entry.opState  = OP_STATUS_PENDING;
    entry.pResult  = (ioResult != NULL) ? ioResult : &DefaultIoResultAddress;
    entry.addr     = DestAddr;
    SPIMgr_SetupDataPassing( DATA_PASSING_PTR, &entry, Buf, Size );

    // If queue is full, tell the caller "a write is in progress" so write-count
    // will be zero, and caller will wait.

    result = ( SPIMgr_CircQueue_Add(&CQ_EEPromOutput, &entry) )
      ? DRV_OK : DRV_EEPROM_ERR_WRITE_IN_PROGRESS;
  }
  return result;
}


/*****************************************************************************
 * Function:    SPIMgr_ReadBlock (EEPROM)
 *
 * Description: Reads data from the EEPROM.
 *
 * Parameters:  [in]  PhysicalOffset: Starting byte address to read the data from
 *                    EEPROM ON CS3 is from address 0x00000000 to EEPROM_SIZE
 *                    EEPROM ON CS5 is from address 0x10000000 to
 *                                 0x10000000+EEPROM_SIZE
 *              [out] Buf:      Buffer to store the received bytes
 *                        (must be at least "Size" elements)
 *              [in]  Size:     Number of bytes to get
 *
 * Returns:     DRV_OK:   Data sent to EEPROM
 *              !DRV_OK:  See ResultCodes.h
 *
 * Notes:
 *
 ****************************************************************************/
RESULT SPIMgr_ReadBlock (UINT32 PhysicalOffset, void* Buf, UINT32 Size)
{
  RESULT result;

  /*
  SPIMGR_ENTRY entry;

  // Set up SPIMGR_ENTRY to add operation onto circular queue.
  entry.opType   = OP_TYPE_READ;
  entry.opStatus = OP_STATUS_WAITING;
  entry.addr     = DestAddr;
  entry.size     = Size;
  memcpy(entry.data, 0, Size);

  // If queue is full, tell the caller "a write is in progress" so write-count
  // will be zero, and caller will wait.

  //  result = ( CircQueue_Add(&CQ_EEPromInput, &entry, opStatus) )
  //    ? DRV_OK : DRV_EEPROM_ERR_WRITE_IN_PROGRESS;

  */

  result = EEPROM_ReadBlock(PhysicalOffset, Buf, Size);
  if (DRV_SPI_TIMEOUT == result)
  {
    SPIMgr_LogError(SYS_ID_SPIMGR_EEPROM_READ_FAIL, result, PhysicalOffset);
  }

  return result;
}

/*****************************************************************************
* Function:    SPIMgr_WriteRTCNvRam
*
* Description: Write a block of data to the RTC NVRAM.
*
* Parameters:  [in] DestAddr:  Destination address (0-255) to write in in the
*                              RTC
*              [in] Buf:       Buffer containing the data to write.
*              [in] Size:      Number of bytes to write
*              [out] ioResult: Location to store the operation result
*
* Returns:     DRV_OK:         Data queued
*              !DRV_OK:        See ResultCodes.h
*
* Notes:
*
****************************************************************************/
RESULT SPIMgr_WriteRTCNvRam(UINT32 DestAddr,void* Buf,size_t Size,
                            IO_RESULT* ioResult)
{
  SPIMGR_ENTRY entry;
  RESULT result;

  if (m_bDirectToDev)
  {
    // Write directly to device
    result = RTC_WriteNVRam(DestAddr, Buf, Size);
    if(DRV_SPI_TIMEOUT == result)
    {
      SPIMgr_LogError(SYS_ID_SPIMGR_RTC_NVRAM_WRITE_FAIL, result, DestAddr);
    }
  }
  else
  {
    // Set up SPIMGR_ENTRY to add operation onto circular queue.
    entry.opType   = OP_TYPE_WRITE;
    entry.opState  = OP_STATUS_PENDING;
    entry.pResult  = (ioResult != NULL) ? ioResult : &DefaultIoResultAddress;
    entry.addr     = DestAddr;

    SPIMgr_SetupDataPassing( DATA_PASSING_VALUE, &entry, Buf, Size );

    // If queue is full, tell the caller "a write is in progress" so write-count
    // will be zero, and caller will wait.

    result = ( SPIMgr_CircQueue_Add(&CQ_RtcNvRamOutput, &entry) )
              ? DRV_OK : DRV_EEPROM_ERR_WRITE_IN_PROGRESS;
  }
  return result;

}

/*****************************************************************************
* Function:    SPIMgr_SensorTest
*
* Description: Returns the current data validity of the selected sensor
*
* Parameters:  [in] nIndex: SPI ADC channel index
*
* Returns:     TRUE if ACD channel data is valid
*              FALSE if data is invalid
*
* Notes:
*
****************************************************************************/
BOOLEAN SPIMgr_SensorTest (UINT16 nIndex)
{
  ASSERT(nIndex < SPI_MAX_DEV);
  return SPI_RuntimeInfo[nIndex].adcResult == DRV_OK;
}

/*****************************************************************************
* Function:    SPIMgr_GetACBusVoltage
*
* Description: Returns the stored value and result of the ACBusVoltage
*
* Parameters:  [out] Pointer to the caller's ACBusVoltage
*
* Returns:     RESULT of the call made from this task.
*
* Notes:      Use this function instead of  ADC_GetACBusVoltage
*
****************************************************************************/
RESULT SPIMgr_GetACBusVoltage  (FLOAT32* ACBusVoltage)
{
  *ACBusVoltage = SPI_RuntimeInfo[SPI_AC_BUS_VOLT].adcValue;
  return SPI_RuntimeInfo[SPI_AC_BUS_VOLT].adcResult;
}

/*****************************************************************************
* Function:    SPIMgr_GetACBattVoltage
*
* Description: Returns the stored value and result of the ACBattVoltage
*
* Parameters:  [out] Pointer to the caller's ACBattVoltage
*
* Returns:     RESULT of the call made from this task.
*
* Notes:      Use this function instead of  ADC_GetACBattVoltage
*
****************************************************************************/
RESULT SPIMgr_GetACBattVoltage (FLOAT32* ACBattVoltage)
{
  *ACBattVoltage = SPI_RuntimeInfo[SPI_AC_BAT_VOLT].adcValue;
  return SPI_RuntimeInfo[SPI_AC_BAT_VOLT].adcResult;

}

/*****************************************************************************
* Function:    SPIMgr_GetLiBattVoltage
*
* Description: Returns the stored value and result of the LiBattVotlage
*
* Parameters:  [out] Voltage: Pointer to the caller's LiBattVotlage
*
* Returns:     RESULT of the call made from this task.
*
* Notes:      Use this function instead of  ADC_GetLiBattVoltage
*
****************************************************************************/
RESULT SPIMgr_GetLiBattVoltage (FLOAT32* Voltage)
{
  *Voltage = SPI_RuntimeInfo[SPI_LIT_BAT_VOLT].adcValue;
  return SPI_RuntimeInfo[SPI_LIT_BAT_VOLT].adcResult;
}

/*****************************************************************************
* Function:    SPIMgr_GetBoardTemp
*
* Description: Returns the stored value and result of the BoardTemp
*
* Parameters:  [out] Pointer to the caller's BoardTemp
*
* Returns:     RESULT of the call made from this task.
*
* Notes:      Use this function instead of  ADC_GetBoardTemp
*
****************************************************************************/
RESULT SPIMgr_GetBoardTemp(FLOAT32* Temp)
{
  *Temp = SPI_RuntimeInfo[SPI_BOARD_TEMP].adcValue;
  return SPI_RuntimeInfo[SPI_BOARD_TEMP].adcResult;
}
/*****************************************************************************
* Function:    SPIMgr_SetupDataPassing
*
* Description: Sets up how data will be sourced for this for this SPIMGR_ENTRY
*              Data can be sent via a pointer to a static location managed by the caller,
*              or from a local buffer within the SPIMGR_ENTRY object
*
*
* Parameters:  [in] DATA_PASSING method: DATA_PASSING_VALUE, DATA_PASSING_PTR.
*              [in] entry: Pointer to the SPIMGR_ENTRY being configured.
*              [in] buffer: Pointer to the buffer to be referenced or copied based
*                   on DATA_PASSING method
*              [in] size: in bytes to be written/read.
* Returns:     void
*
* Notes:
*
****************************************************************************/
void SPIMgr_SetupDataPassing(DATA_PASSING method, SPIMGR_ENTRY* entry,
                             void* buffer, size_t size)
{
  switch (method)
  {
    case DATA_PASSING_VALUE:
      memcpy(entry->data, buffer, size);
      entry->size = size;
      entry->dataMethod = DATA_PASSING_VALUE;
      break;

    case DATA_PASSING_PTR:
      entry->pData = buffer;
      entry->size  = size;
      entry->dataMethod = DATA_PASSING_PTR;
      break;

    default:
      FATAL("Data Passing Method not valid: %d", method);
      break;
  }
}

/*****************************************************************************
* Function:    SPIMgr_LogError
*
* Description: Write a system log entry for a SPI error.
*              Only record one per power-on.
*
* Parameters:  [in] appId   - The SYS_APP_ID for the error. See x5A00 range in SystemLog.h
*              [in] result  - The RESULT code returned by the SPI driver.
*              [in] address - address being written if applicable. Otherwise NULL
*
* Returns:     void
*
* Notes:
*
****************************************************************************/
void SPIMgr_LogError(SYS_APP_ID appId, RESULT result, UINT32 address)
{
  SPIMGR_ERROR_LOG logEntry;
  UINT32 idNormal = appId - SYS_ID_SPIMGR_EEPROM_WRITE_FAIL;
  CHAR* SysIdString[SPI_TO_LOGS] =
  {
    "EEPROM_WRITE_FAIL",
    "EEPROM_READ_FAIL",
    "RTC_WRITE_FAIL",
    "RTC_READ_FAIL",
    "RTC_NVRAM_WRITE_FAIL",
    "BUS_VOLT_READ_FAIL",
    "BAT_VOLT_READ_FAIL",
    "LITBAT_VOLT_READ_FAIL",
    "BOARD_TEMP_READ_FAIL"
  };

  if ( FALSE == m_spiErrorRecorded[idNormal])
  {
      m_spiErrorRecorded[idNormal] = TRUE;
      logEntry.result  = result;
      logEntry.address = address;
      Flt_SetStatus(STA_NORMAL, appId, &logEntry, sizeof(SPIMGR_ERROR_LOG));

      GSE_DebugStr(NORMAL,TRUE, "SPI Timeout: %s", SysIdString[idNormal]);
  }
}


/*****************************************************************************
* Function:    SPIMgr_CircQueue_SetName
*
* Description: Sets the queue name field.
*
* Parameters:  cq - the queue to update
*              aName - the ptr to name string to update to
*              size - size of the queue
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void SPIMgr_CircQueue_SetName( CIRCULAR_QUEUE* cq, const CHAR* aName, UINT8 size)
{
  strncpy_safe(cq->name,sizeof(cq->name),aName,_TRUNCATE);

  ASSERT_MESSAGE((size > 0 && size <= CQ_MAX_ENTRIES),
                  "Circular Queue size %d is not valid", size);

  cq->queueSize = size;

}


/*****************************************************************************
* Function:    SPIMgr_CircQueue_Remove
*
* Description: "Pops" the oldest entry from referenced queue and returns the
*              data in  the provided entry pointer.
* Parameters:  [in] cq:     The queue to be popped.
*              [in/out] data:  Address of the Entry structure into which the
*                                contents of the entry will be copied.
* Returns:     None
*
* Notes:
*
****************************************************************************/
void SPIMgr_CircQueue_Remove( CIRCULAR_QUEUE* cq, SPIMGR_ENTRY* data )
{

#ifdef DEBUG_CIRCULAR_QUEUE
/*vcast_dont_instrument_start*/
  CHAR debugMsg[64];
/*vcast_dont_instrument_end*/
#endif

  if (cq->writeCount > cq->readCount)
  {
    *data = cq->buffer[cq->readFrom];

#ifdef DEBUG_CIRCULAR_QUEUE
/*vcast_dont_instrument_start*/
    snprintf(debugMsg, sizeof(debugMsg), "%s - Removed Entry %d", cq->name, cq->readFrom);
    GSE_DebugStr(NORMAL, TRUE, debugMsg);
/*vcast_dont_instrument_end*/
#endif

    ++cq->readCount;

    if ( ++cq->readFrom >= cq->queueSize)
    {
      cq->readFrom = 0;
    }
  }

#ifdef DEBUG_CIRCULAR_QUEUE
/*vcast_dont_instrument_start*/
    if(cq->writeCount == cq->readCount)
    {
      snprintf(debugMsg, sizeof(debugMsg), "%s -  QUEUE IS EMPTY",cq->name);
      GSE_DebugStr(NORMAL, TRUE, debugMsg);
    }
/*vcast_dont_instrument_end*/
#endif

}

/*****************************************************************************
* Function:    SPIMgr_CircQueue_Add
*
* Description: Accepts a pointer to a SPIMGR_QUEUE and adds it to the indicated
*              CIRCULAR buffer.
*              This function writes an i/o entry to a circular queue.
*              The write to EEPROM is performed by using SPIMgr_UpdateEEPROM.
*
* Parameters:  cq - ptr to queue to add entry
*              newEntry - ptr to entry data to add
*
* Returns:     DRV_OK:                   Data sent to EEPROM
*              DRV_EEPROM_NOT_DETECTED:  The EEPROM device was not detected
*                                        at startup
*
* Notes:       none
*
****************************************************************************/
BOOLEAN SPIMgr_CircQueue_Add(CIRCULAR_QUEUE* cq, const SPIMGR_ENTRY* newEntry)
{

  //CHAR debugMsg[64];
  INT16 depth;
  BOOLEAN result = TRUE;

  depth = cq->writeCount - cq->readCount;

  // Write to the queue if room
  if (depth < cq->queueSize)
  {
    // Add the entry to the circular queue.
    // Signal the caller that the result is pending.
    *(newEntry->pResult) = IO_RESULT_PENDING;

    // COPY the data into the queue.
    cq->buffer[cq->writeTo] = *newEntry;

#ifdef DEBUG_CIRCULAR_QUEUE
/*vcast_dont_instrument_start*/
    GSE_DebugStr(NORMAL, TRUE, "%s - Added Entry %d",cq->name, cq->writeTo);
/*vcast_dont_instrument_end*/
#endif
    ++cq->writeCount;
    // On last entry, wraparound.
    if (++cq->writeTo >= cq->queueSize)
    {
      cq->writeTo = 0;
    }
  }
  else
  {
    result = FALSE;
#ifdef DEBUG_CIRCULAR_QUEUE
/*vcast_dont_instrument_start*/
    GSE_DebugStr(NORMAL, TRUE, "%s -  QUEUE IS FULL",cq->name);
/*vcast_dont_instrument_end*/
#endif
  }
  return result;
}


/*****************************************************************************
* Function:    SPIMgr_CircQueue_Reset
*
* Description: Resets a queue to empty, unamed status.
*
* Parameters:  cq - queue to reset
*
* Returns:     none
*
* Notes:       none
*
****************************************************************************/
void SPIMgr_CircQueue_Reset(CIRCULAR_QUEUE* cq)
{
  INT16 i;

  for ( i = 0; i < CQ_MAX_ENTRIES; ++i)
  {
    memset( (void*)&cq->buffer[i], 0, sizeof(SPIMGR_ENTRY));
  }
  cq->readCount  = 0;
  cq->readFrom   = 0;
  cq->writeCount = 0;
  cq->writeTo    = 0;
  cq->name[0]    = '\0';
  cq->queueSize  = CQ_MAX_ENTRIES;
}

/*****************************************************************************
* Function:    SPIMgr_CircQueue_GetHead()
*
* Description: Returns a pointer to the top of the pass queue.
*
* Parameters:  [in] cq: pointer to the queue
*              [in] head: pointer to receive a pointer to the current top-of-queue
*
* Returns:     None
*
* Notes:
*
****************************************************************************/
void SPIMgr_CircQueue_GetHead(CIRCULAR_QUEUE* cq, SPIMGR_ENTRY** head )
{
  // if queue has entries to be read return pointer and the index of next.
  if (cq->writeCount > cq->readCount)
  {
    *head =  &cq->buffer[cq->readFrom];
  }
  else
  {
    *head = NULL;
  }
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: SPIManager.c $
 * 
 * *****************  Version 30  *****************
 * User: John Omalley Date: 12-11-13   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 1197 - Code Review Updates
 *
 * *****************  Version 29  *****************
 * User: John Omalley Date: 12-11-12   Time: 12:01p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Code Review Updates
 *
 * *****************  Version 28  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:53p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 27  *****************
 * User: Melanie Jutras Date: 12-10-10   Time: 2:00p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 *
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 25  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * FAST 2 Allow different queue sizes
 *
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 6/29/11    Time: 1:07p
 * Updated in $/software/control processor/code/system
 * SCR #767 Enhancement - BIT for Analog Sensors
 * Corrected function header comments
 *
 * *****************  Version 23  *****************
 * User: Contractor2  Date: 5/26/11    Time: 1:17p
 * Updated in $/software/control processor/code/system
 * SCR #767 Enhancement - BIT for Analog Sensors
 *
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 10/18/10   Time: 1:46p
 * Updated in $/software/control processor/code/system
 * SCR #946 Misc - Wrong SysLog in SPIMgr_UpdateRTC
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 9/29/10    Time: 11:51a
 * Updated in $/software/control processor/code/system
 * SCR 889 Modfix.  While closing this SCR, noticed multiple function
 * header commments were incomplete or did not match the function they
 * were associated with.
 *
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 9/23/10    Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR# 889 - Remove dead code
 *
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 9/10/10    Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR# 866 - SPI Timeouts cause DT overruns
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 *
 * *****************  Version 17  *****************
 * User: John Omalley Date: 7/30/10    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR 730 - Changed LogWriteSystem to Flt_SetStatus
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 7/29/10    Time: 9:26a
 * Updated in $/software/control processor/code/system
 * SCR #699 Processing when RTC_ReadTime() returns fail.
 *
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 7/20/10    Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Forgot to set flag indicating the SPI error had been
 * recorded already.
 *
 * *****************  Version 14  *****************
 * User: Jeff Vahue   Date: 7/17/10    Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR# 707 - Add TestPoints for code coverage.
 *
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 6/16/10    Time: 6:11p
 * Updated in $/software/control processor/code/system
 * SCR #623 Implement NV_IsEEPromWriteComplete
 *
 * *****************  Version 12  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #548 Strings in Logs
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 4/28/10    Time: 5:52p
 * Updated in $/software/control processor/code/system
 * SCR #572 - vcast changes
 *
 * *****************  Version 9  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #378  SCR #492
 *
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 3/15/10    Time: 4:14p
 * Updated in $/software/control processor/code/system
 * SCR# 481 - VectorCast comments, optimize SPIMgr_Task
 *
 * *****************  Version 5  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 3/11/10    Time: 11:05a
 * Updated in $/software/control processor/code/system
 * SCR# 481 - add Comments to inhibit VectorCast Coverage statements when
 * instrumenting code
 *
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 7:33p
 * Updated in $/software/control processor/code/system
 * SCR 67 expand direct-to-device mode for all managed devices.
 *
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #67 fix spelling error in funct name
 *
 * *****************  Version 1  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:27p
 * Created in $/software/control processor/code/system
 * SCR 67 Adding SPIManager task.
 *
 *
 *
 *****************************************************************************/

