#define QAR_MANAGER_BODY
/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        QARManager.c

    Description: System level QAR manager functions to support debug display 
                 and QAR system initialization (i.e. register QAR Manager 
                 user cmds)
    
    VERSION
      $Revision: 33 $  $Date: 8/28/12 1:43p $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "qarmanager.h"
#include "stddefs.h"
#include "user.h"
#include "GSE.h"

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct 
{
  QAR_SF       LastServiced;    // For QAR Debug Display 
} QARMGR_DATA; 

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static QAR_MGR_TASK_PARMS QarMgrBlock;

UINT16 lcQARFrame[QAR_SF_MAX_SIZE];

#define QAR_DISPLAY_WORD_SIZE 11
static INT8   Str[QAR_SF_MAX_SIZE * QAR_DISPLAY_WORD_SIZE];

static QARMGR_DATA QarMgr_Data;

static QAR_OUTPUT_TYPE m_QarOutputType;

/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#include "QARUserTables.c" // Include user cmd tables & functions .c 

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:     QARMgr_Initialize
 *
 * Description:
 *
 * Parameters:   None
 *
 * Returns:      None
 *
 * Notes:
 *
 *****************************************************************************/
void QARMgr_Initialize (void)
{
   // Local Data
   TCB TaskInfo;

   //Add an entry in the user message handler table
   User_AddRootCmd(&QarRootTblPtr);

   QAR_Reframe();
   
   // Clear and initialize global variables 
   memset ( &QarMgrBlock, 0, sizeof(QarMgrBlock) ); 

   QarMgr_Data.LastServiced = QAR_SF4;

   //Initialize GSE outstream as disabled.
   m_QarOutputType = QAR_GSE_NONE;

   // Create QAR Manager Task
   memset(&TaskInfo, 0, sizeof(TaskInfo));
   strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "QAR Manager",_TRUNCATE);
   TaskInfo.TaskID         = QAR_Manager;
   TaskInfo.Function       = QARMgr_Task;
   TaskInfo.Priority       = taskInfo[QAR_Manager].priority;
   TaskInfo.Type           = taskInfo[QAR_Manager].taskType;
   TaskInfo.modes          = taskInfo[QAR_Manager].modes;
   TaskInfo.MIFrames       = taskInfo[QAR_Manager].MIFframes;
   TaskInfo.Rmt.InitialMif = taskInfo[QAR_Manager].InitialMif;
   TaskInfo.Rmt.MifRate    = taskInfo[QAR_Manager].MIFrate;
   TaskInfo.Enabled        = TRUE;
// TaskInfo.Enabled        = FALSE;
   TaskInfo.Locked         = FALSE;
   TaskInfo.pParamBlock    = &QarMgrBlock;
   TmTaskCreate (&TaskInfo);

}


/******************************************************************************
 * Function:     QARMgr_Task
 *
 * Description:  Outputs RAW QAR SF to the GSE or UART port for debugging 
 *
 * Parameters:   pParam - Ptr to QarMgrBlock data
 *
 * Returns:      None. 
 *
 * Notes:        
 *  1) Used for debugging only 
 *
 * Rev: 
 *  07/31/2009 QAR_GetSubFrameDisplay() now return byte cnt rather than QAR word cnt 
 *
 *****************************************************************************/
void QARMgr_Task( void *pParam )
{
   // Local Data
   UINT16             i;
   UINT16             nCnt;
   UINT16             j;
   
   // If Display Output Enabled 
   if ( m_QarOutputType != QAR_GSE_NONE ) 
   {

     // Get the next subframe
     nCnt = QAR_GetSubFrameDisplay ( (void *) &lcQARFrame[0], 
                                     (QAR_SF *) &QarMgr_Data.LastServiced, 
                                     QAR_SF_MAX_SIZE * sizeof(UINT16) );
     nCnt = nCnt / sizeof(UINT16);                                      
  
     // if (SubFrame != QAR_SF_MAX)
     if (nCnt != 0)
     {
        j = 0;
        // limit to words returned 
        // for (i=0;((i<nCnt) && (i<64));i++) 
        for (i=0; ((i<nCnt) && (i<QAR_SF_MAX_SIZE)); i++) 
        {                            //1023:000rn
          sprintf ( (char *) &Str[j], "%04d:%03x\r\n\0", i, lcQARFrame[i] ); 
          j = j + QAR_DISPLAY_WORD_SIZE - 1; // Point to "\0" and over write ! 
        }
        GSE_PutLine( Str ); 
     } // End of if nCnt != 0 
   } // End of if ( pCfg->Output != QAR_GSE_NONE ) 
}


/******************************************************************************
 * Function:     QARMgr_DisableLiveStream
 *
 * Description:  Disables the outputting the live stream for QAR 
 *
 * Parameters:   None
 *
 * Returns:      None. 
 *
 * Notes:        
 *  1) Used for debugging only 
 *
 *
 *****************************************************************************/
void QARMgr_DisableLiveStream(void)
{
  m_QarOutputType = QAR_GSE_NONE;
}


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: QARManager.c $
 * 
 * *****************  Version 33  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 3:12p
 * Updated in $/software/control processor/code/system
 * SCR #840 Code Review Updates
 * 
 * *****************  Version 31  *****************
 * User: Contractor3  Date: 7/16/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR #702 - Changes based on code review.
 * 
 * *****************  Version 30  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:25p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 28  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #317 Implement safe strncpy
 * 
 * *****************  Version 27  *****************
 * User: Jeff Vahue   Date: 4/06/10    Time: 3:50p
 * Updated in $/software/control processor/code/system
 * SCR# 526 - Increase Max GSE Tx FIFO size
 * 
 * *****************  Version 26  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 3/12/10    Time: 4:55p
 * Updated in $/software/control processor/code/system
 * SCR# 483 - Function Names
 * 
 * *****************  Version 24  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 1:28p
 * Updated in $/software/control processor/code/system
 * SCR# 364 - remove unused/commented out code
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR# 397
 * 
 * *****************  Version 21  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 * 
 * *****************  Version 20  *****************
 * User: Contractor V&v Date: 12/10/09   Time: 5:46p
 * Updated in $/software/control processor/code/system
 * SCR 106
 * 
 * *****************  Version 19  *****************
 * User: Contractor V&v Date: 11/30/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * SCR 106 XXXUserTable.c consistency
 * 
 * *****************  Version 18  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 4:20p
 * Updated in $/software/control processor/code/system
 * Implement SCR 196  ESC' sequence when outputting continuous data to GSE
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 9/21/09    Time: 3:01p
 * Updated in $/software/control processor/code/system
 * SCR #266 Remove 64 word limit on QAR GSE Debug Display
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 7/31/09    Time: 3:18p
 * Updated in $/software/control processor/code/system
 * SCR 221 Return byte cnt and not word cnt when returning the Raw Data.
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 4:20p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 3:03p
 * Updated in $/control processor/code/system
 * Initialize QarMgrBlock var to 0
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:05p
 * Updated in $/control processor/code/system
 * SCR #97 - QAR SW Updates
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 3:54p
 * Updated in $/control processor/code/system
 * SCR #87 Function Prototype
 * 
 *
 *****************************************************************************/

