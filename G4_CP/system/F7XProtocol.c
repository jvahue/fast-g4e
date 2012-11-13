#define F7X_PROTOCOL_BODY

/******************************************************************************
            Copyright (C) 2008-2011 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        F7XProtocol.c

    Description: Contains all functions and data related to the F7X Protocol 
                 Handler 
    
    VERSION
      $Revision: 20 $  $Date: 12-11-13 1:37p $     

******************************************************************************/



/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/



/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "F7XProtocol.h"
#include "UartMgr.h"
#include "ClockMgr.h"
#include "GSE.h"
#include "NVMgr.h"



/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define F7X_MAX_RAW_BUF      (4 * F7X_MAX_DUMPLIST_SIZE * 2)
                             // 128 words * 4 frames = 1024 bytes


/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
typedef struct 
{
  UINT8 data[F7X_MAX_RAW_BUF];
  UINT8 *pWr; 
  UINT8 *pRd; 
  UINT16 cnt; 
  UINT8 *pSync;
  UINT8 *pStart;  // These are here just for debug 
  UINT8 *pEnd;    // These are here just for debug 
} F7X_RAW_BUFFER, *F7X_RAW_BUFFER_PTR; 

typedef struct 
{
   BOOLEAN bNewFrame; 
   UINT8   sync_byte;
   UINT32  addrChksum;
   UINT16 data[F7X_MAX_DUMPLIST_SIZE]; 
} F7X_DATA_FRAME, *F7X_DATA_FRAME_PTR; 

typedef struct 
{ 
  UINT8  *ptr;
  UINT8  word;
  UINT32 addrChksum; 
} F7X_SYNC_DATA, F7X_SYNC_DATA_PTR; 

#define F7X_INIT_SYNCWORDS 3

/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static F7X_STATUS m_F7X_Status[UART_NUM_OF_UARTS]; 
static F7X_RAW_BUFFER m_F7X_Buff[UART_NUM_OF_UARTS];

static F7X_DUMPLIST_CFG m_F7X_DumplistCfg[F7X_MAX_DUMPLISTS];   // Common to all ch
// static F7X_PARAM m_F7X_ParamList[F7X_MAX_PARAMS]; 

static F7X_DATA_FRAME m_F7X_DataFrame[UART_NUM_OF_UARTS]; 

static F7X_PARAM_LIST m_F7X_ParamList;
static F7X_PARAM_LIST m_F7X_ParamListCfg; 

static F7X_DEBUG m_F7X_Debug; 
static UINT8 Str[(F7X_MAX_DUMPLIST_SIZE * 8) + 64];

static F7X_DISP_DEBUG_TASK_PARMS F7XDispDebugBlock; 

static F7X_APP_DATA m_F7X_AppData[UART_NUM_OF_UARTS]; 

static char GSE_OutLine[128];


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static BOOLEAN F7XProtocol_Decoder_Synced (UINT16 ch); 
static void F7XProtocol_Decoder_NotSynced (UINT16 ch); 
static BOOLEAN F7XProtocol_DumpListRecognized (UINT16 ch); 
static void F7XProtocol_Resync (UINT16 ch); 
static void F7XProtocol_UpdateFrame (UINT16 ch, UINT8 *pStart, UINT8 *pEnd); 
static void F7XProtocol_InitUartMgrData( UINT16 ch, UARTMGR_PARAM_DATA_PTR data_ptr, 
                                         UARTMGR_WORD_INFO_PTR word_info_ptr); 
static void F7XProtocol_UpdateUartMgrData( UINT16 ch, UARTMGR_PARAM_DATA_PTR data_ptr, 
                                           UARTMGR_WORD_INFO_PTR word_info_ptr); 
static void F7XProtocol_RestoreAppData( void ); 
static F7X_PARAM_PTR F7XProtocol_GetParamEntryList (UINT16 entry);

#include "F7XUserTables.c"

/*****************************************************************************/
/* Constant Data                                                             */
/*****************************************************************************/

typedef struct 
{
  char name[16]; 
} F7X_PARAM_NAMES; 

static const F7X_PARAM_NAMES F7X_Param_Names[F7X_MAX_PARAMS] =
{
     "NOT USED      ",    // 00
     "ENG_RUN_TMC   ",
     "EE_ENG_SN1    ",
     "EE_ENG_SN2    ",
     "EE_ENG_SN3    ",
     "DATE_TIME_WD1 ",
     "DATE_TIME_WD2 ",
     "DATE_TIME_WD3 ",
     "EE_FLIGHTCNT  ",
     "EE_FLTTIM_COAR",
     "EE_FLTTIM_FINE",    // 10
     "EE_ENG_RUN_TMC",
     "EE_ENG_RUN_TMF",
     "EE_LCFCOUNT01 ",
     "EE_LCFCOUNT02 ",
     "EE_LCFCOUNT03 ",
     "EE_LCFCOUNT04 ",
     "EE_LCFCOUNT05 ",
     "EE_LCFCOUNT06 ",
     "EE_LCFCOUNT07 ",
     "EE_LCFCOUNT08 ",    // 20
     "EE_LCFCOUNT09 ",
     "EE_LCFCOUNT10 ",
     "EE_LCFCOUNT11 ",
     "EE_LCFCOUNT12 ",
     "EE_LCFCOUNT13 ",
     "EE_LCFCOUNT14 ",
     "EE_STRTCNT    ",
     "EE_TRDPLYCNT  ",
     "TAKEOFF_TREND ",
     "35K_TREND     ",    // 30
     "CRUISE_TREND  ",
     "CRUISETY      ",
     "DIS01         ",
     "DIS02         ",
     "DIS03         ",
     "DIS04         ",
     "DIS05         ",
     "DIS06         ",
     "DIS07         ",
     "DISC1LINX     ",    // 40
     "DISC2LINX     ",
     "DISC2HINX     ",
     "DISC3LO033INX ",
     "DISC4LO030INX ",
     "DISC4HI030INX ",
     "ENGVIB        ",
     "BASFLOW       ",
     "IGVREF        ",
     "IGV_VANE      ",
     "DHSTAT        ",    // 50
     "DHSTATX       ",
     "ILOOP         ",
     "MOTIN         ",
     "MOTINX        ",
     "MOPIN         ",
     "MOPINX        ",
     "N1REF         ",
     "N2IN          ",
     "N2INX         ",
     "RWN1DIS       ",    // 60
     "N1IN          ",
     "N1INX         ",
     "PT            ",
     "PTIN          ",
     "PAMB          ",
     "TT0IN         ",
     "TT0           ",
     "P3IN          ",
     "P3INX         ",
     "T48IN         ",    // 70
     "T48INX        ",
     "T45           ",
     "TLAIN         ",
     "ENGWF         ",
     "TLATRM        ",
     "WFPPH         ",
     "WFRPPH        ",
     "BOARD_TEMP    ",
     "T45TRM        ",
     "N1TINE        ",    // 80
     "FLTWRD_INTNL  ",
     "FLTWRD_L2     ",
     "FLTWRD_L3     ",
     "FLTWRD_L4     ",
     "FLTWRD_L5     ",
     "FLTWRD_L6     ",
     "FLTWRD_L7     ",
     "FLTWRD_L8     ",
     "EVWRD1        ",
     "EVWRD2        ",    // 90
     "SELCODCL      ",
     "BLD_EXTRACT   "
};


// Temp structure for now ..  
/*
static const F7X_PARAM_LIST F7X_ParamList_Temp = 
{
    1, //Version
   93, //Number of params defined below 
 {  
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // Not Used    - 000

  { 64, 0.1f, F7X_PARAM_NO_TIMEOUT },                // ENG_RUN_TMC - 001
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // EE_ENG_SN1  - 002
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // EN_ENG_SN2  - 003
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // EN_ENG_SN3  - 004

  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DATE_TIME_WD1  - 005
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DATE_TIME_WD2  - 006
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DATE_TIME_WD3  - 007

  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_FLIGHTCNT   - 008
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_FLTTIM_COAR - 009
  { 3600, 0.1f, F7X_PARAM_NO_TIMEOUT },              // EE_FLTTIM_FINE - 010

  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // EE_ENG_RUN_TMC - 011
  { 3600, 0.1f, F7X_PARAM_NO_TIMEOUT },              // EE_ENG_RUN_TMF - 012

  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT01  - 013
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT02  - 014
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT03  - 015
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT04  - 016
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT05  - 017
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT06  - 018
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT07  - 019
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT08  - 020
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT09  - 021
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT10  - 022
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT11  - 023
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT12  - 024
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT13  - 025
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_LCFCOUNT14  - 026

  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_STRTCNT     - 027
  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // EE_TRDPLYCNT   - 028
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // TAKEOFF_TREND  - 029
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // 35K_TREND      - 030
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // CRUISE_TREND   - 031

  { DR_HALF_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },     // CRUISETY       - 032
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS01          - 033
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS02          - 034
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS03          - 035
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS04          - 036
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS05          - 037
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS06          - 038
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DIS07          - 039

  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC1LINX      - 040
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC2LINX      - 041
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC2HINX      - 042

  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC3LO033INX  - 043
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC4LO030INX  - 044
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT }, // DISC3HI030INX  - 045
  
  
  { 128, 0.1f, F7X_PARAM_NO_TIMEOUT },              // ENGVIB         - 046
  
  { 256, 0.1f, 500 },                               // BASFLOW        - 047
  { 128, 0.1f, 500 },                               // IGVREF         - 048
  { 64,  0.1f, 500 },                               // IGV_VANE       - 049
  
  
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// DHSTAT         - 050
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// DHSTATX        - 051
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// ILOOP          - 052
  
  { 512, 1.0f, 500 },                               // MOTIN          - 053
  { 512, 1.0f, 500 },                               // MOTINX         - 054
  { 512, 1.0f, 500 },                               // MOPIN          - 055
  { 512, 1.0f, 500 },                               // MOPINX         - 056
  
  { 256, 0.1f, 500 },                               // N1REF          - 057
  { 256, 0.1f, 500 },                               // N2IN           - 058
  { 256, 0.1f, 500 },                               // N2INX          - 059
  { 256, 0.1f, 500 },                               // RWN1DIS        - 060
  { 256, 0.1f, 500 },                               // N1IN           - 061
  { 256, 0.1f, 500 },                               // N1INX          - 062
  
  {  64, 0.2f, 500 },                               // PT             - 063
  {  64, 0.2f, 500 },                               // PTIN           - 064
  {  64, 0.2f, 500 },                               // PAMB           - 065
  
  {2048, 1.0f, 500 },                               // TT0IN          - 066
  {2048, 1.0f, 500 },                               // TT0            - 067
  
  {1024, 0.5f, 500 },                               // P3IN           - 068
  {1024, 0.5f, 500 },                               // P3INX          - 069
  
  {2048, 1.0f, 500 },                               // T48IN          - 070
  {2048, 1.0f, 500 },                               // T48INX         - 071
  {2048, 1.0f, 500 },                               // T45            - 072
  
  { 128, 0.1f, 500 },                               // TLAIN          - 073
  
  {8192,10.0f, 500 },                               // ENGWF          - 074
  
  { 128, 0.1f, 500 },                               // TLATRM         - 075
  
  {8192,10.0f, 500 },                               // WFPPH          - 076
  {8192,10.0f, 500 },                               // WFRPPH         - 077
  
  { 128, 0.5f, 500 },                               // BOARD_TEMP     - 078
  
  {2048, 1.0f, 500 },                               // T4STRM         - 079
  
  { 256, 0.1f, 500 },                               // N1TINE         - 080
  
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_INTNL   - 081
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L2      - 082
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L3      - 083
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L4      - 084
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L5      - 085
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L6      - 086
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L7      - 087
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// FLTWRD_L8      - 088
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// EVWRD1         - 089
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// EVWRD2         - 090
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT },// SELCODCL       - 091
  { DR_NO_RANGE_SCALE, 0.1f, F7X_PARAM_NO_TIMEOUT } // BLD_EXTRACT    - 092
 } 
};
*/
/*
static const F7X_PARAM_LIST F7X_ParamList_Temp = 
{
    1, //Version
   93, //Number of params defined below 
 {  
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // Not Used    - 000

  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // ENG_RUN_TMC - 001
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_ENG_SN1  - 002
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EN_ENG_SN2  - 003
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EN_ENG_SN3  - 004

  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    // DATE_TIME_WD1  - 005
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    // DATE_TIME_WD2  - 006
  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    // DATE_TIME_WD3  - 007

  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_FLIGHTCNT   - 008
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_FLTTIM_COAR - 009
  { 3600,              DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_FLTTIM_FINE - 010

  { DR_NO_RANGE_SCALE, DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    // EE_ENG_RUN_TMC - 011
  { 3600,              DR_ONE_VALUE_TOL, F7X_PARAM_NO_TIMEOUT },    // EE_ENG_RUN_TMF - 012

  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT01  - 013
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT02  - 014
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT03  - 015
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT04  - 016
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT05  - 017
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT06  - 018
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT07  - 019
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT08  - 020
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT09  - 021
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT10  - 022
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT11  - 023
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT12  - 024
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT13  - 025
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_LCFCOUNT14  - 026

  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_STRTCNT     - 027
  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // EE_TRDPLYCNT   - 028
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // TAKEOFF_TREND  - 029
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // 35K_TREND      - 030
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // CRUISE_TREND   - 031

  { DR_HALF_SCALE,     DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // CRUISETY       - 032
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS01          - 033
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS02          - 034
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS03          - 035
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS04          - 036
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS05          - 037
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS06          - 038
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DIS07          - 039

  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC1LINX      - 040
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC2LINX      - 041
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC2HINX      - 042

  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC3LO033INX  - 043
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC4LO030INX  - 044
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // DISC3HI030INX  - 045
  
  
  { 128, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }, // ENGVIB         - 046
  
  { 256, 0.1f, 500 },                               // BASFLOW        - 047
  { 128, 0.1f, 500 },                               // IGVREF         - 048
  { 64,  0.1f, 500 },                               // IGV_VANE       - 049
  
  
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // DHSTAT         - 050
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // DHSTATX        - 051
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // ILOOP          - 052
  
  { 512, 1.0f, 500 },                               // MOTIN          - 053
  { 512, 1.0f, 500 },                               // MOTINX         - 054
  { 512, 1.0f, 500 },                               // MOPIN          - 055
  { 512, 1.0f, 500 },                               // MOPINX         - 056
  
  { 256, 0.1f, 500 },                               // N1REF          - 057
  { 256, 0.1f, 500 },                               // N2IN           - 058
  { 256, 0.1f, 500 },                               // N2INX          - 059
  { 256, 0.1f, 500 },                               // RWN1DIS        - 060
  { 256, 0.1f, 500 },                               // N1IN           - 061
  { 256, 0.1f, 500 },                               // N1INX          - 062
  
  {  64, 0.2f, 500 },                               // PT             - 063
  {  64, 0.2f, 500 },                               // PTIN           - 064
  {  64, 0.2f, 500 },                               // PAMB           - 065
  
  {2048, 1.0f, 500 },                               // TT0IN          - 066
  {2048, 1.0f, 500 },                               // TT0            - 067
  
  {1024, 0.5f, 500 },                               // P3IN           - 068
  {1024, 0.5f, 500 },                               // P3INX          - 069
  
  {2048, 1.0f, 500 },                               // T48IN          - 070
  {2048, 1.0f, 500 },                               // T48INX         - 071
  {2048, 1.0f, 500 },                               // T45            - 072
  
  { 128, 0.1f, 500 },                               // TLAIN          - 073
  
  {8192,10.0f, 500 },                               // ENGWF          - 074
  
  { 128, 0.1f, 500 },                               // TLATRM         - 075
  
  {8192,10.0f, 500 },                               // WFPPH          - 076
  {8192,10.0f, 500 },                               // WFRPPH         - 077
  
  { 128, 0.5f, 500 },                               // BOARD_TEMP     - 078
  
  {2048, 1.0f, 500 },                               // T4STRM         - 079
  
  { 256, 0.1f, 500 },                               // N1TINE         - 080
  
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_INTNL   - 081
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L2      - 082
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L3      - 083
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L4      - 084
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L5      - 085
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L6      - 086
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L7      - 087
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // FLTWRD_L8      - 088
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // EVWRD1         - 089
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // EVWRD2         - 090
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT },   // SELCODCL       - 091
  { DR_NO_RANGE_SCALE, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT }    // BLD_EXTRACT    - 092
 } 
};
*/

#define F7X_PARAM_ENTRIES 92

static const F7X_PARAM F7X_ParamList_DefaultEntry = 
{
   1, DR_EVERY_CHANGE_TOL, F7X_PARAM_NO_TIMEOUT       // Record on every change 
};


static const F7X_DUMPLIST_CFG M_UNDEFINED_DUMPLIST = 
{
  F7X_PROTOCOL_DEFAULT_NOT_USED
}; 


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/

/******************************************************************************
 * Function:    F7XProtocol_Initialize
 *
 * Description: Initializes the F7X Protocol functionality 
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:       None
 *
 *****************************************************************************/
void F7XProtocol_Initialize ( void ) 
{
  // F7X_RAW_BUFFER_PTR buff_ptr; 
  UINT16 i; 
  F7X_PARAM_PTR pParamEntry; 
  F7X_STATUS_PTR pStatus; 
  F7X_RAW_BUFFER_PTR buff_ptr; 
  
  // Local Data
  TCB TaskInfo;


  // Initialize local variables / structures
  memset ( m_F7X_Status, 0, sizeof(m_F7X_Status) ); 
  memset ( m_F7X_Buff, 0, sizeof(m_F7X_Buff) ); 
  memset ( m_F7X_DumplistCfg, 0, sizeof(m_F7X_DumplistCfg) ); 
  memset ( m_F7X_DataFrame, 0, sizeof(m_F7X_DataFrame) ); 
  memset ( &m_F7X_ParamList, 0, sizeof(m_F7X_ParamList) ); 
  
  // Initialize the Write and Read index into Raw Buffer 
  for (i=0;i<UART_NUM_OF_UARTS;i++) 
  {
    F7XProtocol_Resync(i); 
  }
  
  // Update m_F7X_Buff entries - Debug
  buff_ptr = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[0]; 
  for (i=0;i<UART_NUM_OF_UARTS;i++) 
  {
    buff_ptr->pStart = (UINT8 *) &buff_ptr->data[0]; 
    buff_ptr->pEnd = (UINT8 *) &buff_ptr->data[F7X_MAX_RAW_BUF]; 
    buff_ptr++; 
  }
  
  // Add an entry in the user message handler table for F7X Cfg Items 
  User_AddRootCmd(&F7XProtocolRootTblPtr); 
  User_AddRootCmd(&F7XProtocolDumpListTblPtr); 
  User_AddRootCmd(&F7XProtocolParamTblPtr); 
  
  // Restore User Cfg 
  memcpy(m_F7X_DumplistCfg, CfgMgr_RuntimeConfigPtr()->F7XConfig, sizeof(m_F7X_DumplistCfg));
  memcpy(&m_F7X_ParamListCfg, &(CfgMgr_RuntimeConfigPtr()->F7XParamConfig), sizeof(m_F7X_ParamListCfg)); 
  
  // Update runtime var of Param Translation Table 
  // Clear all entries to default 
  pParamEntry = (F7X_PARAM_PTR) &m_F7X_ParamList.param[0]; 
  for (i=0;i<F7X_MAX_PARAMS;i++) 
  {
    *pParamEntry = F7X_ParamList_DefaultEntry; 
    pParamEntry++; 
  }  
  // Update entries that are used 
  m_F7X_ParamList.cnt = m_F7X_ParamListCfg.cnt; 
  pParamEntry = (F7X_PARAM_PTR) &m_F7X_ParamList.param[0]; 
  for (i=0;i<m_F7X_ParamList.cnt;i++)
  {
    *pParamEntry = m_F7X_ParamListCfg.param[i]; 
    pParamEntry++; 
  }

  
  // Restore App Data
  F7XProtocol_RestoreAppData(); 
  
  // Update startup debug 
  m_F7X_Debug.bDebug = FALSE; 
  m_F7X_Debug.Ch = 1; 
  m_F7X_Debug.bFormatted = FALSE; 
  m_F7X_Debug.Param0 = 0xFFFFFFFF;  // Display all 
  m_F7X_Debug.Param1 = 0xFFFFFFFF;  // Display all 
  m_F7X_Debug.Param2 = 0xFFFFFFFF;  // Display all 
  m_F7X_Debug.Param3 = 0xFFFFFFFF;  // Display all 
  m_F7X_Debug.bNewFrame = FALSE; 
  
  // Update Status 
  pStatus = (F7X_STATUS_PTR) &m_F7X_Status[0]; 
  for (i=0;i<UART_NUM_OF_UARTS;i++)
  {
    pStatus->nIndexCfg = F7X_DUMPLIST_NOT_RECOGNIZED_INDEX; 
    pStatus++; 
  }
  
  // Create F7X Protocol Display Task
  memset(&TaskInfo, 0, sizeof(TaskInfo));
  strncpy_safe(TaskInfo.Name, sizeof(TaskInfo.Name), "F7X Display Debug",_TRUNCATE);
  TaskInfo.TaskID         = F7X_Disp_Debug; 
  TaskInfo.Function       = F7XDispDebug_Task;
  TaskInfo.Priority       = taskInfo[F7X_Disp_Debug].priority;
  TaskInfo.Type           = taskInfo[F7X_Disp_Debug].taskType;
  TaskInfo.modes          = taskInfo[F7X_Disp_Debug].modes;
  TaskInfo.MIFrames       = taskInfo[F7X_Disp_Debug].MIFframes;
  TaskInfo.Rmt.InitialMif = taskInfo[F7X_Disp_Debug].InitialMif;
  TaskInfo.Rmt.MifRate    = taskInfo[F7X_Disp_Debug].MIFrate;
  TaskInfo.Enabled        = TRUE;
  TaskInfo.Locked         = FALSE;
  TaskInfo.pParamBlock    = &F7XDispDebugBlock;
  TmTaskCreate (&TaskInfo);
  
}



/******************************************************************************
 * Function:    F7XProtocol_Handler
 *
 * Description: Processes raw serial data  
 *
 * Parameters:  [in] data: Pointer to a location containing a block of data
 *                         read from the UART
 *              [in] cnt:  Size of "data" in bytes
 *              [in] ch:   Channel (UART 1,2,or 3) the data was read from
 *              [in] runtime_data_ptr: Location that has scaling data for 
 *                                     the parameters stored in the 
 *                                     buffer
 *              [in] info_ptr:         Location that has dumplist definition
 *                                     data
 *
 * Returns:     TRUE - Protocol Sync has been obtained 
 *              
 * Notes:       None
 *
 *****************************************************************************/
BOOLEAN  F7XProtocol_Handler ( UINT8 *data, UINT16 cnt, UINT16 ch, 
                               void* runtime_data_ptr, void* info_ptr ) 
{
  UARTMGR_PARAM_DATA_PTR   data_ptr; 
  UARTMGR_WORD_INFO_PTR    word_info_ptr; 
  
  F7X_STATUS_PTR           status_ptr; 
  F7X_RAW_BUFFER_PTR       buff_ptr;
  
  UINT8 *dest_ptr; 
  UINT8 *end_ptr; 
  
  BOOLEAN bNewData; 
  
  UINT16 i; 
  
 
  bNewData = FALSE; 

  data_ptr = (UARTMGR_PARAM_DATA_PTR) runtime_data_ptr; 
  word_info_ptr = (UARTMGR_WORD_INFO_PTR) info_ptr; 
  
  status_ptr = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
  buff_ptr = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[ch]; 
  
  dest_ptr = buff_ptr->pWr; 
  end_ptr = (UINT8 *) &buff_ptr->data[F7X_MAX_RAW_BUF]; 
  
  if ( (cnt > 0) || (buff_ptr->cnt > 0) )
  {
    // Copy any data into buffer 
    for (i=0;i<cnt;i++) 
    { 
      *dest_ptr = *data; 
      dest_ptr++; 
      data++; 
      // Buffer roll over 
      if ( dest_ptr >= end_ptr ) 
      {
        dest_ptr = (UINT8 *) buff_ptr->data; 
      }
    }
    
    // Update index for next write 
    buff_ptr->pWr = dest_ptr; 
    
    // Update buff size 
    buff_ptr->cnt += cnt; 
    
    if (buff_ptr->cnt >= F7X_MAX_RAW_BUF) 
    {
      // Purge old data and force to max buffer size 
      buff_ptr->cnt = F7X_MAX_RAW_BUF; 
      buff_ptr->pRd = buff_ptr->pWr; 
    }
   
    if ( status_ptr->syncInit == TRUE ) 
    {
        bNewData = F7XProtocol_Decoder_Synced(ch); 
    }
    else 
    {
      // Wait till buffer is full before processing 
      if ( buff_ptr->cnt >= F7X_MAX_RAW_BUF )
      {
        F7XProtocol_Decoder_NotSynced(ch); 
      }  
    }
    
    // If new frame update Uart Mgr Data Structure 
    if ( ( bNewData == TRUE ) && (status_ptr->bRecognized == TRUE) )
    {
      // If initial update then, initialize the Uart Mgr Data Structure first 
      if ( status_ptr->bInitUartMgrDataDone == FALSE ) 
      {
         F7XProtocol_InitUartMgrData( ch, data_ptr, word_info_ptr); 
         status_ptr->bInitUartMgrDataDone = TRUE; 
      }
      
      // Update Uart Mgr Data 
      F7XProtocol_UpdateUartMgrData( ch, data_ptr, word_info_ptr); 
      
    }
    
    
    // If no new frame update individual data loss calculation and update Uart Mgr 
    //    Data struct for validity 
    
    // Update display every 
    
  }
  
  return (bNewData); 
  
}



/******************************************************************************
 * Function:    F7XProtocol_GetStatus
 *
 * Description: Utility function to request current F7XProtocol[index] status
 *
 * Parameters:  index - Uart Port Index 
 *
 * Returns:     Ptr to F7X Status data 
 *
 * Notes:       None 
 *
 *****************************************************************************/
F7X_STATUS_PTR F7XProtocol_GetStatus (UINT8 index)
{
  return ( (F7X_STATUS_PTR) &m_F7X_Status[index] ); 
}


/******************************************************************************
 * Function:    F7XProtocol_GetCfg
 *
 * Description: Utility function to request current F7XProtocol[index] Cfg
 *
 * Parameters:  index - Uart Port Index 
 *
 * Returns:     Ptr to F7X Cfg data
 *
 * Notes:       None 
 *
 *****************************************************************************/
F7X_DUMPLIST_CFG_PTR F7XProtocol_GetCfg (UINT8 index)
{
  return ( (F7X_DUMPLIST_CFG_PTR) &m_F7X_DumplistCfg[index] ); 
}


/******************************************************************************
 * Function:    F7XProtocol_GetDebug
 *
 * Description: Utility function to request current F7XProtocol debug state 
 *
 * Parameters:  None
 *
 * Returns:     Ptr to F7X Debug Data
 *
 * Notes:       None 
 *
 *****************************************************************************/
F7X_DEBUG_PTR F7XProtocol_GetDebug (void)
{
  return ( (F7X_DEBUG_PTR) &m_F7X_Debug ); 
}


/******************************************************************************
 * Function:    F7XProtocol_GetParamList
 *
 * Description: Utility function to request current F7XProtocol[index] Cfg
 *
 * Parameters:  None 
 *
 * Returns:     Ptr to F7X Cfg data
 *
 * Notes:       None 
 *
 *****************************************************************************/
F7X_PARAM_LIST_PTR F7XProtocol_GetParamList (void)
{
  return ( (F7X_PARAM_LIST_PTR) &m_F7X_ParamListCfg ); 
}


/******************************************************************************
 * Function:    F7XProtocol_GetParamEntryList
 *
 * Description: Utility function to request current F7XProtocol[index] Cfg
 *
 * Parameters:  entry - config parameter index 
 *
 * Returns:     Ptr to F7X Cfg data
 *
 * Notes:       None 
 *
 *****************************************************************************/
static F7X_PARAM_PTR F7XProtocol_GetParamEntryList (UINT16 entry)
{
  return ( (F7X_PARAM_PTR) &m_F7X_ParamListCfg.param[entry] ); 
}


/******************************************************************************
 * Function:    F7XDispDebug_Task
 *
 * Description: Utility function to display F7X frame data for a specific ch
 *
 * Parameters:  pParam: Not used, only to match Task Mgr call signature
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
void F7XDispDebug_Task ( void *pParam )
{
  F7X_DATA_FRAME_PTR pFrame; 
  F7X_STATUS_PTR pStatus; 
  UINT16 ch; 
  UINT16 i,j; 
  UINT16 word; 
  
  F7X_DUMPLIST_CFG_PTR pCfg; 
  F7X_PARAM_PTR pParamListEntry; 
  FLOAT32 scale_lsb; 
  FLOAT32 fval; 
  SINT16 *sword; 
  
  UINT32 *pfilter; 
  BOOLEAN bDisplay; 

  
  // Determine if debug is enabled and determine which channel to display 
  if (m_F7X_Debug.bDebug == TRUE) 
  {
    // Determine channel to display 
    ch = m_F7X_Debug.Ch; 
    pFrame = (F7X_DATA_FRAME_PTR) &m_F7X_DataFrame[ch]; 
    pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
    pCfg = (F7X_DUMPLIST_CFG_PTR) &m_F7X_DumplistCfg[pStatus->nIndexCfg];
    
    if (m_F7X_Debug.bNewFrame == TRUE) 
    {
      if (m_F7X_Debug.bFormatted == FALSE ) 
      {
        // Display Address Checksum
        sprintf ( (char *) Str, "\r\nChksum=0x%08x\r\n", pStatus->AddrChksum); 
        GSE_PutLine( (const char *) Str ); 
        
        // Display Size 
        sprintf ( (char *) Str, "Size=%d\r\n", pStatus->DumpListSize ); 
        GSE_PutLine( (const char *) Str ); 
        
        // Display Raw Buffer 
          // Display sync char first
          sprintf( (char *) Str, "%02x\r\n\0", pFrame->sync_byte ); 
          GSE_PutLine( (const char *) Str ); 
        
          // Display 4 bytes Addr Checksum 
          sprintf( (char *) Str, "%08x\r\n\0", pFrame->addrChksum ); 
          GSE_PutLine( (const char *) Str ); 
          
          // Display 2 byte params until End Of Frame 
          j = 0; 
          // ->DumpListSize is total bytes. /2 will remove sync byte and - 2 words for 
          //     the 4 byte cksum 
          for (i=0;i<((pStatus->DumpListSize/2) - 2);i++) 
          {
            word = pFrame->data[i]; 
            sprintf ( (char *) &Str[j], "%03d: %04x\r\n\0", i+1, word ); 
            j = j + 11;  // Point ot "\0" and over write 
          }
          GSE_PutLine( (const char *) Str ); 
      } // End of if .bFormatted == TRUE 
      else 
      {
        // Display Address Checksum
        sprintf ( (char *) Str, "\r\nChksum=0x%08x\r\n", pStatus->AddrChksum); 
        GSE_PutLine( (const char *) Str ); 
      
        // Display data but format based on 
        for (i=0;i<((pStatus->DumpListSize/2) - 2);i++) 
        {
          if (i>95)
          {
            pfilter = &m_F7X_Debug.Param3; 
          }
          else if (i>63) 
          {
            pfilter = &m_F7X_Debug.Param2; 
          }
          else if (i>31)
          {
            pfilter = &m_F7X_Debug.Param1; 
          }
          else
          {
            pfilter = &m_F7X_Debug.Param0; 
          }
          
          bDisplay = (*pfilter >> (i % 32)) & 0x1;  
          if (bDisplay == 0x01) 
          {
            word = pFrame->data[i]; 
            // Swap byte
            word = ((word & 0xFF00) >> 8) | ((word & 0x00FF) << 8); 
            sword = (SINT16 *) &word; 
            
            pParamListEntry = &m_F7X_ParamList.param[pCfg->ParamId[i]]; 
            if ( pParamListEntry->scale != DR_NO_RANGE_SCALE )
            {
              // Binary Data "BNR"
              scale_lsb = (FLOAT32) pParamListEntry->scale / 32768; 
            }
            else 
            {
              // Discrete Data
              scale_lsb = 1.0f;  
            }

            
            fval = (FLOAT32) (*sword * scale_lsb); 
            if (strlen(F7X_Param_Names[pCfg->ParamId[i]].name) == 0)
            {
              sprintf( (char *) Str, "UNDEFINED(%d)= %5.3f\r\n", i, fval);
            }
            else
            {
              sprintf( (char *) Str, "%s= %5.3f\r\n", F7X_Param_Names[pCfg->ParamId[i]].name, fval);
            }
            
            GSE_PutLine( (const char *) Str ); 
          }
        } // End for loop 
      } // End else for formatted display 
      
      m_F7X_Debug.bNewFrame = FALSE; 
      
    } // End if (new frame) to display 
    
  } // End of if bDebug == TRUE 

}



/******************************************************************************
 * Function:    F7XProtocol_SensorSetup()
 *
 * Description: Sets the m_UartMgr_WordInfo[] table with parse info for a 
 *              specific F7X NParameter Data Word
 *
 * Parameters:  gpA - General Purpose A (See below for definition) 
 *              gpB - General Purpose B (See below for definition)
 *              param - F7X Parameter to monitor as sensor 
 *              info_ptr- call be ptr to update Uart Mgr Word Info data 
 *
 * Returns:     TRUE if decode Ok 
 *
 * Notes:       
 *  gpA
 *  ===
 *     bits 10 to  0: Not Used 
 *     bits 15 to 11: Word Size 
 *     bits 20 to 16: MSB Position 
 *     bits 22 to 21: 00 = "DISC" where all bits are used 
 *                  : 01 = "BNR" where MSB = bit 15 and always = sign bit
 *                  : 10, 11 = Not Used 
 *     bits 31 to 23: Not Used 
 *
 *  gpB
 *  ===
 *     bits 31 to  0: timeout (msec) 
 *
 *****************************************************************************/
BOOLEAN F7XProtocol_SensorSetup ( UINT32 gpA, UINT32 gpB, UINT8 param, 
                                         void *info_ptr )
{
  BOOLEAN bOk; 
  UARTMGR_WORD_INFO_PTR word_info_ptr; 
  
  
  bOk = FALSE; 
  
  word_info_ptr = (UARTMGR_WORD_INFO_PTR) info_ptr; 
  
  // Parse Word Size 
  word_info_ptr->word_size = (gpA >> 11) & 0x1F; 
  
  // Parse Word MSB Positioin 
  word_info_ptr->msb_position = (gpA >> 16) & 0x1F; 
  
  // Parse Data Loss TimeOut 
  word_info_ptr->dataloss_time = gpB; 
  
  // Update id
  if (param < m_F7X_ParamList.cnt) 
  {
    word_info_ptr->id = param; 
    bOk = TRUE; 
  }
  
  // Update data type
  word_info_ptr->type = (UART_DATA_TYPE) ((gpA >> 21) & 0x0F); 
  
  return (bOk); 
  
}                                         



/******************************************************************************
 * Function:    F7XProtocol_ReturnFileHdr()
 *
 * Description: Function to return the F7X file hdr structure
 *
 * Parameters:  dest - pointer to destination bufffer
 *              max_size - max size of dest buffer 
 *              ch - uart channel requested 
 *
 * Returns:     cnt - byte count of data returned 
 *
 * Notes:       none 
 *
 *****************************************************************************/
UINT16 F7XProtocol_ReturnFileHdr ( UINT8 *dest, const UINT16 max_size, UINT16 ch )
{
  F7X_STATUS_PTR   pStatus; 
  F7X_APP_DATA_PTR pAppData; 
  UINT16           index; 
  
  F7X_DUMPLIST_CFG_PTR pDumpList; 
  
  F7X_FILE_HDR        fileHdr; 
  F7X_DL_FILE_HDR_PTR pFileHdr_DL_Param;
  
  UINT16 i; 
  UINT16 paramId; 
  
  UINT16 cnt; 
  

  // ASSERT if max_size < sizeof(F7X_FILE_HDR)

  pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch];
  pAppData = (F7X_APP_DATA_PTR) &m_F7X_AppData[ch]; 
  
  index = F7X_DUMPLIST_NOT_RECOGNIZED_INDEX; 

  if (pStatus->nIndexCfg != F7X_DUMPLIST_NOT_RECOGNIZED_INDEX) 
  {
    // Return current dump list index that we have synced with
    index = pStatus->nIndexCfg; 
  }
  else 
  {
    // Return prev recorded dump list index if we currently have 
    //   not synced. 
    // If no prev recognized dump list, then return NOT_RECOGNIZED
    if ( pAppData->lastRecognizedDL != F7X_DUMPLIST_NOT_RECOGNIZED_INDEX)
    {
      index = pAppData->lastRecognizedDL; 
    }
  }
  
  if ( index != F7X_DUMPLIST_NOT_RECOGNIZED_INDEX )
  {
    // Return from dump list cfg 
    pDumpList = (F7X_DUMPLIST_CFG_PTR) &m_F7X_DumplistCfg[index]; 
  }
  else 
  {
    // Return unrecognized dump list 
    pDumpList = (F7X_DUMPLIST_CFG_PTR) &M_UNDEFINED_DUMPLIST; 
  }
  
  // Update fileHdr data 
  fileHdr.size = sizeof(F7X_FILE_HDR); 
  fileHdr.ParamTranslationVer = m_F7X_ParamList.ver; 
  fileHdr.dl_AddrChksum = pDumpList->AddrChksum; 
  fileHdr.dl_NumParams = pDumpList->NumParams; 
  
  pFileHdr_DL_Param = (F7X_DL_FILE_HDR_PTR) &fileHdr.dl[0];
  
  // Loop thru all param entries 
  for (i=0;i<F7X_MAX_DUMPLIST_SIZE;i++)
  {
    paramId = pDumpList->ParamId[i]; 
    pFileHdr_DL_Param->id = paramId; 
    pFileHdr_DL_Param->scale = m_F7X_ParamList.param[paramId].scale; 
    pFileHdr_DL_Param->tol = m_F7X_ParamList.param[paramId].tol; 
    pFileHdr_DL_Param->timeout = m_F7X_ParamList.param[paramId].timeout; 
    
    pFileHdr_DL_Param++;   // Move to next entry
  }
  
  // Copy fileHdr data to destination 
  memcpy ( dest, (UINT8 *) &fileHdr, sizeof(F7X_FILE_HDR) ); 
  
  // return cnt 
  cnt = sizeof(F7X_FILE_HDR); 
  
  return (cnt); 
  
}



/******************************************************************************
 * Function:    F7XProtocol_Decoder_NotSynced()
 *
 * Description: Utility function to Decode Frames from Serial Data 
 *
 * Parameters:  ch - Uart Port Index 
 *
 * Returns:     None
 *
 * Notes:       
 *    - TBD add time count to ensure X ms is not expired for early exit ! 
 *    - Assumes buff passed in starts at offset 0, cnt == even 
 *
 *****************************************************************************/
static 
void F7XProtocol_Decoder_NotSynced (UINT16 ch)
{
  F7X_RAW_BUFFER_PTR   buff_ptr;
  
  UINT8  *src_ptr;
  UINT8  *end_ptr; 
  UINT8  syncSearch; 
  UINT8  *temp_ptr; 
  
  UINT16 nIndex; 
  UINT8  word; 
  UINT16 cnt; 
  UINT16 i,j; 

  F7X_SYNC_DATA syncData[F7X_INIT_SYNCWORDS]; 
  UINT16 frameSize; 
  UINT32 addrChksum; 
  
  //BOOLEAN bBadFrame; 
  F7X_STATUS_PTR pStatus; 
  
  UINT8 buff[F7X_MAX_RAW_BUF]; 

  
  buff_ptr = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[ch]; 
  
  // ASSERT if cnt not EVEN !!!! 
  // ASSERT if pRd == start of buff 
  // ASSERT if cnt != F7X_MAX_RAW_BUF
  
  // Copy data from rolling buffer to a flat buffer for simplier processing below 
  i = (UINT16)((UINT32) &buff_ptr->data[F7X_MAX_RAW_BUF] - (UINT32) buff_ptr->pRd); 
  memcpy ( (UINT8 *) &buff[0], buff_ptr->pRd, i ); 
  j = (UINT16) ( (UINT32) buff_ptr->pRd - (UINT32) &buff_ptr->data[0] ); 
  memcpy ( (UINT8 *) &buff[i], (UINT8 *) &buff_ptr->data[0], j); 


  cnt = buff_ptr->cnt;  // ->cnt should be == F7X_MAX_RAW_BUF 
  src_ptr = (UINT8 *) &buff[0]; 
  end_ptr = (UINT8 *) &buff[cnt]; 
  
  // Search for sync words 
  nIndex = 0; 
  syncSearch = F7X_SYNC_WORD1;   
  
  // Search until end of buffer is encountered, or two frame and 3 sync words found. 
/*  
  while ( (nIndex < F7X_INIT_SYNCWORDS) && (src_ptr < end_ptr) )
  {
*/  
    // Look for the sync characters in the buffer
    //   End if we have found two full frames / three sync char OR end of buffer has been reached. 
    while ( (nIndex < F7X_INIT_SYNCWORDS) && (src_ptr < end_ptr) )
    {
       // Current word
       // word = (*src_ptr << 8) | (*(src_ptr + 1)); // Note: buff is always filled to even addr ! 
       word = *src_ptr; 
  
       // Compare word to sync words AND if not near end of buffer 
       // Note: if buffer ends with sync word and addrchksum,
       //       exit and wait for next buffer to continue processing 
       if ( (((nIndex == 0) && ((word == F7X_SYNC_WORD1) || (word == F7X_SYNC_WORD2)) ) ||
             ((nIndex != 0) && ((word == syncSearch)))) &&
             ((src_ptr + 4) < end_ptr) )
       {
       
         syncData[nIndex].word = word; 
         syncData[nIndex].ptr = src_ptr; 
         // src_ptr++;   // move to addr chksum field 
         syncData[nIndex].addrChksum = (*(src_ptr + 1)) | (*(src_ptr + 2) << 8)
                                       | (*(src_ptr + 3) << 16) 
                                       | (*(src_ptr + 4) << 24); 
       
         // If 2nd sync word then determine if it is actually good:
         //    i) must be < 262 bytes away (worst case 128 param dump list) 
         //   ii) if < 262, jump size (1st sync to 2nd sync) to determine 3rd sync
         //  iii) if 3rd sync is Ok, then perform the following: 
         //       a) the addr checksum in frame 1,2 and 3 all match
         //   if any above fail then continue looking for 2nd sync again until End of Buffer
         if (nIndex != 0) // starting with 2nd sync char 
         {
           // Is distance between 2nd and 1st sync < F7X_MAX_DUMPLIST_SIZE + 6 ? 
           frameSize = ( (UINT32) syncData[nIndex].ptr - (UINT32) syncData[nIndex - 1].ptr ) ; 
           if ( frameSize <= ((F7X_MAX_DUMPLIST_SIZE * 2) + 5)  )
           {
             // Find 3rd Sync char 
             //  Determine if not end of buffer + include addrChksum field on next frame 
             if ( (src_ptr + (frameSize + 4)) < end_ptr ) 
             {
               // Is it the correct Sync char ? It should be the ALT of 2nd sync char or the 1st sync char
               if (*(src_ptr + frameSize) == syncData[0].word) 
               {
                 // Update the 3rd sync char
                 temp_ptr = src_ptr + frameSize; 
                 nIndex++;  // Move to 3rd sync char
                 syncData[nIndex].word = *temp_ptr; 
                 syncData[nIndex].ptr = temp_ptr; 
                 syncData[nIndex].addrChksum = (*(temp_ptr + 1)) | (*(temp_ptr + 2) << 8)
                                               | (*(temp_ptr + 3) << 16) 
                                               | (*(temp_ptr + 4) << 24);
                                               
                 // Determine if addrChksum matches, if not, then 2nd sync is bad, 
                 // continue finding the 2nd sync char 
                 addrChksum = syncData[0].addrChksum; 
                 if ( ( addrChksum != syncData[1].addrChksum) || 
                      ( addrChksum != syncData[2].addrChksum) )
                 {
                   // 2nd sync is bad, continue finding the 2nd sync char
                   src_ptr++;
                   nIndex--;  // Go back one to look for 2nd sync char 
                 }
                 else 
                 {
                   // Move src_ptr to end of addrChksum of 3rd sync char 
                   src_ptr = syncData[nIndex].ptr + 5;                                                
                   // Update nIndex to indicate all 3 sync char found 
                   nIndex++; 
                 }
               }
               else 
               {
                 // 2nd sync is bad, continue finding the 2nd sync char, skip over this char
                 // JV to get to this code, enter random sync char in data stream 
                 src_ptr++; 
               }
             }
             else 
             {
               // End of buffer reached and can't find 3rd sync or 2nd frame, 
               //    flush buffer and re-buff again 
               // JV to get to this code, send in char with no sync words 
               nIndex = 0; 
               src_ptr = end_ptr; 
             }
           }
           else 
           {
             // Force re-sync of 1st sync word, but do not exit yet 
             // JV to get to this code, enter random sync char in data stream 
             src_ptr = syncData[0].ptr + 1; 
             // restart finding sync words 
             nIndex = 0; 
           }
         }
         else  // 1st sync char found, update pointer to after 1st sync frame's addrchksum field 
         {
           src_ptr = syncData[nIndex].ptr + 5;  
           nIndex++;  // Look for next sync char 
           syncSearch = (word == F7X_SYNC_WORD1) ? F7X_SYNC_WORD2 : F7X_SYNC_WORD1; 
         }
         
       } // End compare for sync word 
       else 
       {
         src_ptr++;  // adv ptr to next char
       }
    } // End while loop to find F7X_INIT_SYNCWORDS
    
    // else case, not all the sync words were found before buffer end, 
    //    this normally should not occur as buffer should be size to buffer 
    //    at least 4 frames of data.  If this case continue, frame sync will never occur ! 
/*    
  } // End while buff end has not been reached   
*/  
  
  // Frame sync found !!! 
  if ( nIndex >= F7X_INIT_SYNCWORDS ) 
  {
    // Update status
    pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
    pStatus->syncInit = TRUE; 
    pStatus->AddrChksum = addrChksum; 
    pStatus->DumpListSize = frameSize; 
    pStatus->nFrames += F7X_INIT_SYNCWORDS; 
    pStatus->lastFrameTime = CM_GetTickCount(); 
    CM_GetTimeAsTimestamp( (TIMESTAMP*) &pStatus->lastFrameTS); 
    
    // Determine if dump list is recognized 
    pStatus->bRecognized = F7XProtocol_DumpListRecognized ( ch ); 

    // Advance to the last frame if we are not there yet
    //   Get char remaining in buff that has not been processed, from the last (3rd) sync
    //   word position to end of buff. 
    i = (UINT16) ( (UINT32) &buff[cnt] - (UINT32) syncData[F7X_INIT_SYNCWORDS - 1].ptr); 
    //   Determine # of frames that can exist in the unprocessed buff
    i = i / pStatus->DumpListSize; 
    pStatus->nFrames += i; 
    // Number of char to skip in buff till start of next frame to be processed when that 
    //    frame reception is completed. = 2 frames from init sync + any complete 
    //    frames to the end of buffer  
    j = (i * pStatus->DumpListSize) + ((UINT32) syncData[F7X_INIT_SYNCWORDS - 1].ptr - 
                                       (UINT32) &buff[0] ); 
    
    // Reset buffer
    if ( (buff_ptr->pRd + j)  >= (UINT8 *) &buff_ptr->data[F7X_MAX_RAW_BUF] )
    {
      // Wrap case  FINISH HERE !!! 
      i = (UINT16) ( (UINT32) &buff_ptr->data[F7X_MAX_RAW_BUF] - (UINT32) buff_ptr->pRd ); 
      buff_ptr->pRd = (UINT8 *) &buff_ptr->data[0] + (j - i); 
    }
    else 
    {
      buff_ptr->pRd += j; 
    }

    // Update cnt of char in buff to be processed     
    buff_ptr->cnt = buff_ptr->cnt - j; 
    
    // We should be pointing to sync char at this point ! 
    buff_ptr->pSync = buff_ptr->pRd; 
    
    // At this point we should be pointing to the sync word of the 
    //  most recent frame.  If not, let F7XProtocol_Decoder_Synced()
    //  handle this temp out of sync ! 
    
  }  
  else // Frame sync not found 
  {
    // Discard this entire buffer and try again later ! 
    F7XProtocol_Resync(ch); 
  }  

}


/******************************************************************************
 * Function:    F7XProtocol_Decoder_Synced()
 *
 * Description: Utility function to Decode Frames from Serial Data 
 *
 * Parameters:  ch - Uart Port Index 
 *
 * Returns:     TRUE if new frame received 
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
BOOLEAN F7XProtocol_Decoder_Synced (UINT16 ch)
{
  F7X_RAW_BUFFER_PTR   buff_ptr;
  F7X_STATUS_PTR       pStatus; 
  
  UINT8  *pCurrSync; 
  UINT8  *pNextSync; 
  UINT8  *pStart; 
  UINT8  *pEnd; 
  
  UINT8  currSyncWord; 
  UINT8  nextSyncWord; 
  
  UINT16 i,j; 
  
  BOOLEAN bNewFrame; 


  bNewFrame = FALSE; 
  
  buff_ptr = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[ch];
  pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 

  // Wait until ->DumpListSize size reached plus sync word of next frame 
  if ( buff_ptr->cnt >= (pStatus->DumpListSize + 1) ) 
  {
    pStart = (UINT8 *) &buff_ptr->data[0]; 
    pEnd = (UINT8 *) &buff_ptr->data[F7X_MAX_RAW_BUF]; 
    
    // Is current sync word and the next sync word in proper sequence ? 
    pCurrSync = buff_ptr->pRd; 
    if ( (buff_ptr->pRd + pStatus->DumpListSize) >= pEnd )
    {
      // Wrap case
      pNextSync = pStart + 
                  ((pStatus->DumpListSize) - 
                  ((UINT32) &buff_ptr->data[F7X_MAX_RAW_BUF] - (UINT32) (buff_ptr->pRd)) );
    }   
    else 
    {
      pNextSync = buff_ptr->pRd + pStatus->DumpListSize;
    }
    
    currSyncWord = *pCurrSync; 
    nextSyncWord = *pNextSync; 
    
    if ( (( currSyncWord == F7X_SYNC_WORD1 ) && ( nextSyncWord == F7X_SYNC_WORD2)) ||
         (( currSyncWord == F7X_SYNC_WORD2 ) && ( nextSyncWord == F7X_SYNC_WORD1)) )
    {
      bNewFrame = TRUE; 
      pStatus->bResync = FALSE; 
      
      // Update current frame time 
      pStatus->lastFrameTime = CM_GetTickCount(); 
      CM_GetTimeAsTimestamp((TIMESTAMP*) &pStatus->lastFrameTS); 
      
      pStatus->nFrames += 1; 
      
      // We are currently synced ! 
      pStatus->sync = TRUE; 
      
      // Update display buffer  TBD only if debug is enabled to save processing time 
      // if (m_F7X_Debug.bDebug == TRUE) 
      // {
        F7XProtocol_UpdateFrame(ch, pCurrSync, pNextSync); 
      // }
      
      // If we resynced due to timeout then update counter
      if  (buff_ptr->pSync == 0)
      {
        pStatus->nResyncs++; 
      }
      
      // Update buff ptr to next frame 
      buff_ptr->pRd = pNextSync; 
      buff_ptr->pSync = pNextSync; 
      buff_ptr->cnt -= pStatus->DumpListSize; 
      
    }
    else 
    {
      // Clear sync ptr, we must have lost sync !!! 
      buff_ptr->pSync = 0; 
      
      // Update run time sync flag 
      pStatus->sync = FALSE; 
      
      // Search and find next sync word 
      j = buff_ptr->cnt; 
      
      for (i=0;i<j;i++) 
      {
        currSyncWord = *buff_ptr->pRd; 
        if ( (currSyncWord == F7X_SYNC_WORD1) || (currSyncWord == F7X_SYNC_WORD2) )
        {  
          buff_ptr->pSync = buff_ptr->pRd; 
          // Only update nResyncs on transition to find sync again 
          if (pStatus->bResync == FALSE) 
          {
            // We resynced update counter 
            pStatus->nResyncs++; 
            pStatus->bResync = TRUE; 
          }
          // Exit for loop 
          break; 
        }
        else 
        {
          // Advance to the next char 
          buff_ptr->pRd++;   
          // Handle Wrap case 
          if (buff_ptr->pRd >= pEnd) 
          {
            buff_ptr->pRd = pStart; 
          }
        }
      } // End of for loop 
      
      // Update char count 
      buff_ptr->cnt -= i;  
      
    } // end of else lost sync ! 
    
  } // End if Dump List Size received. 
  else 
  {
     // Don't have a complete frame yet.. continue buffering data 
     // If 10 sec transpire, then go back to initial sync (is this really necesary ?) 
     // Once frame sync has been found, unless EEC dump list is changed, the 
     //   frame should continue to be valid, why look for framing again ?  
  }  
  
  return (bNewFrame);  
  
}


/******************************************************************************
 * Function:    F7XProtocol_Resync()
 *
 * Description: Utility function to force re-sync of incomming data 
 *
 * Parameters:  ch - Uart Port Index 
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
void F7XProtocol_Resync (UINT16 ch)
{
  F7X_RAW_BUFFER_PTR   buff_ptr;
  
  
  buff_ptr = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[ch]; 
  
  buff_ptr->cnt = 0; 
  buff_ptr->pRd = (UINT8 *) &buff_ptr->data[0];
  buff_ptr->pWr = buff_ptr->pRd;  
  buff_ptr->pSync = 0; 
  
}  


/******************************************************************************
 * Function:    F7XProtocol_DumpListRecognized()
 *
 * Description: Utility function to determine if dump list is recognized 
 *
 * Parameters:  ch - Uart Port Index 
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
BOOLEAN F7XProtocol_DumpListRecognized (UINT16 ch)
{
  F7X_STATUS_PTR pStatus; 
  F7X_DUMPLIST_CFG_PTR pCfg; 
  BOOLEAN bRecognized; 
  UINT16 i; 
  F7X_NEW_DL_FOUND_LOG newDLFoundLog; 
  UINT16 index; 
  
  
  pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
  pCfg = (F7X_DUMPLIST_CFG_PTR) &m_F7X_DumplistCfg[0]; 
  bRecognized = FALSE; 
  memset ( &newDLFoundLog, 0, sizeof(F7X_NEW_DL_FOUND_LOG) ); 
  
  // Loop thru all Cfg and compare 
  for (i=0;i<F7X_MAX_DUMPLISTS;i++) 
  {
    // Compare addr_chksum and dump list size 
    //   Note: ->NumParams does not include AddrChksum nor SyncWords (need to add +5)
    if ( (pCfg->AddrChksum == pStatus->AddrChksum) &&
         ( ((pCfg->NumParams * sizeof(UINT16)) + 5) == pStatus->DumpListSize) )
    {
      bRecognized = TRUE; 
      pStatus->nIndexCfg = i; 
      
      break; 
    }
    // Test case only
    // If size matches and ignore AddrChksum field cfg, then sync just on 
    //    size ! 
    if ( (pCfg->AddrChksum == F7X_IGNORE_ADDRCHKSUM) &&
         ( ((pCfg->NumParams * sizeof(UINT16)) + 5) == pStatus->DumpListSize) )
    {
      bRecognized = TRUE; 
      pStatus->nIndexCfg = i; 
      
      break; 
    }
    // Test case only 
    
    pCfg++; 
  }


  // Get prev recognized dump list info if exist ! 
  index = m_F7X_AppData[ch].lastRecognizedDL; 
  newDLFoundLog.prev.index = index; 
  if ( index != F7X_DUMPLIST_NOT_RECOGNIZED_INDEX ) 
  {
    newDLFoundLog.prev.numParams = m_F7X_DumplistCfg[index].NumParams; 
    newDLFoundLog.prev.chksum = m_F7X_DumplistCfg[index].AddrChksum; 
  }

  // If recognized, update F7X App Data in EEPROM 
  if ( bRecognized == TRUE ) 
  {
    if ( pStatus->nIndexCfg != m_F7X_AppData[ch].lastRecognizedDL ) 
    {
    
      // Record Log that Update Has been made
      newDLFoundLog.result = SYS_UART_F7X_NEW_DL_FOUND;
      newDLFoundLog.ch     = ch;
      
      index = pStatus->nIndexCfg;
      newDLFoundLog.current.index = index; 
      newDLFoundLog.current.numParams = m_F7X_DumplistCfg[index].NumParams;
      newDLFoundLog.current.chksum = m_F7X_DumplistCfg[index].AddrChksum;

      // Update 
      m_F7X_AppData[ch].lastRecognizedDL = pStatus->nIndexCfg; 
      NV_Write( NV_UART_F7X, 0, (void *) m_F7X_AppData, 
                sizeof(F7X_APP_DATA) * UART_NUM_OF_UARTS);
      
      // Record log after the EEPROM has been queued for update. 
      LogWriteSystem (SYS_ID_UART_F7X_NEW_DL_FOUND, LOG_PRIORITY_LOW, &newDLFoundLog, 
                      sizeof(F7X_NEW_DL_FOUND_LOG), NULL);
      sprintf ( GSE_OutLine, "\r\nF7XProtocol: New Dump List found (new index=%d, old index=%d)\r\n",
                newDLFoundLog.current.index, newDLFoundLog.prev.index ); 
      GSE_DebugStr(NORMAL,TRUE,GSE_OutLine);
    }
  }
  else 
  {
    newDLFoundLog.result = SYS_UART_F7X_DL_NOT_RECOGNIZED; 
    
    // should be == F7X_DUMPLIST_NOT_RECOGNIZED_INDEX
    newDLFoundLog.current.index = pStatus->nIndexCfg;  
    newDLFoundLog.current.numParams = (pStatus->DumpListSize - 5)/2; 
    newDLFoundLog.current.chksum = pStatus->AddrChksum; 
    
    // Record log after the EEPROM has been queued for update. 
    LogWriteSystem (SYS_ID_UART_F7X_DL_NOT_RECOGNIZED, LOG_PRIORITY_LOW, &newDLFoundLog, 
                    sizeof(F7X_NEW_DL_FOUND_LOG), NULL);
  }

  // Output debug msg  
  if ( bRecognized == FALSE ) 
  {
    sprintf( GSE_OutLine, "\r\nF7XProtocol: SYNC Detected, Dump List Not Recognized (Ch = %d)\r\n", 
             ch ); 
  }
  else
  {
    sprintf( GSE_OutLine, 
             "\r\nF7XProtocol: SYNC Detected, Dump List Recognized (Ch = %d, DumpList Index = %d)\r\n", 
             ch, pStatus->nIndexCfg ); 
  }
  GSE_DebugStr(NORMAL,TRUE,GSE_OutLine); 
  
  
  return (bRecognized); 
}  



/******************************************************************************
 * Function:    F7XProtocol_UpdateFrame
 *
 * Description: Utility function to update the RAM display frame 
 *
 * Parameters:  ch     - Uart Port Index 
 *              pStart - pointer to data start in raw buffer (FIFO)
 *              pEnd   - pointer to data end in raw buffer (FIFO)
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
void F7XProtocol_UpdateFrame (UINT16 ch, UINT8 *pStart, UINT8 *pEnd)
{
  F7X_DATA_FRAME_PTR pFrame; 
  F7X_RAW_BUFFER_PTR pBuffRaw; 
  
  UINT16 i, j; 
  UINT8 *pBuffFrame; 
  UINT16 size; 
  
    
  pFrame = (F7X_DATA_FRAME_PTR) &m_F7X_DataFrame[ch]; 
  pBuffFrame = (UINT8 *) &pFrame->data[0]; 
  
  pBuffRaw = (F7X_RAW_BUFFER_PTR) &m_F7X_Buff[ch]; 

  
  pFrame->bNewFrame = TRUE; 
  m_F7X_Debug.bNewFrame = TRUE;   // For Debug display purposes  
  
  // Determine if wrap case 
  if (pEnd < pStart)
  {
    // Copy 1st half to flat buffer
    i = (UINT32) &pBuffRaw->data[F7X_MAX_RAW_BUF] - (UINT32) pStart; 
    memcpy ( pBuffFrame, pStart, i);  
    
    // Copy 2nd half to flat buffer
    j = (UINT32) pEnd - (UINT32) &pBuffRaw->data[0]; 
    memcpy ( (UINT8 *) (pBuffFrame + i), (UINT8 *) &pBuffRaw->data[0], j); 
    size = i + j; 
  }
  else 
  {
    // Copy entire to flat buffer
    size = (UINT32) pEnd - (UINT32) pStart; 
    memcpy ( pBuffFrame, pStart, size); 
  }
  
  // Get sync_byte 
  pFrame->sync_byte = *pBuffFrame; 
  
  // Get addrChksum 
  pFrame->addrChksum = ( *(pBuffFrame+1)) | ( *(pBuffFrame+2) << 8) | 
                       ( *(pBuffFrame+3) << 16) | ( *(pBuffFrame+4) << 24 ); 
  
  // Move para to start of ->data[0], remove sync_byte and addrChksum 
  memcpy ( (UINT8 *) pBuffFrame, (UINT8 *) (pBuffFrame+5), size - 5); 

}



/******************************************************************************
 * Function:    F7XProtocol_InitUartMgrData
 *
 * Description: Utility function to initialize Uart Mgr Data on init frame 
 *              sync 
 *
 * Parameters:  ch - Uart Port Index 
 *              data_ptr - Ptr to UART Mgr Runtime Data
 *              word_info_ptr - Ptr to UART Mgr Word info data
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
void F7XProtocol_InitUartMgrData( UINT16 ch, UARTMGR_PARAM_DATA_PTR data_ptr, 
                                  UARTMGR_WORD_INFO_PTR word_info_ptr)
{
   F7X_DUMPLIST_CFG_PTR pCfg; 
   F7X_STATUS_PTR pStatus; 
   F7X_PARAM_PTR pParam; 
   F7X_PARAM_PTR pParamStart; 
   
   UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
   REDUCTIONDATASTRUCT_PTR data_red_ptr; 
   
   UINT16 i,k; 
   UINT16 size; 
   UINT16 *pid; 


   // From Dump List assign Uart Mgr Data entries, using translation table
   pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
   pCfg = (F7X_DUMPLIST_CFG_PTR) &m_F7X_DumplistCfg[pStatus->nIndexCfg]; 
   pParamStart = (F7X_PARAM_PTR) &m_F7X_ParamList.param[0]; 
   
   // determine number of param words, minus delimiter and addr chksum fields 
   size = ((pStatus->DumpListSize - 5) / 2) ; 
   
   for (i=0;i<size;i++) 
   {
     // Point to param entry based on dump list that was recognized 
     pParam = (pParamStart + pCfg->ParamId[i]); 
     
     // Update pointers 
     runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data; 
     data_red_ptr = (REDUCTIONDATASTRUCT_PTR) &data_ptr->reduction_data; 
     
     // Update Param id and scale 
     runtime_data_ptr->id = pCfg->ParamId[i]; 
     runtime_data_ptr->scale = pParam->scale; 
     if ( pParam->scale != DR_NO_RANGE_SCALE )
     {
       // Binary Data "BNR"
       runtime_data_ptr->scale_lsb = (FLOAT32) pParam->scale / 32768; 
     }
     else 
     {
       // Discrete Data
       runtime_data_ptr->scale_lsb = 1.0f;  
     }
     runtime_data_ptr->cfgTimeOut = pParam->timeout; 
     
     // Update tol 
     data_red_ptr->Tol = pParam->tol; 
     
     // Move to the next entry 
     data_ptr++; 
   }
   
   // Update Word Info Data 
   for (i=0;i<UARTMGR_MAX_PARAM_WORD;i++)
   {
     if ( word_info_ptr->id != UARTMGR_WORD_ID_NOT_USED )
     {
       // Loop thru dump list to find matching Id and then update word info nIndex 
       pid = (UINT16 *) &pCfg->ParamId[0];
       for (k=0;k<pCfg->NumParams;k++)
       {
         if (*pid == word_info_ptr->id)
         {
           word_info_ptr->nIndex = k; 
           break; // Check next word info entry 
         }
         pid++;  // Move to next dump list entry 
       }
       
       // If we can't find param to setup up word info, record sys log  
       //   nIndex remains 255 for unused or not found (init on startup ! to avoid
       //   having "else" case ! JV should appreciate this)
       
     }
     else 
     {
       // Encounter End of Entry used.  Exit. 
       break; 
     }
     word_info_ptr++; // Move to next word info entry until ->id == UARTMGR_WORD_ID_NOT_USED
   }
   
}                                  


/******************************************************************************
 * Function:    F7XProtocol_UpdateUartMgrData
 *
 * Description: Utility function to update Uart Mgr Data on new frame 
 *
 * Parameters:  ch - Uart Port Index 
 *              data_ptr - Ptr to UART Mgr Runtime Data
 *              word_info_ptr - Ptr to UART Mgr Word info data
 *
 * Returns:     None
 *
 * Notes:       None 
 *
 *****************************************************************************/
static 
void F7XProtocol_UpdateUartMgrData( UINT16 ch, UARTMGR_PARAM_DATA_PTR data_ptr, 
                                  UARTMGR_WORD_INFO_PTR word_info_ptr)
{
   F7X_STATUS_PTR pStatus; 
   
   UARTMGR_RUNTIME_DATA_PTR runtime_data_ptr;
   
   UINT16 i; 
   UINT16 size; 
   
   UINT8 *pWord; 


   // From Dump List assign Uart Mgr Data entries, using translation table
   pStatus = (F7X_STATUS_PTR) &m_F7X_Status[ch]; 
   pWord = (UINT8 *) &m_F7X_DataFrame[ch].data[0]; 
   
   // determine number of param words, minus delimiter and addr chksum fields 
   size = ((pStatus->DumpListSize - 5) / 2); 
   
   for (i=0;i<size;i++) 
   {
     // Update pointers 
     runtime_data_ptr = (UARTMGR_RUNTIME_DATA_PTR) &data_ptr->runtime_data; 
     
     // Update val, rxTime, bNewVal, and bValid, LSB first then MSB
     runtime_data_ptr->val_raw = (UINT16) (*(pWord + 1) << 8) | (*pWord); 
     runtime_data_ptr->rxTime = pStatus->lastFrameTime; 
     runtime_data_ptr->rxTS = pStatus->lastFrameTS; 
     runtime_data_ptr->bNewVal = TRUE; 
     runtime_data_ptr->bValid = TRUE; 
     
     // Move to the next entry 
     data_ptr++; 
     
     // Move to next word 
     pWord += 2; 
   }
   
}    



/******************************************************************************
 * Function:    F7XProtocol_RestoreAppData
 *
 * Description: Utility function to restore F7X app data from eeprom
 *
 * Parameters:  none
 *
 * Returns:     none
 *
 * Notes:       none 
 *
 *****************************************************************************/
static 
void F7XProtocol_RestoreAppData( void )
{
  RESULT result;  
  
  result = NV_Open(NV_UART_F7X); 
  if ( result == SYS_OK )
  {
    NV_Read(NV_UART_F7X, 0, (void *) m_F7X_AppData, 
             sizeof(F7X_APP_DATA) * UART_NUM_OF_UARTS);
  }
  
  // If open failed or eeprom reset to 0xFFFF, re-init app data 
  if ( ( result != SYS_OK ) || (m_F7X_AppData[0].lastRecognizedDL == 0xFFFF) )
  {
    F7XProtocol_FileInit();
  }
}

/******************************************************************************
 * Function:     F7XProtocol_FileInit
 *
 * Description:  Reset the file to the initial state.
 *
 * Parameters:   [in]  none
 *               [out] none
 *
 * Returns:      BOOLEAN TRUE  The file reset was successful.
 *                       FALSE The file reset failed.
 *
 * Notes:        None
 *
 *****************************************************************************/
BOOLEAN F7XProtocol_FileInit(void)
{
  UINT16 i;

  // Init App data
  // Note: NV_Mgr will record log if data is corrupt
  for (i = 0; i < UART_NUM_OF_UARTS; i++)
  {
    m_F7X_AppData[i].lastRecognizedDL = F7X_DUMPLIST_NOT_RECOGNIZED_INDEX;
  }

  // Update App data
  NV_Write(NV_UART_F7X, 0, (void *) m_F7X_AppData,
                       sizeof(F7X_APP_DATA) * UART_NUM_OF_UARTS);
  return TRUE;
}


/******************************************************************************
 * Function:     F7XProtocol_DisableLiveStream
 *
 * Description:  Disables the outputting the live stream for F7X 
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
void F7XProtocol_DisableLiveStream(void)
{
  m_F7X_Debug.bDebug = FALSE;
}

/*****************************************************************************/
/* Local Functions                                                           */ 
/*****************************************************************************/

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: F7XProtocol.c $
 * 
 * *****************  Version 20  *****************
 * User: Melanie Jutras Date: 12-11-13   Time: 1:37p
 * Updated in $/software/control processor/code/system
 * SCR #1142 File Format Errors
 * 
 * *****************  Version 19  *****************
 * User: Melanie Jutras Date: 12-10-19   Time: 1:23p
 * Updated in $/software/control processor/code/system
 * SCR #1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 18  *****************
 * User: Melanie Jutras Date: 12-10-04   Time: 3:49p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 17  *****************
 * User: Melanie Jutras Date: 12-10-04   Time: 2:02p
 * Updated in $/software/control processor/code/system
 * SCR 1172 PCLint 545 Suspicious use of & Error
 * 
 * *****************  Version 16  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 2:36p
 * Updated in $/software/control processor/code/system
 * SCR# 1142 - more format corrections
 * 
 * *****************  Version 15  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 * 
 * *****************  Version 14  *****************
 * User: Contractor2  Date: 5/23/11    Time: 1:56p
 * Updated in $/software/control processor/code/system
 * SCR #1033 Enhancement - F7X New DumpList Found Log
 * 
 * *****************  Version 13  *****************
 * User: Contractor2  Date: 4/13/11    Time: 11:29a
 * Updated in $/software/control processor/code/system
 * SCR 987: F7X - Data Display
 * Added test for empty sensor label and display "UNDEFINED(index#)"
 * instead of blank.
 * 
 * *****************  Version 12  *****************
 * User: Jim Mood     Date: 10/29/10   Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR 936
 * 
 * *****************  Version 11  *****************
 * User: Peter Lee    Date: 9/30/10    Time: 5:45p
 * Updated in $/software/control processor/code/system
 * SCR #913 F7X decoder can not sync to 128 words, only up to 127 words. 
 * 
 * *****************  Version 10  *****************
 * User: Contractor V&v Date: 7/21/10    Time: 7:17p
 * Updated in $/software/control processor/code/system
 * SCR #538 SPIManager startup & NV Mgr Issues
 * 
 * *****************  Version 9  *****************
 * User: Peter Lee    Date: 6/25/10    Time: 6:09p
 * Updated in $/software/control processor/code/system
 * SCR #635 Swap Dump List Addr checksum Endianess.  Updated comment for
 * GPA in _SensorSetup(). 
 * 
 * *****************  Version 8  *****************
 * User: Contractor V&v Date: 6/22/10    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #485 Escape Sequence and Box Configuration
 * 
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 6/21/10    Time: 3:37p
 * Updated in $/software/control processor/code/system
 * SCR #635 items 17 and 18.  Update debug display to display new frames
 * only.  Make all f7x_dumplist.cfg[] and f7x_param.cfg[] entries blank.  
 * 
 * *****************  Version 6  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #635 Create log to indicate "unrecognized" dump list detected.
 * 
 * *****************  Version 5  *****************
 * User: Jim Mood     Date: 6/11/10    Time: 3:31p
 * Updated in $/software/control processor/code/system
 * SCR 623 Batch configuration implementation updates
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 6/10/10    Time: 2:32p
 * Updated in $/software/control processor/code/system
 * SCR #365 Update better debug display for F7X. 
 * 
 * *****************  Version 3  *****************
 * User: Contractor V&v Date: 6/09/10    Time: 7:21p
 * Updated in $/software/control processor/code/system
 * SCR #614 fast.reset=really should initialize files
 * 
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 8:12p
 * Updated in $/software/control processor/code/system
 * SCR #618 SW-SW Integration problems. Task Mgr updated to allow only one
 * Task Id creation. 
 * 
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 5:55p
 * Created in $/software/control processor/code/system
 * Initial Check In.  Implementation of the F7X and UartMgr Requirements. 
 * 
 * 
 *
 *****************************************************************************/
