#define TIMEHISTORY_BODY
/******************************************************************************
                    Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
                      All Rights Reserved. Proprietary and Confidential.

    File:        TimeHistory.c

    Description: Contains a buffer sufficient to hold 6 minutes of data from
                 128 sensor objects at 20Hz.  The buffer stores configured
                 sensors only, at the rate configured in time history
                 configuration.

                 Real-time sensor data and bufferd sensor data is recorded to
                 ETM logs when requested by another module.

                 The post-time history uses the millisecond counter and will
                 not operate after 49.7 days of continous operation.

    VERSION
    $Revision: 12 $  $Date: 12-11-12 9:49a $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "timehistory.h"
#include "clockmgr.h"
#include "sensor.h"
#include "CfgManager.h"
#include "GSE.h"
#include "user.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define TH_SENSOR_CT_TO_VALID_BITS_SZ(cnt) ( cnt/8 + MIN((cnt % 8),1) )

//Each record is 1 timestamp, 1..128 sensor values, 1 bit for each validitiy flag
#define TH_SENSOR_CT_TO_REC_SZ(cnt) (sizeof(TH_DATA_RECORD_HEADER) + (sizeof(FLOAT32)*cnt) \
                                    + TH_SENSOR_CT_TO_VALID_BITS_SZ(cnt) )

//Convert number of seconds into
#define TH_SENSOR_REC_PER_SEC(s) (CfgMgr_ConfigPtr()->TimeHistoryConfig.sample_rate * s)

//Data buffer size (Number of records * (size of record header+size of biggest data record))
#define TH_PRE_HISTORY_DATA_BUFFER_SZ  \
                                (TH_PRE_HISTORY_REC_CNT * TH_SENSOR_CT_TO_REC_SZ(MAX_SENSORS))

//TH_Task activities for writing TH records to log memeory
#define TH_TASK_SEARCH 0
#define TH_TASK_WRITE  1
#define TH_TASK_WAIT   2

//Write flags for m_THDataBuf.write
#define TH_WRITE_NONE  0
#define TH_WRITE_START 1
#define TH_WRITE_STOP  2

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
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

  7200 records * ((4 bytes/value * 128 values + (128/8 bits/byte)) + 20 bytes/header)
                           = 3945600B data buffer
  */
typedef struct
{
  BOOLEAN write;    //Indicates [data] is pending to write to log memory
  BOOLEAN written;  //Indicates write to log memeory complete
  void*   data;     //Location of data to write to log memory (each block must be contigous)
}TH_RECORD_CTL_BLK;

/*RECORD_HEADER - Header for the time history data written to log memory.  The ETM Header fields
  are structure and format are mandated outside of the Time History definition and should always
  match that definition.  The TH Specific timestamp is recorded after the ETM Header fields */
#pragma pack(1)
typedef struct
{
  UINT32    cs;           //ETM Header: record checksum - (all bytes, excluding CS itself)
  TIMESTAMP ts_written;   //ETM Header: Timestamp - When the record is written to flash
  UINT16    id;           //ETM Header: LogID - Should always be the TH log id
                          //                   (APP_ID_TIMEHISTORY_ETM_LOG) from systemlog.h
  UINT16    size;         //ETM Header: Size - Size of bytes follwing this header (depends on # of
                          //                    sensors configured)

  TIMESTAMP ts_recorded;  //TH Specific: Timestamp - Time when the sensor values were put
                          //                         in the TH buffer
} TH_DATA_RECORD_HEADER;
#pragma pack

/*Time History sensor data buffering
  Access is performed though PutBuf/GetBuf*/
typedef struct
{
  BYTE *dwrite_ptr;                     //data[] buffer write pointer
  TH_RECORD_CTL_BLK *cwrite_ptr;        //ctrl[] buffer write pointer
  TH_RECORD_CTL_BLK *csearch_ptr;       //ctrl[] buffer pointer for searching records.
  TH_RECORD_CTL_BLK ctrl[TH_PRE_HISTORY_REC_CNT];   //Holds control structures
                                                    //(semaphors etc.) for records in data[]
  BYTE data[TH_PRE_HISTORY_DATA_BUFFER_SZ]; //TH record to be written to data flash on request
}TH_BUF;

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static INT32  m_SensorList[MAX_SENSORS+1]; //List of configured sensors, +1 for a terminator.
static UINT16 m_SensorCnt;
static TH_BUF m_THDataBuf;
static UINT32 m_OpenCnt;
static UINT32 m_StopTime;
static TIMEHISTORY_CONFIG m_Cfg;
static UINT32 m_WriteReqCnt;   //Number of records to write to log mem since PO
                                     // used to track app BUSY/IDLE status
static UINT32 m_WrittenCnt;    //Number of records written to log mem
                                     // used to track app BUSY/IDLE status

/*write array data*/
static UINT32 m_req_cnt;


/*Busy/Not Busy event data*/
static INT32 m_event_tag;
static void (*m_event_func)(INT32 tag,BOOLEAN is_busy) = NULL;


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void  TH_BuildRec(BYTE *rec);
static void  TH_PutNewRec(INT32 sz, BOOLEAN init_write);
static BYTE* TH_GetDataBufPtr(INT32 size);
static INT32 TH_FindRecToWriteToLog(TH_RECORD_CTL_BLK **start_blk);
static void  TH_WriteRecToLog(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt,
                               LOG_REQ_STATUS *lm_sta);
static void  TH_MarkTHRecordWritten(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt);
static void  TH_Task ( void* pParam );

#include "TimeHistoryUserTables.c"
/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

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
  TCB task_info;
  USER_MSG_TBL root_msg = {"TH", time_history_root, NULL, NO_HANDLER_DATA};

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
    m_THDataBuf.ctrl[i].write   = TH_WRITE_NONE;
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
  memset(&task_info, 0, sizeof(task_info));
  strncpy_safe(task_info.Name, sizeof(task_info.Name),"Time History",_TRUNCATE);
  task_info.TaskID           = TH_Task_ID;
  task_info.Function         = TH_Task;
  task_info.Priority         = taskInfo[TH_Task_ID].priority;
  task_info.Type             = taskInfo[TH_Task_ID].taskType;
  task_info.modes            = taskInfo[TH_Task_ID].modes;
  task_info.MIFrames         = taskInfo[TH_Task_ID].MIFframes;
  task_info.Enabled          = (TH_OFF != m_Cfg.sample_rate);
  task_info.Locked           = FALSE;

  task_info.Rmt.InitialMif   = taskInfo[TH_Task_ID].InitialMif;
  task_info.Rmt.MifRate      = taskInfo[TH_Task_ID].MIFrate;
  task_info.pParamBlock      = NULL;

  TmTaskCreate(&task_info);

  User_AddRootCmd(&root_msg);
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
 *
 *
 *********************************************************************************************/
void TH_Open(UINT32 pre_s)
{
  TH_RECORD_CTL_BLK* stop_ptr, *start_ptr;
  INT32 int_save;
  INT32 tmp_req_cnt;

  m_OpenCnt++;
  //range-check input parameter
  pre_s = MIN(TH_PRE_HISTORY_S,pre_s);

  //Set stop point to marking pre-history.  cwrite_ptr is pointing to the next
  //location to write, so back it up (with wrap-around) to the previously written location
  stop_ptr = m_THDataBuf.cwrite_ptr == &m_THDataBuf.ctrl[0] ?
            &m_THDataBuf.ctrl[TH_PRE_HISTORY_REC_CNT-1] : m_THDataBuf.cwrite_ptr-1;

  //Set start point to begin this time history section.  Start pre_s seconds before
  //the stop ptr.  Must past the beginning of the buffer if necessary.
  //Wrap case is:
  //END_OF_BUFFER - (PRE_HIST_RECORDS_REQUESTED - (BEGINNING_OF_BUFFER - STOP_PTR))
  start_ptr = (stop_ptr - TH_SENSOR_REC_PER_SEC(pre_s)) < &m_THDataBuf.ctrl[0] ?
    &m_THDataBuf.ctrl[TH_PRE_HISTORY_REC_CNT-1] -
    (TH_SENSOR_REC_PER_SEC(pre_s) - (&m_THDataBuf.ctrl[0] - stop_ptr)) :
    (stop_ptr - TH_SENSOR_REC_PER_SEC(pre_s));

  GSE_DebugStr(NORMAL,TRUE,"TH: Open; pre-hist: %ds; open: %d",pre_s,m_OpenCnt);

  int_save = __DIR();

  /*Assumptions:
    Start markers define where to start writing the buffer to logs
    Stop markers define where to stop writing the buffer to logs

    Part of req_count defines the number of start-stop pairs in the array
    Task will start writing logs one it encounters a start
    Task will decrement the req_count for each stop marker it consumes
    When the req_count goes to zero, the Task will stop writing logs

    It is possible to overlap endpoints, so start or stop markers are overwritten
    by subsequent requests.  This changes the 1 and only 1 start of each stop relationship, so
    a seperate counter "req_count" defines the number of requests.  The Open call increments req_count when it "Produces" stops,
    the Task "Consumes" stops.  The req_count is decremented by the Task when it consumes the stop and the write is complete.
    req_count also defines when to continue recording for during and post history requests. (NOTE: HOw exactly will this work.*/
  /*Place START marker
    Need to consider 3 cases:
    */
  tmp_req_cnt = m_req_cnt;
  switch(start_ptr->write)
  {
    //1. NEW REQUEST:             EMPTY write flag, just write the marker, bump req_count
    case TH_WRITE_NONE:
      start_ptr->write = TH_WRITE_START;
      m_req_cnt++;
      break;

    //2. RESIZE EXISTING REQUEST: STOP marker is in the position, remove STOP and place new STOP in mutex, don't bump req_count
    case TH_WRITE_STOP:
      start_ptr->write = TH_WRITE_NONE;
      break;

    //3. NEW REQUEST:             START marker is in the position, don't place a marker, bump req_count
    case TH_WRITE_START:
      m_req_cnt++;
      break;
  }

  switch(stop_ptr->write)
  {
    //1. NEW REQUEST:             EMPTY write flag, just write the STOP marker
    case TH_WRITE_NONE:
      stop_ptr->write = TH_WRITE_STOP;
      break;

    //2. RESIZE EXISTING REQUEST: START marker is in the position, remove START, don't bump req_count
    case TH_WRITE_START:
      stop_ptr->write = TH_WRITE_NONE;
      m_req_cnt = tmp_req_cnt;
      break;

    //3. NEW REQUEST:             STOP marker is in the position, decrement req_count*/
    case TH_WRITE_STOP:
      m_req_cnt--;
      break;

  }

  __RIR(int_save);

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
void TH_Close(UINT32 time_s)
{
  INT32 int_save;

  m_OpenCnt -= (m_OpenCnt == 0) ? 0 : 1;

  GSE_DebugStr(NORMAL,TRUE,"TH: Close; post-hist: %ds; open: %d",time_s,m_OpenCnt);
  //range-check input parameter
  time_s = MIN(TH_POST_HISTORY_S,time_s);

  int_save = __DIR();
  if(m_StopTime < CM_GetTickCount() + (time_s*1000)  )
  {
    m_StopTime = CM_GetTickCount() + (time_s*1000);
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
 * Returns:     TRUE: Time History is busy, 1 or more of the 3 conditions are true
 *              FALSE: Time History is idle, none of the 3 conditions above are true
 *
 * Notes:
 *
 *********************************************************************************************/
BOOLEAN TH_FSMAppBusyGetState(INT32 param)
{
  return (m_OpenCnt != 0)                 ||
         (m_StopTime > CM_GetTickCount()) ||
         (m_WriteReqCnt != m_WrittenCnt);
}



/**********************************************************************************************
 * Function:    TH_SetRecStateChangeEvt
 *
 * Description: Set the callback fuction to call when the busy/not busy status of the
 *              Time History module changes
 *
 *
 *
 * Parameters:  [in] tag:  Integer value to use when calling "func"
 *              [in] func: Function to call when busy/not busy status changes
 *
 * Returns:      none
 *
 * Notes:       Only remembers one event handler.  Subsequent calls overwrite the last
 *              event handler.
 *
 *********************************************************************************************/
void TH_SetRecStateChangeEvt(INT32 tag,void (*func)(INT32,BOOLEAN))
{
  m_event_func = func;
  m_event_tag  = tag;
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

   header.sampleRate = m_Cfg.sample_rate;

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
 * Description:  Time history task.  Performs the following:
 *
 *               -Save sensor data to the pre-history buffer at the
 *                configured rate.
 *                  -Pending the records for write if during or post-history
 *                   is active.
 *               -Search for time history records in the pre-history buffer
 *                pending write.
 *               -Writing time history records to Log Memory.
 *
 * Parameters:   [in] pParam: Not used, to match TM call signature
 *
 * Returns:
 *
 * Notes:
 *
 *********************************************************************************************/
static void TH_Task ( void* pParam )
{
  BYTE* rec_buf;
  BOOLEAN is_post_during;
  static UINT32 frame_cnt  = 0;
  static UINT32 task_state = TH_TASK_SEARCH;
  static TH_RECORD_CTL_BLK *blk_start_ptr;   //First record to be written to logs
  static INT32              blk_write_cnt;   //# of blocks after first record to write to logs
  static LOG_REQ_STATUS     log_mgr_state;
  static BOOLEAN            is_busy = FALSE;

  if (   (  (frame_cnt++ - ( m_Cfg.sample_offset_ms/MIF_PERIOD_mS )) %
            (MIFs_PER_SECOND/m_Cfg.sample_rate)  )              == 0  )
  {
    //For # of sensors, read value/validity and build TH record
    rec_buf = TH_GetDataBufPtr(TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt));
    TH_BuildRec(rec_buf);

    //Write record to buffer, set the write flag if post/during requests writing of current
    //data. Increment write request count is write flag is to be set
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
        TH_WriteRecToLog(blk_start_ptr,blk_write_cnt,&log_mgr_state);
        task_state = TH_TASK_WAIT;
      break;

      case TH_TASK_WAIT:
        if(log_mgr_state == LOG_REQ_COMPLETE)
        {
          TH_MarkTHRecordWritten(blk_start_ptr,blk_write_cnt);
          task_state = TH_TASK_SEARCH;
        }
        else if(log_mgr_state == LOG_REQ_FAILED)
        {
          task_state = TH_TASK_SEARCH;
        }
      break;

      default:
        FATAL("TH: Task State %d, not valid",task_state);
      break;
    }
  }
  //Update TH recording busy/not busy status on change only.
  if(((m_req_cnt == 0) || is_post_during) != is_busy)
  {
    is_busy = ((m_req_cnt == 0) || is_post_during);
    if(m_event_func != NULL)
    {
      m_event_func(m_event_tag,is_busy);
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
 *               3. NO BUFFER-WRAPPING RECORDS ARE RETURNED. This makes down the line
 *                  processing simpler as logs are written straight from the buffer memory
 *                  and a wrapped buffer cannot be passed to log write
 *
 * Parameters:   [out] start_blk: Returns a pointer to the start of the control blocks to be
 *                                written to log memory
 *
 *
 * Returns:      INT32: Count of records to be written to log memory including and following
 *                      the start_blk
 *
 * Notes:
 *********************************************************************************************/
static INT32 TH_FindRecToWriteToLog(TH_RECORD_CTL_BLK **start_blk)
{
  INT32 i;
  INT32 blk_cnt = 0;
  //static BOOLEAN got_start = FALSE;
  *start_blk = NULL;

  for( i = 0; i < TH_REC_SEARCH_PER_FRAME; i++)
  {
    //Wrap pointer if pointing past last record.
    if(m_THDataBuf.csearch_ptr == &m_THDataBuf.ctrl[TH_PRE_HISTORY_REC_CNT])
    {
      m_THDataBuf.csearch_ptr = m_THDataBuf.ctrl;
    }

    //Find the start of a section needing writes.  Add to count as long as the records are
    //contigous.
    if(m_THDataBuf.csearch_ptr->write == TH_WRITE_START && !m_THDataBuf.csearch_ptr->written)
    {
      for( blk_cnt = 0;(blk_cnt < TH_MAX_LOG_WRITE_PER_FRAME) &&
                       (!m_THDataBuf.csearch_ptr->written && (m_req_cnt != 0)) &&
                       (m_THDataBuf.csearch_ptr != &m_THDataBuf.ctrl[TH_PRE_HISTORY_REC_CNT]);
                        blk_cnt++ )
      {
        //Set start block only for the first block found
        *start_blk = *start_blk == NULL ? m_THDataBuf.csearch_ptr : *start_blk;
        if(m_THDataBuf.csearch_ptr->write == TH_WRITE_START)
        {
          m_THDataBuf.csearch_ptr->write = TH_WRITE_NONE;
        }
        else if(m_THDataBuf.csearch_ptr->write == TH_WRITE_STOP)
        {
          m_THDataBuf.csearch_ptr->write = TH_WRITE_NONE;
          m_req_cnt--;
        }
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
 *               parameter specifies a list of records to be written.  Records to be written
 *               must NOT wrap to the beginning of the buffer.
 *
 *               The current timestamp and checksum is added to each record header
 *
 * Parameters:   [in]     start_blk:     Pointer to first control block to write
 *               [in]     blk_write_cnt: Number of contiguous block to write
 *               [in/out] lm_sta:        Pointer to a location to save LogWrite status flag
 *
 * Returns:       none
 *
 * Notes:         Max write size is dependant on number of configured sensors and the value of
 *                TH_MAX_LOG_WRITE_PER_FRAME.  For 128 sensors each block will be 564 bytes.
 *                At a MAX of 10 logs/frame this is 5,640 bytes.
 *********************************************************************************************/
static void TH_WriteRecToLog(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt,
                             LOG_REQ_STATUS *lm_sta)
{
  INT32 i;
  TIMESTAMP now;
  TH_DATA_RECORD_HEADER* log_hdr;
  UINT32 record_sz;
  UINT32 log_write_cs = 0;
  LOG_SOURCE source;

  CM_GetTimeAsTimestamp(&now);

  //Do checksums/timestamps
  for(i = 0; i < blk_write_cnt; i++)
  {
    log_hdr             = (TH_DATA_RECORD_HEADER*)start_blk[i].data;
    log_hdr->ts_written = now;
    //total size of log is data size + header size - TS (because it is included in data size)
    record_sz           = log_hdr->size+sizeof(*log_hdr)-sizeof(log_hdr->ts_recorded);
#pragma ghs nowarning 1545  //supress packed structure alignment warning
    log_hdr->cs = ChecksumBuffer(&log_hdr->ts_written, record_sz-sizeof(log_hdr->cs),0xFFFFFFFF);
    log_write_cs += ChecksumBuffer(&log_hdr->cs,sizeof(log_hdr->cs),0xFFFFFFFF) + log_hdr->cs;
#pragma ghs endnowarning
  }
  //Write logs
  source.id = APP_ID_TIMEHISTORY_ETM_LOG;
  LogWrite(LOG_TYPE_ETM,
           source,
           TH_LOG_PRIORITY,
           start_blk->data,
           TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt)*blk_write_cnt,
           log_write_cs,
           lm_sta);
}



/**********************************************************************************************
 * Function:     TH_MarkTHRecordWritten
 *
 * Description:  Set the written flag on a contiguous set of time history records.
 *
 * Parameters:   [in/out] start_blk:     First block to start marking at
 *               [in]     blk_write_cnt: Number of contiguous blocks to mark after the first
 *
 * Returns:      none
 *
 * Notes:
 *********************************************************************************************/
static void TH_MarkTHRecordWritten(TH_RECORD_CTL_BLK *start_blk,INT32 blk_write_cnt)
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
 *               ValidFlags         = Variable size sensor validity booleans as packed
 *                                    bitfields
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
 * Returns:     none
 *
 * Notes:
 *********************************************************************************************/
static void TH_BuildRec(BYTE *rec)
{
  // Local Data
  BYTE *bit_ptr;
  FLOAT32 *value_ptr;
  UINT32 i;
  TH_DATA_RECORD_HEADER *hdr = (void*)rec;

  memset(hdr,0,sizeof(*hdr));

  //Set pointer to ValidFlag bitfields after the header in the buffer
  bit_ptr   = rec + sizeof(*hdr);

  //Set pointer to the sensor values after the ValidFlag bitfields
  value_ptr = (void*)(bit_ptr + TH_SENSOR_CT_TO_VALID_BITS_SZ(m_SensorCnt));

  //Add timestamp and size to header, the checksum and record time
  //will be added later if this record is recorded to FLASH.
  CM_GetTimeAsTimestamp(&hdr->ts_recorded);
  hdr->size = TH_SENSOR_CT_TO_REC_SZ(m_SensorCnt) - sizeof(*hdr) + sizeof(hdr->ts_recorded);
  hdr->id   = APP_ID_TIMEHISTORY_ETM_LOG;

  //Read sensor values/validity and build the rest of the buffer
  //TEST: Check buffer pointer math for the number of bit field bytes,
  //and validity bit math for boundary conditions
  for (i = 0; (i < MAX_SENSORS) && m_SensorList[i] != SENSOR_UNUSED; i++)
  {
    //Sensor Values
    *value_ptr = SensorGetValue((SENSOR_INDEX)m_SensorList[i]);
    value_ptr++;

    //Increment valid bit pointer one byte each 8 sensors.
    bit_ptr += ((i / 8) > 0) && (i % 8 == 0) ? 1 : 0;

    //Sensor Validity
    if (SensorIsValid((SENSOR_INDEX)m_SensorList[i]))
    {
      *bit_ptr |= 1 << (i%8);
    }
    else
    {
      *bit_ptr &= ~(1 << (i%8));
    }
  }
}



/**********************************************************************************************
 * Function:     TH_PutNewRec
 *
 * Description:  Add a new time history record to the m_THDataBuf time history buffer.  This
 *               routine modifies the control buffer, adding a reference to the new th_rec
 *               data, and set the control flags "write" and "written" to zero. Then, it
 *               increments the data ptr
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
 * Returns:      none
 *
 * Notes:        not reentrant
 *
 *********************************************************************************************/
static void TH_PutNewRec(INT32 sz, BOOLEAN init_write)
{
  //Setup control pointer to store the new TH
  if (m_THDataBuf.cwrite_ptr == &m_THDataBuf.ctrl[TH_PRE_HISTORY_REC_CNT])
  {
    m_THDataBuf.cwrite_ptr = &m_THDataBuf.ctrl[0];
  }

  //Add control data for the new time history record, increment pointer
  m_THDataBuf.cwrite_ptr->data = m_THDataBuf.dwrite_ptr;
  m_THDataBuf.cwrite_ptr->write = init_write;
  m_THDataBuf.cwrite_ptr->written = FALSE;
  m_THDataBuf.cwrite_ptr++;

  //Data and control write complete, increment write pointer to the next record location.
  m_THDataBuf.dwrite_ptr += sz;
}



/**********************************************************************************************
 * Function:     TH_GetDataBufPtr
 *
 * Description:  Returns a pointer to a location in the time history data buffer that is at
 *               least size bytes long.  The buffer returned is contiguous, and does not
 *               wrap.
 *
 *               The write to the buffer must be completed by calling TH_PutNewRec, which
 *               creates a data control record and increments the data buffer write pointer.
 *
 * Parameters:   [in] size: Number of bytes the caller needs to store in the TH buffer
 *
 * Returns:      Pointer to a location in the TH buffer that the caller may store data.
 *
 * Notes:        not reentrant
 *
 *********************************************************************************************/
static BYTE* TH_GetDataBufPtr(INT32 size)
{
  //Wrap to beginning if the new TH record won't fit before the end of the buffer
  // size <= BufferEndAddr - BufferWriteAddr
  if ( (m_THDataBuf.dwrite_ptr + size) > (m_THDataBuf.data + sizeof(m_THDataBuf.data))  )
  {
    m_THDataBuf.dwrite_ptr = m_THDataBuf.data;
  }

  return m_THDataBuf.dwrite_ptr;
}


/**********************************************************************************************
 *  MODIFICATIONS
 *    $History: TimeHistory.c $
 *
 * *****************  Version 12  *****************
 * User: John Omalley Date: 12-11-12   Time: 9:49a
 * Updated in $/software/control processor/code/application
 * SCR 1105, 1107, 1131, 1154 - Code Review Updates
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 11/06/12   Time: 11:49a
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 9/27/12    Time: 4:51p
 * Updated in $/software/control processor/code/application
 * SCR 1107
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:49p
 * Updated in $/software/control processor/code/application
 * SCR 1107: 2.0.0 Requirements - Time History
 *
 * *****************  Version 8  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:44p
 * Updated in $/software/control processor/code/application
 * SCR 1107: 2.0.0 Requirements - Time History
 *
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 9/21/12    Time: 5:12p
 * Updated in $/software/control processor/code/application
 * SCR 1107: 2.0.0 Requirements - Time History
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
