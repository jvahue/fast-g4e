#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

/******************************************************************************
            Copyright (C) 2010-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:         SPIManager.h
    
    Description:  Function prototypes and define for SPI Manager
    
    VERSION
      $Revision: 10 $  $Date: 7/18/12 6:27p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "EEProm.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#undef SPI_DEV
#define SPI_DEV(Id, Rate, HndlrFunc)
//                         Rate    Handler               
//         Id             (mSec)   Function              
#define SPI_DEV_LIST                                          \
  SPI_DEV(SPI_RTC,          250,  SPIMgr_UpdateRTC),          \
  SPI_DEV(SPI_AC_BUS_VOLT,   10,  SPIMgr_UpdateAnalogDevice), \
  SPI_DEV(SPI_AC_BAT_VOLT,   10,  SPIMgr_UpdateAnalogDevice), \
  SPI_DEV(SPI_LIT_BAT_VOLT, 250,  SPIMgr_UpdateAnalogDevice), \
  SPI_DEV(SPI_BOARD_TEMP,   250,  SPIMgr_UpdateAnalogDevice), \
  SPI_DEV(SPI_EEPROM,        10,  SPIMgr_UpdateEEProm),       \
  SPI_DEV(SPI_RTC_NVRAM,     10,  SPIMgr_UpdateRTCNVRam)

#define CQ_MAX_ENTRIES         8    // Number of items in queue
#define CQ_ENTRY_BUFFER_SIZE 512    // size of data buffer in queue item.
/******************************************************************************
                                 Package Typedefs
******************************************************************************/

// enum of the SPI devices 
#undef  SPI_DEV
#define SPI_DEV(Id, Rate, HndlrFunc) Id
typedef enum {
  SPI_DEV_LIST,
  SPI_MAX_DEV
}SPIMGR_DEV_ID;

// Decl for SPI Mgr Handling Function
typedef void (*SPI_DEV_FUNC )(SPIMGR_DEV_ID dev);

// Decl for ADC supplier function
typedef RESULT (*ADC_FUNC )(FLOAT32* adcValue);

//SPI info structure
typedef struct {
  UINT32       rate;
  SPI_DEV_FUNC HandlerFunc;
}SPI_MGR_INFO;

//SPI runtime info structure

typedef struct {
  UINT32       rate;        // Rate at which the managed device is updated
  UINT32       counterMs;   // Count in mSecs since the device was last updated.
  SPI_DEV_FUNC HandlerFunc; // Pointer to SPIManager handler-function for this device.
  ADC_FUNC     AdcFunc;     // Pointer to ADC_GetXXXX function for this device.
  FLOAT32      adcValue;    // Current value of the analog device when applicable
  RESULT       adcResult;   // Last result of call to analog device when applicable
}SPI_RUNTIME_INFO;

// type of operation to perform
typedef enum
{
  OP_TYPE_NONE,
  OP_TYPE_READ,
  OP_TYPE_WRITE
}OP_TYPE;

// I/O operation state
typedef enum
{
  OP_STATUS_PENDING,
  OP_STATUS_IN_PROGRESS  
}OP_STATE;

// I/O operation results
typedef enum
{
  IO_RESULT_READY,
  IO_RESULT_PENDING,
  IO_RESULT_TRANSMITTED
}IO_RESULT;

// Data-passing method.
typedef enum
{
  DATA_PASSING_VALUE, // pass the data stored in SPIMGR_ENTRY.data buffer
  DATA_PASSING_PTR    // pass the data stored at the address SPIMGR_ENTRY.pData
}DATA_PASSING;

typedef struct  
{
  OP_TYPE      opType;       // Op type to be if read and writes are handled from single queue
  OP_STATE     opState;  
  IO_RESULT*   pResult;      // Pointer to a user-supplied flag to signal the op is compl
  UINT32       addr;         // address for writing  
  DATA_PASSING dataMethod;   // Defines the data-transfer method for this entity, data or pData
  UINT8        data[CQ_ENTRY_BUFFER_SIZE];
  UINT8*       pData;        // Pointer to data be sent/recvd.  
  size_t       size;         // size to read/write
}SPIMGR_ENTRY;

// SPI CircularQueue structure for Reading/Writing from EEPROM, RTC, RTC-NVRAM
#define MAX_CIRCULAR_QUEUE_NAME  33
typedef struct
{
  SPIMGR_ENTRY buffer[CQ_MAX_ENTRIES];
  UINT8  queueSize;   // Property to limit  buffer size: 0 < queueSize <= CQ_MAX_ENTRIES;
  UINT32 writeCount;
  UINT32 readCount;
  INT16 readFrom;
  INT16 writeTo;  
  char name[MAX_CIRCULAR_QUEUE_NAME];
}CIRCULAR_QUEUE;

// SPI Manager error logging structure
#pragma pack(1)
typedef struct {
  RESULT result;
  UINT32 address;
}SPIMGR_ERROR_LOG;
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( SPI_MANAGER_BODY )
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
// Task Init and execution functions.
EXPORT void SPIMgr_Initialize (void);
EXPORT void SPIMgr_Task       (void *pParam );

// Service functions for ADC to retrieve analog values managed by this task.
EXPORT RESULT SPIMgr_GetACBusVoltage  (FLOAT32* ACBusVoltage);
EXPORT RESULT SPIMgr_GetACBattVoltage (FLOAT32* ACBattVoltage);
EXPORT RESULT SPIMgr_GetLiBattVoltage (FLOAT32* Voltage);
EXPORT RESULT SPIMgr_GetBoardTemp     (FLOAT32* Temp);
EXPORT BOOLEAN SPIMgr_SensorTest      (UINT16 nIndex);

// Service functions for RTC Time and NVRAM operations
EXPORT RESULT SPIMgr_GetTime      (TIMESTRUCT *Ts);
EXPORT RESULT SPIMgr_SetRTC       (TIMESTRUCT *Ts, IO_RESULT* ioResult);
EXPORT RESULT SPIMgr_WriteRTCNvRam(UINT32 DestAddr,void* Buf,size_t Size, IO_RESULT* ioResult);

// Service functions for EEPROM operations
EXPORT RESULT SPIMgr_ReadBlock  (UINT32 PhysicalOffset, void* Buf, UINT32 Size);
EXPORT RESULT SPIMgr_WriteEEPROM(UINT32 DestAddr,void* Buf, size_t Size, IO_RESULT* ioResult);

// Accessor function to setting direct-write / queued-write mode
EXPORT void SPIMgr_SetModeDirectToDevice(void);



#endif // SPI_MANAGER_H        
/*****************************************************************************
 *  MODIFICATIONS
 *    $History: SPIManager.h $
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 7/18/12    Time: 6:27p
 * Updated in $/software/control processor/code/system
 * FAST 2 Allow different queue sizes
 * 
 * *****************  Version 9  *****************
 * User: Contractor2  Date: 5/26/11    Time: 1:17p
 * Updated in $/software/control processor/code/system
 * SCR #767 Enhancement - BIT for Analog Sensors
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 9/01/10    Time: 4:51p
 * Updated in $/software/control processor/code/system
 * SCR #845 Code Review Updates
 * 
 * *****************  Version 7  *****************
 * User: Contractor V&v Date: 8/17/10    Time: 6:43p
 * Updated in $/software/control processor/code/system
 * SCR #746 CfgMgr - Primary and Backup copies corrupted
 * 
 * *****************  Version 6  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:55p
 * Updated in $/software/control processor/code/system
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 5  *****************
 * User: Contractor V&v Date: 5/07/10    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #548 Strings in Logs
 * 
 * *****************  Version 4  *****************
 * User: Jeff Vahue   Date: 3/15/10    Time: 4:15p
 * Updated in $/software/control processor/code/system
 * LI Battery should only be read at 4Hz not 100Hz
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
 *****************************************************************************/
