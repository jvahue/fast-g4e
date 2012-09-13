#define TIMEHISTORY_BODY
/******************************************************************************
                    Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                      All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.c

    Description: Contains a buffer sufficient to hold 6 minutes of data from 128 sensor
                 objects at 20Hz.  The buffer stores configured sensors only, at the rate
                 configured in time history configuration.

                 Real-time sensor data and bufferd sensor data is recorded to ETM logs when
                 requested by another module.

                 The post-time history uses the millisecond counter and will not operate
                 after 49.7 days of continous operation.

    VERSION
    $Revision: 6 $  $Date: 12-09-11 1:59p $   

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                                */
/*****************************************************************************/
#include "timehistory.h"
#include "clockmgr.h"
#include "sensor.h"
#include "CfgManager.h"
/*********************************************************************************************/
/* Local Defines                                                                             */
/*********************************************************************************************/
//Each record is 1 timestamp, 1..128 sensor values, 1 bit for each validitiy flag
#define TH_SENSOR_CT_TO_REC_SZ(cnt) (sizeof(TIMESTAMP)+(sizeof(FLOAT32)*cnt)+(cnt/8 + 1))

//Convert number of seconds into
#define TH_SENSOR_REC_PER_SEC(s) (CfgMgr_ConfigPtr()->TimeHistoryConfig.SampleRate * s)

//TH_Task activities for writing TH records to log memeory
#define TH_TASK_SEARCH 0
#define TH_TASK_WRITE  1
#define TH_TASK_WAIT   2
/*********************************************************************************************/
/* Local Typedefs                                                                            */
/*********************************************************************************************/
/*Time history buffer consists of values of all configured sensors written to a buffer
  at a configured rate.

  The data buffer that keeps no less than 6 minutes of sensor data.  The data buffer contains
  time history data in the same format it is written to log memory so that a reference to it
  can be directly passed to DataMgr without copying the data, avoiding a resource intensive
  memcpy.  The size of the records in the data buffer varies based on the number of configured
  sensors, and so does the total number of records in the buffer.  Only the last 6 minutes is
  accessable through the API

  The control structure needed by TH to keep track of writing the buffer is seperate, and
  references the log data through a pointer to another buffer.

  Stats:
  6 minutes * 60 s/m *20Hz = 7200 records

  7200 records * ((4 bytes/value * 128 values + (128/8 bits/byte)) + 6 bytes/timestamp)
                           = 3844800B data buffer //TODO redo-math for new records with header
  */
typedef struct
{
  BOOLEAN write;    //Indicates [data] is pending to write to log memory
  BOOLEAN written;  //Indicates write to log memeory complete
  void*   data;     //Location of data to write to log memory (each block must be contigous)
}TH_RECORD_CTL_BLK;

/*RECORD_HEADER - Header for the time history data written to log memory*/
#pragma pack(1)
typedef struct
{
  UINT32 cs;              //record checksum (all bytes, excluding CS itself)
  TIMESTAMP ts_written;   //Timestamp - When the record is written to flash
  TIMESTAMP ts_recorded;  //Timestamp - Time when the sensor values were put in the TH buffer
  UINT16 size;            //Size - Size of bytes follwing this header (depends on # of sensors configured)
} TH_DATA_RECORD_HEADER;
#pragma pack

/*Time History sensor data buffering TODO: Define actual sizes
  Access is performed though PutBuf/GetBuf*/
typedef struct
{
  CHAR *dwrite_ptr;                     //data[] buffer write pointer
  TH_RECORD_CTL_BLK *cwrite_ptr;        //ctrl[] buffer write pointer
  TH_RECORD_CTL_BLK *csearch_ptr;       //ctrl[] buffer pointer for searching records.
  TH_RECORD_CTL_BLK ctrl[TH_PRE_HISTORY_REC_CNT];    //Holds control structures (semaphors etc.) for
  //records in data[]
  CHAR data[100];            //TH record to be written to data flash on request TODO Define real value
}TH_BUF;
/*********************************************************************************************/
/* Local Variables                                                                           */
/*********************************************************************************************/
static INT32  m_SensorList[MAX_SENSORS+1]; //List of configured sensors, 1 extra for a terminator.
static INT32  m_SensorCnt;
static TH_BUF m_THDataBuf;
static INT32  m_OpenCnt;
static UINT32 m_StopTime;
static TIMEHISTORY_CONFIG m_Cfg;
static UINT32 m_WriteReqCnt;   //Number of records to write to log mem since PO
                                     // used to track app BUSY/IDLE status
static UINT32 m_WrittenCnt;    //Number of records written to log mem
                                     // used to track app BUSY/IDLE status //TODO # records pending write good debug info
/*********************************************************************************************/
/* Local Function Prototypes                                                                 */
/*********************************************************************************************/
static void  TH_BuildRec(CHAR *rec);
static void  TH_PutNewRec(INT32 sz, BOOLEAN init_write);
static CHAR* TH_GetDataBufPtr(INT32 size);
static INT32 TH_FindRecToWriteToLog(TH_RECORD_CTL_BLK **start_blk);
static void  TH_WriteRecToLog(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt,
                               LOG_REQ_STATUS *lm_sta);
static void  TH_MarkTHRecordWritten(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt);
static void  TH_Task(void);
/*********************************************************************************************/
/* Public Functions                                                                          */
/*********************************************************************************************/

/**********************************************************************************************
 * Function:    TH_Init
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
void TH_Init ( void )
{
  INT32 list_idx = 0;
  INT32 i;
  //TCB TaskInfo;

  //Generate a list of sensors that are used
  for ( i = 0; i < MAX_SENSORS; i++ )
  {
    if ( TRUE == SensorIsUsed((SENSOR_INDEX)i) )
    {
      m_SensorList[list_idx++] = i;
      m_SensorCnt++;
    }
  }
  //Terminate List
  m_SensorList[list_idx++] = SENSOR_UNUSED;

  //Setup inital state for TH pre-history buffer (set written so it won't attempt to write an
  //empty control block
  for (i = 0; i < TH_PRE_HISTORY_REC_CNT; i++)
  {
    m_THDataBuf.ctrl[i].data    = NULL;
    m_THDataBuf.ctrl[i].write   = FALSE;
    m_THDataBuf.ctrl[i].written = TRUE;
  }

  m_THDataBuf.csearch_ptr = &m_THDataBuf.ctrl[0];
  m_THDataBuf.cwrite_ptr  = &m_THDataBuf.ctrl[0];
  m_THDataBuf.dwrite_ptr  = &m_THDataBuf.data[0];

  memcpy(&m_Cfg,&CfgMgr_ConfigPtr()->TimeHistoryConfig,sizeof(m_Cfg));
  m_OpenCnt  = 0;
  m_StopTime = 0;
  m_WriteReqCnt = 0;
  m_WrittenCnt = 0;

  // Create Time History Task - DT
  /*memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Time History",_TRUNCATE);
  TaskInfo.TaskID           = TH_Task_ID;
  TaskInfo.Function         = TH_Task;
  TaskInfo.Priority         = taskInfo[Sensor_Task].priority;
  TaskInfo.Type             = taskInfo[Sensor_Task].taskType;
  TaskInfo.modes            = taskInfo[Sensor_Task].modes;
  TaskInfo.MIFrames         = taskInfo[Sensor_Task].MIFframes;
  TaskInfo.Enabled          = TRUE;
  TaskInfo.Locked           = FALSE;

  TaskInfo.Rmt.InitialMif   = taskInfo[Sensor_Task].InitialMif;
  TaskInfo.Rmt.MifRate      = taskInfo[Sensor_Task].MIFrate;
  TaskInfo.pParamBlock      = NULL;
   */
}



/**********************************************************************************************
 * Function:    TH_Open
 *
 * Description: Start a time history recording.  Requesting to record time history previous to
 *              this call can be done by setting a time in the parameter.  Set the parameter to
 *              0 for no pre-history.
 *
 *              Each Open() call must eventually be followed by one Close() call, time history
 *              recording stop only when the number of calls to open equals the number of calls
 *              to close;
 *
 *              The full pre-history may not be recorded if the box has been powered on for
 *              a duration less than the requested time history.  In that case, the duration is
 *              since power on.
 *
 *
 * Parameters:  [in] pre_s: Pre-time history to record, range 0-360 seconds
 *
 * Returns:     none
 *
 * Notes:       reentrant ok
 *
 *          TODO: if DT TH task interrupts someone opening a TH record, will this cause
 *                multiple access to the th buffer issues?
 *
 *********************************************************************************************/
void TH_Open(INT32 pre_s)
{
#ifndef WIN32
  TH_RECORD_CTL_BLK* pre_ptr;
  INT32 i;
  INT32 int_save;
  m_OpenCnt++;

  pre_ptr = m_THDataBuf.cwrite_ptr;

  //range-check input parameter
  pre_s = MAX(0,pre_s);
  pre_s = MIN(TH_PRE_HISTORY_S,pre_s);

  //Mark records from now back to the number of seconds requested by the caller.
  //TEST: boundary condidions here, wrap around at end of buffer
  for (i = 0; i < TH_SENSOR_REC_PER_SEC(pre_s); i++ )
  {
    //Handle wrap to beginning of buffer
    if (pre_ptr < m_THDataBuf.cwrite_ptr)
    {
      pre_ptr = &m_THDataBuf.cwrite_ptr[TH_PRE_HISTORY_REC_CNT-1];
    }

    int_save = __DIR();
    m_WriteReqCnt += pre_ptr->write ? 0 : 1;  //Increment write req only if
                                                    //write is not already set
    __RIR(int_save);

    pre_ptr->write = TRUE;
    pre_ptr--;
  }
  #endif
}



/**********************************************************************************************
 * Function:    TH_Close
 *
 * Description:  Stop time history "during" recording and set a duration to continue
 *              "post-history" recording after the call to close.  During recording only stops
 *               when all callers to Open() have called Close().
 *
 *               Number of callers calling open and close is tracked with a counter, so Open
 *               must be called once, and then Close must be called, only once, and the end of
 *               the duration.
 *
 * Parameters:   [in] time_s: Pre-time history to record, range 0-360 seconds
 *
 * Returns:
 *
 * Notes:       reentrant ok
 *
 *********************************************************************************************/
void TH_Close(INT32 time_s)
{
  INT32 int_save;

  m_OpenCnt -= (m_OpenCnt == 0) ? 0 : 1;

  //range-check input parameter
  time_s = MAX(0,time_s);
  time_s = MIN(TH_POST_HISTORY_S,time_s);

  int_save = __DIR();
  if(m_StopTime < CM_GetTickCount() + time_s)
  {
    m_StopTime = CM_GetTickCount() + time_s;
  }
  __RIR(int_save);

}



/**********************************************************************************************
 * Function:    TH_FSMAppBusyGetState   | IMPLEMENTS GetState() INTERFACE to
 *                                      | FAST STATE MGR
 *
 * Description: Returns the busy/not busy state of the Time History module
 *              based on if the program is busy:
 *
 *              1) During TH is being recorded
 *              2) Post TH is being recorded
 *              3) Waiting for TH records to complete writing to flash
 *
 *
 * Parameters:  [in] param: ignored.  Included to match FSM call sig.
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
BOOLEAN TH_FSMAppBusyGetState(INT32 param)
{
  return FALSE;//(if during or post, if log writing is active);
}

/******************************************************************************
 * Function:     TH_GetBinaryHeader
 *
 * Description:  Retrieves the binary header for the time history
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
UINT16 TH_GetBinaryHeader ( void *pDest, UINT16 nMaxByteSize )
{
   // Local Data
   TH_HDR  header;
   INT8    *pBuffer;
   UINT16  nRemaining;
   UINT16  nTotal;

   // Initialize Local Data
   pBuffer    = (INT8 *)pDest;
   nRemaining = nMaxByteSize;
   nTotal     = 0;

   memset ( &header, 0, sizeof(header) );

   header.sampleRate = m_Cfg.SampleRate;

   // Increment the total number of bytes and decrement the remaining
   nTotal     += sizeof (header);
   nRemaining -= sizeof (header);

   // Copy the TH header to the buffer
   memcpy ( pBuffer, &header, nTotal );
   // Return the total number of bytes written
   return ( nTotal );
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/**********************************************************************************************
 * Function:     TH_Task
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
static void TH_Task ( void )
{
  CHAR* rec_buf;
  BOOLEAN is_post_during;
  static UINT32 frame_cnt;
  static UINT32 task_state = TH_TASK_SEARCH;
  static TH_RECORD_CTL_BLK *blk_start_ptr;   //First record to be written to logs
  static INT32              blk_write_cnt;   //# of blocks after first record to write to logs
  static LOG_MNG_STATE      log_mgr_status;

  //TEST: Make sure this mess of offset and sample rate is correct
  //if (Current frame - Offset) is a multiple of the configured sample rate
  if ( ( (  (frame_cnt++ - ( m_Cfg.nSampleOffset_ms/MIF_PERIOD_mS )) %
            (m_Cfg.SampleRate/MIFs_PER_SECOND)  )              == 0)  )
  {
    //For # of sensors, read value/validity and build TH record
    rec_buf = TH_GetDataBufPtr(TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt));
    TH_BuildRec(rec_buf);

    //Write record to buffer, set the write flag if post/during requests writing of current data
    //Increment write request count is write flag is to be set
    is_post_during = (CM_GetTickCount() < m_StopTime) || (m_OpenCnt != 0);
    m_WriteReqCnt += is_post_during ? 1 : 0;
    TH_PutNewRec(TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt),is_post_during);
  }
  else  //Use other frames to write TH logs
  {
    switch(task_state)
    {
      case TH_TASK_SEARCH:
        blk_write_cnt = TH_FindRecToWriteToLog(&blk_start_ptr);
        if(blk_write_cnt != 0)
        {
          task_state = TH_TASK_WRITE;
        }
      break;


      case TH_TASK_WRITE:
        TH_WriteRecToLog(blk_start_ptr,blk_write_cnt,&log_mgr_status);
      break;

      case TH_TASK_WAIT:
        if(log_mgr_status == LOG_REQ_COMPLETE)
        {
          TH_MarkTHRecordWritten(blk_start_ptr,blk_write_cnt);
          task_state = TH_TASK_SEARCH;
        }
        else if(log_mgr_status == LOG_REQ_FAILED)
        {
          task_state = TH_TASK_SEARCH;
          //TODO: Write a system log w/ timestamps, limit occurances
        }
      break;
    }
  }
}



/**********************************************************************************************
 * Function:     TH_FindRecToWriteToLog
 *
 * Description:  Search the time history buffer for time history records pending write to
 *               log memory.
 *
 *               The number of records searched per call is set in TH_REC_SEARCH_PER_FRAME.
 *               Each call will pick up where it left off.
 *
 *               Uses global csearch ptr to keep track of current position, returns:
 *
 *               1. The position of the first record
 *               2. A count of consecutive records after the first that need to be written
 *
 * Parameters:   none
 *
 * Returns:      none
 *
 * Notes:
 *********************************************************************************************/
static INT32 TH_FindRecToWriteToLog(TH_RECORD_CTL_BLK **start_blk)
{
  INT32 i;
  INT32 blk_cnt = 0;

  for( i = 0; i < TH_REC_SEARCH_PER_FRAME; i++)
  {
    if(m_THDataBuf.csearch_ptr->write && !m_THDataBuf.csearch_ptr->written)
    {
      //Found first TH record needing to be written
      *start_blk = m_THDataBuf.csearch_ptr++;
      //Count up other logs needing to be written up to the max per frame and as long as they
      // are consecutive
      //TEST: verify blk_cnt and search ptr are correct when the loop exits on blk_cnt == MAX or
      //      non-consectuive TH.
      for( blk_cnt = 1; (blk_cnt < TH_MAX_LOG_WRITE_PER_FRAME) &&
                        (m_THDataBuf.csearch_ptr->write && !m_THDataBuf.csearch_ptr->written);
                         blk_cnt++ )
      {
        m_THDataBuf.csearch_ptr++;
      }
      break;
    }
    else
    {
      m_THDataBuf.csearch_ptr++;
    }
  }
  return blk_cnt;
}



/**********************************************************************************************
 * Function:     TH_WriteRecToLog
 *
 * Description:  Write a set of TH records from the TH data buffer to log memory.  The input
 *               parameter specifies a list of records to be written.
 *
 *               The current timestamp and checksum is added to each record header
 *
 * Parameters:
 *
 * Returns:       none
 *
 * Notes:         Max write size is dependant on number of configured sensors and the value of
 *                TH_MAX_LOG_WRITE_PER_FRAME.  For 128 sensors each block will be 564 bytes.
 *                At a MAX of 10 logs/frame this is 5,640 bytes.  TODO: Might be too much.
 *********************************************************************************************/
static void TH_WriteRecToLog(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt,
                             LOG_REQ_STATUS *lm_sta)
{
  INT32 i;
  TIMESTAMP now;
  TH_DATA_RECORD_HEADER* log_hdr;
  INT32 record_sz;
  UINT32 log_write_cs = 0;
  LOG_SOURCE source;

  CM_GetTimeAsTimestamp(&now);
  //TODO: Timing for this activity per log

  //Do checksums/timestamps
  //TEST: Need to verify log_write_cs is correct, b/c is not checked by LM and won't fail even
  // if it is wrong
  for(i = 0; i < blk_write_cnt; i++)
  {
    log_hdr             = (TH_DATA_RECORD_HEADER*)start_blk[i].data;
    log_hdr->ts_written = now;
    record_sz           = log_hdr->size+sizeof(*log_hdr);
#pragma ghs nowarning 1545  //supress packed structure alignment warning
    log_hdr->cs = ChecksumBuffer(&log_hdr->size, record_sz-sizeof(log_hdr->cs),0xFFFFFFFF);
    log_write_cs += ChecksumBuffer(&log_hdr->cs,sizeof(log_hdr->cs),0xFFFFFFFF) + log_hdr->cs;
#pragma ghs endnowarning
  }

  //TEST: Verify log format and boundaries are correct, no extra or missing bytes.
  //Write logs
  source.ID = APP_ID_TIMEHISTORY_ETM_LOG;
  LogWrite(LOG_TYPE_ETM,
           source,
           TH_LOG_PRIORITY,
           log_hdr,
           TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt)*blk_write_cnt,
           log_write_cs,
           lm_sta);
}



/**********************************************************************************************
 * Function:     TH_MarkTHRecordWritten
 *
 * Description:
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *********************************************************************************************/
void TH_MarkTHRecordWritten(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt)
{
  INT32 i;

  for (i = 0; i < blk_write_cnt; i++)
  {
    start_blk[i].written = TRUE;
  }
  m_WrittenCnt += blk_write_cnt;
}



/**********************************************************************************************
 * Function:     TH_BuildRecord
 *
 * Description:  Creates a time history record containing a timestamp, sensor value and sensor
 *               validity at the location pointed to rec.  Format and size is dependant on the
 *               number of sensors configured.
 *
 *               Build the record in the follwing format:
 *
 *               |Log/Record Header|ValidFlags|Value 0|...|Value n|
 *
 *               where:
 *               Log/Record Header  = Checksum and timestamp information, in the format of the
 *                                    log.
 *               TimeStamp          = 48-bit Altair Timestamp
 *               ValidFlags         = Variable size sensor validity booleans as packed bitfields
 *               Value 0.. Value n  = Variable size list of float formatted sensor values of
 *                                    configured sensors
 *
 *               In referencing each item in the record:
 *
 *               |Log/Record Header|ValidFlags|Value 0|...|Value n|
 *                                  ^               ^
 *                                  |               |
 *                                  BitPtr          ValuePtr
 *                                  buf+sizeof(hdr) buf+6+sizeof(ValidFlags)
 *
 * Parameters:  [out] rec: Location to store the built record.  Size required is dependant on
 *                         the number of sensors configured.
 *
 * Returns:
 *
 * Notes:
 *********************************************************************************************/
static void TH_BuildRec(CHAR *rec)
{
  // Local Data
  CHAR *bit_ptr;
  CHAR *value_ptr;
  INT32 i;
  TH_DATA_RECORD_HEADER *hdr = (void*)rec;

  memset(hdr,0,sizeof(*hdr));

  //Set pointer to ValidFlag bitfields after the header in the buffer
  bit_ptr   = rec + sizeof(*hdr);

  //Set pointer to the sensor values after the ValidFlag bitfields
  value_ptr = bit_ptr + (m_SensorCnt / 8) + 1;

  //Add timestamp and size to header, the checksum and record time will be added later if this
  //record is recorded to FLASH.
  CM_GetTimeAsTimestamp(&hdr->ts_recorded);
  hdr->size = TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt);


  //Read sensor values/validity and build the rest of the buffer
  //TEST: Check buffer pointer math for the number of bit field bytes, and validity bit math
  // for boundary conditions
  for (i = 0; (i < MAX_SENSORS) && m_SensorList[i] != SENSOR_UNUSED; i++)
  {
    //Sensor Values
    *value_ptr = SensorGetValue((SENSOR_INDEX)m_SensorList[i]);
    value_ptr += sizeof(FLOAT32);

    //Sensor Validity
    if (SensorIsValid((SENSOR_INDEX)m_SensorList[i]))
    {
      *bit_ptr |= 1 << (i%8);
    }
    else
    {
      *bit_ptr &= ~(1 << (i%8));
    }

    //Increment valid bit pointer on byte each 8 sensors.
    bit_ptr += ((i / 8) > 0) && (i % 8 == 0) ? 1 : 0;
  }
}



/**********************************************************************************************
 * Function:     TH_PutNewRec
 *
 * Description:  Add a new time history record to the m_THDataBuf time history buffer.  This
 *               routine modifies the control buffer, adding a reference to the new th_rec data,
 *               and set the control flags "write" and "written" to zero. Then, it increments the
 *               data ptr
 *
 *               See TH_GetDataBuf
 *
 * Parameters:   [in] sz          - size of the data written to the data buffer
 *               [in] init_write  - Initial state of the write flag
 *                                  TRUE:  Initial state is to write this record to log mem
 *                                         (meaning it will be written)
 *                                  FALSE: Initial state is not to write this record to log mem
 *                                         (meaning it will be written only if the write flag
 *                                          is set later)
 *
 * Returns:
 *
 * Notes:        not reentrant
 *
 *********************************************************************************************/
static void TH_PutNewRec(INT32 sz, BOOLEAN init_write)
{
  //Setup control pointer to store the new TH
  if (m_THDataBuf.cwrite_ptr == &m_THDataBuf.ctrl[TH_PRE_HISTORY_DATA_SZ])
  {
    m_THDataBuf.cwrite_ptr = &m_THDataBuf.ctrl[0];
  }

  //Add control data for the new time history record, increment pointer
  m_THDataBuf.cwrite_ptr->data = m_THDataBuf.dwrite_ptr;
  m_THDataBuf.cwrite_ptr->write = FALSE;
  m_THDataBuf.cwrite_ptr->written = FALSE;
  m_THDataBuf.cwrite_ptr++;

  //Data and control write complete, increment write pointer to the next record location.
  m_THDataBuf.dwrite_ptr += sz;
}



/**********************************************************************************************
 * Function:     TH_GetDataBufPtr
 *
 * Description:  Returns a pointer to a location in the time history data buffer that is at
 *               least size bytes long.
 *
 *               The write to the buffer must be completed by calling PutCtrlBuf, which creates
 *               a data control record and increments the data buffer write pointer.
 *
 * Parameters:   [in] size: Number of bytes the caller needs to store in the TH buffer
 *
 * Returns:      Pointer to a location in the TH buffer that the caller may store data.
 *
 * Notes:        not reentrant
 *
 *********************************************************************************************/
CHAR* TH_GetDataBufPtr(INT32 size)
{
  //Wrap to beginning if the new TH record won't fit before the end of the buffer
  // size <= BufferEndAddr - BufferWriteAddr
  if (size > ((m_THDataBuf.data + sizeof(m_THDataBuf.data)) - m_THDataBuf.dwrite_ptr))
  {
    m_THDataBuf.dwrite_ptr = m_THDataBuf.data;
  }

  return m_THDataBuf.dwrite_ptr;
}


/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.c $
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 12-09-11   Time: 1:59p
 * Updated in $/software/control processor/code/application
 * SCR 1107 - Added Binary ETM Header Function
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 9/06/12    Time: 5:58p
 * Updated in $/software/control processor/code/application
 * SCR #1107 Time History implementation changes
 *
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 12:43p
 * Updated in $/software/control processor/code/application
 * SCR# 1142
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
 *****************************************************************************/
