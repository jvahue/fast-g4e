#ifndef BUSTO_H
#define BUSTO_H

/******************************************************************************
Copyright (C) 2012 Pratt & Whitney Engine Services, Inc.
All Rights Reserved. Proprietary and Confidential..

  File:         BusTO.h

  Description: This header file includes the variables and funtions necessary
               for the Bus Timout Handler. See the c module for a detailed
               description.

   VERSION
   $Revision: 3 $  $Date: 12-11-01 2:48p $

******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "ResultCodes.h"
#include "SystemLog.h"

/******************************************************************************
                              Package Defines
******************************************************************************/


/******************************************************************************
                              Package Typedefs
******************************************************************************/

#pragma pack(1)

typedef struct
{
  RESULT  result;
} BTO_DRV_PBIT_LOG;

#pragma pack()

/******************************************************************************
                              Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( BUSTO_BODY )
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
EXPORT RESULT BTO_Init(SYS_APP_ID *SysLogId, void *pdata, UINT16 *psize);
EXPORT INT8*  BTO_Handler(void);
EXPORT void   BTO_WriteToEE(void);


#endif // BUSTO_H

/*************************************************************************
 *  MODIFICATIONS
 *    $History: BusTO.h $
 * 
 * *****************  Version 3  *****************
 * User: Melanie Jutras Date: 12-11-01   Time: 2:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 File Format Error
 *
 * *****************  Version 2  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 1  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:42p
 * Created in $/software/control processor/code/drivers
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 *
 *
 ***************************************************************************/


