#ifndef FAULT_H
#define FAULT_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        fault.h
    
    Description: Fault Definitions

    Notes: 
  
    VERSION
      $Revision: 8 $  $Date: 8/28/12 1:43p $     
  
******************************************************************************/
/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_basic.h"
#include "trigger.h"

/******************************************************************************
                                   Package Defines                             
******************************************************************************/
#define MAX_FAULTS             32
#define MAX_FAULTNAME          32
#define MAX_FAULT_TRIGGERS      4

// Default Configuration for a fault
#define FAULT_CRITERIA_DEFAULT 0.0F,                  /* fValue           */\
                               NO_COMPARE             /* Compare          */

#define FAULT_TRIG_DEFAULT     HAS_NO_FAULT,          /* SenseOf          */\
                               SENSOR_UNUSED,         /* SensorIndex      */\
                               FAULT_CRITERIA_DEFAULT

#define FAULT_CONFIG_DEFAULT  "Unused",               /* FaultName        */\
                              FAULT_TRIG_DEFAULT,     /* Trigger A        */\
                              FAULT_TRIG_DEFAULT,     /* Trigger B        */\
                              FAULT_TRIG_DEFAULT,     /* Trigger C        */\
                              FAULT_TRIG_DEFAULT,     /* Trigger D        */\
                         
//Default configuration for the faults
#define FAULT_CONFIGS_DEFAULT  {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT},\
                               {FAULT_CONFIG_DEFAULT}


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
  HAS_A_FAULT = 1,
  HAS_NO_FAULT,
} FAULT_SENSE;

typedef struct
{
   FAULT_SENSE   SenseOfSignal;
   SENSOR_INDEX   SensorIndex;
   TRIG_CRITERIA SensorTrigger;   
} FAULT_TRIGGER;

typedef struct
{
   CHAR          faultName[MAX_FAULTNAME];
   FAULT_TRIGGER faultTrigger[MAX_FAULT_TRIGGERS];
} FAULT_CONFIG;

//A type for an array of the maximum number of faults
//Used for storing fault configurations in the configuration manager
typedef FAULT_CONFIG FAULT_CONFIGS[MAX_FAULTS];

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( FAULT_BODY )
  #define EXPORT
#else
  #define EXPORT extern
#endif

/******************************************************************************
                             Package Exports Variables
******************************************************************************/
#if defined ( FAULT_BODY )
USER_ENUM_TBL FaultIndexType[] = 
{ { "0"  , FAULT_0   }, { "1"  , FAULT_1   }, { "2"  , FAULT_2   }, 
  { "3"  , FAULT_3   }, { "4"  , FAULT_4   }, { "5"  , FAULT_5   }, 
  { "6"  , FAULT_6   }, { "7"  , FAULT_7   }, { "8"  , FAULT_8   },
  { "9"  , FAULT_9   }, { "10" , FAULT_10  }, { "11" , FAULT_11  },
  { "12" , FAULT_12  }, { "13" , FAULT_13  }, { "14" , FAULT_14  },
  { "15" , FAULT_15  }, { "16" , FAULT_16  }, { "17" , FAULT_17  },
  { "18" , FAULT_18  }, { "19" , FAULT_19  }, { "20" , FAULT_20  },
  { "21" , FAULT_21  }, { "22" , FAULT_22  }, { "23" , FAULT_23  },
  { "24" , FAULT_24  }, { "25" , FAULT_25  }, { "26" , FAULT_26  },
  { "27" , FAULT_27  }, { "28" , FAULT_28  }, { "29" , FAULT_29  },
  { "30" , FAULT_30  }, { "31" , FAULT_31  }, 
  { "UNUSED", FAULT_UNUSED }, {NULL, 0}
};
#else
EXPORT USER_ENUM_TBL FaultIndexType[];
#endif

/******************************************************************************
                             Package Exports Functions
******************************************************************************/
EXPORT void    FaultInitialize    ( void );
EXPORT BOOLEAN FaultCompareValues ( FAULT_INDEX Fault );


#endif  // FAULT_H 
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Fault.h $
 * 
 * *****************  Version 8  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 8/31/10    Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 * 
 * *****************  Version 6  *****************
 * User: Jeff Vahue   Date: 6/18/10    Time: 4:40p
 * Updated in $/software/control processor/code/system
 * SCR# 649 - add NO_COMPARE back in for use in Signal Tests.
 * 
 * *****************  Version 5  *****************
 * User: Contractor2  Date: 5/03/10    Time: 4:07p
 * Updated in $/software/control processor/code/system
 * SCR 530: Error: Trigger NO_COMPARE infinite logs
 * Removed NO_COMPARE from code.
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 12/16/09   Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR 374
 * - Removed the Fault check type and deleted the Fault Update function.
 * 
 * *****************  Version 3  *****************
 * User: John Omalley Date: 10/21/09   Time: 4:09p
 * Updated in $/software/control processor/code/system
 * SCR 311
 * - Removed the comparison to previous for everything except NOT_EQUAL.
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:27p
 * Updated in $/software/control processor/code/system
 * Updated user tables
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 9/30/09    Time: 11:15a
 * Created in $/software/control processor/code/system
 * 
 ****************************************************************************/
