#define SYS_CLOCKMGR_BODY

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

 ECCN:        9D991

 File:        ClockMgr.c

 Description: The clock manager module maintains two clocks and other services
              for use by the application and other system modules.
              The system RTC:
                Maintains current date and time at sub-second resolution,
                and is driven from a hardware timer or the PIT timer.
                Actual resolution is dependent on the timer driving the clock.
              The wallclock:
                Maintains current date and time at 1 second resolution and
                it is driven from the real-time clock on the SPI bus.

 VERSION
     $Revision: 59 $  $Date: 3/22/16 12:12p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include "mcf548x.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "RTC.h"
//#include "ResultCodes.h"
#include "ClockMgr.h"
#include "TaskManager.h"
#include "GSE.h"
#include "LogManager.h"

#include "FASTMgr.h"
#include "MSStsCtl.h"

//extern TIME_SOURCE_ENUM FAST_TimeSourceCfg(void);
//extern BOOLEAN MSSC_GetIsAlive(void);
//extern TIMESTRUCT MSSC_GetLastMSSIMTime(void);
//extern BOOLEAN MSSC_GetMSTimeSynced(void);



/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define CM_INIT_CLK_SEC    0
#define CM_INIT_CLK_MIN    0
#define CM_INIT_CLK_HR     12
#define CM_INIT_CLK_DAY    1
#define CM_INIT_CLK_DATE   1
#define CM_INIT_CLK_MON    1
#define CM_INIT_CLK_YR     1997

//Leap years:
//Every fourth years are leap-years in most of the cases.
//Years divisible with 100 aren't leap-years but years
//divisible with 400 are leap-years.
#define IS_LEAP_YEAR(Yr) ( ((Yr & 0x3) != 0) ? FALSE:\
                           ((Yr % 100) != 0) ? TRUE : \
                           ((Yr % 400) != 0) ? FALSE: TRUE )

#define CM_MONTH_FEB   2   // Month maintained as 1..12
#define CM_DAYS_FEB_LEAP_YR_INDEX  0  // Index to DaysPerMonth [] = {29, //Feb leap year

#define CM_DAYS_IN_YEAR_NO_LEAP    365          // Ignoring leap year
#define CM_DAYS_IN_YEAR_WITH_LEAP  365.242199f  // Including leap year

#define CM_SEC_IN_MIN  60
#define CM_MIN_IN_HR   60
#define CM_HR_IN_DAY   24
#define CM_SEC_IN_DAY  (CM_SEC_IN_MIN * CM_MIN_IN_HR * CM_HR_IN_DAY)

#define CM_SEC_IN_HR   3600

#define CM_MS_TIME_DIFF_SECS  60  // Hardcode the delta time difference (in seconds)
                                  //   when syncing to MS.

#define CM_SYS_MS_CLOCK_DRIFT_MAX_SECS  6 //Max acceptable clock diff for sys and ms clocks.
#define CM_SYS_RTC_CLOCK_DRIFT_MAX_SECS 6 //Max acceptable clock diff for sys and rtc clocks.


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
//CM static variables
static TIMESTRUCT     CM_SYS_CLOCK;           //System Real-time clock

//static UINT32         CM_SystemOnTimeSec;   //Shadow copy of system up-time in seconds

static UINT32         m_TickCount;            //Tick count for CM_GetTickCount

static BOOLEAN        m_bPreInitFlag;         //Used during "pre-initlization"
                                              //to indicate the interrupt
                                              //driven clock is not operating
                                              //and forces "Get" time routines
                                              //to read time from the RTC chip.

static INT16          m_nRecRefCnt;           // Recording/DownLoading Ref cnt.
                                              // Updated by FastMgr and DataMgr to signal CM
                                              // when recording/downloading is in progress
                                              // CM will only perform a clock-sync
                                              // when this value is zero. This
                                              // prevents time gaps or overlaps in logs.


static const UINT8    DaysPerMonth [] =    {29, //Feb leap year
                                            31, //Jan
                                            28, //Feb
                                            31, //Mar
                                            30, //Apr
                                            31, //May
                                            30, //Jun
                                            31, //Jul
                                            31, //Aug
                                            30, //Sep
                                            31, //Oct
                                            30, //Nov
                                            31};//Dec

static CHAR gse_OutLine[512];


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
void CM_CBit(void* pParam);
static void CM_TimeSourceSyncToRemote ( TIMESTRUCT cm_time_struct );
static void CM_CreateClockUpdateLog( SYS_APP_ID SysId, TIMESTRUCT *currTime,
                                     TIMESTRUCT *newTime );


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/

// Initial value of CM_SYS_CLOCK
static const TIMESTRUCT CM_SYS_CLOCK_DEFAULT =
{
  CM_INIT_CLK_YR,   // Year
  CM_INIT_CLK_MON,  // Month
  CM_INIT_CLK_DAY,  // Day
  CM_INIT_CLK_HR,   // Hour
  CM_INIT_CLK_MIN,  // Minute
  CM_INIT_CLK_SEC,  // Second
  0                 // MilliSecond
};

#ifdef GENERATE_SYS_LOGS
  // CM System Logs - For Test Purposes
  static CM_TIME_INVALID_LOG CmTimeInvalidLog[] = {
    {DRV_RTC_TIME_NOT_VALID, {2000, 12, 25, 16, 15, 14, 13} }
  };

  static CM_TIME_INVALID_LOG CmTimeSetFailedLog[] = {
    {DRV_RTC_ERR_CANNOT_SET_TIME, {1997, 1, 1, 0, 0, 0, 0} }
  };

  static CM_RTC_SYS_TIME_DRIFT_LOG CmTimeDriftLog[] = {
    { {2001, 1, 2, 12, 0,  5, 0},
      {2001, 1, 2, 12, 0, 15, 1},
      {6}
    }
  };

  static CM_CLOCKUPDATE_LOG CmClockUpdateLog[] = {
    {1999, 12, 25, 12, 10, 5, 0},
    {1999, 11, 25, 12, 10, 5, 0}
  };

  static CM_RTC_DRV_PBIT_LOG CmRTCDrvFailLog[] = {
    {SYS_CM_RTC_PBIT_FAIL}
  };
#endif


#ifdef GENERATE_SYS_LOGS

#include "User.h"

USER_HANDLER_RESULT CMMsg_CreateLogs(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);

USER_MSG_TBL CMRoot[] =
{
  {"CREATELOGS",NO_NEXT_TABLE,CMMsg_CreateLogs,
  USER_TYPE_ACTION,USER_RO,NULL,-1,-1,NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

#endif


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************
 * Function:    CM_Init
 *
 * Description: Initial setup for the Clock Manager and clock manager task
 *              It is expected that this routine is called at startup
 *              after the SPI and RTC periperals are intialized.
 *
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_Init(void)
{
  TCB taskBlock;
  TIMESTRUCT timeStruct;
  CM_RTC_DRV_PBIT_LOG log;

 #ifdef GENERATE_SYS_LOGS
  USER_MSG_TBL CMRootTblPtr = {"CM",CMRoot,NULL,NO_HANDLER_DATA};
 #endif

  //Initialize clock manager data
  CM_SYS_CLOCK    = CM_SYS_CLOCK_DEFAULT;
  m_TickCount     = 0;
  m_bPreInitFlag  = FALSE;
  m_nRecRefCnt    = 0;

  //Read the battery backed time and load it to
  //the system clock
  CM_GetRTCClock(&timeStruct);
  CM_SetSystemClock(&timeStruct, FALSE);

  memset(&taskBlock, 0, sizeof(taskBlock));
  strncpy_safe(taskBlock.Name, sizeof(taskBlock.Name),"Clock CBIT",_TRUNCATE);
  taskBlock.TaskID         = Clock_CBIT;
  taskBlock.Function       = CM_CBit;
  taskBlock.Priority       = taskInfo[Clock_CBIT].priority;
  taskBlock.Type           = taskInfo[Clock_CBIT].taskType;
  taskBlock.modes          = taskInfo[Clock_CBIT].modes;
  taskBlock.MIFrames       = taskInfo[Clock_CBIT].MIFframes;
  taskBlock.Rmt.InitialMif = taskInfo[Clock_CBIT].InitialMif;
  taskBlock.Rmt.MifRate    = taskInfo[Clock_CBIT].MIFrate;
  taskBlock.Enabled        = TRUE;
  taskBlock.Locked         = FALSE;
  taskBlock.pParamBlock    = NULL;
  TmTaskCreate (&taskBlock);

  if ( RTC_GetStatus() != DRV_OK )
  {
    CM_SYS_CLOCK = CM_SYS_CLOCK_DEFAULT;  // reset system clock to default
    log.result = SYS_CM_RTC_PBIT_FAIL;
    Flt_SetStatus( STA_CAUTION, SYS_CM_PBIT_RTC_BAD, &log, sizeof(CM_RTC_DRV_PBIT_LOG) );
  }

  #ifdef GENERATE_SYS_LOGS

  /*vcast_dont_instrument_start*/
     //Add an entry in the user message handler table
     User_AddRootCmd(&CMRootTblPtr);
  /*vcast_dont_instrument_end*/

  #endif
}


/*****************************************************************************
 * Function:    CM_PreInit
 *
 * Description: Setup the clock manager using minimal other resources and it is
 *              intended to be called during initialization to early access to
 *              the get clock functions.  This function is only dependent on
 *              RTC driver being ready and it does not need the interrupt
 *              driven tick clock working.  "Get" time calls will read the time
 *              directly from the RTC chip until CM_Init is called and the
 *              interrupt driven clock is initialized.
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 *****************************************************************************/
void CM_PreInit(void)
{
  m_bPreInitFlag               = TRUE;   //Forces SynchToRTC to stay set
}


/*****************************************************************************
 * Function:    CM_GetSystemClock
 *
 * Description: Returns the current value of the system real-time clock
 *
 * Parameters:  pTime - ptr to return current system time
 *
 * Returns:     none
 *
 * Notes:       May encur a SPI overhead time penalty (See RTC_GetTime)
 *
 ****************************************************************************/
void CM_GetSystemClock(TIMESTRUCT* pTime)
{
  UINT32 intrLevel;

  //If system clock is not yet running (during system start "pre-init" phase)
  //Need to silently discard errors.  If an error occurs, RTCClock returns the
  //default time.  This case only applies in the pre-init stage.
  if(m_bPreInitFlag)
  {
    // During init, read RTC directly, afterwards use CM_GetRTCClock(pTime)
    if (RTC_ReadTime(pTime) != DRV_OK)
    {
      // RTC Failure - Time returned not valid for timestamp format
      *pTime = CM_SYS_CLOCK_DEFAULT;      // Set to default date
    }
  }
  else
  {
    intrLevel = __DIR();
    *pTime = CM_SYS_CLOCK;    // Copy system clock
    __RIR(intrLevel);
  }
}


/*****************************************************************************
 * Function:    CM_SetSystemClock
 *
 * Description: Sets the time values in the system real-time clock
 *
 * Parameters:  pTime time to set
 *              bRecordLog record update change log
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_SetSystemClock(TIMESTRUCT* pTime, BOOLEAN bRecordLog )
{
  UINT32 intrLevel;
  TIMESTRUCT currTime;

  intrLevel = __DIR();
  currTime = CM_SYS_CLOCK;

  CM_SYS_CLOCK.MilliSecond = pTime->MilliSecond;
  CM_SYS_CLOCK.Second = pTime->Second;
  CM_SYS_CLOCK.Minute = pTime->Minute;
  CM_SYS_CLOCK.Hour   = pTime->Hour;
  CM_SYS_CLOCK.Day    = pTime->Day;
  CM_SYS_CLOCK.Month  = pTime->Month;
  CM_SYS_CLOCK.Year   = pTime->Year;
  __RIR(intrLevel);

  if (bRecordLog == TRUE)
  {
    CM_CreateClockUpdateLog( SYS_CM_CLK_UPDATE_CPU, &currTime, pTime );

    // Output Debug Message
    snprintf (gse_OutLine, sizeof(gse_OutLine),
             "\r\nCM_SetSystemClock: Auto update time (old=%02d/%02d/%4d %02d:%02d:%02d)\r\n",
             currTime.Month, currTime.Day, currTime.Year, currTime.Hour, currTime.Minute,
             currTime.Second);
    GSE_DebugStr(NORMAL,TRUE,gse_OutLine);

    snprintf (gse_OutLine, sizeof(gse_OutLine),
             "\r\nCM_SetSystemClock: Auto update time (new=%02d/%02d/%4d %02d:%02d:%02d)\r\n",
             CM_SYS_CLOCK.Month,CM_SYS_CLOCK.Day, CM_SYS_CLOCK.Year, CM_SYS_CLOCK.Hour,
             CM_SYS_CLOCK.Minute, CM_SYS_CLOCK.Second);
    GSE_DebugStr(NORMAL,TRUE,gse_OutLine);
  }

}


/*****************************************************************************
 * Function:    CM_GetRTCClock
 *
 * Description: Returns the RTC clock time as read from the real-time clock
 *              on the SPI bus.  Note that the system maintained clock is different
 *              than the real-time clock on the SPI bus.
 *
 * Parameters:  pTime  -  ptr to TIMESTRUCT
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_GetRTCClock(TIMESTRUCT* pTime)
{
  RESULT result;
  CM_TIME_INVALID_LOG invalidTimeLog;

  result = RTC_GetTime(pTime);
  if(DRV_OK != result)
  {
    if(!m_bPreInitFlag)
    {
      invalidTimeLog.Result   = result;
      invalidTimeLog.TimeRead = *pTime;
      LogWriteSystem(SYS_CM_CBIT_TIME_INVALID_LOG,
          LOG_PRIORITY_LOW,&invalidTimeLog,sizeof(invalidTimeLog),NULL);
    }

    //Set return time to default value
    *pTime = CM_SYS_CLOCK_DEFAULT;
    result = RTC_SetTime(pTime);
    if (DRV_OK != result)
    {
      if(!m_bPreInitFlag)
      {
        //Cannot set the time, log a fault (with "Result")
        invalidTimeLog.Result = result;
        invalidTimeLog.TimeRead = *pTime;
        LogWriteSystem(SYS_CM_CBIT_SET_RTC_FAILED, LOG_PRIORITY_LOW, &invalidTimeLog,
                       sizeof(invalidTimeLog),NULL);
      }
    }
  }
}


/*****************************************************************************
 * Function:    CM_SetRTCClock
 *
 * Description: Writes the time passed in the Time paramter to the real-time
 *              clock on the SPI bus.  Note that the system maintained clock
 *              is different than the real-time clock on the SPI bus.
 *
 * Parameters:  pTime - new time to set
 *              bRecordLog - Record time update change log
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_SetRTCClock(TIMESTRUCT* pTime, BOOLEAN bRecordLog)
{
  TIMESTRUCT currTime;
  CM_TIME_INVALID_LOG invalidTimeLog;
  RESULT result;

  CM_GetRTCClock( &currTime );

  result = RTC_SetTime(pTime);

  if(DRV_OK != result )
  {
    //Cannot set the time, log a fault (with "Result")
    invalidTimeLog.Result = result;
    invalidTimeLog.TimeRead = *pTime;
    LogWriteSystem(SYS_CM_CBIT_SET_RTC_FAILED, LOG_PRIORITY_LOW, &invalidTimeLog,
                   sizeof(invalidTimeLog),NULL);
  }
  else
  {
    CM_CreateClockUpdateLog( SYS_CM_CLK_UPDATE_RTC, &currTime, pTime );
  }

}



/*****************************************************************************
 * Function:    CM_GetTimeAsTimestamp
 *
 * Description: Returns the current time from the RTC in a packed 48-bit word
 *              aka: Altair "timestamp" format.
 *
 *      Bit 47           37   32   28      22     16         0
 *           :            :    :    :       :      :
 *           yyyy.yyyy.yyyd.dddd.nnnn.mmmm.mmss.ssss.mmmm.mmmm
 *                       |     |    |       |      |         |
 *                       |     |    |       |      |         Milliseconds
 *                       |     |    |       |      |         (0-999)
 *                       |     |    |       |      Seconds within minute(0-59)
 *                       |     |    |       |
 *                       |     |    |       Minutes within hour (0-59)
 *                       |     |    |
 *                       |     |    Month within year (1-12)
 *                       |     |
 *                       |     Day within month (1 - 31)
 *                       |
 *                       Year/Hour ((Year since 1997*24) + Hour within day)
 *
 *              To decode year and hour from the Year/Hour field:
 *                  Hour = Year/Hour % 24
 *                  Year = BASE_YEAR + (Year/Hour / 24)
 *
 * Parameters:  timeStamp - Pointer to the time stamp structure.
 *
 * Returns:     none
 *
 * Notes:       The time returned is a combination of the real-time clock and
 *              wall clock.  The software requirements state that the second,
 *              minute, month, day, and year are retrieved directly from
 *              the wallclock.  The milliseconds are retrived from the system RTC.
 *
 ****************************************************************************/
void CM_GetTimeAsTimestamp (TIMESTAMP* timeStamp)
{
 TIMESTRUCT ts;

  CM_GetSystemClock(&ts);

  timeStamp->Timestamp =   ((UINT32) ((ts.Second & 0x3F)))       +
                           ((UINT32) ((ts.Minute & 0x3F)) << 6)  +
                           ((UINT32) ((ts.Month  & 0x0F)) << 12) +
                           ((UINT32) ((ts.Day    & 0x1F)) << 16) +
                           ((UINT32) ((ts.Year - BASE_YEAR) * 24 +
                              ts.Hour) << 21);
  timeStamp->MSecond   =   CM_SYS_CLOCK.MilliSecond;

}



/*****************************************************************************
 * Function:    CM_ConvertTimeStamptoTimeStruct
 *
 * Description: Convert a TIMESTAMP type packed word into a TIMESTRUCT
 *
 * Parameters:  [in]  Tp: Pointer to a TIMESTAMP structure to convert
 *              [out] Ts: Pointer to a TIMESTRUCT location to receive the
 *                        converted data
 * Returns:     none
 *
 * Notes:       garbage-in-garbage-out
 *
 ****************************************************************************/
void CM_ConvertTimeStamptoTimeStruct(const TIMESTAMP* Tp, TIMESTRUCT* Ts)
{
  Ts->MilliSecond = Tp->MSecond;
  Ts->Second = (UINT8) (Tp->Timestamp & 0x3F);
  Ts->Minute = (UINT8) ( (Tp->Timestamp >> 6) & 0x3F);
  Ts->Month = (UINT8) ( (Tp->Timestamp >> 12) & 0x0F);
  Ts->Day = (UINT8) ( (Tp->Timestamp >> 16) & 0x1F);
  Ts->Hour = (UINT8) ( (Tp->Timestamp >> 21) % 24);
  Ts->Year = (UINT16) ( ((Tp->Timestamp >> 21) / 24) + BASE_YEAR);
}



/*****************************************************************************
 * Function:    CM_SynchRTCtoWallClock
 *
 * Description: Syncs the RTC to the internal system maintained clock
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_SynchRTCtoWallClock(void)
{
  TIMESTRUCT localTm;
  //Get seconds difference
  if(RTC_GetTime(&localTm) == DRV_OK)          //Get wallclock time
  {
    localTm.MilliSecond = CM_SYS_CLOCK.MilliSecond;  //Keep the ms the same
    CM_SetSystemClock(&localTm, TRUE);
  }
}



/*****************************************************************************
 * Function:    CM_GetTickCount
 *
 * Description: Returns the number of milliseconds elapsed since the
 *              system powered up.  The caller must take into account that this
 *              counter will roll-over every 49.7 days.  Addition/Subtraction
 *              of two tick counts is okay, but < or > comparisons are NOT!
 *
 * Parameters:  None
 *
 * Returns:     TickCount: Number of milliseconds since power on.
 *
 * Notes:       COUNTER ROLLS OVER EVERY 49.7 DAYS, < > COMPARISONS OF TICK
 *              COUNT VALUES ARE NOT VALID! ONLY THE DIFFERNCE OF TWO TIMES
 *              WILL GIVE A VALID DURATION.  TIMERS ARE CIRCLES, NOT LINES!
 *
 ****************************************************************************/
UINT32 CM_GetTickCount(void)
{
  //Return of interrupt TickCount without disabling interrupts
  //is okay because the read is atomic.
  return m_TickCount;
}



/*****************************************************************************
 * Function:    CM_GenerateDebugLogs
 *
 * Parameters:
 *
 * Returns:
 *
 * Notes:
 *
 ****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
void CM_GenerateDebugLogs(void)
{
  CM_RTC_SYS_TIME_DRIFT_LOG TimeDriftLog;
  CM_TIME_INVALID_LOG TimeInvalidLog;

  //Initialize the test log date/times to a couple different dates.
  TimeDriftLog.RTC.Year           = 1979;
  TimeDriftLog.RTC.Month          = 9;
  TimeDriftLog.RTC.Day            = 4;
  TimeDriftLog.RTC.Hour           = 4;
  TimeDriftLog.RTC.Minute         = 0;
  TimeDriftLog.RTC.Second         = 0;
  TimeDriftLog.RTC.MilliSecond    = 0;

  TimeDriftLog.System.Year        = 1981;
  TimeDriftLog.System.Month       = 3;
  TimeDriftLog.System.Day         = 11;
  TimeDriftLog.System.Hour        = 4;
  TimeDriftLog.System.Minute      = 0;
  TimeDriftLog.System.Second      = 0;
  TimeDriftLog.System.MilliSecond = 0;
  //InvalidTimeLog.Result           = DRV_RTC_TIME_NOT_VALID;
  //InvalidTimeLog.TimeRead         = TimeDriftLog.RTC;

//  LogWriteSystem(SYS_CM_CBIT_TIME_DRIFT_GT_1s,LOG_PRIORITY_LOW,
//      &TimeDriftLog,sizeof(TimeDriftLog),NULL);
  LogWriteSystem(SYS_CM_CBIT_TIME_DRIFT_LIMIT_EXCEEDED,LOG_PRIORITY_LOW,
      &TimeDriftLog,sizeof(TimeDriftLog),NULL);
//  LogWriteSystem(SYS_CM_CBIT_TIME_INVALID_LOG,
//      LOG_PRIORITY_LOW,&InvalidTimeLog,sizeof(InvalidTimeLog),NULL);



}
/*vcast_dont_instrument_end*/

#endif //GENERATE_SYS_LOGS

/*****************************************************************************
 * Function:    CM_RTCTick | TASK
 *
 * Description: Increments the real-time clock, rolls over each time unit,
 *              accounting for leap-years.  Also schedules a CBIT routine to
 *              run every 10 minutes
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_RTCTick(void)
{

  m_TickCount += MIF_PERIOD_mS;              //Update millisecond tick count
  CM_SYS_CLOCK.MilliSecond += MIF_PERIOD_mS;        //Update RTC

  if (CM_SYS_CLOCK.MilliSecond >= MILLISECONDS_PER_SECOND)    // Update Seconds
  {
    CM_SYS_CLOCK.MilliSecond = CM_SYS_CLOCK.MilliSecond % MILLISECONDS_PER_SECOND;
    CM_SYS_CLOCK.Second  = (CM_SYS_CLOCK.Second + 1) % SECONDS_PER_MINUTE;

    if (CM_SYS_CLOCK.Second == 0)                             // Update Minutes
    {
      CM_SYS_CLOCK.Minute = (CM_SYS_CLOCK.Minute + 1) % MINUTES_PER_HOUR;

      if(CM_SYS_CLOCK.Minute == 0)
      {
        CM_SYS_CLOCK.Hour = (CM_SYS_CLOCK.Hour + 1) % HOURS_PER_DAY;

        if(CM_SYS_CLOCK.Hour == 0)
        {
          if(IS_LEAP_YEAR(CM_SYS_CLOCK.Year) && CM_SYS_CLOCK.Month == 2)
          {
            CM_SYS_CLOCK.Day = (CM_SYS_CLOCK.Day + 1) % (DaysPerMonth[0] + 1);
          }
          else
          {
            CM_SYS_CLOCK.Day = (CM_SYS_CLOCK.Day + 1) % (DaysPerMonth[CM_SYS_CLOCK.Month] + 1);
          }

          if(CM_SYS_CLOCK.Day == 0)
          {
            CM_SYS_CLOCK.Day = 1;
            CM_SYS_CLOCK.Month = (CM_SYS_CLOCK.Month + 1) % (MONTHS_PER_YEAR + 1);

            if(CM_SYS_CLOCK.Month == 0)
            {
              CM_SYS_CLOCK.Month = 1;
              CM_SYS_CLOCK.Year += 1;
            }//Month ==0
          }//Day == 0
        }//Hour == 0
      }//Minute == 0
    }//Second == 0

    // Check sync to Cfg Time Source every 2 sec
    if ( (((CM_SYS_CLOCK.Second) % 2) == 0) )
    {
      CM_TimeSourceSyncToRemote( CM_SYS_CLOCK );
    }

  }
}




/*****************************************************************************
 * Function:    CM_GetSecSinceBaseYr
 *
 * Description: Returns the number of seconds since the Base Year
 *                                                    (1/1/1997 00:00:00)
 *
 * Parameters:  ts     -  ptr to TimeStamp to convert to sec since base yr
 *              time_s -  ptr to TimeStruct to conver to sec since base yr
 *
 * Returns:     Seconds since Base Year or "0" if Date-Time invalid / out of range
 *
 * Notes:
 *  - To make algorithm simple will ignore leap year for century.  Next
 *    century milestone will be 2100 and all of us will have pasted away.
 *
 ****************************************************************************/
UINT32 CM_GetSecSinceBaseYr ( TIMESTAMP *ts, TIMESTRUCT *time_s )
{
  UINT16 i;
  UINT32 sec;
  UINT32 itemp;
  TIMESTRUCT time_struct;

  // We need one of these to be NONE NULL
  ASSERT( ts != NULL || time_s != NULL);

  if ( ts != NULL )
  {
    CM_ConvertTimeStamptoTimeStruct((const TIMESTAMP *) ts, &time_struct);
  }

  if ( time_s != NULL )
  {
    time_struct = *time_s;
  }

  // CM_ConvertTimeStamptoTimeStruct((const TIMESTAMP *) &ts, &time_struct);


  // Determine years since 1997 in sec
  // Note: Year will be > 1997 as year in ts format is years since Base Year !
  // Note: Leap yr calc not compliant to /4 and /100 is not a leap yr unless also
  //       /400.  Leap yr calc would fail for 2100, because this code will calc that 
  //       2100 is a leap year when it is not.  If this code is to be used to include 2100
  //       this code must be updated. 
  itemp = 0;
  if ( time_struct.Year >= 2000 )
  {
    // Determine # leap days, from base year of 1997.
    itemp = ((time_struct.Year - 1997) / 4);
  }
  // else date between 1997 and 2000, no leap yr calc required
  itemp = (CM_DAYS_IN_YEAR_NO_LEAP * (time_struct.Year - BASE_YEAR)) + itemp;
  sec = itemp * CM_SEC_IN_DAY;


  // Determine # months for current year in sec
  itemp = 0;
  for (i=1;i<time_struct.Month;i++)
  {
    if ((i == CM_MONTH_FEB) && IS_LEAP_YEAR(time_struct.Year)) {
      itemp += DaysPerMonth[CM_DAYS_FEB_LEAP_YR_INDEX];
    }
    else {
      itemp += DaysPerMonth[i];
    }
  }
  sec += (itemp * CM_SEC_IN_DAY);


  // Determine # days for current year in sec
  sec += (time_struct.Day * CM_SEC_IN_DAY);


  // Determine # hours in sec
  sec += (time_struct.Hour * CM_SEC_IN_MIN * CM_MIN_IN_HR);

  // Determine # min in sec
  sec += (time_struct.Minute * CM_SEC_IN_MIN);

  // Add in current sec
  sec += time_struct.Second;

  return sec;

}



/*****************************************************************************
 * Function:    CM_CreateAllInternalLogs
 *
 * Description: Creates the Clock Update Change Log
 *
 *              Log Write Test will write one copy of every log the Clock
 *              Manager generates.  The purpose if to test the log format
 *              for the team writing the log file decoding tools.  This
 *              code will be ifdef'd out of the flag GENERATE_SYS_LOGS is not
 *              defined.
 *
 *              Example logs in the following order:
 *              SYS_CM_CLK_UPDATE_RTC
 *              SYS_CM_CLK_UPDATE_CPU
 *              SYS_CM_CBIT_TIME_DRIFT_GT_6s
 *              SYS_CM_CBIT_TIME_INVALID_LOG
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
void CM_CreateAllInternalLogs( void )
{
  // 1 SYS_CM_CBIT_TIME_DRIFT_GT_6s
  LogWriteSystem( SYS_CM_CBIT_TIME_DRIFT_LIMIT_EXCEEDED, LOG_PRIORITY_LOW, &CmTimeDriftLog,
                  sizeof(CM_RTC_SYS_TIME_DRIFT_LOG), NULL );

  // 2 SYS_CM_CBIT_TIME_INVALID_LOG
  LogWriteSystem( SYS_CM_CBIT_TIME_INVALID_LOG, LOG_PRIORITY_LOW, &CmTimeInvalidLog,
                  sizeof(CM_TIME_INVALID_LOG), NULL );

  // 3 SYS_CM_CBIT_SET_RTC_FAILED
  LogWriteSystem( SYS_CM_CBIT_SET_RTC_FAILED, LOG_PRIORITY_LOW, &CmTimeSetFailedLog,
                  sizeof(CM_TIME_INVALID_LOG), NULL );

  // 4 SYS_CM_CLK_UPDATE_RTC
  LogWriteSystem( SYS_CM_CLK_UPDATE_RTC, LOG_PRIORITY_LOW, &CmClockUpdateLog,
                  sizeof(CM_CLOCKUPDATE_LOG), NULL );

  // 5 SYS_CM_CLK_UPDATE_CPU
  LogWriteSystem( SYS_CM_CLK_UPDATE_CPU, LOG_PRIORITY_LOW, &CmClockUpdateLog,
                  sizeof(CM_CLOCKUPDATE_LOG), NULL );

  // 6 SYS_CM_PBIT_RTC_BAD
  LogWriteSystem( SYS_CM_PBIT_RTC_BAD, LOG_PRIORITY_LOW, &CmRTCDrvFailLog,
                  sizeof(CM_RTC_DRV_PBIT_LOG), NULL );
}
/*vcast_dont_instrument_end*/

#endif


/******************************************************************************
 * Function:    CMMsg_CreateLogs (clock.createlogs)
 *
 * Description: Create all the internal clock system logs
 *
 * Parameters:  USER_DATA_TYPE DataType
 *              USER_MSG_PARAM Param
 *              UINT32 Index
 *              const void *SetPtr
 *              void **GetPtr
 *
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Error processing command.
 *
 * Notes:       none
 *
 *****************************************************************************/
#ifdef GENERATE_SYS_LOGS

/*vcast_dont_instrument_start*/
USER_HANDLER_RESULT CMMsg_CreateLogs(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;

  CM_CreateAllInternalLogs();

  return result;
}
/*vcast_dont_instrument_end*/

#endif



/*****************************************************************************
 * Function:    CM_CompareSystemToMsClockDrift
 *
 * Description: Used to check clock drift between Control Processor and MicroServer.
 *              If difference is greater than prescribed limit, a sys log entry is written.
 *
 * Parameters:  none
 * Returns:     none
 *
 * Notes:
 *
 ****************************************************************************/
void CM_CompareSystemToMsClockDrift(void)
{
  #pragma ghs nowarning 1545 //Suppress packed structure alignment warning

  // If this is the first time since start that  MSSIM has been detected,
  // check for clock drift.
  // SRS-3091

  UINT32     msClockTime;
  UINT32     systemClockTime;
  CM_MS_SYS_TIME_DRIFT_LOG timeDriftLog;

  // Retrieve system and ms clock times
  timeDriftLog.MaxAllowedDriftSecs = CM_SYS_MS_CLOCK_DRIFT_MAX_SECS;
  timeDriftLog.MS = MSSC_GetLastMSSIMTime();
  CM_GetSystemClock(&timeDriftLog.System);

  // Convert to elapsed seconds
  msClockTime     = CM_GetSecSinceBaseYr( NULL, &timeDriftLog.MS);
  systemClockTime = CM_GetSecSinceBaseYr( NULL, &timeDriftLog.System);

  // If the difference between the clocks exceeds the limit, create log entry.
  if( abs(msClockTime - systemClockTime) > timeDriftLog.MaxAllowedDriftSecs )
  {
    LogWriteSystem(SYS_CM_MS_TIME_DRIFT_LIMIT_EXCEEDED, LOG_PRIORITY_LOW, &timeDriftLog,
                   sizeof(timeDriftLog), 0);
    GSE_DebugStr(NORMAL,FALSE, "ClockMgr: Sys/MS Clock delta > %ds",
                 CM_SYS_MS_CLOCK_DRIFT_MAX_SECS);
  }
#pragma ghs endnowarning
}

/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
/*****************************************************************************
 * Function:      CM_CBit | TASK
 *
 * Description:   Detects and corrects drift between the processor maintained
 *                RTC clock (RTC), and the RTC peripheral clock (Wallclock)
 *
 *
 * Parameters:    pParam - not used.  Generic header for system tasks
 *
 * Returns:       none
 *
 * Notes:  The clock test shall ensure the system clock is accurate to +/-
 *         six second every 10 minutes.
 *         The clock test shall record a fault when the time delta is
 *         greater than six seconds.
 *         The clock test shall set the system status to CAUTION if
 *         a fault is detected.
 *         The clock failure log shall contain the delta time when the
 *         error is recorded.
 *
 ****************************************************************************/
void CM_CBit(void* pParam)
{
  TIMESTRUCT sysTime, rtcTime;
  INT32 sysS = 0, rtcS = 0;
  INT32 timeDif;
  UINT32 intrLevel;
  CM_RTC_SYS_TIME_DRIFT_LOG timeDriftLog;

  //Get Clocks and convert to seconds
  //past midnight
  intrLevel = __DIR();
  if(RTC_GetTime(&rtcTime) == DRV_OK)
  {
    CM_GetSystemClock(&sysTime);
    __RIR(intrLevel);
    rtcS += rtcTime.Hour * CM_SEC_IN_HR;
    rtcS += rtcTime.Minute * CM_SEC_IN_MIN;
    rtcS += rtcTime.Second;

    sysS += sysTime.Hour * CM_SEC_IN_HR;
    sysS += sysTime.Minute * CM_SEC_IN_MIN;
    sysS += sysTime.Second;

    //Now, check for midnight roll over and compensate
    if(rtcTime.Day != sysTime.Day)
    {
      //The clock that has rolled over midnight will have a lower value
      //than the clock captured before midnight, add 24 hours of seconds
      if(rtcS < sysS)
      {
        rtcS += (CM_SEC_IN_MIN * CM_MIN_IN_HR * CM_HR_IN_DAY);
      }
      else
      {
        sysS += (CM_SEC_IN_MIN * CM_MIN_IN_HR * CM_HR_IN_DAY);
      }
    }

    //Get time difference
    timeDif = abs(rtcS - sysS);

    if(timeDif > CM_SYS_RTC_CLOCK_DRIFT_MAX_SECS)
    {
      timeDriftLog.RTC    = rtcTime;
      timeDriftLog.System = sysTime;
      timeDriftLog.MaxAllowedDriftSecs = CM_SYS_RTC_CLOCK_DRIFT_MAX_SECS;
        Flt_SetStatus(STA_CAUTION, SYS_CM_CBIT_TIME_DRIFT_LIMIT_EXCEEDED,
                      &timeDriftLog, sizeof(timeDriftLog));
      //6 second difference exceeded
      CM_SynchRTCtoWallClock();
      GSE_DebugStr(NORMAL,FALSE, "ClockMgr: Clock drift > %ds in 10 min",
                   CM_SYS_RTC_CLOCK_DRIFT_MAX_SECS);
    }

  }
  else
  {
    __RIR(intrLevel);
    //Wallclock read failed
  }

}

/*****************************************************************************
 * Function:    CM_TimeSourceSyncToRemote
 *
 * Description: Sync the FAST internal system clock(s) to the Remote Time Source
 *
 * Parameters:  TIMESTRUCT cm_time_struct - Remote Time Source
 *
 * Returns:     none
 *
 * Notes:       As of FAST 2.1 'Remote' refers to the MS, since CP can
 *              no longer be configured to sync directly to Ground Station
 *
 ****************************************************************************/
static
void CM_TimeSourceSyncToRemote ( TIMESTRUCT cm_time_struct )
{
  TIMESTRUCT mssc_TimeStruct;
  UINT32 mssc_SecSinceBaseYr;
  UINT32 cm_SecSinceBaseYr;
  UINT32 time_diff;

  // Have we received heart beat from MS ?
  // NOTE: Expect that when heart beat goes away ("data loss"), FALSE will be
  //       returned
  // NOTE: The time out for heart beat loss must be < 1 min otherwise
  //       the logic below will re-sync the system clock
  // NOTE: To make logic simplier, will not have test to ensure time is
  //       "ticking from ms".

  // Update system clock and RTC if configured for non-local,
  // MS is alive and time difference is > 1 minute !
  if ( FAST_TimeSourceCfg() != TIME_SOURCE_LOCAL &&
       MSSC_GetIsAlive() == TRUE )
  {
    // Get current ms time.
    // NOTE: Heart beat is at 1 sec and timeout to be 10 sec
    mssc_TimeStruct = MSSC_GetLastMSSIMTime();

    // If ms < 1997 then it is not a correct time !
    if ( (mssc_TimeStruct.Year >= CM_BASE_YEAR) &&
         (mssc_TimeStruct.Year <= CM_BASE_YEAR_MAX) )
    {
      // Convert ms timestruct to sec since base year
      mssc_SecSinceBaseYr = CM_GetSecSinceBaseYr( NULL, &mssc_TimeStruct );

      // Get current time timestruct to sec since base year
      cm_SecSinceBaseYr = CM_GetSecSinceBaseYr( NULL, &cm_time_struct );

      // Determine time difference
      if ( mssc_SecSinceBaseYr > cm_SecSinceBaseYr )
      {
        time_diff = mssc_SecSinceBaseYr - cm_SecSinceBaseYr;
      }
      else
      {
        time_diff = cm_SecSinceBaseYr - mssc_SecSinceBaseYr;
      }

      // If not recording and time diff is >  CM_MS_TIME_DIFF, then sync system clk to MS
      if ( 0 == m_nRecRefCnt && time_diff > CM_MS_TIME_DIFF_SECS )
      {
        // Update system clock, log created within func call
        CM_SetSystemClock( &mssc_TimeStruct, TRUE );

        // Update RTC clock, log created within func call
        CM_SetRTCClock( &mssc_TimeStruct, TRUE );
      }
    }

  } // End of MSSC_GetIsAlive()

}


/*****************************************************************************
 * Function:    CM_CreateClockUpdateLog
 *
 * Description: Creates the Clock Update Change Log
 *
 * Parameters:  SysId  SYS_CM_CLK_UPDATE_RTC or SYS_CM_CLK_UPDATE_CPU
 *              currTime Current time
 *              newTime  Time to change to
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
static
void CM_CreateClockUpdateLog( SYS_APP_ID SysId, TIMESTRUCT *currTime,
                              TIMESTRUCT *newTime )
{
  CM_CLOCKUPDATE_LOG clockUpdateLog;

  clockUpdateLog.currTime = *currTime;
  clockUpdateLog.newTime = *newTime;

  LogWriteSystem( SysId, LOG_PRIORITY_LOW, &clockUpdateLog,
                  sizeof(CM_CLOCKUPDATE_LOG), NULL );
}

/*****************************************************************************
 * Function:    CM_UpdateRecordingState
 *
 * Description: Updates global variable m_nRecRefCnt when FAST recording state
 *              changes. ClockMgr uses this to control whether to allow a
 *              clock sync to MS to occur. Sync is not allowed while
 *              recording/ downloading.
 *
 * Parameters:  bRec  TRUE  - Increment the ref count by one
 *                    FALSE - Decrement the ref count by one
 *
 * Returns:     none
 *
 * Notes:       none
 *
 ****************************************************************************/
void CM_UpdateRecordingState(BOOLEAN bRec)
{
  // incr/decr the ref count of recorders/downloaders accordingly.
  m_nRecRefCnt += bRec ? 1 : -1;

  // Prevent ref count from going negative.
  m_nRecRefCnt = (m_nRecRefCnt >= 0) ? m_nRecRefCnt : 0;

  /*GSE_DebugStr(NORMAL,TRUE,
               "Clk Sync inhibit ref cnt %s to: %d", bRec ? "incremented" : "decremented",
               m_nRecRefCnt);
  */
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: ClockMgr.c $
 * 
 * *****************  Version 59  *****************
 * User: Peter Lee    Date: 3/22/16    Time: 12:12p
 * Updated in $/software/control processor/code/system
 * Code review updates.  
 * 
 * *****************  Version 58  *****************
 * User: Peter Lee    Date: 3/02/16    Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #1305.  Base year is 1997 not 1996.  Func will still work for "how"
 * it's used (as a comparison between two times), but will not accurately
 * return "seconds since base year" without this update if this is
 * required in a future update. 
 * 
 * *****************  Version 57  *****************
 * User: Peter Lee    Date: 2/26/16    Time: 1:49p
 * Updated in $/software/control processor/code/system
 * SCR #1305. Update copywrite and "LEAR" to "LEAP".
 * 
 * *****************  Version 56  *****************
 * User: Peter Lee    Date: 2/24/16    Time: 7:45p
 * Updated in $/software/control processor/code/system
 * SCR #1305 Time Rollover @ MidNight
 * 
 * *****************  Version 55  *****************
 * User: John Omalley Date: 1/29/15    Time: 4:43p
 * Updated in $/software/control processor/code/system
 * SCR 1164 - Code Review Update!!!
 * 
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 1/23/15    Time: 7:53p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Code Review compliance update
 * 
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 12/02/14   Time: 2:26p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording CR
 * compliance
 *
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 11/12/14   Time: 3:46p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording Remove
 * DebugStr
 *
 * *****************  Version 51  *****************
 * User: Contractor V&v Date: 9/04/14    Time: 2:44p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording - Add
 * download constraint.
 *
 * *****************  Version 50  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording - FSM
 * handling.and CR updates
 *
 * *****************  Version 49  *****************
 * User: Contractor V&v Date: 8/26/14    Time: 4:20p
 * Updated in $/software/control processor/code/system
 * SCR #1164 - Permit CP Time Syncing only when Not Recording
 *
 * *****************  Version 48  *****************
 * User: Contractor V&v Date: 7/28/14    Time: 6:48p
 * Updated in $/software/control processor/code/system
 * Removed support for TIME_SOURCE_REMOTE
 *
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 12-11-27   Time: 7:06p
 * Updated in $/software/control processor/code/system
 * SCR #1156
 *
 * *****************  Version 46  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 1:11p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Error
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 9/22/10    Time: 2:28p
 * Updated in $/software/control processor/code/system
 * SCR #841 PBIT RTC faults are recorded with invalid time
 * Added default time/date initialization.
 * Also made sure system clock year is 4 digits. Timestamp conversion
 * depends on 4 digit year. Added validity check of call to RTC_ReadTime
 * during pre-init operation.
 *
 * *****************  Version 43  *****************
 * User: John Omalley Date: 9/20/10    Time: 11:33a
 * Updated in $/software/control processor/code/system
 * SCR 843 - Changed Default Time to 97
 *
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 5:22p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 *
 * *****************  Version 41  *****************
 * User: Jeff Vahue   Date: 8/28/10    Time: 12:22p
 * Updated in $/software/control processor/code/system
 * SCR# 830 - GENERATE_SYS_LOGS exclude area from coverage results
 *
 * *****************  Version 40  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 *
 * *****************  Version 39  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 *
 * *****************  Version 38  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 37  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 *
 * *****************  Version 36  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #579 Change LogWriteSys to return void
 *
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:53p
 * Updated in $/software/control processor/code/system
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 34  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/system
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 *
 * *****************  Version 33  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:10p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 *
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 *
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 5:03p
 * Updated in $/software/control processor/code/system
 * Fix comment error
 *
 * *****************  Version 29  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR 67 Use RTC for time until init complete
 *
 * *****************  Version 28  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:58p
 * Updated in $/software/control processor/code/system
 * SCR# 455 - remove LINT issues
 *
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 9:20a
 * Updated in $/software/control processor/code/system
 * SCR #427 ClockMgr.c Issues and SRS-3090
 *
 * *****************  Version 25  *****************
 * User: Peter Lee    Date: 1/27/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #427 Misc Clock Updates
 *
 * *****************  Version 24  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:05p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 *
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 12/04/09   Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR# 350
 *
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 2:13p
 * Updated in $/software/control processor/code/system
 * Misc non-code updates to support WIN32 and spelling.
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 9/29/09    Time: 1:25p
 * Updated in $/software/control processor/code/system
 * SCR #280 Fix Compiler Warnings
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:16p
 * Updated in $/software/control processor/code/system
 * SCR 275 Support routines for Power On Usage Requirements
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 9/16/09    Time: 10:27a
 * Updated in $/software/control processor/code/system
 * SCR #258 CM_GetSystemClock() returns "0" during system initialization.
 *
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 4/29/09    Time: 11:27a
 * Updated in $/software/control processor/code/system
 * SCR #168
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 4/20/09    Time: 9:31a
 * Updated in $/control processor/code/system
 * SCR #161, #51 and added required logs
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/23/08   Time: 3:07p
 * Updated in $/control processor/code/system
 * Added clock pre-init that forces clock manager to server "get" time
 * requests from the RTC chip.  This is useful for maintaining time during
 * system startup, when the interrupt driven clock is not yet running
 *
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/16/08   Time: 5:46p
 * Updated in $/control processor/code/system
 * Added ConvertTimeStampToTimeStruct function and updated the SDD to
 * refelect the new function.
 *
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 10:20a
 * Updated in $/control processor/code/system
 * Remove unnecessary #define
 *
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 2:42p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 *
 *
 *
 ***************************************************************************/


