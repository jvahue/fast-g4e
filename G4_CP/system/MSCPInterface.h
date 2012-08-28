#ifndef MSCPINTERFACE_H
#define MSCPINTERFACE_H
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
               All Rights Reserved. Proprietary and Confidential.

  File:          MSCPInterface.h
 
  Description:   Contains the command/response packet structure, packet ID
                 enumerations and data structures for the data payload of 
                 the different packet IDs

  Note:          This file shall contain ONLY the enumerations and data 
                 structure related to the cmd/rsp packet and the data payload 
                 structures.  Function prototypes and inclusion of other
                 #include are forbidden.    
                 
                 THIS FILE IS SHARED BETWEEN THE CONTROL PROCESSOR AND
                 MICRO SERVER PROJECTS.

                 FILE IS UNDER THE VERSION CONTROL OF THE CONTROL PROCESSOR

      VERSION
      $Revision: 43 $  $Date: 8/28/12 1:43p $    

 ******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/ 
#include "alt_basic.h"
/*SHARED HEADER FILE, DO NOT INCLUDE ANY OTHER PROJECT SPECIFIC HEADER FILES*/

/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
/*SHARED HEADER FILE, DO NOT INCLUDE ANY PROJECT SPECIFIC HEADER FILES*/

/******************************************************************************
                                 Package Defines
******************************************************************************/
#ifdef CONTROL_PROCESSOR_GHS    //Control Processor - Green Hills compiler
  #define PACKED
#else
#ifdef MICRO_SERVER_GNU         //Micro Server - GNU compiler
  #define PACKED __attribute__ ((packed))
#else
  #error "Compiler type not defined"
#endif
#endif

#define ACS_MODEL_SIZE          16
#define ACS_LOG_FILE_EXT_SIZE   8

#define MSCP_MAX_STRING_SIZE     64   //Default string buffer size used in cmds
#define MSCP_PATH_FN_STR_SIZE    128  //Default max file path and name string
                                      //length. Rational: Path to
                                      //CP_SAVED ~28chars +FN ~25 chars, rounded
                                      //up to even 64 and doubled
#define MSCP_MAX_PENDING_RSP     10   //Max number of CP commands that can be
                                     // awaiting responses from the MS
#define MSCP_NUM_OF_CMD_HANDLERS 10   //The number of entries in the command
                                     //handler table needed to define every
                                     //command that can originate from the
                                     //microserver.
                                     
#define MSCP_CMDRSP_PACKET_SIZE      (8180) //Maximum size of the CMDRSP packet
#define MSCP_CMDRSP_MAX_DATA_SIZE    (MSCP_CMDRSP_PACKET_SIZE - 18) //Max size
                                            //of the data payload of the
                                            //CMDRSP packet
                                      
#define MSCP_FILE_UPLOAD_DATA_SIZE (1024 + 768)
#define MSCP_USER_MGR_STR_LEN       2048

#define MSCP_MAX_NUM_IP         2   // report IP for WLAN and GSM
#define MSCP_MAX_NUM_SIGNAL     2   // report signal for WLAN and GSM

#define MSCP_AC_TYPE_LEN 16
#define MSCP_AC_FLEETID_LEN 8
#define MSCP_AC_NUMBER_LEN 8
#define MSCP_AC_TAILNUM_LEN 16
#define MSCP_AC_STYLE_LEN 16
#define MSCP_AC_OPERATOR_LEN 32
#define MSCP_AC_OWNER_LEN 64

#define MSCP_MAX_FILE_DATA_SIZE 1024

#define MSCP_MAX_STATUS_STR_SIZE 32
#define MSCP_MAX_ERROR_STR_SIZE  8

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
// command ids
typedef enum 
{
    CMD_ID_NONE            = 0x0000,
    CMD_ID_CONNECT_GSM     = 0x0101,  /*Cmd: MSCP_GSM_CONFIG             Rsp: None*/
    CMD_ID_GET_GSM_STATUS  = 0x0102,  /*Cmd: ?                           Rsp: ?*/
    CMD_ID_SETUP_WLAN      = 0x0103,  /*Cmd: MSCP_WLAN_CONFIG            Rsp: None*/ 
    CMD_ID_GET_WLAN_STATUS = 0x0104,  /*Cmd: ?                           Rsp: ?*/
    CMD_ID_SET_TX_METHOD   = 0x0105,  /*Cmd: MSCP_TX_CONFIG              Rsp: ?*/
    CMD_ID_START_LOG_DATA  = 0x0106,  /*Cmd: MSCP_LOGFILE_INFO           Rsp: ?*/
    CMD_ID_LOG_DATA        = 0x0107,  /*Cmd: MSCP_FILE_PACKET_INFO       Rsp: ?*/
    CMD_ID_END_LOG_DATA    = 0x0108,  /*Cmd: MSCP_LOGFILE_INFO           Rsp: ?*/
    CMD_ID_FTP_CHECK       = 0x0109,  /*Cmd: ? Rsp: ?*/      /*reserved in FAST*/ 
    CMD_ID_FTP_FILE        = 0x010A,  /*Cmd: MSCP_CMD_FTP_FILE_INFO      
                                        Rsp: ?*/             /*reserved in FAST*/
    CMD_ID_REMOVE_LOG_FILE = 0x010B,  /*Cmd: MSCP_LOGFILE_INFO           
                                        Rsp: MSCP_REMOVE_FILE_RSP  */
    CMD_ID_USER_MGR_MS     = 0x010C,  /*Cmd: MSCP_USER_MGR_CMD           
                                        Rsp: MSCP_USER_MGR_RSP     */  
    CMD_ID_USER_MGR_GSE    = 0x010D,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/ 
    CMD_ID_SETUP_GSM       = 0x010E,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/ 
    CMD_ID_UPLOAD_START    = 0x010F,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/   
    CMD_ID_UPLOAD_DATA     = 0x0110,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/ 
    CMD_ID_UPLOAD_END      = 0x0111,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/     
    CMD_ID_UPLOAD_STATUS   = 0x0112,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/  
    CMD_ID_FTPBACKGND_CTL  = 0x0113,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/  
    CMD_ID_UPDATE_CFG      = 0x0114,
    CMD_ID_ONGROUND_STATE  = 0x0115,  /*Cmd: ?                           
                                        Rsp: ?*/             /*reserved in FAST*/
    CMD_ID_VALIDATE_CRC    = 0x0116,  /*Cmd: MSCP_VALIDATE_CRC_CMD       
                                        Rsp: MSCP_VALIDATE_CRC_RSP      */  
    CMD_ID_CHECK_FILE      = 0x0117,  /*Cmd: MSCP_CHECK_FILE_CMD         
                                        Rsp: MSCP_CHECK_FILE_RSP        */  
    CMD_ID_SET_ROUTE       = 0x0118,  /*Cmd: ?                           Rsp: ?*/  
    CMD_ID_GET_ROUTE       = 0x0119,  /*Cmd: ?                           Rsp: ?*/  
    CMD_ID_SET_URLS        = 0x011A,  /*Cmd: ?                           Rsp: ?*/  
    CMD_ID_GET_URLS        = 0x011B,  /*Cmd: ?                           Rsp: ?*/  
    CMD_ID_GET_MSSIM_INFO  = 0x011C,  /*Cmd: No command data             Rsp: MSCP_MS_GET_INFO_RSP;*/  
    CMD_ID_GET_CP_INFO     = 0x011D,  /*Cmd: No command data             Rsp  MSCP_CP_GET_INFO_RSP*/
                                      /*Rsp: MSCP_GET_CP_INFO_RSP       */  
    CMD_ID_HEARTBEAT       = 0x011E,  /*Cmd: MSCP_CP_HEARTBEAT_CMD       
                                        Rsp: MSCP_MS_HEARTBEAT_RSP      */  
    CMD_ID_SHELL_CMD       = 0x011F,  /*Cmd: MSCP_CP_SHELL_CMD           
                                        Rsp: MSCP_MS_SHELL_RSP          */
    CMD_ID_GET_FILE_START  = 0x0120,  /*Cmd: MSCP_CP_GET_FILE_START_CMD  
                                        Rsp: MSCP_MS_GET_FILE_START_RSP */
    CMD_ID_GET_FILE_DATA   = 0x0121,  /*Cmd: MSCP_MS_GET_FILE_DATA_CMD   
                                        Rsp: MSCP_CP_GET_FILE_DATA_RSP  */
    CMD_ID_FILE_XFRD       = 0x0122,  /*Cmd: MSCP_CP_FILE_XFRD_CMD       
                                        Rsp: MSCP_MS_FILE_XFRD_RSP      */
    CMD_ID_MSNGR_CTL       = 0x0123,  /*Cmd: MSCP_CP_MSNGR_CTL_CMD       
                                        Rsp: MSCP_MS_MSNGR_CTL_RSP      */
    CMD_ID_PUT_FILE_START  = 0x0124,  /*Cmd: MSCP_CP_PUT_FILE_START_CMD  
                                        Rsp: None                       */
    CMD_ID_PUT_FILE_DATA   = 0x0125,  /*Cmd: MSCP_MS_PUT_FILE_DATA_CMD   
                                        Rsp: None                       */
    CMD_ID_REQ_CFG_FILES   = 0x0126,  /*Cmd: MSCP_CP_REQ_CFG_FILES_CMD   
                                        Rsp: MSCP_MS_REQ_CFG_FILES_RSP  */
    CMD_ID_REQ_START_RECFG = 0x0127,  /*Cmd: MSCP_CP_REQ_START_RECFG_CMD 
                                        Rsp: None                       */
    CMD_ID_REQ_STOP_RECFG  = 0x0128,  /*Cmd: MSCP_CP_REQ_START_RECFG_CMD 
                                        Rsp: None                       */
    CMD_ID_GET_RECFG_STS   = 0x0129,  /*Cmd: MSCP_CP_GET_RECFG_STS_CMD   
                                        Rsp: MSCP_MS_GET_RECFG_STS_RSP  */
    
    CMD_ID_LAST,

} MSCP_CMD_ID;

//Command/Response packet structure
#ifdef CONTROL_PROCESSOR_GHS
 #pragma pack(1)
#endif

typedef struct {
    UINT32 checksum;              // check sum for data in this structure    
    UINT16 type;                  // Defines the packet as a command from MS
                                  // or a response from the MS.
    UINT16 id;                    // command id or id associate with this response,
                                  // (MSCP_CMD_ID)
    UINT32 sequence;              // response/command sequence id 
    UINT16 status;                // status for response only (MSCP_RSP_STATUS)
    UINT8  UmRspSource;           // see UM_RSP_SOURCE
    UINT8  UmDataFType;           // see UM_CMD_TYPE
    UINT16 nDataSize;             // size of data buffer
    UINT8  data[MSCP_CMDRSP_MAX_DATA_SIZE]; // data

} PACKED MSCP_CMDRSP_PACKET;


//command / response type for the "Type" field of MSCP_CMDRSP_PACKET
typedef enum
{
  MSCP_PACKET_RESPONSE,   //Packet is a response from the micro-server
  MSCP_PACKET_COMMAND     //Packet is a command from the micro-server
} MSCP_PACKET_TYPE;

//command/response packet source (Legacy field, always set to MS).  
typedef enum
{
  MSCP_GSE,
  MSCP_MS,
  MSCP_ACS
} MSCP_PACKET_SOURCE;

//command/response status for the "Status" field of MSCP_CMDRSP_PACKET
// response status
typedef enum
{
  MSCP_RSP_STATUS_FAIL      = 0x00000000,
  MSCP_RSP_STATUS_DATA      = 0x00000001,
  MSCP_RSP_STATUS_SUCCESS   = 0x00000002,
  MSCP_RSP_STATUS_BUSY      = 0x00000003
} MSCP_RSP_STATUS;

//Command ID: Connect GSM
typedef enum
{
  MSCP_GSM_MODE_GPRS,
  MSCP_GSM_MODE_GSM
} MSCP_GSM_MODE;

typedef struct {
  INT8   Phone[MSCP_MAX_STRING_SIZE];     
  UINT16 Mode; 
  INT8   APN[MSCP_MAX_STRING_SIZE];
  INT8   User[MSCP_MAX_STRING_SIZE];
  INT8   Password[MSCP_MAX_STRING_SIZE];
  INT8   Carrier[MSCP_MAX_STRING_SIZE];
  INT8   MCC[MSCP_MAX_STRING_SIZE];
  INT8   MNC[MSCP_MAX_STRING_SIZE];
}  PACKED MSCP_GSM_CONFIG_CMD;


//Command ID: Setup WLAN
typedef enum
{
  MSCP_WLAN_MODE_MANAGED,
  MSCP_WLAN_MODE_MONITOR,
  MSCP_WLAN_MODE_ADHOC
} MSCP_WLAN_MODE;

typedef struct
{
  UINT16  Mode;           // see WLAN_MODE
  INT8    EssId[MSCP_MAX_STRING_SIZE];
  INT8    Wep[MSCP_MAX_STRING_SIZE];
  INT8    Ip[MSCP_MAX_STRING_SIZE];
  INT8    GateWay[MSCP_MAX_STRING_SIZE];
  INT8    NetMask[MSCP_MAX_STRING_SIZE];
}  PACKED MSCP_WLAN_CONFIG;


//Command ID: Get WLAN Status
typedef enum
{
    DTUMGR_RSP_DISCONNECTED,
    DTUMGR_RSP_CONNECTED
} MSCP_CONNECT_STATUS;

typedef struct
{
    INT8    SwRev[MSCP_MAX_STRING_SIZE];
    INT16   Signal[MSCP_MAX_NUM_SIGNAL]; // always report signal for WLAN and GSM
    INT16   Noise;
    UINT16  QualityPercent;
    UINT32  Time;
    UINT16  ConnectStatus;   // see MSCP_CONNECT_STATUS
    INT8    Ip[MSCP_MAX_NUM_IP][MSCP_MAX_STRING_SIZE];  // always report IP for WLAN and GSM
    INT8    GateWay[MSCP_MAX_STRING_SIZE];
    INT8    NetMask[MSCP_MAX_STRING_SIZE];

    // WLAN use only
    INT8    Essid[MSCP_MAX_STRING_SIZE];
    INT8    Mode[MSCP_MAX_STRING_SIZE];
    INT8    Wep[MSCP_MAX_STRING_SIZE];

    // GSM use only
    INT8    SimSerial[MSCP_MAX_STRING_SIZE];
    INT8    CountryCodeOperator[MSCP_MAX_STRING_SIZE];
    INT8    Cell[MSCP_MAX_STRING_SIZE];
    INT8    Lac[MSCP_MAX_STRING_SIZE];
    INT8    Ncc[MSCP_MAX_STRING_SIZE];
    INT8    Bcc[MSCP_MAX_STRING_SIZE];
    // GSM settings
    INT8    Phone[MSCP_MAX_STRING_SIZE];
    INT8    APN[MSCP_MAX_STRING_SIZE];
    INT8    User[MSCP_MAX_STRING_SIZE];
    INT8    Password[MSCP_MAX_STRING_SIZE];

}  PACKED MSCP_NET_INFO;


//Command ID: Set TX Method
typedef enum
{
    MSCP_TX_METHOD_WLAN,
    MSCP_TX_METHOD_GSM,
    MSCP_TX_METHOD_AUTO
} MSCP_TX_METHOD;

//---Command ID: Start Log Data---
/*Data Structure for pre-2.0.0 FAST Control Processor software*/
typedef struct 
{
    INT8    FileName[MSCP_MAX_STRING_SIZE];
}  PACKED MSCP_LOGFILE_INFO_LEGACY;

//---Command ID: Start Log Data---
typedef struct 
{
    INT8    FileName[MSCP_MAX_STRING_SIZE];
    INT32   Priority;
}  PACKED MSCP_LOGFILE_INFO;

//---Command ID: Log Data---
typedef struct 
{
    UINT16 id;
    UINT16 seq;
}  PACKED MSCP_FILE_PACKET_INFO;


//---Command ID: End Log Data---
//Same data as Start Log Data

//---Command ID: FTP Check---
//CP Command - Same data as Start Log Data
//MS Response
typedef enum
{
    MSCP_FILE_TX_STATE_NONE,          // 0
    MSCP_FILE_TX_STATE_INPROGRESS,    // 1
    MSCP_FILE_TX_STATE_ERROR,         // 2
    MSCP_FILE_TX_STATE_COMPLETED      // 3

} MSCP_FILE_TX_STATE;

typedef enum
{
    MSCP_FILE_TX_STATE_ERROR_NONE,                    // 0

    MSCP_FILE_TX_STATE_ERROR_EVAL_LOG_FILEOPEN,       // 1
    MSCP_FILE_TX_STATE_ERROR_EVAL_LOG_FILESIZE,       // 2
    MSCP_FILE_TX_STATE_ERROR_EVAL_LOG_CHKSUM,         // 3

    MSCP_FILE_TX_STATE_ERROR_FTP,                     // 4

    MSCP_FILE_TX_STATE_ERROR_EVAL_RSP_RETRIEVE,       // 5
    MSCP_FILE_TX_STATE_ERROR_EVAL_RSP_TTRSP           // 6

} MSCP_FILE_TX_STATE_ERROR_TYPE;

typedef struct 
{
    CHAR    AcsName[MSCP_MAX_STRING_SIZE];  // ACS install id-version id-time stamp
                                            // not used when log type is DTUMGR_LOG_TYPE_DTU
    UINT16  Priority;                       // priority of log data of this ACS
                                            // not used when log type is DTUMGR_LOG_TYPE_DTU
    UINT16  Status;                         // TBD added def for DTUMGR_TT_LOG_STATUS
    UINT16  Detail;                         // TBD added def for 
                                            //     see DTUMGR_TT_LOG_STATUS_DETAIL - TBD
    UINT32  nDataSize;                      // size of data block
    UINT32  nCheckSum;                      // checksum for this data block, this is long word
                                            // will not be cacluated
} PACKED MSCP_TT_RSP;
 
typedef struct
{
    UINT16 state;                          // see MS_FILE_TX_STATE
    UINT16 error;                          // see MS_FILE_TX_STATE_ERROR_TYPE
    CHAR errmsg[MSCP_MAX_STRING_SIZE];     // explained what failed in the Setrix
    CHAR FileName[MSCP_MAX_STRING_SIZE];
    MSCP_TT_RSP LogRsp;
} PACKED MSCP_FILE_TX_STATUS;

//---Command ID: FTP File---
typedef struct
{ 
    INT8    FilePath[MSCP_MAX_STRING_SIZE];
}  MSCP_CMD_FTP_FILE_INFO;

//---Command ID: Remove Log File---
//Cmd: Same data as Start Log Data

typedef struct {
  UINT16 Result;
}
PACKED MSCP_REMOVE_FILE_RSP;

typedef enum{
  MSCP_REMOVE_FILE_OK,                  //File was found and deleted
  MSCP_REMOVE_FILE_FNF                  //File was not found, not action taken
} MSCP_REMOVE_FILE_RESULT;

//---Command ID: User Mgr MS---
typedef struct{
  INT8  cmd[MSCP_USER_MGR_STR_LEN];    //Buffer to hold the maximum size string
}PACKED MSCP_USER_MGR_CMD;


typedef struct{
  INT8  rsp[MSCP_USER_MGR_STR_LEN];    //Buffer to hold the maximum size string
}PACKED MSCP_USER_MGR_RSP;

//---Command ID: Setup GSM---
//Same data as connect GSM

//---Command ID: Upload Start---
typedef struct
{
    INT8    srcFileName[MSCP_MAX_STRING_SIZE];    // source file name
    INT8    tgtFilePathName[MSCP_MAX_STRING_SIZE];// destination (i.e. /home/mssim
    INT8    fileMode[MSCP_MAX_STRING_SIZE];       // Linux file attribute, (i.e."+x")
    UINT32  nFileSize;                     // size of entire file
    UINT32  nFileChkSum;                   // checksum of entire file
    UINT16  status;                        // see MS_FILE_UPLOAD_STATUS
    UINT16  state;                         // see MS_FILE_UPLOAD_STATE
    UINT32  nCurUploadBlk;                 // current block of data upload to MS
} PACKED MSCP_FILE_UPLOAD_INFO;


//---Command ID: Upload Data---
typedef struct
{
    UINT32  nTotalBlk;                          // number of full block
    UINT32  nLastBlkSize;                       // number of bytes in the last block
    UINT32  nCurBlkNum;                         // current block number
    UINT32  nNextBlkNum;                        // expected next block number
    UINT32  size;                               // size of this data block
    UINT32  nCurOffset;                         // current offset to file
    UINT32  nNextOffset;                        // expected next offset to file
    UINT8   data[MSCP_FILE_UPLOAD_DATA_SIZE];   // raw data
} PACKED MSCP_FILE_UPLOAD_DATA;


//---Command ID: Upload End---
//Same data as Upload Start

//---Command ID: Upload Status---
//Same data as file upload info, uses these enums for status and state fields
typedef enum
{
    // CP specific
    MSCP_FILE_UPLOAD_STATUS_IDLE,
    MSCP_FILE_UPLOAD_STATUS_INPROGRESS_STARTED,
    MSCP_FILE_UPLOAD_STATUS_INPROGRESS_DTU,
    MSCP_FILE_UPLOAD_STATUS_INPROGRESS_MS,
    MSCP_FILE_UPLOAD_STATUS_ABORT_MS,
    MSCP_FILE_UPLOAD_STATUS_DONE,
    // MS specific
    MSCP_FILE_UPLOAD_STATUS_SUCCESS,
    MSCP_FILE_UPLOAD_STATUS_FAIL,
    MSCP_FILE_UPLOAD_STATUS_FAIL_SRC_FILE,
    MSCP_FILE_UPLOAD_STATUS_FAIL_TGT_FILE,
    MSCP_FILE_UPLOAD_STATUS_FAIL_SIZE,
    MSCP_FILE_UPLOAD_STATUS_FAIL_CHKSUM
} MSCP_FILE_UPLOAD_STATUS;

typedef enum
{
    MSCP_FILE_UPLOAD_STATE_START,
    MSCP_FILE_UPLOAD_STATE_DATA,
    MSCP_FILE_UPLOAD_STATE_END,
    MSCP_FILE_UPLOAD_STATE_STATUS
} MSCP_FILE_UPLOAD_STATE;


//---Command ID: Update Cfg---
typedef struct AircraftCfgData
{
    UINT16        CfgVer;
    UINT16        Update_Method;    
    UINT16        CfgStatus;
    INT8          ClientStatusStr[MSCP_MAX_STRING_SIZE];  //Store the cfg error code generated
                                                          //by the client connected to MSSIM
    INT8          SerialNumber[MSCP_MAX_STRING_SIZE];
    INT8          Type[MSCP_AC_TYPE_LEN];
    INT8          FleetIdent[MSCP_AC_FLEETID_LEN];
    INT8          Number[MSCP_AC_NUMBER_LEN];
    INT8          TailNumber[MSCP_AC_TAILNUM_LEN];
    INT8          Style[MSCP_AC_STYLE_LEN];
    INT8          Operator[MSCP_AC_OPERATOR_LEN];
    INT8          Owner[MSCP_AC_OWNER_LEN];
} PACKED AIRCRAFT_CFG_INFO;

//enum for the Update_Method field of the AIRCRAFT_CFG_INFO structure
typedef enum
{
   AC_UPDATE_NONE,
   AC_UPDATE_SN,
//   AC_UPDATE_ID,
   AC_UPDATE_CHECK_VER,
   AC_UPDATE_CHECK_NO_VALIDATE,
   AC_UPDATE_MAX
} UPDATE_METHOD;

typedef enum
{
  MSCP_AC_CFG_SUCCESS,             // 0    0 - not used
  MSCP_AC_NOT_USED,                // 1    1 - not used 
  MSCP_AC_XML_SERVER_NOT_FOUND,    // 2    2 - Server not found OK
  MSCP_AC_XML_FILE_NOT_FOUND,      // 3    3 - File not found 
  MSCP_AC_XML_FAILED_TO_OPEN,      // 4    4 - File failed to open ???
  MSCP_AC_XML_FAST_NOT_FOUND,      // 5    5 - File format error (any type)
  MSCP_AC_XML_SN_NOT_FOUND,        // 6    6 - Serial number not in file
  MSCP_AC_CFG_SERVER_NOT_FOUND,    // 7    7 - Configuration server not found
  MSCP_AC_CFG_FILE_NOT_FOUND,      // 8    8 - Configuration file not found
  MSCP_AC_CFG_FILE_NO_VER,         // 9    9 - Configuration version is missing
  MSCP_AC_CFG_FILE_FMT_ERR,        // 10   10- Configuration file format error NEW
  MSCP_AC_CFG_FAILED_TO_UPDATE,    // 11   11- Reconfiguration of FAST failed
  MSCP_AC_ID_MISMATCH,             // 12   12- Aircraft ID mismatch (was 001)     
  MSCP_AC_CFG_NO_VALIDATE,         // 13   13- Validate data == no
  MSCP_AC_CFG_NOTHING,
  MSCP_AC_CFG_STARTED,
  MSCP_AC_CFG_CLIENT_ERROR
} AC_CFG_STATUS;


//---Command ID: CMD_ID_ONGROUND_STATE---
typedef struct 
{
    UINT16  OnGround;             // TRUE = On Ground, FALSE = In Air 
                                  // Current State of DtuMgrConfig->OnGround
    UINT32  CurrentTime;          // Current DTU Time from TIMESTAMP->Timestamp fmt 
                                  //   See CmGetTimeStamp() for fmt 
    UINT16  StartupComplete;      // Flag to indicate Dtu Re-starting.  
                                  // Used by mssim to restart TC35
} PACKED MSCP_ONGROUND_STATE_AND_TIME; 


//---Command ID: CMD_ID_VALIDATE_CRC---
//Command from MS to CP 
typedef struct
{
  INT8    FN[MSCP_MAX_STRING_SIZE];
  UINT32  CRC;
  UINT16  CRCSrc;
}PACKED MSCP_VALIDATE_CRC_CMD;

typedef enum
{
  MSCP_CRC_SRC_NULL = 0,          //No source (for testing)
  MSCP_CRC_SRC_MS = 1,            //CRC from the MicroServer
  MSCP_CRC_SRC_GS = 2,            //CRC from the ground server
}MSCP_CRC_SRC;

//Response from CP to MS
typedef struct
{
  UINT16  CRCRes;                 //See: MSCP_CRC_RES
}PACKED MSCP_VALIDATE_CRC_RSP;

typedef enum
{
  MSCP_CRC_RES_PASS = 0,          //CRC values match
  MSCP_CRC_RES_FAIL = 1,          //CRC values do not match
  MSCP_CRC_RES_FNF  = 2           //The file name sent was not found.
}MSCP_CRC_RES;

//---Command ID: CMD_ID_CHECK_FILE
//Command from CP to MS
typedef struct
{
  INT8    FN[MSCP_MAX_STRING_SIZE];
}PACKED MSCP_CHECK_FILE_CMD;

//Response from MS to CP
typedef struct
{
  UINT16  FileSta;                 //See: MSCP_FILE_STA  
}PACKED MSCP_CHECK_FILE_RSP;

typedef enum
{
  MSCP_FILE_STA_FNF         = 0,     //File not found.
  MSCP_FILE_STA_NOT_YET_VALIDATED  = 1, //File validation not performed yet
  MSCP_FILE_STA_MSFAIL      = 2,     //File on compact flash failed CRC check
  MSCP_FILE_STA_MSVALID     = 3,     //File validated on the Compact Flash
  MSCP_FILE_STA_GSFAIL      = 4,     //File validation failed on the Ground 
  MSCP_FILE_STA_GSVALID     = 5      //File validated on the Ground Server
}MSCP_FILE_STA;                      //The order of enum is expected to be the same order as 
                                     //the verification process i.e. 
                                     //not verified->ms failed || ms verified->gs failed || gs verified


//----???
typedef enum
{
    MS_FILE_TX_STATE_NONE,          // 0
    MS_FILE_TX_STATE_INPROGRESS,    // 1
    MS_FILE_TX_STATE_ERROR,         // 2
    MS_FILE_TX_STATE_COMPLETED      // 3
} MS_FILE_TX_STATE;

// Log file Destination Server response status (file)
typedef struct DtuMgrTTRspStruct
{
    CHAR    AcsName[MSCP_MAX_STRING_SIZE];  // ACS install id-version id-time stamp
                                            // not used when log type is DTUMGR_LOG_TYPE_DTU
    UINT16  Priority;                       // priority of log data of this ACS
                                            // not used when log type is DTUMGR_LOG_TYPE_DTU
    UINT16  Status;                         // see DTUMGR_TT_LOG_STATUS
    UINT16  Detail;                         // see DTUMGR_TT_LOG_STATUS_DETAIL - TBD
    UINT32  nDataSize;                      // size of data block
    UINT32  nCheckSum;                      // checksum for this data block, this is long word
                                            // will not be cacluated
} PACKED DTUMGR_TT_RSP, *DTUMGR_TT_RSP_PTR;

// FTP Status
typedef struct 
{
    UINT16 state;                        // see MS_FILE_TX_STATE
    UINT16 error;                        // see MS_FILE_TX_STATE_ERROR_TYPE
    CHAR errmsg[MSCP_MAX_STRING_SIZE];   // explained what failed in the Setrix
    CHAR FileName[MSCP_MAX_STRING_SIZE];
    DTUMGR_TT_RSP LogRsp;
} PACKED MS_FILE_TX_STATUS;

//---Command ID: CMD_ID_GET_CP_INFO---
typedef struct
{
  INT8  SN[MSCP_MAX_STRING_SIZE];
//  CHAR  SwVer[MSCP_MAX_STRING_SIZE];
  INT32 Pri4DirMaxSizeMB;  
}MSCP_GET_CP_INFO_RSP;


//---Command ID: CMD_ID_GET_CP_INFO---
/*Data Structure for pre-2.0.0 FAST Control Processor software*/
typedef struct
{
  INT8 SN[MSCP_MAX_STRING_SIZE];
}MSCP_GET_CP_INFO_RSP_LEGACY;

//---Command ID: CMD_ID_GET_MSSIM_INFO---
typedef struct
{
  CHAR Imei[MSCP_MAX_STRING_SIZE];     //IMEI decoded from /var/run/gsm
  CHAR Scid[MSCP_MAX_STRING_SIZE];     //SCID decoded from /var/run/gsm
  CHAR Signal[MSCP_MAX_STRING_SIZE];   //Signal strength from /var/run/gsm   
  CHAR Operator[MSCP_MAX_STRING_SIZE]; //Current Operator from /var/run/gsm   
  CHAR Cimi[MSCP_MAX_STRING_SIZE];
  CHAR Creg[MSCP_MAX_STRING_SIZE];
  CHAR Lac[MSCP_MAX_STRING_SIZE];
  CHAR Cell[MSCP_MAX_STRING_SIZE];
  CHAR Ncc[MSCP_MAX_STRING_SIZE];
  CHAR Bcc[MSCP_MAX_STRING_SIZE];
  CHAR Cnum[MSCP_MAX_STRING_SIZE];
  CHAR Sbv[MSCP_MAX_STRING_SIZE];
  CHAR Sctm[MSCP_MAX_STRING_SIZE];
  CHAR Other[1024];                    //List of additional lines from /var/run/gsm not 
}MSCP_MS_GSM_INFO;                        // decoded into the other memebers of MS_GSM_INFO

//Static information about the micro-server
typedef struct
{
  CHAR MssimVer[MSCP_MAX_STRING_SIZE];
  CHAR SetrixVer[MSCP_MAX_STRING_SIZE];
  CHAR PWEHVer[MSCP_MAX_STRING_SIZE];
  MSCP_MS_GSM_INFO Gsm;
}MSCP_GET_MS_INFO_RSP;

//---Command ID: CMD_ID_HEARTBEAT---
//Log transfer status
typedef enum
{
  MSCP_XFR_NONE,
  MSCP_XFR_DATA_LOG_TX,
  MSCP_XFR_SPECIAL_TX,
  //MSCP_XFR_SPECIAL_RX
}MSCP_XFR;

//RF lamp status
typedef enum
{
  MSCP_STS_OFF,
  MSCP_STS_ON_W_LOGS,
  MSCP_STS_ON_WO_LOGS
}MSCP_STS;

//Time data for the command and response
typedef struct
{
    UINT16          Year;              // 1997..2082
    UINT16          Month;             // 1..12
    UINT16          Day;               // 1..31   Day of Month
    UINT16          Hour;              // 0..23
    UINT16          Minute;            // 0..59
    UINT16          Second;            // 0..59
    UINT16          MilliSecond;       // 0..999
} PACKED MSCP_TIME;


#ifdef ENV_TEST
  // BUG: ENV_TEST filler size reduced by 1000 as full buffer size crashes the MS 6/10/09
  #define CP_MS_SIZE (MSCP_CMDRSP_MAX_DATA_SIZE-(sizeof(MSCP_TIME)+sizeof(BOOLEAN)+1000))
  #define MS_CP_SIZE (MSCP_CMDRSP_MAX_DATA_SIZE\
                      -(sizeof(MSCP_TIME)+(2*sizeof(BOOLEAN))+(2*sizeof(UINT16)+1000)))
#endif

typedef struct
{
  MSCP_TIME   Time;
  BOOLEAN     OnGround;
#ifdef ENV_TEST
  UINT8       filler[CP_MS_SIZE];
#endif
} PACKED MSCP_CP_HEARTBEAT_CMD;

typedef struct
{
  MSCP_TIME  Time;
  BOOLEAN    TimeSynched;
  BOOLEAN    ClientConnected;
  BOOLEAN    VPNConnected;
  BOOLEAN    CompactFlashMounted;
  UINT16     Xfr;  //Use MSCP_XFR Enum
  UINT16     Sta;  //Use MSCP_STS Enum
#ifdef ENV_TEST
  UINT8       filler[MS_CP_SIZE];
#endif

} PACKED MSCP_MS_HEARTBEAT_RSP;


//---Command ID: CMD_ID_SHELL_CMD---
//MSCP_CP_SHELL_CMD
typedef struct{
  UINT32 Len;
  INT8   Str[MSCP_CMDRSP_MAX_DATA_SIZE];
}PACKED MSCP_CP_SHELL_CMD;

//MSCP_CP_SHELL_RSP
typedef struct{
  UINT32 Len;
  INT8   Str[MSCP_CMDRSP_MAX_DATA_SIZE];
}PACKED MSCP_MS_SHELL_RSP;


//---Command ID: CMD_ID_GET_FILE_START---
typedef struct{
  INT8   Str[MSCP_PATH_FN_STR_SIZE];
}PACKED MSCP_CP_GET_FILE_START_CMD;

typedef enum{
  MSCP_FILE_START_OK,
  MSCP_FILE_START_FNF
} FILE_START_STATUS;

typedef struct{
  UINT32    Status;
}PACKED MSCP_MS_GET_FILE_START_RSP;

//---Command ID: CMD_ID_GET_FILE_DATA---
typedef struct{
  UINT32    Size;
  UINT32    UnixTime;
  INT8      FN[MSCP_PATH_FN_STR_SIZE];
}PACKED MSCP_FILE_DATA_INFO;

/*Data field is".FileInfo" for BlockNum == 0
  Data field is ".Block" for file data, where block number
  increments every block and rolls over from 255 to 1*/
typedef struct{
  UINT8  BlockNum; /*BlockNum 0 indicates start of new file, */
  UINT32 Len;
  union  {INT8 Block[MSCP_CMDRSP_MAX_DATA_SIZE-5]; //Data block size is the maximum 
                                                   //data size, minus the size of the 
                                                   //"BlockNum" and "Len" memebers
          MSCP_FILE_DATA_INFO FileInfo;}Data;
}PACKED MSCP_MS_GET_FILE_DATA_CMD;

typedef enum{
  MSCP_FILE_DATA_OK,      /*data received okay*/
  MSCP_FILE_DATA_CAN      /*cancel data transfer*/
} MSCP_FILE_DATA_STATUS;

typedef struct{
  UINT32 Status;
}PACKED MSCP_CP_GET_FILE_DATA_RSP;


//---Command ID: CMD_ID_FILE_XFRD
/*Cmd: MSCP_CP_FILE_XFRD_CMD  Rsp: MSCP_MS_FILE_XFRD_RSP*/
typedef struct{
  INT8      FN[MSCP_PATH_FN_STR_SIZE];
}PACKED MSCP_CP_FILE_XFRD_CMD;

typedef enum{
  MSCP_FILE_XFRD_OK                      = 0,
  MSCP_FILE_XFRD_CRC_FILE_EXISTS         = 1,
  MSCP_FILE_XFRD_LOG_ALREADY_MOVED       = 2,
  MSCP_PRI4_FILE_XFRD_OK                 = 3,
  MSCP_PRI4_FILE_XFRD_LOG_ALREADY_MOVED  = 4,
  MSCP_FILE_XFRD_FNF                = -1,
  MSCP_FILE_XFRD_ERROR              = -2
}MSCP_FILE_XFRD_STATUS;

typedef struct{
  INT32 Status;
}PACKED MSCP_MS_FILE_XFRD_RSP;

//---Command ID: CMD_ID_MSNGR_CTL
typedef struct{
  BOOLEAN Pause;   //TRUE: Pause; FALSE: Run 
}PACKED MSCP_CP_MSNGR_CTL_CMD;

//---Command ID: CMD_ID_PUT_FILE_START
/*Cmd: MSCP_CP_PUT_FILE_START_CMD Rsp: no response data*/
typedef MSCP_FILE_DATA_INFO MSCP_CP_PUT_FILE_START_CMD;

//---Command ID: CMD_ID_PUT_FILE_DATA
/*Cmd: MSCP_CP_PUT_FILE_DATA_CMD  Rsp: no response data*/
typedef struct{
  UINT8     Block;
  UINT32    Len;
  INT8      Data[MSCP_MAX_FILE_DATA_SIZE];
}PACKED MSCP_CP_PUT_FILE_DATA_CMD;

//---Command ID: CMD_ID_REQ_CFG_FILES
/*Cmd: MSCP_CP_REQ_CFG_FILES_CMD   Rsp: MSCP_MS_REQ_CFG_FILES_RSP */
typedef struct{
  INT8          CP_SN[MSCP_MAX_STRING_SIZE];
  INT8          Type[MSCP_MAX_STRING_SIZE];
  INT8          FleetIdent[MSCP_MAX_STRING_SIZE];
  INT8          Number[MSCP_MAX_STRING_SIZE];
  INT8          TailNumber[MSCP_MAX_STRING_SIZE];    
  INT8          Style[MSCP_MAX_STRING_SIZE];
  INT8          Operator[MSCP_MAX_STRING_SIZE];
  INT8          Owner[MSCP_MAX_STRING_SIZE];  
  BOOLEAN       ValidateData;
}PACKED MSCP_CP_REQ_CFG_FILES_CMD;

typedef struct{
  INT8          CfgStatus[MSCP_MAX_STATUS_STR_SIZE];
  INT8          XMLStatus[MSCP_MAX_STATUS_STR_SIZE];
  INT32         CfgFileVer;
  BOOLEAN       XMLDataMatch;
}PACKED MSCP_MS_REQ_CFG_FILES_RSP;


//---Command ID: CMD_ID_REQ_START_RECFG
/*Cmd: MSCP_CP_REQ_START_RECFG_CMD Rsp: None*/
typedef enum {
  MSCP_START_RECFG_MANUAL,
  MSCP_START_RECFG_AUTO
}MSCP_REQ_START_RECFG_MODE;

typedef struct{
  INT8  CP_SN[MSCP_MAX_STRING_SIZE];  
  INT32 Mode;                         //Contains MSCP_REQ_START_RECFG_MODE
}MSCP_CP_REQ_START_RECFG_CMD;

//---Command ID: CMD_ID_REQ_STOP_RECFG
/*Cmd: no command data Rsp: no response data*/



//---Command ID: CMD_ID_GET_RECFG_STS
/*Cmd: none  Rsp: MSCP_MS_GET_RECFG_STS_RSP*/
typedef enum{
  MSCP_RECFG_STS_WORKING,
  MSCP_RECFG_STS_DONE,
  MSCP_RECFG_STS_ERROR
}MSCP_RECFG_STS;

typedef struct{
  INT8  StatusStr[MSCP_MAX_STRING_SIZE];   //Descriptive status string of the current 
                                           //reconfiguration activity
  INT8  ErrorStr[MSCP_MAX_ERROR_STR_SIZE]; //Error code string (ERR###) (If the Status 
                                           //field is error)
  INT32 Status;           //MSCP_RECFG_STS enum.  
}PACKED MSCP_MS_GET_RECFG_STS_RSP;



#ifdef CONTROL_PROCESSOR_GHS
 #pragma pack()
#endif

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined( MSCPINTERFACE_BODY )
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


#endif // MSCPINTERFACE_H
/*************************************************************************
 *  MODIFICATIONS
 *    $History: MSCPInterface.h $
 * 
 * *****************  Version 43  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 42  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 * 
 * *****************  Version 41  *****************
 * User: John Omalley Date: 8/05/10    Time: 11:40a
 * Updated in $/software/control processor/code/system
 * SCR 750 - Remove TX Method
 * 
 * *****************  Version 40  *****************
 * User: Jim Mood     Date: 7/30/10    Time: 8:57a
 * Updated in $/software/control processor/code/system
 * SCR 327 Fast transmission test functionality
 * 
 * *****************  Version 39  *****************
 * User: Contractor3  Date: 7/29/10    Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR #703 - Fixes for code review findings
 * 
 * *****************  Version 38  *****************
 * User: Jim Mood     Date: 7/23/10    Time: 7:40p
 * Updated in $/software/control processor/code/system
 * SCR 151 Delete FVT entry after a good ground server CRC
 * 
 * *****************  Version 37  *****************
 * User: Contractor3  Date: 7/06/10    Time: 10:38a
 * Updated in $/software/control processor/code/system
 * SCR #672 - Changes based on Code Review.
 * 
 * *****************  Version 36  *****************
 * User: Jim Mood     Date: 7/02/10    Time: 4:06p
 * Updated in $/software/control processor/code/system
 * SCR 671 Added "busy" status to the MS-CP response packet
 * 
 * *****************  Version 35  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:28p
 * Updated in $/software/control processor/code/system
 * SCR 623 Reconfiguration updates
 * 
 * *****************  Version 34  *****************
 * User: Jim Mood     Date: 6/16/10    Time: 4:24p
 * Updated in $/software/control processor/code/system
 * 
 * *****************  Version 33  *****************
 * User: Jim Mood     Date: 6/14/10    Time: 2:46p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration updates
 * 
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 5/26/10    Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR #607 The CP shall query Ms CF mounted before File Upload pr
 * 
 * *****************  Version 30  *****************
 * User: Jim Mood     Date: 5/25/10    Time: 5:29p
 * Updated in $/software/control processor/code/system
 * Added reconfiguration commands
 * 1) Request configuration files (126)
 * 2) Start reconfiguration (127)
 * 3) Get reconfiguration status (128)
 * 
 * *****************  Version 29  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 28  *****************
 * User: Jim Mood     Date: 2/23/10    Time: 3:43p
 * Updated in $/software/control processor/code/system
 * Added commands for putting a file from the CP to the Micro-Server
 * 
 * *****************  Version 27  *****************
 * User: Jim Mood     Date: 1/20/10    Time: 3:43p
 * Updated in $/software/control processor/code/system
 * Added definition for commands: Control JMessenger; Mark log file
 * transfered
 * 
 * *****************  Version 26  *****************
 * User: Jim Mood     Date: 11/20/09   Time: 3:28p
 * Updated in $/software/control processor/code/system
 * Updated FILE_DATA command
 * 
 * *****************  Version 25  *****************
 * User: Jim Mood     Date: 11/02/09   Time: 11:24a
 * Updated in $/software/control processor/code/system
 * Added file transfer commands
 * 
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 10/26/09   Time: 5:41p
 * Updated in $/software/control processor/code/system
 * Modifed recently added Shell cmd
 * 
 * *****************  Version 23  *****************
 * User: Jim Mood     Date: 10/26/09   Time: 2:48p
 * Updated in $/software/control processor/code/system
 * Added shell command 
 * 
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 8/28/09    Time: 10:02a
 * Updated in $/software/control processor/code/system
 * Added MNC and MCC fields to the GSM config structure.  Removed Netmask
 * and Gateway.
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 6/30/09    Time: 3:23p
 * Updated in $/software/control processor/code/system
 * SCR #204 Misc Code Review Updates
 * 
 * *****************  Version 20  *****************
 * User: Jim Mood     Date: 5/15/09    Time: 5:15p
 * Updated in $/software/control processor/code/system
 * Update STS definition
 * 
 * *****************  Version 19  *****************
 * User: Jim Mood     Date: 4/02/09    Time: 9:03a
 * Updated in $/control processor/code/system
 * Added USER_MGR_MS_CMD message definitions.
 * Added CHECK_NO_VALIDATE to the AC_UPDATE_METHOD structure
 * 
 * *****************  Version 18  *****************
 * User: Jim Mood     Date: 1/29/09    Time: 10:27a
 * Updated in $/control processor/code/system
 * Added a response structure for the CMD_ID_GET_CP_INFO command
 * 
 * *****************  Version 17  *****************
 * User: Jim Mood     Date: 1/22/09    Time: 1:18p
 * Updated in $/control processor/code/system
 * Added "UPDATE METHOD" enum for the aircraft config command
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 1/06/09    Time: 9:56a
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 12/17/08   Time: 11:01a
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 12/12/08   Time: 4:23p
 * Updated in $/control processor/code/system
 * Updated the Check File Status enumeration to match FAST ICD v5
 * 
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 12/11/08   Time: 11:11a
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 12/04/08   Time: 5:58p
 * Updated in $/control processor/code/system
 * Added data structures for the commands:
 * CMD_ID_VALIDATE_CRC
 * CMD_ID_CHECK_FILE
 * 
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 11/12/08   Time: 1:40p
 * Updated in $/control processor/code/system
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/28/08   Time: 11:54a
 * Updated in $/control processor/code/system
 * Added version, date, history comments.
 * 
 *
 ***************************************************************************/


