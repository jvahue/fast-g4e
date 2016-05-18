#ifndef F7X_PROTOCOL_H
#define F7X_PROTOCOL_H

/******************************************************************************
            Copyright (C) 2008-2016 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    ECCN:        9D991

    File:        F7XProtocol.h

    Description: Contains data structures related to the F7X Serial Protocol 
                 Handler 
    
    VERSION
      $Revision: 20 $  $Date: 5/17/16 9:06a $     

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/    


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"

#include "TaskManager.h"

//#include "FaultMgr.h"




/******************************************************************************
                                 Package Defines
******************************************************************************/
#define F7X_MAX_PARAMS        256
#define F7X_MAX_DUMPLIST_SIZE 128
#define F7X_MAX_DUMPLISTS     10

#define F7X_SYNC_WORD1 0x55
#define F7X_SYNC_WORD2 0xAA

#define F7X_PARAM_NO_TIMEOUT 0x0

#define F7X_IGNORE_ADDRCHKSUM 0xFFFFFFFF



#define F7X_PROTOCOL_DEFAULT_82_PARAM_55555555 \
        F7X_IGNORE_ADDRCHKSUM,  /* Addr Checksum */\
        82,          /* Num Param */\
        {    1,   2,   3,   4,   5,   6,   7,   8,   9,  10, \
            11,  12,  13,  14,  15,  16,  17,  18,  19,  20, \
            21,  22,  23,  24,  25,  26,  27,  28,  29,  30, \
            31,  32,  33,  34,  35,  36,  37,  38,  39,  40, \
            41,  42,  43,  44,  45,  46,  47,  48,  49,  50, \
            51,  52,  53,  54,  55,  56,  57,  58,  59,  60, \
            61,  62,  63,  64,  65,  66,  67,  68,  69,  70, \
            71,  72,  73,  74,  75,  76,  77,  78,  79,  80, \
            81,  82 }
            
#define F7X_PROTOCOL_DEFAULT_92_PARAM_55555555 \
        F7X_IGNORE_ADDRCHKSUM,  /* Addr Checksum */\
        92,          /* Num Param */\
        {    1,   2,   3,   4,   5,   6,   7,   8,   9,  10, \
            11,  12,  13,  14,  15,  16,  17,  18,  19,  20, \
            21,  22,  23,  24,  25,  26,  27,  28,  29,  30, \
            31,  32,  33,  34,  35,  36,  37,  38,  39,  40, \
            41,  42,  43,  44,  45,  46,  47,  48,  49,  50, \
            51,  52,  53,  54,  55,  56,  57,  58,  59,  60, \
            61,  62,  63,  64,  65,  66,  67,  68,  69,  70, \
            71,  72,  73,  74,  75,  76,  77,  78,  79,  80, \
            81,  82,  83,  84,  85,  86,  87,  88,  89,  90, \
            91,  92 }
            
                        
#define F7X_PROTOCOL_DEFAULT_NOT_USED \
        0x00000000, /* Addr Checksum */\
        32,         /* Num Param */\
        {    0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0, \
             0,   0,   0,   0,   0,   0,   0,   0 }
             
#define F7X_PROTOCOL_DEFAULT F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 1*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 2*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 3*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 4*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 5*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 6*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 7*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 8*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED, /*DumpList 9*/\
                             F7X_PROTOCOL_DEFAULT_NOT_USED  /*DumpList 10*/


#define F7X_PROTOCOL_PARAM_DEFAULT_TEST \
\
    1, /*Version*/\
   93, /*Number of params defined below */\
  {\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* Not Used    - 000*/\
\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* ENG_RUN_TMC - 001*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_ENG_SN1  - 002*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EN_ENG_SN2  - 003*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EN_ENG_SN3  - 004*/\
\
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    /* DATE_TIME_WD1  - 005*/\
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    /* DATE_TIME_WD2  - 006*/\
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    /* DATE_TIME_WD3  - 007*/\
\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_FLIGHTCNT   - 008*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_FLTTIM_COAR - 009*/\
  { 3600,              DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_FLTTIM_FINE - 010*/\
\
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    /* EE_ENG_RUN_TMC - 011*/\
  { 3600,              DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    /* EE_ENG_RUN_TMF - 012*/\
\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT01  - 013*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT02  - 014*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT03  - 015*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT04  - 016*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT05  - 017*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT06  - 018*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT07  - 019*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT08  - 020*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT09  - 021*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT10  - 022*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT11  - 023*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT12  - 024*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT13  - 025*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_LCFCOUNT14  - 026*/\
\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_STRTCNT     - 027*/\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EE_TRDPLYCNT   - 028*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* TAKEOFF_TREND  - 029*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* 35K_TREND      - 030*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* CRUISE_TREND   - 031*/\
\
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* CRUISETY       - 032*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS01          - 033*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS02          - 034*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS03          - 035*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS04          - 036*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS05          - 037*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS06          - 038*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DIS07          - 039*/\
\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC1LINX      - 040*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC2LINX      - 041*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC2HINX      - 042*/\
\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC3LO033INX  - 043*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC4LO030INX  - 044*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DISC3HI030INX  - 045*/\
\
  { 128, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },               /* ENGVIB         - 046*/\
\
  { 256, 0.1f, 500 },                                               /* BASFLOW        - 047*/\
  { 128, 0.1f, 500 },                                               /* IGVREF         - 048*/\
  { 64,  0.1f, 500 },                                               /* IGV_VANE       - 049*/\
\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DHSTAT         - 050*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* DHSTATX        - 051*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* ILOOP          - 052*/\
\
  { 512, 1.0f, 500 },                                               /* MOTIN          - 053*/\
  { 512, 1.0f, 500 },                                               /* MOTINX         - 054*/\
  { 512, 1.0f, 500 },                                               /* MOPIN          - 055*/\
  { 512, 1.0f, 500 },                                               /* MOPINX         - 056*/\
\
  { 256, 0.1f, 500 },                                               /* N1REF          - 057*/\
  { 256, 0.1f, 500 },                                               /* N2IN           - 058*/\
  { 256, 0.1f, 500 },                                               /* N2INX          - 059*/\
  { 256, 0.1f, 500 },                                               /* RWN1DIS        - 060*/\
  { 256, 0.1f, 500 },                                               /* N1IN           - 061*/\
  { 256, 0.1f, 500 },                                               /* N1INX          - 062*/\
\
  {  64, 0.2f, 500 },                                               /* PT             - 063*/\
  {  64, 0.2f, 500 },                                               /* PTIN           - 064*/\
  {  64, 0.2f, 500 },                                               /* PAMB           - 065*/\
\
  {2048, 1.0f, 500 },                                               /* TT0IN          - 066*/\
  {2048, 1.0f, 500 },                                               /* TT0            - 067*/\
\
  {1024, 0.5f, 500 },                                               /* P3IN           - 068*/\
  {1024, 0.5f, 500 },                                               /* P3INX          - 069*/\
\
  {2048, 1.0f, 500 },                                               /* T48IN          - 070*/\
  {2048, 1.0f, 500 },                                               /* T48INX         - 071*/\
  {2048, 1.0f, 500 },                                               /* T45            - 072*/\
\
  { 128, 0.1f, 500 },                                               /* TLAIN          - 073*/\
\
  {8192,10.0f, 500 },                                               /* ENGWF          - 074*/\
\
  { 128, 0.1f, 500 },                                               /* TLATRM         - 075*/\
\
  {8192,10.0f, 500 },                                               /* WFPPH          - 076*/\
  {8192,10.0f, 500 },                                               /* WFRPPH         - 077*/\
\
  { 128, 0.5f, 500 },                                               /* BOARD_TEMP     - 078*/\
\
  {2048, 1.0f, 500 },                                               /* T4STRM         - 079*/\
\
  { 256, 0.1f, 500 },                                               /* N1TINE         - 080*/\
\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_INTNL   - 081*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L2      - 082*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L3      - 083*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L4      - 084*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L5      - 085*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L6      - 086*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L7      - 087*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* FLTWRD_L8      - 088*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EVWRD1         - 089*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* EVWRD2         - 090*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* SELCODCL       - 091*/\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }  /* BLD_EXTRACT    - 092*/\
  }


#define F7X_PROTOCOL_PARAM_DEFAULT \
    0, /*Version*/\
    0, /*Number of params defined below */\
  {\
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, /* Not Used    - 000*/\
  }

#define F7X_DUMPLIST_NOT_RECOGNIZED_INDEX 255


#define F7X_GENERAL_DEFAULT \
        FALSE,           /* esn_enb */\
        0x00000000       /* esn_gpa */

#define F7X_GENERAL_DEFAULT_CFGS \
        F7X_GENERAL_DEFAULT,  /* CH 0 - Not Used */\
        F7X_GENERAL_DEFAULT,  /* CH 1  */\
        F7X_GENERAL_DEFAULT,  /* CH 2  */\
        F7X_GENERAL_DEFAULT   /* CH 3  */


#define DR_ONE_VALUE_TOL    100.0f
#define DR_EVERY_CHANGE_TOL 0.0f

#define DR_NO_RANGE_SCALE   0x0000   // Treat as DISC where MSB != sign bit
                                     //    When treating as BNR MSB == sign bit
#define DR_HALF_SCALE       0x8000   // 32768


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef struct 
{
  UINT16  scale; 
  FLOAT32 tol; 
  UINT32 timeout;    
} F7X_PARAM, *F7X_PARAM_PTR; 

typedef struct 
{
  UINT16 ver; 
  UINT16 cnt; 
  F7X_PARAM param[F7X_MAX_PARAMS]; 
} F7X_PARAM_LIST, *F7X_PARAM_LIST_PTR; 



typedef struct 
{
  UINT32 AddrChksum; // Dump List Address Checksum
  UINT16 NumParams;  // Dump list size, #param only, excluding sync word and addrchksum
  UINT16 ParamId[F7X_MAX_DUMPLIST_SIZE];    
                     // Dump List Parameter.  Id == index of param in Translation 
                     //     table.  Array element index == param word position 
                     //     in dump list.   
} F7X_DUMPLIST_CFG, *F7X_DUMPLIST_CFG_PTR; 


//A type for an array of the maximum number of uarts
//Used for storing f7x configurations in the configuration manager
typedef F7X_DUMPLIST_CFG F7X_DUMPLIST_CFGS[F7X_MAX_DUMPLISTS];

typedef struct {
  BOOLEAN esn_enb;     // ESN decode enable
  UINT32 esn_gpa;      // ESN General Purpose A Register
} F7X_OPTION_CFG, *F7X_OPTION_CFG_PTR; 

typedef struct {
  F7X_OPTION_CFG option;
} F7X_GENERAL_CFG, *F7X_GENERAL_CFG_PTR;

typedef F7X_GENERAL_CFG F7X_GENERAL_CFGS[UART_NUM_OF_UARTS];

#define F7X_ESN_CHAR_MAX 7  // 6 CHAR + NULL term. 
#define F7X_ESN_CHAR_RAW_MAX 3  // Three 16-bit words encode the ESN
//#define F7X_ESN_CHAR_2  2
//#define F7X_ESN_CHAR_1  1
//#define F7X_ESN_CHAR_0  0
#define F7X_ESN_GPA_REVERSE(x) ( (x & 0x01000000) >> 24 )
#define F7X_ESN_GPA_W2(x) ( (x & 0x00FF0000) >> 16 )
#define F7X_ESN_GPA_W1(x) ( (x & 0x0000FF00) >>  8 )
#define F7X_ESN_GPA_W0(x) ( (x & 0x000000FF) )
#define F7X_ESN_MSB(x) ( (x & 0xFF00) >> 8 )
#define F7X_ESN_LSB(x) ( (x & 0x00FF) )
#define F7X_ESN_SYNC_THRES  20  // The same ESN val must be decoded at least 20 times
                                //  before considered "real'. At 20 Hz, this normally is 
                                //  1 sec. 
#define F7X_ESN_GPA_W2_REMAP(x) ( x << 16 )
#define F7X_ESN_GPA_W1_REMAP(x) ( x << 8 )
#define F7X_ESN_GPA_W0_REMAP(x) ( x ) 
#define F7X_ESN_GPA_WORD_BITS   0x00FFFFFF   // GPA Word Bit Locations


typedef struct 
{
  BOOLEAN bDecoded;  // ESN has been decoded once from the bus for this curr pwr up
  CHAR esn[F7X_ESN_CHAR_MAX];  // ESN in format "XX0000" where "XX" - Eng Code Alph Char
  CHAR esn_sync[F7X_ESN_CHAR_MAX];  // ESN temp before threshold exceed to indicate new ESN
  UINT32 nCntSync;   // If esn_sync has not changed for threshold, indicate new ESN
  UINT32 nCntChanged;// Cnt of the # times ESN raw has changed.  
  UINT32 nCntNew;    // Cnt of the # times ESN has changed.  1 = first time sync. After thres. 
  UINT32 nCntBad;    // Cnt of the # times ESN has bad format decoded. 
} F7X_ESN_STATUS, *F7X_ESN_STATUS_PTR; 



typedef struct 
{
  BOOLEAN sync;      // Run time sync flag.  Current state.
  BOOLEAN syncInit;  // Initial sync occurred - Internal flag only 
  UINT32  AddrChksum; 
  UINT16  DumpListSize; 
  BOOLEAN bRecognized; 
  UINT16  nIndexCfg; 
  UINT32  nFrames; 
  UINT32  nResyncs;
  UINT32  lastFrameTime; 
  TIMESTAMP lastFrameTS;  // Rx time in TS format for snap shot recording
  BOOLEAN bResync;        // Currrently hunting for sync after lost of sync ! 
  BOOLEAN bInitUartMgrDataDone; 
  F7X_OPTION_CFG optionRunTime; // Run Time Option Data - ESN only today
} F7X_STATUS, *F7X_STATUS_PTR; 

typedef struct
{
  BOOLEAN bDebug;   // Enable debug frame display 
  UINT16 Ch;        // Channel to debug - default is firt UART (not GSE) 
  BOOLEAN bFormatted; 
  UINT32 Param0;    // Bit field to determine which param (32 to 1) to display
  UINT32 Param1;    // Bit field to determine which param (64 to 33) to display
  UINT32 Param2;    // Bit field to determine which param (96 to 65) to display
  UINT32 Param3;    // Bit field to determine which param (128 to 97) to display
  
  BOOLEAN bNewFrame;  // For Debug display purposes   
} F7X_DEBUG, *F7X_DEBUG_PTR; 


typedef struct
{
    TASK_INDEX      MgrTaskID;
} F7X_DISP_DEBUG_TASK_PARMS;



/**********************************
  F7X Protocol File Header
**********************************/
#pragma pack(1)
typedef struct 
{
  UINT16  id; 
  UINT16  scale; 
  FLOAT32 tol; 
  UINT32  timeout; 
} F7X_DL_FILE_HDR, *F7X_DL_FILE_HDR_PTR; 

typedef struct 
{
  UINT16 size;     // Protocol Hdr, including "size" field. 
  UINT16 ParamTranslationVer;   
  UINT32 dl_AddrChksum; 
  UINT16 dl_NumParams; 
  F7X_DL_FILE_HDR dl[F7X_MAX_DUMPLIST_SIZE]; 
} F7X_FILE_HDR, *F7X_FILE_HDR_PTR; 
#pragma pack()


/**********************************
  F7X App Non-Volatile Data
**********************************/
typedef struct 
{
  UINT16 lastRecognizedDL;   // Index in prev recognized dump list 
} F7X_APP_DATA, *F7X_APP_DATA_PTR; 


/*********************************
  F7X App Logs
*********************************/
#pragma pack(1)
typedef struct 
{
  UINT16 index;
  UINT16 numParams;
  UINT32 chksum;
} F7X_DL_INFO; 

typedef struct 
{
  RESULT result;
  UINT8  ch;            // UART Channel Index
  F7X_DL_INFO current; 
  F7X_DL_INFO prev; 
} F7X_NEW_DL_FOUND_LOG; 
#pragma pack()

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( F7X_PROTOCOL_BODY )
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
EXPORT BOOLEAN  F7XProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch, 
                                      void *runtime_data_ptr, void *info_ptr );
EXPORT void F7XProtocol_Initialize ( void ); 
                                      
EXPORT F7X_STATUS_PTR F7XProtocol_GetStatus (UINT8 index); 
EXPORT F7X_DEBUG_PTR F7XProtocol_GetDebug (void); 

EXPORT void F7XDispDebug_Task ( void *pParam ); 

EXPORT BOOLEAN F7XProtocol_SensorSetup ( UINT32 gpA, UINT32 gpB, UINT8 param, 
                                         void *info_ptr ); 

EXPORT UINT16 F7XProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch );

EXPORT BOOLEAN F7XProtocol_FileInit(void);

EXPORT void F7XProtocol_DisableLiveStream(void);

EXPORT BOOLEAN F7XProtocol_GetESN ( UINT16 ch, CHAR *esn_ptr, UINT16 cnt );

#endif // F7X_PROTOCOL_H               


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: F7XProtocol.h $
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 5/17/16    Time: 9:06a
 * Updated in $/software/control processor/code/system
 * SCR #1317 Item M-1. ESN decode to be independent of raw dumplist data
 * frame word location.   
 * 
 * *****************  Version 19  *****************
 * User: John Omalley Date: 3/02/16    Time: 8:26a
 * Updated in $/software/control processor/code/system
 * SCR 1314 - Code Review Update
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/30/16    Time: 7:37p
 * Updated in $/software/control processor/code/system
 * SCR #1314 N-Param ESN Decode, Byte Reversing Option
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 11/30/15   Time: 6:56p
 * Updated in $/software/control processor/code/system
 * SCR #1304 APAC processing. Update to ESN processing. Ref AI-6 of APAC
 * Mgr PRD.
 * 
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 15-10-19   Time: 9:54p
 * Updated in $/software/control processor/code/system
 * SCR #1304 ESN update to support APAC Processing
 * 
 * *****************  Version 15  *****************
 * User: John Omalley Date: 4/14/15    Time: 2:34p
 * Updated in $/software/control processor/code/system
 * SCR 1289 - Removed Dead Code
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 2/05/15    Time: 9:50a
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Code Review Update
 * 
 * *****************  Version 13  *****************
 * User: John Omalley Date: 11/11/14   Time: 4:00p
 * Updated in $/software/control processor/code/system
 * SCR 1267 - Made the default num_params 32 to match the GSE limits that
 * can be programmed.
 * 
 * *****************  Version 12  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 11  *****************
 * User: Contractor2  Date: 5/23/11    Time: 1:56p
 * Updated in $/software/control processor/code/system
 * SCR #1033 Enhancement - F7X New DumpList Found Log
 * 
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/29/10   Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 936
 * 
 * *****************  Version 9  *****************
 * User: John Omalley Date: 10/15/10   Time: 9:32a
 * Updated in $/software/control processor/code/system
 * SCR 938 - Code Review Updates
 * 
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 7/27/10    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR #743 Timeout processing when cfg set to '0'. 
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 7/22/10    Time: 5:37p
 * Updated in $/software/control processor/code/system
 * SCR #700 Default values to "0" for F7X Param table. 
 * 
 * *****************  Version 6  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 6/21/10    Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR #635 items 17 and 18.  Update debug display to display new frames
 * only.  Make all f7x_dumplist.cfg[] and f7x_param.cfg[] entries blank.  
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #635 Create log to indicate "unrecognized" dump list detected.
 * 
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:19p
 * Updated in $/software/control processor/code/system
 * SCR #365 Add better debug display of F7X data. 
 * 
 * *****************  Version 2  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:21p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements. 
 * 
 * 
 *
 *****************************************************************************/

