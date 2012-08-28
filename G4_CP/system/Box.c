#define BOXMGR_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:      Box.c       
    
    Description:  
    
    VERSION
    $Revision: 43 $  $Date: 10/27/10 4:02p $
   
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "User.h"
#include "GSE.h"
#include "ResultCodes.h"
#include "user.h"
#include "Box.h"
#include "LogManager.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define BOX_MAGIC_NUMBER 0x400DF00D
#define BOX_DEFAULT_SERINAL_NUMBER "000000"
#define BOX_STR_LIMIT 0,BOX_INFO_STR_LEN
#define POWER_OFF_ADJUSTMENT 500.0f   

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct {
  TIMESTAMP StartupPowerOnTime; // ts read from RTC on startup 
  UINT32    PrevPowerOnUsage;   // Prev Power On Usage from non-volatile memory
  UINT32    PowerOnCount;       // Current Power On Count
  UINT32    ElapsedPowerOnUsage_s; // Elapsed Power On Usage for current power on
                                //     + Prev Power On Usage
  BOOLEAN   bBoxInitCompleted;  // Box init completed and system clock running 
                                //   If FALSE, then determine power on time 
                                //   using delta from StartupPowerOnTime to 
                                //   current time in RTC (since System clock
                                //   not running yet)
                                //   If TRUE, then use elapsed time from 
                                //   system clock to calculate elapsed time. 
  UINT32    UserSetAdjustment;  // When the user updates the Power On Usage
                                //   we need to subtract the current usage time
                                //   from when the user updated the value to 
                                //   how long the box has been powered on 
                                //   current power on, in order for usage calc to 
                                //   be correct.  (Clock Tick Value - 10 msec)
                                
  UINT32    CPU_StartupBeforeClockInit;  // Time from when TTMR is initialized to  
                                         //   when .StartupPowerOnTime read.  
                                         // This time does not include processor.s
                                         //   time and DIO_Init().  It will be assumed 
                                         //   that this "non counted" time will be << 1 sec. 
                                         // Units in msec. 
                                  
} BOX_POWER_ON_INFO_RUNTIME; 

#define BOX_CPU_INIT_TIME  3500.0f   // 3.5 seconds 
#define MSEC_PER_TICK      10


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
BOX_UNIT_INFO Box_Info;
//BOX_POWER_ON_INFO Box_PowerOn_Info; 
//TIMESTAMP Box_StartupPowerOnTime; 
BOX_POWER_ON_INFO_RUNTIME Box_PowerOn_RunTime; 

// Include command tables here so local dependencies have been made.
#include "BoxUserTables.c"

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
void Box_ReInitFile(void);
void Box_BackGroundTask ( void *pParam ); 


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/******************************************************************************
 * Function:    Box_Init  
 *  
 * Description: Initialize the box system.  Open and check the NV data 
 *              containing the box serial number and board revisions. Start
 *              a task to update the power on time.
 *
 * Parameters:  void 
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void Box_Init(void)
{
  TCB    TaskInfo;
  RESULT result;

  User_AddRootCmd(&BoxRoot);

  //Open box file
  result =  NV_Open(NV_BOX_CFG);  
  if(SYS_OK == result)
  {
    NV_Read(NV_BOX_CFG,0,&Box_Info,sizeof(Box_Info));
    if(Box_Info.MagicNumber == BOX_MAGIC_NUMBER)
    {
      //File OK
    }
    else
    {
      GSE_DebugStr(NORMAL,TRUE,"Box: Bad magic number, restoring defaults");
      //Re-init file
      Box_ReInitFile();
    }
  }
  else
  {    
    // SRS-1957: The G4 system shall set the system condition to FAULT if both copies of 
    // system condition fail the CRC check.
    if (SYS_NV_FILE_OPEN_ERR == result)
    {
      Flt_SetStatus(STA_FAULT, SYS_ID_BOX_SYS_INFO_CRC_FAIL, NULL, 0);
    }    
    
    GSE_DebugStr(NORMAL,TRUE,"Box: Failed to open box data file, restoring defaults");
    //Re-init file
    Box_ReInitFile();
  }
  //If file invalid...
  //else file okay
  
  // Create Box Background Processing Task 
  memset(&TaskInfo, 0, sizeof(TaskInfo));  
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name),"Box BackGnd Processing",_TRUNCATE);
  TaskInfo.TaskID         = Box_Background_Processing;
  TaskInfo.Function       = Box_BackGroundTask;
  TaskInfo.Priority       = taskInfo[Box_Background_Processing].priority;
  TaskInfo.Type           = taskInfo[Box_Background_Processing].taskType;
  TaskInfo.modes          = taskInfo[Box_Background_Processing].modes;
  TaskInfo.MIFrames       = taskInfo[Box_Background_Processing].MIFframes;     
  TaskInfo.Rmt.InitialMif = taskInfo[Box_Background_Processing].InitialMif;      
  TaskInfo.Rmt.MifRate    = taskInfo[Box_Background_Processing].MIFrate;    
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = NULL;
  TmTaskCreate (&TaskInfo);
}


/******************************************************************************
 * Function:    Box_GetSerno  
 *  
 * Description: Get the box serial number string from the box configuration
 *
 * Parameters:  [out] SNStr: Buffer to receive the string
 *
 * Returns:     none
 *
 * Notes:       Receiving buffer must be at least BOX_INFO_STR_LEN long
 *
 *****************************************************************************/
void Box_GetSerno(INT8* SNStr)
{
  strncpy_safe(SNStr,BOX_INFO_STR_LEN,Box_Info.SN,_TRUNCATE);
}



/******************************************************************************
 * Function:    Box_GetRev  
 *  
 * Description: Get the box revision string from the box configuration
 *
 * Parameters:  [out] RevStr: Buffer to receive the string
 *
 * Returns:     none
 *
 * Notes:       Receiving buffer must be at least BOX_INFO_STR_LEN long
 *
 *****************************************************************************/
void Box_GetRev(INT8* RevStr)
{
  strncpy_safe(RevStr,BOX_INFO_STR_LEN,Box_Info.Rev,_TRUNCATE);
}



/******************************************************************************
 * Function:    Box_GetMfg  
 *  
 * Description: Get the box manufacture date string from the box configuration
 *
 * Parameters:  [out] MfgStr: Buffer to receive the string
 *
 * Returns:     none
 *
 * Notes:       Receiving buffer must be at least BOX_INFO_STR_LEN long
 *
 *****************************************************************************/
void Box_GetMfg(INT8* MfgStr)
{
  strncpy_safe(MfgStr,BOX_INFO_STR_LEN,Box_Info.MfgDate,_TRUNCATE);  
}


/******************************************************************************
 * Function:    Box_GetPowerOnTime  
 *  
 * Description: Get the total box up time in seconds 
 *
 * Parameters:  none
 *
 * Returns:     UINT32
 *
 * Notes:
 *
 *****************************************************************************/
UINT32 Box_GetPowerOnTime(void)
{
  Box_PowerOn_UpdateElapsedUsage ( FALSE, POWERCOUNT_UPDATE_BACKGROUND, 
                                   NV_PWR_ON_CNTS_RTC );

  return Box_PowerOn_RunTime.ElapsedPowerOnUsage_s;
}



/******************************************************************************
 * Function:    Box_GetPowerOnCount
 *  
 * Description: Get the number of times the box has powered on
 *
 * Parameters:  none
 *
 * Returns:     UINT32: PowerOnCount is the number of times the box
 *                      has been powered on
 *
 * Notes:      
 *
 *****************************************************************************/
UINT32 Box_GetPowerOnCount(void)
{
  return Box_PowerOn_RunTime.PowerOnCount;
}




/******************************************************************************
 * Function:    Box_PowerOn_SetStartupTime
 *  
 * Description: Set the Power On Time 
 *
 * Parameters:  ts to set Box_StartupPowerOnTime 
 *
 * Returns:     none
 *
 * Notes:       
 *
 *****************************************************************************/
void Box_PowerOn_SetStartupTime( TIMESTAMP ts )
{

  Box_PowerOn_RunTime.StartupPowerOnTime = ts; 
  Box_PowerOn_RunTime.PrevPowerOnUsage = 0; 
  Box_PowerOn_RunTime.PowerOnCount = 0; 
  Box_PowerOn_RunTime.ElapsedPowerOnUsage_s = 0; 
  Box_PowerOn_RunTime.UserSetAdjustment = 0; 
  
  Box_PowerOn_RunTime.bBoxInitCompleted = FALSE; 
  
  Box_PowerOn_RunTime.CPU_StartupBeforeClockInit = TTMR_Get10msTickCount() * MSEC_PER_TICK; 
  
}


/******************************************************************************
 * Function:    Box_PowerOn_InitializeUsage
 *  
 * Description: Initializes the Power On Time logic
 *
 * Parameters:  none 
 *
 * Returns:     none
 *
 * Notes:       
 *  - Retrieves the stored Timer and Count from Non-Volatile Memory 
 *  - Stores current count back into Non-Volatile Memory to capture this
 *    Power Up. 
 *  - Should be called after PowerOn_SetStartupTime() is called
 *  - Should be called before PM_Init().  Once PM_Init() is called, 
 *    PM shutdown processing can be called where Box_UpdateElapsedPowerOnUsage()
 *    will be exec. 
 *  - Create system log if both EE and RTC values are corrupted 
 *
 *****************************************************************************/
void Box_PowerOn_InitializeUsage( void )
{
  RESULT resultEE; 
  RESULT resultRTC;
  
  BOX_POWER_USAGE_RETRIEVE_LOG pbit_log; 
  
  BOX_POWER_ON_INFO info; 
  

  // Retrieves the stored counters from non-voltaile 
  //  Note: NV_Open() performs checksum and creates sys log if checksum fails !
  resultEE  = NV_Open(NV_PWR_ON_CNTS_EE); 
  resultRTC = NV_Open(NV_PWR_ON_CNTS_RTC); 
  
  // Clear both copies to start   
  memset( (void *) &info, 0, sizeof(BOX_POWER_ON_INFO));
  
  
  if ((resultEE != SYS_OK) && (resultRTC != SYS_OK))
  {
    pbit_log.logReason = SYS_ID_BOX_POWER_ON_COUNT_RETRIEVE_FAIL; 
  
    // Both locations are corrupt ! Create sys log ? Will this be a duplicate ? 
    LogWriteSystem( SYS_ID_BOX_POWER_ON_COUNT_RETRIEVE_FAIL, LOG_PRIORITY_LOW, &pbit_log, 
                    sizeof(BOX_POWER_USAGE_RETRIEVE_LOG), NULL );
  }
  else if (resultRTC != SYS_OK) 
  {
    // Read from EE version.
    NV_Read(NV_PWR_ON_CNTS_EE, 0, &info, sizeof(BOX_POWER_ON_INFO));
  }
  else // resultEE != SYS_OK or both are GOOD 
  {
    // If both are GOOD then since RTC is saved first, it will most likely be the 
    //   most update to date.  
    
    // Read from RTC version
    NV_Read(NV_PWR_ON_CNTS_RTC, 0, &info, sizeof(BOX_POWER_ON_INFO)); 
  }  
  
  // At this point, we conside a Box Power Up 
  info.PowerOnCount++; 
  
  // Save run time copies of the counts 
  Box_PowerOn_RunTime.PrevPowerOnUsage = info.PowerOnUsage; 
  Box_PowerOn_RunTime.PowerOnCount = info.PowerOnCount; 

  // Update PowerOnTime.  Add in box init time, up to the point where this func is 
  //   called.  Will have to adjust for the entire box init time later in  
  //   Box_StartPowerOnTimeCounting().  
  Box_PowerOn_UpdateElapsedUsage( FALSE, POWERCOUNT_UPDATE_NOW, NV_PWR_ON_CNTS_RTC ); 
  Box_PowerOn_UpdateElapsedUsage( FALSE, POWERCOUNT_UPDATE_NOW, NV_PWR_ON_CNTS_EE ); 
  
}


/******************************************************************************
 * Function:    Box_PowerOn_StartTimeCounting
 *  
 * Description: Starts the Box Power On time counting using system clock()
 *
 * Parameters:  none 
 *
 * Returns:     none
 *
 * Notes:       
 *  - Set bBoxInitCompleted to indicate box init complete and system clock 
 *    will be running 
 *  - Called after Box_IntializePowerOnTimer() has been executed
 *  - This func must be called at end of Box Init, just before operation, 
 *    where system clock will be running (i.e. Interrupts are enabled and 
 *    system clock tick is running). 
 *
 *****************************************************************************/
void Box_PowerOn_StartTimeCounting( void )
{
  // At this point all of Box Init has completed, incl the potentially long 
  //    data flash erase.  Update power on counts and update .PrevPowerOnUsage 
  Box_PowerOn_UpdateElapsedUsage( FALSE, POWERCOUNT_UPDATE_BACKGROUND, NV_PWR_ON_CNTS_RTC );
  Box_PowerOn_UpdateElapsedUsage( FALSE, POWERCOUNT_UPDATE_BACKGROUND, NV_PWR_ON_CNTS_EE ); 
  
  Box_PowerOn_RunTime.bBoxInitCompleted = TRUE; 
  
  // Re-Update the Time to include transition from using RTC to using CM_GetTickCount() in
  //   Box_PowerOn_UpdateElapsedUsage() now that TaskManager is running.  
  Box_PowerOn_RunTime.CPU_StartupBeforeClockInit = TTMR_Get10msTickCount() * MSEC_PER_TICK;
  
}


/******************************************************************************
 * Function:    Box__PowerOn_UpdateElapsedUsage
 *  
 * Description: Updates the elapsed power on usage and stores to non-volatile 
 *              memory 
 *
 * Parameters:  BOOLEAN                   bPadForTruncation,
 *              POWERCOUNT_UPDATE_ACTION  UpdateAction,
 *              NV_FILE_ID                nv_location
 *
 * Returns:     void
 *
 * Notes:       
 *  - Calc power on usage from RTC if system clock not initialized yet. 
 *  - Adjust for the 0.5 second loss due to power off over time (power on
 *    usage has resolution of 1 sec, thus 0.5 sec over time is a good 
 *    adjustment) 
 *
 *****************************************************************************/
void Box_PowerOn_UpdateElapsedUsage ( BOOLEAN bPadForTruncation, 
                                      POWERCOUNT_UPDATE_ACTION UpdateAction,
                                      NV_FILE_ID nv_location ) 
{
  FLOAT32 ftemp;
  
  TIMESTAMP ts; 
  BOX_POWER_ON_INFO info; 
  UINT32 startup_time_sec; 
  
  if ( Box_PowerOn_RunTime.bBoxInitCompleted == FALSE) 
  {
    // Elapse time calc from RTC ts since System Clock not running yet. 
    CM_GetTimeAsTimestamp( &ts ); 
    startup_time_sec = CM_GetSecSinceBaseYr(&ts, NULL) - 
                       CM_GetSecSinceBaseYr(&Box_PowerOn_RunTime.StartupPowerOnTime, NULL);
    // use msec for intermediate calc
    ftemp = (FLOAT32) (startup_time_sec * MILLISECONDS_PER_SECOND); 
  }
  else 
  {
    // use msec for intermediate calc 
    ftemp = (FLOAT32) CM_GetTickCount(); 
  }
  
  // Adjust for CPU Processor.s init time 
  // ftemp += BOX_CPU_INIT_TIME; 
  ftemp += Box_PowerOn_RunTime.CPU_StartupBeforeClockInit; 
  
  // Adjust for any user updates / changes to Power On Usage value 
  ftemp = ftemp - Box_PowerOn_RunTime.UserSetAdjustment; 

  // Adjust for the 0.5 sec loss on every power off.  
  //   This has worked well for G3 
  if (bPadForTruncation)
  {
    ftemp += POWER_OFF_ADJUSTMENT;    // Since the power on monitor runs every 1 sec 
                                      //   we can missed between 0 and 0.999 sec due to 
                                      //   power down.  If we add 0.500 sec, over time 
                                      //   this will approx the actually usage.  
  }
  // Convert to seconds resolution 
  ftemp *= 1.0f / ((FLOAT32) MILLISECONDS_PER_SECOND); 
  
  
  info.PowerOnUsage = Box_PowerOn_RunTime.PrevPowerOnUsage + (UINT32) ftemp; 
  info.PowerOnCount = Box_PowerOn_RunTime.PowerOnCount; 
  
  Box_PowerOn_RunTime.ElapsedPowerOnUsage_s = info.PowerOnUsage; 
  
  switch ( UpdateAction ) 
  {
    case POWERCOUNT_UPDATE_NOW:
      NV_WriteNow( (NV_FILE_ID) nv_location, 0, (void *) &info, 
                             sizeof(BOX_POWER_ON_INFO) );
/*                             
      NV_WriteNow( (NV_FILE_ID) BOX_POWER_ON_COUNTS_EE, 0, (void *) &info, 
                             sizeof(BOX_POWER_ON_INFO) );
*/                             
      break; 
      
    case POWERCOUNT_UPDATE_BACKGROUND:
      NV_Write( (NV_FILE_ID) nv_location, 0, (void *) &info, 
                             sizeof(BOX_POWER_ON_INFO) );
/*                             
      NV_Write( (NV_FILE_ID) BOX_POWER_ON_COUNTS_EE, 0, (void *) &info, 
                             sizeof(BOX_POWER_ON_INFO) );
*/                             
      break; 
      
    default:
      FATAL ("Unexpected POWERCOUNT_UPDATE_ACTION = %d ", UpdateAction);
      break;   
  }
  
}



/*****************************************************************************/
/* Local Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    Box_ReInitFile
 *  
 * Description: Set the NV data (box serial number, board revisions) to 
 *              the default values
 *
 * Parameters:  void 
 *
 * Returns:     void
 *
 * Notes:
 *
 *****************************************************************************/
void Box_ReInitFile(void)
{
  CHAR ResultStr[RESULTCODES_MAX_STR_LEN];

  memset(&Box_Info,0,sizeof(Box_Info));
  strncpy_safe(Box_Info.SN, sizeof(Box_Info.SN), BOX_DEFAULT_SERINAL_NUMBER,_TRUNCATE);
  Box_Info.MagicNumber = BOX_MAGIC_NUMBER;
  NV_Write( NV_BOX_CFG, 0, &Box_Info, sizeof(Box_Info));
  GSE_StatusStr( NORMAL, RcGetResultCodeString(SYS_OK, ResultStr));
}

/******************************************************************************
 * Function:    Box_BackGroundTask
 *  
 * Description: Repository of box background tasks 
 *
 * Parameters:  void *pParam
 *
 * Returns:     none
 *
 * Notes:       
 *   - Periodically updates and stores Power On Usage to RTC memory 
 *   - Task runs at 1 second 
 *
 *****************************************************************************/
void Box_BackGroundTask ( void *pParam ) 
{
  // BOX_TASK_PARAMS_PTR pBoxBackGroundBlock; 
  
  // pBoxBackGroundBlock = (BOX_TASK_PARAMS_PTR) pParam; 
  
  // Update Power On Usage 
  Box_PowerOn_UpdateElapsedUsage( FALSE, POWERCOUNT_UPDATE_BACKGROUND, 
                                  NV_PWR_ON_CNTS_RTC );
}

/*************************************************************************
 *  MODIFICATIONS
 *    $History: Box.c $
 * 
 * *****************  Version 43  *****************
 * User: Jim Mood     Date: 10/27/10   Time: 4:02p
 * Updated in $/software/control processor/code/system
 * SCR 958 Update comment blocks found in code review
 * 
 * *****************  Version 42  *****************
 * User: Jeff Vahue   Date: 10/26/10   Time: 9:35p
 * Updated in $/software/control processor/code/system
 * SCR# 958 - Missed one JO
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:03p
 * Updated in $/software/control processor/code/system
 * SCR 961 - Code Review Updates
 * 
 * *****************  Version 40  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/system
 * SCR 858 - Added FATAL to default cases
 * 
 * *****************  Version 39  *****************
 * User: John Omalley Date: 9/09/10    Time: 2:15p
 * Updated in $/software/control processor/code/system
 * SCR 855 - Removed Unused functions
 * 
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 8/30/10    Time: 3:28p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 37  *****************
 * User: Contractor3  Date: 7/29/10    Time: 11:10a
 * Updated in $/software/control processor/code/system
 * SCR #698 - Fix code review findings
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 5:48p
 * Updated in $/software/control processor/code/system
 * SCR #582   Update Power On Usage to better measure processor init time.
 * Fix bug when transitioning to use CM_GetClock(). 
 * 
 * *****************  Version 35  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:16p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #680, SCR #681
 * 
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:09p
 * Updated in $/software/control processor/code/system
 * SCR #483 Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 32  *****************
 * User: Contractor3  Date: 6/10/10    Time: 10:44a
 * Updated in $/software/control processor/code/system
 * SCR #642 - Changes based on Code Review
 * 
 * *****************  Version 31  *****************
 * User: Contractor2  Date: 5/28/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR #613 Box Board array needs to hold at least 7 items
 * Increased max num of boards value to 10.
 * Changed magic number for file reset to new size.
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:10p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc Code review changes
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:50p
 * Updated in $/software/control processor/code/system
 * SCR #543 Misc - Fix spelling retreive -> retrieve
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:47p
 * Updated in $/software/control processor/code/system
 * "Box Background Processing" task name too long.  Shorten to 
 * "Box BackGnd Processing"
 * 
 * *****************  Version 26  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:09p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 24  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:30p
 * Updated in $/software/control processor/code/system
 * SCR #279 Making NV filenames uppercase
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 22  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:57p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 21  *****************
 * User: Contractor V&v Date: 2/03/10    Time: 2:57p
 * Updated in $/software/control processor/code/system
 * SCR 279, SCR 18
 * 
 * *****************  Version 20  *****************
 * User: Jeff Vahue   Date: 1/26/10    Time: 5:20p
 * Updated in $/software/control processor/code/system
 * SCR# 397 Correct a cut and paste error during implementation
 * 
 * *****************  Version 19  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:02p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 18  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 17  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:42p
 * Updated in $/software/control processor/code/system
 * SCR 106
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/system
 * SCR# 359
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:15p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 5:57p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 * 
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:50p
 * Updated in $/software/control processor/code/system
 * Implement SCR 138 Consistency with naming index of [0]
 * 
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/19/09   Time: 4:13p
 * Updated in $/software/control processor/code/system
 * Added externally callable functions for getting the power on count and
 * power on time.
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 11:48a
 * Updated in $/software/control processor/code/system
 * SCR #275 Fix comment 
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 10/08/09   Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #292 Misc Code Review Findings 
 * 
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 9/29/09    Time: 11:58a
 * Updated in $/software/control processor/code/system
 * SCR# 178 Updated User Manager Tables
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:15p
 * Updated in $/software/control processor/code/system
 * SCR 275 Box Power On Usage Requirements
 * 
 * *****************  Version 7  *****************
 * User: Jim Mood     Date: 8/18/09    Time: 2:31p
 * Updated in $/software/control processor/code/system
 * SCR #247 box.board parameters not set correctly
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/29/09    Time: 3:15p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 *
 ***************************************************************************/
