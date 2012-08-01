#define DATAREDUCTION_BODY
/******************************************************************************
            Copyright (C) 2009-2010 Pratt & Whitney Engine Services, Inc.
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.

    File:         DataReduction.c

    Description:  The data reduction software is used to reduce the amount 
                  data that needs to be stored in the data flash. The data 
                  reduction will reduce the data based on tolerance
                  by linear curve fitting.
    
VERSION
     $Revision: 7 $  $Date: 9/27/10 2:55p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
#include <float.h>
#include <math.h>
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "datareduction.h"
/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/

/****************************************************************************/
/* Local Constants                                                          */
/****************************************************************************/

/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/

/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    DataReductionInit
 *
 * Description: This function must be called for the first point received before
 *              the data reduction can begin. 
 *
 * Parameters:  REDUCTIONDATASTRUCT *pData [in/out]
 *
 * Returns:     REDUCE_STATUS - RS_KEEP,
 *                              RS_DELETE,
 *                              RS_ERROR
 *
 * Notes:       Will Always return RS_KEEP
 *              pData->EngVal must be initialized with the engineering value
 *              pData->Time must be initialized with the time since power up
 *              pData->Tol must be initialized with the tolerance value
 *
 
 *****************************************************************************/
REDUCE_STATUS DataReductionInit(REDUCTIONDATASTRUCT *pData)
{
   // Local Data
   REDUCE_STATUS Status;
   
   // Initialize Local Data
   Status       = RS_KEEP;
   
   pData->b1    = pData->EngVal + pData->Tol;
   pData->b2    = pData->EngVal - pData->Tol;
   pData->m1    = 0;
   pData->m2    = 0;
   pData->T0    = pData->Time;
   pData->T_Old = pData->Time;
   
   return Status;
}


/******************************************************************************
 * Function:    DataReduction
 *
 * Description: The data reduction will reduce the data based on tolerance by 
 *              linear curve fitting.
 *
 * Parameters:  REDUCTIONDATASTRUCT *pData [in/out]
 *
 * Returns:     REDUCE_STATUS - RS_KEEP,
 *                              RS_DELETE,
 *                              RS_ERROR
 *              
 * Notes:       Each point must have called DataReductionInit before calling
 *              this function.
 *
 *****************************************************************************/
REDUCE_STATUS DataReduction (REDUCTIONDATASTRUCT *pData)
{
   // Local Data
   REDUCE_STATUS Status;
   FLOAT32       m1;
   FLOAT32       m2;
   FLOAT32       EngVal; 
   
   // Initialize Local Data
   Status = RS_DELETE;   // There are only two paths to keep the point so initialize
                         // the status to delete

   if ((pData->m1 == 0) && (pData->m2 == 0) && (pData->Tol != 0))
   {
      // First Point after initialization - Delete Point
      // Initilize Time
      pData->T_Old = pData->Time;

      // Compute initial Slopes      
      pData->m1 = (pData->EngVal - pData->b1) / (pData->Time - pData->T0);
      pData->m2 = (pData->EngVal - pData->b2) / (pData->Time - pData->T0);
   }
   // Manage "0" tolerance for duplicate data reduction
   else if (pData->Tol == 0)
   {
      // Check if the floating point value has changed
      if (fabs(pData->EngVal - pData->b1) >= FLT_EPSILON)
      {
         // Adjust Time
         pData->T_Old = pData->Time;
         pData->T0    = pData->Time;
         pData->b1    = pData->EngVal;
         pData->b2    = pData->EngVal; 
         Status       = RS_KEEP;
      }
      else // Delete Point
      {
         // Save Previous Time
         pData->T_Old = pData->Time;
      }
   }
   else
   {
      // Check that two points haven't been received with the same time
      // This would cause a divide by zero
      if (pData->Time != pData->T0)
      {
         // Compute Upper and Lower Slopes
         m1 = (pData->EngVal - pData->b1) / (pData->Time - pData->T0);
         m2 = (pData->EngVal - pData->b2) / (pData->Time - pData->T0);
      
         m1 = MAX(m1, pData->m1);
         m2 = MIN(m2, pData->m2);

         if (m1 > m2)
         {
           EngVal = pData->EngVal;  // Save current value
      
           // Tolerance Exceeded - Set New Point Values
           // Compute EngVal
           pData->b1     = pData->m1 * (pData->T_Old - pData->T0) + pData->b1;
           pData->b2     = pData->m2 * (pData->T_Old - pData->T0) + pData->b2;
           pData->EngVal = (pData->b1 + pData->b2)/2;

           // Compute b1 and b2 
           pData->b1 = pData->EngVal + pData->Tol;
           pData->b2 = pData->EngVal - pData->Tol;

           // Save Time
           pData->T0    = pData->T_Old;
           pData->T_Old = pData->Time;

           // Compute m1 and m2
           pData->m1 =(EngVal - pData->b1) / (pData->Time - pData->T0);
           pData->m2 =(EngVal - pData->b2) / (pData->Time - pData->T0);

           Status = RS_KEEP;
         }
         else // In tolerance - Delete Point
         {
            // Save Previous Time
            pData->T_Old = pData->Time;
         
            // Adjust Upper and Lower Slope
            pData->m1 = MAX(m1, pData->m1);
            pData->m2 = MIN(m2, pData->m2);
         }
      }
   }
   return Status;  
}



/******************************************************************************
 * Function:    DataReductionGetCurrentValue
 *
 * Description: Returns the current "extrapolated" value 
 *
 * Parameters:  REDUCTIONDATASTRUCT *pData [in/out]
 *
 * Returns:     void
 *              
 * Notes:       Each point must have called DataReductionInit before calling
 *              this function.
 *
 *****************************************************************************/
void DataReductionGetCurrentValue (REDUCTIONDATASTRUCT *pData)
{
  FLOAT32 b1; 
  FLOAT32 b2; 
  
  b1 = (pData->m1 * (pData->Time - pData->T0)) + pData->b1; 
  b2 = (pData->m2 * (pData->Time - pData->T0)) + pData->b2; 
  
  pData->EngVal = (b1 + b2) / 2; 
  
}




/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/
 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: DataReduction.c $
 * 
 * *****************  Version 7  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 8/16/10    Time: 8:16p
 * Updated in $/software/control processor/code/drivers
 * SCR #797 DataReductionGetCurrentValue() does not return proper value
 * when tol == 0. 
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 7/15/10    Time: 4:43p
 * Updated in $/software/control processor/code/drivers
 * SCR 523 - Updated Floating Point Check for duplicate data
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 6/29/10    Time: 5:05p
 * Updated in $/software/control processor/code/drivers
 * SCR 664 - Updates for divide by zero and duplicate data reduction
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 5/28/10    Time: 6:57p
 * Updated in $/software/control processor/code/drivers
 * Fixed the calc of new m1 and m2 based on current 'EngVal' vs new
 * computed 'pData->EngVal'.   
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 5/25/10    Time: 9:23a
 * Updated in $/software/control processor/code/drivers
 * Add function DataReductionGetCurrentValue() 
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 5/17/10    Time: 11:22a
 * Created in $/software/control processor/code/drivers
 *
 *
 ***************************************************************************/


