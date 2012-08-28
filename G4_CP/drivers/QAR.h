#ifndef QAR_H
#define QAR_H

/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

    File:        QAR.h      
    
    Description: This file defines the interface for the Quick Access Recorder
                 software.  See the QAR.c module for a detailed description.
    
    VERSION
      $Revision: 49 $  $Date: 8/28/12 1:06p $    
    
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "fpga.h"
#include "resultcodes.h"
#include "faultmgr.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
#define QAR_CFG_DEFAULT    QAR_BIPOLAR_RETURN_ZERO,      /* Format */\
                           QAR_64_WORDS,                 /* Number Words */\
                           {0x247, 0x5b8, 0xa47, 0xdb8}, /* Barker Codes */\
                           SECONDS_120,                  /* Startup Timeout */\
                           SECONDS_120,                  /* Channel Timeout */\
                           STA_CAUTION,                  /* Channel Sys Cond */\
                           STA_CAUTION,                  /* PBIT Sys Cond */\
                           FALSE,                        /* Enable */\
                           "QAR",                        /* Name */
  
#define QAR_SF_MAX_SIZE      0x0400
#define QAR_MAX_NAME         32

#ifndef WIN32
#define QAR_W(addr, field, data) QARwrite( addr, field, data)
#define QAR_R(addr, field)       QARread( addr, field)
#else
#include "HwSupport.h"
#define QAR_W(addr,field, data) hw_QARwrite( addr, field, data)
#define QAR_R(addr, field)      hw_QARread( addr, field)
#endif

#define QAR_WORD_TIMEOUT_FAIL_MSG_SIZE   32   // CBIT Word timeout failure 
                                              //     message size 

#define QAR_DSB_TIMEOUT    0 
// The LOFS Timeout is needed because the FPGA declares a sub-frame received 
// if a good barker code is detected and the total expected bit times has been reached.
// This FPGA logic is flawed because the sub-frame could have gone bad one bit after the 
// barker code was recieved and continued until the next frame. The software should not
// accept the sub-frame until the next barker code is received. If the next barker code is
// not received the FPGA will flag a Loss of Frame Sync (LOFS). The software should wait
// a minimum time of one barker code from the frame detected interrupt before saving the 
// new sub-frame. The worst case time for this delay would be based on a 64 word sub-frame
// which is received at 1 Hz. 1/64 = 0.015625 seconds/12bit word. 
#define LOFS_TIMEOUT_MS    16

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
// These must match the order in QAR.c FieldInfo
typedef enum {
    C_RESYNC,
    C_NUM_WORDS,
    C_FORMAT,
    C_ENABLE,
    C_TSTN,
    C_TSTP,
    S_FRAME_VALID,
    S_LOST_SYNC,
    S_SUBFRAME,
    S_DATA_PRESENT,
    S_BARKER_ERR,
    S_BIIN,
    S_BIIP,
    FN_MAX
} FIELD_NAMES;

typedef enum {
 QAR_TEST_TSTP_HIGH = 0,   
 QAR_TEST_TSTN_HIGH,   
 QAR_TEST_TSTPN_HIGH,
 QAR_TEST_TSTPN_MAX    
} QAR_LOOPBACK_TEST; 

typedef enum
{
   QAR_BIPOLAR_RETURN_ZERO = QCR_FORMAT_BIP_VAL,
   QAR_HARVARD_BIPHASE     = QCR_FORMAT_HBP_VAL
}QAR_FORMAT_CONTROL;

typedef enum
{
   QAR_64_WORDS   = QCR_BYTES_SF_64WORDS_VAL,
   QAR_128_WORDS  = QCR_BYTES_SF_128WORDS_VAL,
   QAR_256_WORDS  = QCR_BYTES_SF_256WORDS_VAL,
   QAR_512_WORDS  = QCR_BYTES_SF_512WORDS_VAL,
   QAR_1024_WORDS = QCR_BYTES_SF_1024WORDS_VAL
} QAR_NUM_WORDS_CONTROL;

typedef enum
{
  QAR_NORMAL_OPERATION = 0,
  QAR_FORCE_REFRAME    = 1
} QAR_RESYNC_CONTROL;

typedef enum
{
   QAR_NO_FRAME_LOSS   = 0,
   QAR_FRAME_SYNC_LOSS = 1
} QAR_LOSS_STATUS;

typedef enum
{
   QAR_FRAME_INVALID = 0,
   QAR_FRAME_SYNC    = 1
} QAR_SYNC_STATUS;

typedef enum
{
   QAR_SF1    = 0,
   QAR_SF2    = 1,
   QAR_SF3    = 2,
   QAR_SF4    = 3,
   QAR_SF_MAX = 4
} QAR_SF;

typedef enum
{
   QAR_NO_STATE = 0,
   QAR_SYNCD    = 1,
   QAR_LOSF     = 2
}QAR_SF_STATE;

typedef enum
{
  QAR_GSE_ASCII    = 2, 
  QAR_GSE_NONE     = 3, 
} QAR_OUTPUT_TYPE;

typedef struct 
{
    UINT8 lsb;
    UINT8 size;
} FIELD_INFO;

/*
typedef struct
{
   UINT16 Unused          :  8;
   UINT16 Tstp            :  1;
   UINT16 Tstn            :  1;
   UINT16 Enable          :  1;
   UINT16 Format          :  1;
   UINT16 NumWords        :  3;
   UINT16 Resync          :  1;
}QAR_CTRL_REGISTER;

typedef struct
{
   UINT16 Unused          :  8;
   UINT16 Biip            :  1;
   UINT16 Biin            :  1; 
   UINT16 BarkerErr       :  1;
   UINT16 DataPresent     :  1;
   UINT16 SubFrame        :  2;
   UINT16 LossOfFrameSync :  1;
   UINT16 FrameValid      :  1;
}QAR_STATUS_REGISTER;
*/

#define QAR_CTRL_REGISTER UINT16 
#define QAR_STATUS_REGISTER UINT16 

/*typedef struct
{
    UINT16 Unused : 4;
    UINT16 Code   : 12;
} QAR_BARKER_CODE;*/

typedef struct
{
   UINT16 Barker;
   UINT16 Word[QAR_SF_MAX_SIZE - 1];
}QAR_SF_BUFFER;

/* 03/10/2008 PLee Updated to latest FPGA Rev 4
typedef struct
{
   QAR_CTRL_REGISTER   Control;
   QAR_STATUS_REGISTER Status;
   UINT16              Unused[3];    // Removed from Rev 4 
   QAR_BARKER_CODE     Barker[QAR_SF_MAX];
} QAR_REGISTERS;
*/

typedef struct
{
   QAR_CTRL_REGISTER   Control;
   QAR_STATUS_REGISTER Status;
   UINT16              Barker[QAR_SF_MAX];
} QAR_REGISTERS;

typedef struct
{
   QAR_SF_BUFFER       Buffer1;
   QAR_SF_BUFFER       Buffer2;
} QAR_DATA;

typedef enum
{
  QAR_RAW
} QAR_PROTOCOL;

typedef struct
{
  QAR_FORMAT_CONTROL    Format;
  QAR_NUM_WORDS_CONTROL NumWords;
  UINT16                BarkerCode[QAR_SF_MAX];  
  
  // New P Lee 04/01/2008 
  UINT32                ChannelStartup_s; // Startup period in seconds
  UINT32                ChannelTimeOut_s; // Timeout period in seconds
  // UINT16                ChannelSysCond; 
  FLT_STATUS            ChannelSysCond; 
  FLT_STATUS            PBITSysCond; 
  
  
  // New P Lee 11/18/2008 
  BOOLEAN               Enable;      // QAR Enable or Disable ! 
  CHAR                  Name[QAR_MAX_NAME]; 
    
} QAR_CONFIGURATION, *QAR_CONFIGURATION_PTR;

// QAR System Status 
typedef enum 
{
  QAR_STATUS_OK = 0,           // QAR enabled and OK
//QAR_STATUS_FAULTED, 
//QAR_STATUS_DISABLED,
  QAR_STATUS_FAULTED_PBIT,     // QAR enabled but currently Faulted PBIT
                               //    QAR Processing disabled until power cycle. 
  QAR_STATUS_FAULTED_CBIT,     // QAR enabled but currently Faulted CBIT 
                               //    QAR Processing is active. 
  QAR_STATUS_DISABLED_OK,      // QAR Not enabled, but PBIT OK
  QAR_STATUS_DISABLED_FAULTED, // QAR Not enabled, but PBIT failed 
  QAR_STATUS_MAX
} QAR_SYSTEM_STATUS; 


typedef struct
{
   QAR_SF       CurrentFrame; 
   QAR_SF       PreviousFrame;
   QAR_SF       LastServiced; 
   UINT16       TotalWords;
   
   BOOLEAN      LossOfFrame;  // Transition of SYNC Ok to SYNC Loss.
                              //   Based on FOGA Frame Loss Status Bit. 
   UINT32       LossOfFrameCount; // Instances that Frame Loss after a good 
                                  //    SYNC has occurred.  Cnt of 
                                  //    transitions from SYNC OK to SYNC LOSS
   BOOLEAN      BarkerError;  // Echo of FPGA BARKER Error Status Bit.  
                              //   Cleared on Loss Of Frame 
   UINT32       BarkerErrorCount; // Count of Barker Error Status.  Equivalent
                                  //   to number of SF where Barker Error 
                                  //   is indicated. 
   BOOLEAN      FrameSync;  // Echo of FPGA Frame Sync Status Bit.  Read during  
                            //    Sub Frame read. Real time indication. 
   QAR_SF_STATE FrameState; 
   
   BOOLEAN      DataPresent; // Echo of FPGA Data Present Status Bit Flag. 
   // New P Lee 11/20/2008 
   UINT32       DataPresentLostCount;  // Count of transitions from Data Present to Data Lost
   // New P Lee 11/20/2008
   
   // New P Lee 03/28/2008 
   QAR_SF       PreviousSubFrameOk;  // Last read good subframe 
   BOOLEAN      bGetNewSubFrame;  // When FPGA INT to indicate new SF, QAR_ISR() will 
                                  // set this flag.  QAR_ReadSubFrame() checks flag 
                                  // and read FPGA.  Expect / assume FPGA INT to be  
                                  // approx 1 sec, thus R/W conflict with flag 
                                  // should be avoided.  
   UINT32       LastIntTime;   // Counter based on System Tick
   UINT32       BadIntFreqCnt; // Count of # of INT occurring faster than 2 Hz ! 
   // UINT32       LastSubFrameUpdateTime[QAR_SF_MAX];  // Last Update of SubFrame in 
                                                        // system tick time.   
   BOOLEAN      bSubFrameOk[QAR_SF_MAX]; // Subframe marked Ok or Bad. Init to OK on startup. 
                                                // SCR #29 
   BOOLEAN      bSubFrameReceived[QAR_SF_MAX];  // Subframe has been received.  Init to FALSE. 
   
   // New P Lee 04/01/2008 
   BOOLEAN      bQAREnable;   // QAR is user cfg to be enabled. Tracks QarCfg.enable
                              
   UINT32       LastActivityTime;  // System clock time of last receive activity based on 
                                   //   Data Present Bit of FPGA QAR Status ! 
                                   //   NOTE: == 0 on startup and no startup activity 

   BOOLEAN      bChannelStartupTimeOut;  // Flag to indicate ChannelStartupTimeout
   BOOLEAN      bChannelTimeOut;         // Flag to indicate ChannelTimeOut
                                         //  NOTE: Set by QAR processing, cleared by 
                                         //        read of current state ! 
                                      
   UINT32       InterruptCnt;      // Cnt num of Ints
   UINT32       InterruptCntLOFS;  // Cnt num of Ints caused by LOFS
   
   // New P Lee 10/02/2009 
   BOOLEAN      bFrameSyncOnce;    // Flag to indicate QAR sync has been received at least once
                                   // since startup. 
   
   QAR_SYSTEM_STATUS  SystemStatus; 
   BOOLEAN  bChanActive;         // TRUE on first channel activity FALSE on timeout
   
} QAR_STATE, *QAR_STATE_PTR;

#define QAR_ALLOWED_INT_FREQ 500   // Based on system tick of 1 msec * 500 -> 500 msec


typedef struct 
{
   BOOLEAN bSubFrameData[QAR_SF_MAX];  // SubFrame(S) which contains data
   UINT16 WordLocation;                // Last Word Location within sub frame. 
                                       //   Since data return from FPGA are 1 sec subframe
                                       //   only the last word is the most recent !
                                       //   All other data will be ignored, thus if we 
                                       //   get spikes, an event could be triggered on these 
                                       //   spikes.  Recommend 3-4 sec duration for any 
                                       //   processing. 
   UINT16 MSBPosition;   // MSB position of word 
                         // To handle "packed" data words and disc bit processing 
   UINT16 WordSize;      // Word size from MSB position. 
                         // To handle "packed" data words and enum types 
   UINT32 DataLossTime;  // In system tick time. Data Loss timeout exceedance value. 
   BOOLEAN SignBit;      // Is the MSB a sign bit (note for Disc this must be 
                         //    FALSE !
   BOOLEAN TwoComplement; // If Signed value, is it represented by TwosComplement 
                          //    or positive logic with sign value only ? 
                          // NOTE: we can no handle two embedded analog signed value !!
                          // NOTE: always assume signed value to be bit 11 !!
   BOOLEAN bFailed;       // Word Param data loss timeout exceeded ! 
   
   UINT16  nSensor;       // Sensor requesting this Word Parameter Definition 
} QAR_WORD_INFO, *QAR_WORD_INFO_PTR; 


#pragma pack(1)
// QAR Log Data Payload Structures 
typedef struct {
  UINT32 ResultCode;    // 0x00180000, SYS_QAR_PBIT_LOOPBACK
  UINT16 Test;          // Enum QAR_LOOPBACK_TEST
} SYS_QAR_PBIT_FAIL_LOG_LOOPBACK_STRUCT;

typedef struct {
  UINT32 ResultCode;   // 0x00180001, SYS_QAR_PBIT_DPRAM
  UINT16 ExpData;      // Expected Data
  UINT16 ActualData;   // Actual Data
} SYS_QAR_PBIT_FAIL_LOG_DPRAM_STRUCT;

typedef struct {
  UINT32 ResultCode;   // 0x00180002, SYS_QAR_FPGA_BAD_STATE
} SYS_QAR_PBIT_FAIL_LOG_FPGA_STRUCT;

typedef struct 
{
  RESULT result;
  UINT32 cfg_timeout; 
} QAR_TIMEOUT_LOG; 

typedef struct 
{
  RESULT result; // SYS_QAR_PBIT_FAIL
} QAR_SYS_PBIT_STARTUP_LOG; 

typedef struct 
{
  RESULT result;      // 0x00184000, SYS_QAR_WORD_TIMEOUT
  char FailMsg[QAR_WORD_TIMEOUT_FAIL_MSG_SIZE];
} SYS_QAR_CBIT_FAIL_WORD_TIMEOUT_LOG; 


// QAR CBIT Health Status 
typedef struct 
{
  QAR_SYSTEM_STATUS SystemStatus; 
  BOOLEAN           bQAREnable; 

  QAR_SF_STATE FrameState; 
                          
  UINT32       DataPresentLostCount;  
                    // Count of transitions from Data Present to Data Lost  
                    
  UINT32       LossOfFrameCount; // Instances that Frame Loss after a good 
                                //    SYNC has occurred.  Cnt of 
                                //    transitions from SYNC OK to SYNC LOSS
  UINT32       BarkerErrorCount; // Count of Barker Error Status.  Equivalent
                                //   to number of SF where Barker Error 
                                //   is indicated. 
} QAR_CBIT_HEALTH_COUNTS; 

typedef struct
{
   CHAR Name[QAR_MAX_NAME];
} QAR_SYS_HDR;

typedef struct
{
   UINT16                TotalSize;
   CHAR                  Name[QAR_MAX_NAME];
   QAR_PROTOCOL          Protocol;
   QAR_NUM_WORDS_CONTROL NumWords;
   UINT16                BarkerCode[QAR_SF_MAX];
} QAR_CHAN_HDR;
#pragma pack()


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( QAR_BODY )
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
EXPORT RESULT       QAR_Initialize      ( SYS_APP_ID *SysLogId, void *pdest, UINT16 *psize);
EXPORT void         QAR_Reframe         (void);

EXPORT void         QAR_Init_Tasks      (void); 

EXPORT void         QAR_Reconfigure     (void);

EXPORT UINT16       QAR_GetSubFrame     ( void *pDest, UINT32 nop, UINT16 nMaxByteSize ); 
EXPORT UINT16       QAR_GetSubFrameDisplay ( void *pDest, QAR_SF *lastServiced, 
                                             UINT16 nMaxByteSize ); 
EXPORT UINT16       QAR_GetFrameSnapShot( void *pDest, UINT32 nop, UINT16 nMaxByteSize, 
                                          BOOLEAN bStartSnap ); 

EXPORT UINT16       QAR_SensorSetup     (UINT32 gpA, UINT32 gpB, UINT16 nSensor); 
EXPORT FLOAT32      QAR_ReadWord        (UINT16 nIndex);

// EXPORT void         QAR_Enable          (void); 

EXPORT void         QAR_GetRegisters    (QAR_REGISTERS *Registers);
EXPORT QAR_STATE_PTR QAR_GetState       (void);
EXPORT QAR_CONFIGURATION_PTR QAR_GetCfg (void); 
EXPORT void         QAR_SetCfg          (QAR_CONFIGURATION *Cfg);
//EXPORT void         QAR_DisableAnyStreamingDebugOuput ( void );
EXPORT BOOLEAN      QAR_SensorTest (UINT16 nIndex); 
EXPORT BOOLEAN      QAR_InterfaceValid (UINT16 nIndex); 
EXPORT void         QAR_ProcessISR(void); 

EXPORT void  QARwrite( volatile UINT16* addr, FIELD_NAMES field, UINT16 data);
EXPORT UINT16 QARread( volatile UINT16* addr, FIELD_NAMES field);

EXPORT QAR_CBIT_HEALTH_COUNTS QAR_GetCBITHealthStatus ( void ); 
EXPORT QAR_CBIT_HEALTH_COUNTS QAR_CalcDiffCBITHealthStatus ( QAR_CBIT_HEALTH_COUNTS 
                                                             PrevCount );
EXPORT QAR_CBIT_HEALTH_COUNTS QAR_AddPrevCBITHealthStatus ( 
                                                     QAR_CBIT_HEALTH_COUNTS CurrCnt, 
                                                     QAR_CBIT_HEALTH_COUNTS PrevCnt ); 
                                                             
EXPORT void QAR_SyncTime( UINT32 chan );
EXPORT UINT16 QAR_GetFileHdr ( void *pDest, UINT32 chan, UINT16 nMaxByteSize );
EXPORT UINT16 QAR_GetSystemHdr ( void *pDest, UINT16 nMaxByteSize );

#ifdef GENERATE_SYS_LOGS
  EXPORT void QAR_CreateAllInternalLogs ( void ); 
#endif

#endif // QAR_H      


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: QAR.h $
 * 
 * *****************  Version 49  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 48  *****************
 * User: John Omalley Date: 9/30/11    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * SCR 1015 - Interface Valid Logic Update
 * 
 * *****************  Version 47  *****************
 * User: John Omalley Date: 9/12/11    Time: 9:22a
 * Updated in $/software/control processor/code/drivers
 * SCR 1070 - LOFS causes bad data retrieval
 * 
 * *****************  Version 46  *****************
 * User: John Omalley Date: 4/21/11    Time: 4:58p
 * Updated in $/software/control processor/code/drivers
 * SCR 1032 - Add Binary Data Header
 * 
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 10/01/10   Time: 11:36a
 * Updated in $/software/control processor/code/drivers
 * SCR #912 Code Review Updates
 * 
 * *****************  Version 44  *****************
 * User: Peter Lee    Date: 7/26/10    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #743 Update TimeOut Processing for val == 0
 * 
 * *****************  Version 43  *****************
 * User: John Omalley Date: 7/20/10    Time: 9:04a
 * Updated in $/software/control processor/code/drivers
 * SCR 294 - Add system binary header
 * 
 * *****************  Version 42  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:20p
 * Updated in $/software/control processor/code/drivers
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 6/08/10    Time: 12:04p
 * Updated in $/software/control processor/code/drivers
 * SCR 627 - Updated for LJ60
 * 
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 5/24/10    Time: 6:37p
 * Updated in $/software/control processor/code/drivers
 * SCR #608 Update QAR Interface Status logic to match Arinc / UART. 
 * 
 * *****************  Version 39  *****************
 * User: Contractor2  Date: 5/11/10    Time: 12:54p
 * Updated in $/software/control processor/code/drivers
 * SCR #587 Change TmTaskCreate to return void
 * 
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 5/06/10    Time: 2:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #579 Change LogWriteSys to return void
 * 
 * *****************  Version 37  *****************
 * User: Contractor3  Date: 5/06/10    Time: 11:27a
 * Updated in $/software/control processor/code/drivers
 * Check In SCR #586
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 5/03/10    Time: 11:12a
 * Updated in $/software/control processor/code/drivers
 * SCR #576 Updated, QAR_ALLOWED_INT_FREQ to 500 msec.
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 4/12/10    Time: 5:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #541 - all XXX_CreateLogs functions are now dependent on
 * GENERATE_SYS_LOGS
 * 
 * *****************  Version 34  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:40p
 * Updated in $/software/control processor/code/drivers
 * SCR #437 All passive user commands should be RO
 * 
 * *****************  Version 33  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 32  *****************
 * User: Jeff Vahue   Date: 2/19/10    Time: 12:57p
 * Updated in $/software/control processor/code/drivers
 * SCR# 455 - remove LINT issues
 * 
 * *****************  Version 31  *****************
 * User: Jeff Vahue   Date: 2/17/10    Time: 12:58p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused QAR debug output stream types
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 2/03/10    Time: 5:14p
 * Updated in $/software/control processor/code/drivers
 * SCR #439 Remove unneeded  NO_BYTES setting. 
 * 
 * *****************  Version 29  *****************
 * User: John Omalley Date: 1/15/10    Time: 5:30p
 * Updated in $/software/control processor/code/drivers
 * SCR 396
 * * Renamed Sensor Test to interface validation.
 * 
 * *****************  Version 28  *****************
 * User: Jeff Vahue   Date: 1/15/10    Time: 5:01p
 * Updated in $/software/control processor/code/drivers
 * SCR# 397
 * 
 * *****************  Version 27  *****************
 * User: Contractor V&v Date: 1/05/10    Time: 4:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 18
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 12/22/09   Time: 3:43p
 * Updated in $/software/control processor/code/drivers
 * SCR #29 Error: QAR Snapshot Data
 * 
 * *****************  Version 25  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:04p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - remove unused functions
 * 
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 12/16/09   Time: 2:17p
 * Updated in $/software/control processor/code/drivers
 * QAR Req 3110, 3041, 3114 and 3118
 * 
 * *****************  Version 23  *****************
 * User: Jeff Vahue   Date: 12/07/09   Time: 4:23p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - QAR startup times per requirements
 * 
 * *****************  Version 22  *****************
 * User: Jeff Vahue   Date: 12/03/09   Time: 4:59p
 * Updated in $/software/control processor/code/drivers
 * SCR# 350
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 11/12/09   Time: 10:13a
 * Updated in $/software/control processor/code/drivers
 * SCR #315 PWEH SEU Processing Support
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 11/02/09   Time: 1:51p
 * Updated in $/software/control processor/code/drivers
 * Misc Code Update to support WIN32 and spelling corrections
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 10/22/09   Time: 10:27a
 * Updated in $/software/control processor/code/drivers
 * SCR #171 Add correct System Status (Normal, Caution, Fault) enum to
 * UserTables
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 4:12p
 * Updated in $/software/control processor/code/drivers
 * SCR #229 Update Ch/Startup TimeOut to 1 sec from 1 msec resolution.
 * Updated GPB channel loss timeout to 1 msec from 1 sec resolution. 
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 10/20/09   Time: 6:49p
 * Updated in $/software/control processor/code/drivers
 * Updates for JV to support PC version of program
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #287 QAR_SensorTest() records log and debug msg. 
 * 
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 9/23/09    Time: 11:53a
 * Updated in $/software/control processor/code/drivers
 * SCR #268 Updates to _SensorTest() to include Ch Loss.  Have _ReadWord()
 * return FLOAT32
 * 
 * *****************  Version 14  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:09p
 * Updated in $/software/control processor/code/drivers
 * SCR #94, #95 PBIT returns log structure to Init Mgr
 * 
 * *****************  Version 13  *****************
 * User: Peter Lee    Date: 1/22/09    Time: 6:13p
 * Updated in $/control processor/code/drivers
 * SCR #136 PACK system log structures 
 * 
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 11/25/08   Time: 4:45p
 * Updated in $/control processor/code/drivers
 * SCR #109 Add _SensorValid() function
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 6:37p
 * Updated in $/control processor/code/drivers
 * SCR #107 qar.createlogs
 * 
 * *****************  Version 10  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:10p
 * Updated in $/control processor/code/drivers
 * SCR #97 QAR SW Updates
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 10/07/08   Time: 1:51p
 * Updated in $/control processor/code/drivers
 * SCR #87 Function Prototype
 * 
 *
 *****************************************************************************/
                       
