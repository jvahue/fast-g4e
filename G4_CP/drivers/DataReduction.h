#ifndef DATAREDUCTION_H
#define DATAREDUCTION_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        DataReduction.h
    
    Description: Reduce the amount of data that needs to be stored
                 in the data flash.
    
VERSION
     $Revision: 3 $  $Date: 9/27/10 2:55p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/


/******************************************************************************
                                 Package Defines
******************************************************************************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct 
{
  UINT32  Time;   // Current time, is the time when EngVal was received
  FLOAT32 EngVal; // Engineering value, is the current engineering value
  FLOAT32 Tol;    // Tolerance on the current parameter, same scale as EngVal
  FLOAT32 m1;     // Upper Slope (scaled to the engineering value)
  FLOAT32 b1;     // Upper slope y-intercept (EngVal + Tol) of a point recorded
  FLOAT32 m2;     // Lower slope (scaled to the engineering value)
  FLOAT32 b2;     // Lower slope y-intercept (EngVal - Tol) of a point recorded
  UINT32  T_Old;  // Is the time value of the previouis point received.
  UINT32  T0;     // Time at y-intercept (b1 and b2)
} REDUCTIONDATASTRUCT, *REDUCTIONDATASTRUCT_PTR;

typedef enum
{
   RS_KEEP,
   RS_DELETE,
   RS_ERROR
} REDUCE_STATUS;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( DATAREDUCTION_BODY )
   #define EXPORT
#else
   #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT REDUCE_STATUS DataReductionInit   ( REDUCTIONDATASTRUCT *pData );
EXPORT REDUCE_STATUS DataReduction       ( REDUCTIONDATASTRUCT *pData );
EXPORT void DataReductionGetCurrentValue ( REDUCTIONDATASTRUCT *pData ); 

#endif // DATAREDUCTION_H               


/*************************************************************************
 *  MODIFICATIONS
 *    $History: DataReduction.h $
 * 
 * *****************  Version 3  *****************
 * User: Contractor2  Date: 9/27/10    Time: 2:55p
 * Updated in $/software/control processor/code/drivers
 * SCR #795 Code Review Updates
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
