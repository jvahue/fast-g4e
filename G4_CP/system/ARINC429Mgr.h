#ifndef ARINC429MGR_H
#define ARINC429MGR_H
/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        Arinc429Mgr.h      
    
    Description: Contains data structures related to the Arinc429 
    
VERSION
     $Revision: 12 $  $Date: 8/28/12 1:43p $
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "arinc429.h"
#include "datareduction.h"
#include "faultmgr.h"
#include "fifo.h"
/******************************************************************************
                                 Package Defines
******************************************************************************/
#define ARINC429_MAX_WORDS          0x400   // TBD find actual number.  Should be less
                                            //   than the number of sensors.  
#define ARINC429_MAX_PARAMS         256
#define ARINC429_SSM_FAIL_MSG_SIZE  32      // CBIT SSM failure message size 

#define MAX_CHANNEL_NAME            32

// Arinc429 Raw Rx Software Buffer 
#define ARINC429_RAW_RX_BUFFER_SIZE 16384
#define ARINC429_MAX_RECORDS        3200

#define MFD_CYCLE_TIMEOUT           212

#define ARINC429_DSB_TIMEOUT 0 
#define ARINC429_MAX_READS          32      // Max number of words to read per frame
/******************** ARINC429 Manager GPA and GPB Decodes************************************/
#define GPA_RX_CHAN(gpa)          ((gpa >>  0) & 0x03)
#define GPA_WORD_FORMAT(gpa)      ((gpa >>  2) & 0x03)                
#define GPA_WORD_SIZE(gpa)        ((gpa >>  5) & 0x1F)
#define GPA_SDI_DEFINITION(gpa)   ((gpa >> 10) & 0x03)
#define GPA_IGNORE_SDI(gpa)       ((gpa >> 12) & 0x01)
#define GPA_WORD_POSITION(gpa)    ((gpa >> 13) & 0x1F)
#define GPA_PACKED_DISCRETE(gpa)  ((gpa >> 18) & 0x03)
#define GPA_SSM_FILTER(gpa)       ((gpa >> 20) & 0x0F)
#define GPA_ALL_CALL(gpa)         ((gpa >> 24) & 0x01)

#define ARINC_MSG_GET_SSM(Msg) ((Msg & ARINC_MSG_SSM_BITS) >> ARINC_MSG_SHIFT_SSM)


/********** CONFIG DEFAULTS *************/
#define RX_PARAM_DEFAULT   0   , /* Label       */\
                           0   , /* GPA         */\
                           0   , /* GPB         */\
                           0   , /* Scale       */\
                           0.0f  /* Tolerance   */

#define ARINC429_RX_PARAM_ARRAY_DEFAULT \
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT, RX_PARAM_DEFAULT,\
   RX_PARAM_DEFAULT
 
#define ARINC429_RX_DEFAULT(RxStr)\
                                     RxStr, /* Name                 */\
                       ARINC429_SPEED_HIGH, /* Speed - High         */\
               ARINC429_RX_PROTOCOL_STREAM, /* Protocol - Stream    */\
                      ARINC429_RX_SUB_NONE, /* Sub-Protocol         */\
           ARINC429_RX_PARAM_ARRAY_DEFAULT, /* Parameter Config     */\
                        SECONDS_120,        /* Channel Startup      */\
                        SECONDS_120,        /* Channel Timeout      */\
                        STA_CAUTION,        /* Channel SysCondition */\
                        STA_FAULT,          /* PBIT SysCondition    */\
                       ARINC429_PARITY_ODD, /* Parity               */\
                    ARINC429_RX_SWAP_LABEL, /* Label Swap Bits      */\
                           { 0xFFFFFFFFUL,  /* Filter Array [8]     */\
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL,                            \
                             0xFFFFFFFFUL },                          \
                                      FALSE  /* Enable Ch            */

#define ARINC429_RX_ARRAY_DEFAULT    ARINC429_RX_DEFAULT("ARINC RX 1"),\
                                     ARINC429_RX_DEFAULT("ARINC RX 2"),\
                                     ARINC429_RX_DEFAULT("ARINC RX 3"),\
                                     ARINC429_RX_DEFAULT("ARINC RX 4")
                                    
                                            
#define ARINC429_TX_DEFAULT(TxStr)   TxStr,                     /* Name */\
                                     ARINC429_SPEED_HIGH,       /* Speed */\
                                     ARINC429_PARITY_ODD,       /* Parity   */\
                                     FALSE,                     /* Enable   */\
                                 ARINC429_TX_DONT_SWAP_LABEL    /* No label bit flip */
                                     

#define ARINC429_TX_ARRAY_DEFAULT    ARINC429_TX_DEFAULT("ARINC TX 1"),\
                                     ARINC429_TX_DEFAULT("ARINC TX 2")
                              
#define ARINC429_CFG_DEFAULT         ARINC429_RX_ARRAY_DEFAULT,\
                                     ARINC429_TX_ARRAY_DEFAULT
                                     
/********** CONFIG DEFAULTS END *************/

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
   SDI_00,
   SDI_01,
   SDI_10,
   SDI_11,
   SDI_IGNORE,
   SDI_MAX
} ARINC_SDI;

typedef enum
{
   SSM_00,
   SSM_01,
   SSM_10,
   SSM_11
} ARINC_SSM;

typedef enum
{
  BNR,
  BCD,
  DISCRETE,
  OTHER,
  END_OF_ARINC_FORMATS
} ARINC_FORM;

typedef enum
{
  DISC_STANDARD,
  DISC_BNR,
  DISC_BCD,
  END_OF_DISC_TYPES
} DISC_TYPE;

// Note: This enumeration is used for the FailureCondition Array
//       and ValidSSM and Arinc429_ErrMsg[] field of the ArincFilter. 
//       The order is instrumental in the proper execution of failure logic.
typedef enum {
  SSM_FAIL_COUNT, 
  SSM_NO_COMP_COUNT,
  SSM_TEST_DATA_COUNT,
  SSM_NORMAL,
//PARITY_COUNT,
  END_OF_FAIL_COUNT
} ARINC_FAIL_COUNT;

typedef enum 
{
  ARINC429_RX_PROTOCOL_STREAM, 
  ARINC429_RX_PROTOCOL_CHANGE_ONLY,
  ARINC429_RX_PROTOCOL_REDUCE,
  ARINC429_RX_PROTOCOL_MAX
} ARINC429_RX_PROTOCOL; 

typedef enum 
{
  ARINC429_RX_SUB_NONE               = 0,
  ARINC429_RX_SUB_PW305AB_MFD_REDUCE = 1, 
  ARINC429_RX_SUB_PROTOCOL_MAX
} ARINC429_RX_SUB_PROTOCOL; 

typedef enum 
{
  ARINC429_STATUS_OK = 0,           // Arinc429 Enabled and OK
  ARINC429_STATUS_FAULTED_PBIT,     // Arinc429 Enabled but currently Faulted PBIT
                                    //    Arinc Processing disabled until power cycle or 
                                    //    "arinc429.reconfigure". 
  ARINC429_STATUS_FAULTED_CBIT,     // Arinc429 Enabled but currently Faulted CBIT
                                    //    Arinc Processing active. 
  ARINC429_STATUS_DISABLED_OK,      // Arinc429 Not Enabled but PBIT OK 
  ARINC429_STATUS_DISABLED_FAULTED, // Arinc429 Not Enabled but PBIT failed 
  ARINC429_STATUS_MAX
} ARINC429_SYSTEM_STATUS; 

typedef enum
{
   MFD_START   = 0,
   MFD_PROCESS = 1,
} MFD_STATE;

typedef enum
{
   MFD_300 = 192,
   MFD_301 = 193,
   MFD_302 = 194,
   MFD_303 = 195,
   MFD_304 = 196,
   MFD_305 = 197,
   MFD_306 = 198
} MFD_LABEL;

typedef enum
{
   MFD_AMBER = 0x2,
   MFD_GREEN = 0x4,
   MFD_CYAN  = 0x5
} MFD_COLOR;

typedef enum
{
   DEBUG_OUT_RAW      = 0,
   DEBUG_OUT_PROTOCOL = 1
} ARINC429_DEBUG_OUT_TYPE ;
/*********************************************************************************************/
/* Arinc429 Configuration                                                                    */
/*********************************************************************************************/
#pragma pack(1)
typedef struct 
{
   UINT8   Label;
   UINT32  GPA;
   UINT32  GPB;
   UINT32  Scale;
   FLOAT32 Tolerance;
} ARINC429_RX_PROTOCOL_CFG, *ARINC429_RX_PROTOCOL_CFG_PTR;

typedef struct 
{
    CHAR                     Name[MAX_CHANNEL_NAME];    // Configured Name
    ARINC429_SPEED           Speed;       // 1 = High, 0 = Low.  Should Track FPGA value. 
    ARINC429_RX_PROTOCOL     Protocol;    // 0 = ARINC429_RX_PROTOCOL_STREAM
                                          // 1 = ARINC429_RX_PROTOCOL_CHANGE_ONLY
    ARINC429_RX_SUB_PROTOCOL SubProtocol; 
    ARINC429_RX_PROTOCOL_CFG Parameter[ARINC429_MAX_PARAMS];
    
    UINT32                   ChannelStartup_s; // Startup period in seconds  
    UINT32                   ChannelTimeOut_s; // Timeout period in seconds

    FLT_STATUS               ChannelSysCond; 
    FLT_STATUS               PBITSysCond;  
    
    ARINC429_PARITY          Parity;         // 1 = Odd, 0 = Even.  Should Track FPGA value. 
    ARINC429_RX_SWAP         LabelSwapBits;  // 0 = Swap Arinc Label, 1 = Don't Swap Arinc Label 
    UINT32                   FilterArray[8]; // Each bit represent labels 0..256 
                                             //   1 = Allow label (allow label)
                                             //   0 = Disallow label (filter out label)
                           
    BOOLEAN                  Enable;  // Enable the Arinc429 Rx Ch.  Default is currently TRUE ! 
                                      //    Update default to FALSE.                         
} ARINC429_RX_CFG, *ARINC429_RX_CFG_PTR; 

typedef struct 
{
    CHAR             Name[MAX_CHANNEL_NAME];  
    ARINC429_SPEED   Speed;          // 1 = High, 0 = Low.  Should Track FPGA value. 
    ARINC429_PARITY  Parity;         // 1 = Odd, 0 = Even.  Should Track FPGA value. 
    BOOLEAN          Enable;         // 1 = Enable the Arinc429 Tx Ch.  
    ARINC429_TX_SWAP LabelSwapBits;  // 1 = Label filter bits flipped 
} ARINC429_TX_CFG, *ARINC429_TX_CFG_PTR; 

typedef struct 
{
    ARINC429_RX_CFG RxChan[FPGA_MAX_RX_CHAN];
    ARINC429_TX_CFG TxChan[FPGA_MAX_TX_CHAN];
} ARINC429_CFG, *ARINC429_CFG_PTR; 
#pragma pack()
/*********************************************************************************************/
/* Data Reduction                                                                            */
/*********************************************************************************************/
#define MFD_MAX_LINE      12
#define MFD_LABEL_TIMEOUT 5240  // A complete MFD Cycle is going to take 2.620 Seconds
                                // Chose a timeout that is 2 times that

typedef struct
{
   BOOLEAN bRcvd;
   UINT32  RcvTime;
   UINT32  Data;
} MFD_MSG;

typedef struct
{
   BOOLEAN   bRcvdOnce;
   MFD_COLOR Color;
   MFD_MSG   Msg[MFD_306 - MFD_300 + 1];
} MFD_LINE;

typedef struct
{
   MFD_STATE State;
   UINT32    StartTime;
   MFD_LABEL PrevLabel;
   UINT8     CurrentLine;
   MFD_LINE  Line[MFD_MAX_LINE];
} MFD_DATA_TABLE, *MFD_DATA_TABLE_PTR;

/*********************************************************************************************/
/* Channel Data                                                                              */
/*********************************************************************************************/
typedef struct
{
   UINT8               Label;            
   // GPA Data
   ARINC429_CHAN_ENUM  RxChan;       // Rx Chan 0,1,2 or 3 from FPGA  
   ARINC_FORM          Format;       // Format (BNR, BCD, Discrete)
   UINT8               WordSize;     // Arinc Word Size 1 - 32 bits
   UINT8               SDBits;       // SDBit definition   
   BOOLEAN             IgnoreSDI;    // Ignore SDI bits   
   UINT8               WordPos;      // Arinc Word Start Pos within 32 bit word (Starting from 0)
   DISC_TYPE           DiscType;     // Discrete Type (Standard, BNR, BCD)
   UINT8               ValidSSM;     // Valid SSM for Word
                                     //   NOTE: Valid SSM is directly related to the 
                                     //         FailureCondition[] array index.
                                     //   (0 = Invalid 1 = Valid)
                                     //   bit 0 = SSM '00'
                                     //   bit 1 = SSM '01'
                                     //   bit 2 = SSM '10'
                                     //   bit 3 = SSM '11'
   BOOLEAN             SDIAllCall;   // SDI All Call Mode  
   // GPB Data
   UINT32              DataLossTime; // Data Loss Timeout in milliseconds   
} ARINC429_WORD_INFO, *ARINC429_WORD_INFO_PTR;

typedef struct
{
   FLOAT32             Scale;
   UINT8               WordInfoIndex;
   //ARINC429_WORD_INFO  WordInfo;
} ARINC429_CONV_DATA, *ARINC429_CONV_DATA_PTR;

typedef struct
{
   BOOLEAN             bReduce;
   BOOLEAN             bValid;
   BOOLEAN             bInitialized;
   BOOLEAN             bIgnoreSDI;
   UINT32              Reduced429Msg;
   UINT32              LastGoodMsg;
   REDUCTIONDATASTRUCT ReduceData;
   ARINC429_CONV_DATA  ConversionData;
   UINT8               TotalTests;
   UINT32              TimeSinceLastGoodValue;
   UINT32              FailCount[4];
   BOOLEAN             bFailedOnce;
} ARINC429_REDUCE_PROTOCOL, *ARINC429_REDUCE_PROTOCOL_PTR;

typedef struct
{
   UINT32                   CurrentMsg;
   UINT32                   PreviousMsg;
   UINT32                   RxTime;
   TIMESTAMP                RxTimeStamp;
   UINT32                   RxCnt;
   BOOLEAN                  Accept;
   ARINC429_REDUCE_PROTOCOL ProtocolStorage;
} ARINC429_SDI_DATA, *ARINC429_SDI_DATA_PTR;

typedef struct
{
   UINT8             label;        // Arinc429 Octal Word Label
   ARINC429_SDI_DATA sd[SDI_MAX];
} ARINC429_LABEL_DATA, *ARINC429_LABEL_DATA_PTR;

typedef struct
{
   BOOLEAN              bRecordingActive;                // Flag to indicate recording active
   ARINC429_LABEL_DATA  LabelData[ARINC429_MAX_LABELS];  // Dynamic Storage for all labels rcv
   MFD_DATA_TABLE       MFDTable;
   UINT8                ReduceParamCount;
   ARINC429_WORD_INFO   ReduceWordInfo[ARINC429_MAX_PARAMS];
   UINT32               SyncStart;
   UINT8                TimeSyncDelta;                   // Delta time from sync command 
                                                         //    (10ms Resolution) 
} ARINC429_CHAN_DATA, *ARINC429_CHAN_DATA_PTR;

/*********************************************************************************************/
/* Data Manager Binary File Header                                                           */
/*********************************************************************************************/
#pragma pack(1)
typedef struct 
{
   UINT8   Label;
   UINT32  GPA;
   UINT32  GPB;
   UINT32  Scale;
   FLOAT32 Tolerance;
} ARINC429_RX_CFG_HDR;

typedef struct
{
   UINT16               TotalSize;
   UINT16               Channel;
   CHAR                 Name[MAX_CHANNEL_NAME];
   ARINC429_RX_PROTOCOL Protocol;
   UINT16               NumParams;
} ARINC429_CHAN_HDR;

typedef struct
{
   ARINC429_RX_CFG_HDR ParamCfg[ARINC429_MAX_PARAMS];
} ARINC429_RX_REDUCE_HDR;

typedef struct
{
   CHAR Name[MAX_CHANNEL_NAME]; 
} ARINC429_SYS_HDR;

#pragma pack()

/*********************************************************************************************/
/* Sensor Data                                                                               */
/*********************************************************************************************/
typedef struct
{
  BOOLEAN Accept; 
} ARINC429_SENSOR_SDI_DATA, *ARINC429_SENSOR_SDI_DATA_PTR;

typedef struct 
{
   ARINC429_SENSOR_SDI_DATA sd[ARINC429_MAX_SD]; 
} ARINC429_SENSOR_LABEL_FILTER, *ARINC429_SENSOR_LABEL_FILTER_PTR;

// Arinc 429 Sensor Definitions 
typedef struct 
{
   ARINC429_WORD_INFO WordInfo;
   UINT8              TotalTests;   // Total number of tests to run on Words received
                                    //   related to ValidSSM, ARINC_FAIL_COUNT, and 
                                    //   FailureCondition[]
   // Run Time Variables                             
   SINT32      value_prev;   // Stores the last "GOOD" word read from m_ArincLabelData[][]
                             //   If current word indicates bad "SSM", then the "value_prev"
                             //   will be returned to the calling app. 
                             // Value "decoded and parsed" to SINT32 

   UINT32      FailCount[END_OF_FAIL_COUNT]; // Array of ARINC Word Fail Counters
   UINT32      FailCountBCD;                 // Special cnt for BCD bad digit processing
                                             //    (For BCD fmt only). 
   UINT32      TimeSinceLastGoodValue;       // Time since last valid data read
                                             //     Used for Arinc429 BIT Processing 
   UINT32      TimeSinceLastUpdate;          // Time since last update 
  
   BOOLEAN     bFailed;      // Flag to indicate received data has been faulted 
                             //   due to bad SSM or data loss.  Processing 
                             //   logic in _SensorTest(). 
                            
   UINT32      WordLossCnt;  // Number of times word has been determined to 
                             //   have failed. Processing logic in _SensorTest()

   UINT16      nSensor;      // Sensor associated with this Word Parse Info.  
                             //   set in Arinc429_SensorSetup()

} ARINC429_SENSOR_INFO, *ARINC429_SENSOR_INFO_PTR; 

/*********************************************************************************************/
/* RX/TX Buffers                                                                             */
/*********************************************************************************************/
#pragma pack(1)
typedef struct
{
   TIMESTAMP TimeStamp;
   UINT32    Arinc429Msg;
} ARINC429_SNAPSHOT_RECORD, *ARINC429_SNAPSHOT_RECORD_PTR;

typedef struct
{
   UINT8  DeltaTime;
   UINT32 ARINC429Msg;
} ARINC429_RECORD, *ARINC429_RECORD_PTR;
#pragma pack()

typedef struct 
{
   FIFO    RecordFIFO;
   UINT32  RecordCnt; 
   BOOLEAN bOverFlow;
   UINT16  RecordSize;
} ARINC429_RAW_RX_BUFFER, *ARINC429_RAW_RX_BUFFER_PTR; 

/*********************************************************************************************/
/* Arinc429 Task Manager Block - Start                                                       */
/*********************************************************************************************/
// Arinc429 System Status 
typedef struct 
{
    BOOLEAN  Enabled;             // Indicates if the channel is enabled for processing
    
    UINT32   SwBuffOverFlowCnt;   // Count of times ARINC429_RAW_BUFFER->bOverFlow indicates OF
    UINT32   ParityErrCnt;        // Count of times HW RSR indicates parity error, check 10 msec
    UINT32   FramingErrCnt;       // Count of times HW RSR indicates framing error, check 10 msec
    UINT16   FPGA_Status;
    
    BOOLEAN  bChanActive;         // Updated on first channel activity 
    UINT32   LastActivityTime;    // System clock time of last receive activity     
    BOOLEAN  bChanTimeOut;        // Chan Time Out     
    BOOLEAN  bChanStartUpTimeOut; // Chan Startup Up Time Out    

    UINT32   DataLossCnt;         // Count of times Data Loss TimeOut Occurs 

    UINT32   RxCnt;               // Total Rx Data Byte Count since power up.
    
    ARINC429_SYSTEM_STATUS  Status;
} ARINC429_SYS_RX_STATUS, *ARINC429_SYS_RX_STATUS_PTR;;

typedef struct 
{
    ARINC429_SYSTEM_STATUS  Status;  // Overall Arinc429 Tx[] Driver Status   
} ARINC429_SYS_TX_STATUS, *ARINC429_SYS_TX_STATUS_PTR; 

typedef struct 
{
    ARINC429_SYS_RX_STATUS Rx[FPGA_MAX_RX_CHAN]; 
    ARINC429_SYS_TX_STATUS Tx[FPGA_MAX_TX_CHAN]; 
    
    UINT32 InterruptCnt; 
} ARINC429_MGR_TASK_PARAMS, *ARINC429_MGR_TASK_PARAMS_PTR; 

/*********************************************************************************************/
/* Arinc429 Debug Output                                                                     */
/*********************************************************************************************/
typedef struct
{
  BOOLEAN  bOutputRawFilteredBuff;
  BOOLEAN  bOutputRawFilteredFormatted; 
  BOOLEAN  bOutputMultipleArincChan; 
  ARINC429_DEBUG_OUT_TYPE OutputType;
} ARINC429_DEBUG; 

/*********************************************************************************************/
/* Arinc429 Logs                                                                             */
/*********************************************************************************************/
#pragma pack(1)

typedef struct 
{
  RESULT result; // SYS_A429_DATA_LOSS_TIMEOUT
                 // SYS_A429_STARTUP_TIMEOUT
  UINT32 cfg_timeout; 
  UINT8  ch; 
} ARINC429_SYS_TIMEOUT_LOG; 

typedef struct 
{
  RESULT result; // SYS_A429_SSM_FAIL
  char FailMsg[ARINC429_SSM_FAIL_MSG_SIZE];
} ARINC429_SYS_CBIT_SSM_FAIL_LOG; 

typedef struct 
{
  UINT32  a429word;           // full arinc data word
  UINT32  bcdData;            // shifted BCD data
  UINT16  digit;              // out of range BCD digit
} ARINC429_SYS_BCD_FAIL_LOG; 

typedef struct 
{
  UINT32 SwBuffOverFlowCnt;    // Count of times ARINC429_RAW_BUFFER->bOverFlow indicates OF
  UINT32 ParityErrCnt;         // Count of times HW RSR indicates parity error, check 10 msec
  UINT32 FramingErrCnt;        // Count of times HW RSR indicates framing error, check 10 msec
  
  UINT32 FIFO_HalfFullCnt;     // Count of times HW FIFO indicates half full int 
  UINT32 FIFO_FullCnt;         // Count of times HW FIFO indicates full int 
  
  UINT32 DataLossCnt;          // Count of times Data Loss TimeOut Occurs 
  
  // TODO: There should be a deliniation between System and Driver Status
  ARINC429_SYSTEM_STATUS Status; // Current System Status 
  
} ARINC429_CBIT_HEALTH_COUNTS_CH, *ARINC429_CBIT_HEALTH_COUNTS_CH_PTR; 

typedef struct 
{
  ARINC429_CBIT_HEALTH_COUNTS_CH ch[FPGA_MAX_RX_CHAN];
} ARINC429_CBIT_HEALTH_COUNTS; 
#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( ARINC429MGR_BODY )
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
EXPORT void    Arinc429MgrInitialize              ( void );
EXPORT void    Arinc429MgrInitTasks               ( void ); 
EXPORT void    Arinc429MgrBITTask                 ( void *pParam ); 
EXPORT void    Arinc429MgrProcessMsgsTask         ( void *pParam );
EXPORT void    Arinc429MgrDisplaySWBufferTask     ( void *pParam ); 

EXPORT UINT16  Arinc429MgrReadFilteredRaw         ( void *pDest, UINT32 chan, UINT16 nMaxByteSize ); 
EXPORT UINT16  Arinc429MgrReadFilteredRawSnapshot ( void *pDest, UINT32 chan, UINT16 nMaxByteSize,
                                                    BOOLEAN bStartSnap ); 
EXPORT void    Arinc429MgrSyncTime                ( UINT32 chan   );
EXPORT UINT16  Arinc429MgrGetFileHdr              ( void *pDest, UINT32 chan, UINT16 nMaxByteSize );
EXPORT UINT16  Arinc429MgrGetSystemHdr            ( void *pDest, UINT16 nMaxByteSize );
EXPORT BOOLEAN Arinc429MgrSensorTest              ( UINT16 nIndex );
EXPORT BOOLEAN Arinc429MgrInterfaceValid          ( UINT16 nIndex );  
EXPORT FLOAT32 Arinc429MgrReadWord                ( UINT16 nIndex );

EXPORT UINT16  Arinc429MgrSensorSetup             ( UINT32 gpA,   UINT32 gpB, 
                                                    UINT8  label, UINT16 nSensor );

EXPORT void    Arinc429MgrDisableLiveStream       (void);

EXPORT ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrGetCBITHealthStatus( void ); 

EXPORT ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrCalcDiffCBITHealthStatus
                                   ( ARINC429_CBIT_HEALTH_COUNTS PrevCount );
                                   
EXPORT ARINC429_CBIT_HEALTH_COUNTS Arinc429MgrAddPrevCBITHealthStatus ( 
                                      ARINC429_CBIT_HEALTH_COUNTS CurrCnt, 
                                      ARINC429_CBIT_HEALTH_COUNTS PrevCnt ); 

#ifdef GENERATE_SYS_LOGS 
  EXPORT void Arinc429MgrCreateAllInternalLogs ( void ); 
#endif

#endif // ARINC429MGR_H               


/*************************************************************************
 *  MODIFICATIONS
 *    $History: ARINC429Mgr.h $
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 11  *****************
 * User: John Omalley Date: 12-07-13   Time: 9:03a
 * Updated in $/software/control processor/code/system
 * SCR 1124 - Packed the configuration structures for ARINC429 Mgr,
 * Sensors and Triggers. Suppressed the alignment warnings for those three
 * objects also.
 * 
 * *****************  Version 10  *****************
 * User: John Omalley Date: 6/08/11    Time: 12:28p
 * Updated in $/software/control processor/code/system
 * SCR 1025 - Discrete Most Significant bit drop fix
 * SCR 1040 - Sign Magnitude BNR to 2's complement fix
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 9/07/10    Time: 12:03p
 * Updated in $/software/control processor/code/system
 * SCR #854 Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: John Omalley Date: 7/29/10    Time: 7:51p
 * Updated in $/software/control processor/code/system
 * SCR 754 - Added a FIFO flush after initialization
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/system
 * SCR #743 Update TimeOut Processing for val == 0
 * 
 * *****************  Version 6  *****************
 * User: John Omalley Date: 7/20/10    Time: 7:34p
 * Updated in $/software/control processor/code/system
 * SCR 720 - Fixed Debug Message display problem
 * 
 * *****************  Version 5  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/system
 * SCR 294 - Add system binary header
 * 
 * *****************  Version 4  *****************
 * User: John Omalley Date: 7/08/10    Time: 2:51p
 * Updated in $/software/control processor/code/system
 * SCR 650
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 2  *****************
 * User: John Omalley Date: 6/11/10    Time: 5:49p
 * Updated in $/software/control processor/code/system
 * SCR 627 - Fixed default config for scale
 * 
 * *****************  Version 1  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:19p
 * Created in $/software/control processor/code/system
 * SCR 627 - Updated for LJ60
 * 
 *
 ***************************************************************************/
              
