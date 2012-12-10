#ifndef CREEP_H
#define CREEP_H

/******************************************************************************
            Copyright (C) 2008-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        Creep.h

    Description: Contains data structures related to the Creep Processing

    VERSION
      $Revision: 4 $  $Date: 12-12-09 6:39p $

******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "alt_stdtypes.h"
#include "TaskManager.h"
#include "FaultMgr.h"
#include "sensor.h"


/******************************************************************************
                                 Package Defines
******************************************************************************/
//#define CREEP_100_PERCENT   (1E10) // 1E12 units * 100%  for 1 sec interval
//#define CREEP_100_PERCENT   (1E11) // 1E13 units * 100%  for 0.1 sec interval

#define CREEP_MAX_ROWS 20
#define CREEP_MAX_COLS 10
#define CREEP_MAX_NAME 32

#define CREEP_MAX_OBJ 2
#define CREEP_MAX_TBL 2
#define CREEP_TABLE_UNUSED 255

#define CREEP_MAX_SENSOR_VAL 100   // Worst case,

#define CREEP_SENSOR_RATE_MINIMUM  1000  // 1 msec tick * 1000 = 1 second (HZ)

#define CREEP_SENSOR_RATE_BUFF_SIZE 20
// #define CREEP_SENSOR_RATE_TOL       2   // Basically 20 msec

// #define CREEP_TASK_EXE_TIME  10  // 10 msec between Creep Task exe

#define CREEP_DEFAULT_TBL    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,/*10 Row 0 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 1 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 2 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 3 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 4 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 5 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 6 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 7 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 8 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row 9 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row10 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row11 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row12 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row13 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row14 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row15 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row16 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row17 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row18 Val*/\
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /*10 Row19 Val*/\
                             \
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Row Value 0 -  4*/\
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Row Value 5 -  9*/\
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Row Value10 - 14*/\
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Row Value15 - 19*/\
                             \
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Col Value 0 - 4*/\
                             0.0f, 0.0f, 0.0f, 0.0f, 0.0f, /* Col Value 5 - 9*/\
                             \
                             "Default Table Name"          /* Table Name */



#define CREEP_DEFAULT_140A_PER_HOUR  \
\
200000    ,200000     ,200000     ,200000     ,250000     ,250000     ,300000     ,350000     ,400000     ,450000    ,   /*10 Row 0 Val*/\
200000    ,200000     ,250000     ,250000     ,300000     ,350000     ,400000     ,450000     ,500000     ,600000    ,   /*10 Row 1 Val*/\
200000    ,250000     ,250000     ,300000     ,300000     ,350000     ,400000     ,500000     ,550000     ,650000    ,   /*10 Row 2 Val*/\
250000    ,300000     ,300000     ,350000     ,400000     ,450000     ,500000     ,600000     ,650000     ,750000    ,   /*10 Row 3 Val*/\
300000    ,350000     ,400000     ,450000     ,500000     ,550000     ,650000     ,800000     ,850000     ,1000000   ,   /*10 Row 4 Val*/\
400000    ,500000     ,550000     ,600000     ,650000     ,750000     ,850000     ,1050000    ,1150000    ,1350000   ,   /*10 Row 5 Val*/\
600000    ,650000     ,750000     ,800000     ,950000     ,1050000    ,1250000    ,1500000    ,1650000    ,1900000   ,   /*10 Row 6 Val*/\
750000    ,850000     ,950000     ,1050000    ,1150000    ,1350000    ,1550000    ,1850000    ,2050000    ,2350000   ,   /*10 Row 7 Val*/\
1000000   ,1150000    ,1300000    ,1400000    ,1550000    ,1800000    ,2050000    ,2500000    ,2750000    ,3150000   ,   /*10 Row 8 Val*/\
1500000   ,1750000    ,1950000    ,2100000    ,2350000    ,2700000    ,3100000    ,3750000    ,4200000    ,4800000   ,   /*10 Row 9 Val*/\
2300000   ,2650000    ,2950000    ,3150000    ,3500000    ,4000000    ,4600000    ,5600000    ,6200000    ,7100000   ,   /*10 Row10 Val*/\
3150000   ,3600000    ,4000000    ,4400000    ,4900000    ,5650000    ,6500000    ,7850000    ,8750000    ,10000000  ,   /*10 Row11 Val*/\
4450000   ,5150000    ,5750000    ,6300000    ,7050000    ,8100000    ,9300000    ,11250000   ,12500000   ,14250000  ,   /*10 Row12 Val*/\
7850000   ,9050000    ,10150000   ,11050000   ,12300000   ,14150000   ,16150000   ,19500000   ,21450000   ,24050000  ,   /*10 Row13 Val*/\
12800000  ,14650000   ,16350000   ,17750000   ,19750000   ,22350000   ,25300000   ,29950000   ,32900000   ,37000000  ,   /*10 Row14 Val*/\
19000000  ,21700000   ,24100000   ,26050000   ,28850000   ,32750000   ,37100000   ,44050000   ,48500000   ,54650000  ,   /*10 Row15 Val*/\
29000000  ,33200000   ,36950000   ,40000000   ,44100000   ,49750000   ,56050000   ,66100000   ,72500000   ,81300000  ,   /*10 Row16 Val*/\
70400000  ,79200000   ,86950000   ,93200000   ,102150000  ,114350000  ,127850000  ,149100000  ,162600000  ,180950000 ,   /*10 Row17 Val*/\
102400000 ,115250000  ,126550000  ,135650000  ,148700000  ,166550000  ,186250000  ,218800000  ,240350000  ,270000000 ,   /*10 Row18 Val*/\
169050000 ,190400000  ,209650000  ,225600000  ,248600000  ,280250000  ,315450000  ,371250000  ,407250000  ,458000000 ,   /*10 Row19 Val*/\
                             \
                             742.1561083, 746.8094249, 749.1360831, 753.7893997, 760.7693745, /* Row Value 0 -  4*/\
                             767.7493494, 774.7293242, 779.3826408, 786.3626156, 795.7255363, /* Row Value 5 -  9*/\
                             805.118765,  812.1636866, 819.2086081, 830.9501441, 842.729649,  /* Row Value10 - 14*/\
                             852.2033202, 861.6769915, 882.9927518, 894.8348409, 905.000000,  /* Row Value15 - 19*/\
                             \
                             35800, 36300,  36700,  37000,  37400,   /* Col Value 0 - 4*/\
                             37900, 38400,  39100,  39500,  40000,   /* Col Value 5 - 9*/\
                             \
                             "Default Table Name"          /* Table Name */



#define CREEP_DEFAULT_140A_PER_SEC_1E12  \
\
5556    ,5556     ,5556     ,5556     ,6944     ,6944     ,8333     ,9722     ,11111    ,12500,     /*10 Row 0 Val*/\
5556    ,5556     ,6944     ,6944     ,8333     ,9722     ,11111    ,12500    ,13889    ,16667,     /*10 Row 1 Val*/\
5556    ,6944     ,6944     ,8333     ,8333     ,9722     ,11111    ,13889    ,15278    ,18056,     /*10 Row 2 Val*/\
6944    ,8333     ,8333     ,9722     ,11111    ,12500    ,13889    ,16667    ,18056    ,20833,     /*10 Row 3 Val*/\
8333    ,9722     ,11111    ,12500    ,13889    ,15278    ,18056    ,22222    ,23611    ,27778,     /*10 Row 4 Val*/\
11111   ,13889    ,15278    ,16667    ,18056    ,20833    ,23611    ,29167    ,31944    ,37500,     /*10 Row 5 Val*/\
16667   ,18056    ,20833    ,22222    ,26389    ,29167    ,34722    ,41667    ,45833    ,52778,     /*10 Row 6 Val*/\
20833   ,23611    ,26389    ,29167    ,31944    ,37500    ,43056    ,51389    ,56944    ,65278,     /*10 Row 7 Val*/\
27778   ,31944    ,36111    ,38889    ,43056    ,50000    ,56944    ,69444    ,76389    ,87500,     /*10 Row 8 Val*/\
41667   ,48611    ,54167    ,58333    ,65278    ,75000    ,86111    ,104167   ,116667   ,133333,    /*10 Row 9 Val*/\
63889   ,73611    ,81944    ,87500    ,97222    ,111111   ,127778   ,155556   ,172222   ,197222,    /*10 Row10 Val*/\
87500   ,100000   ,111111   ,122222   ,136111   ,156944   ,180556   ,218056   ,243056   ,277778,    /*10 Row11 Val*/\
123611  ,143056   ,159722   ,175000   ,195833   ,225000   ,258333   ,312500   ,347222   ,395833,    /*10 Row12 Val*/\
218056  ,251389   ,281944   ,306944   ,341667   ,393056   ,448611   ,541667   ,595833   ,668056,    /*10 Row13 Val*/\
355556  ,406944   ,454167   ,493056   ,548611   ,620833   ,702778   ,831944   ,913889   ,1027778,   /*10 Row14 Val*/\
527778  ,602778   ,669444   ,723611   ,801389   ,909722   ,1030556  ,1223611  ,1347222  ,1518056,   /*10 Row15 Val*/\
805556  ,922222   ,1026389  ,1111111  ,1225000  ,1381944  ,1556944  ,1836111  ,2013889  ,2258333,   /*10 Row16 Val*/\
1955556 ,2200000  ,2415278  ,2588889  ,2837500  ,3176389  ,3551389  ,4141667  ,4516667  ,5026389,   /*10 Row17 Val*/\
2844444 ,3201389  ,3515278  ,3768056  ,4130556  ,4626389  ,5173611  ,6077778  ,6676389  ,7500000,   /*10 Row18 Val*/\
4695833 ,5288889  ,5823611  ,6266667  ,6905556  ,7784722  ,8762500  ,10312500 ,11312500 ,12722222,  /*10 Row19 Val*/\
                             \
                             742.1561083, 746.8094249, 749.1360831, 753.7893997, 760.7693745, /* Row Value 0 -  4*/\
                             767.7493494, 774.7293242, 779.3826408, 786.3626156, 795.7255363, /* Row Value 5 -  9*/\
                             805.118765,  812.1636866, 819.2086081, 830.9501441, 842.729649,  /* Row Value10 - 14*/\
                             852.2033202, 861.6769915, 882.9927518, 894.8348409, 905.000000,  /* Row Value15 - 19*/\
                             \
                             35800, 36300,  36700,  37000,  37400,   /* Col Value 0 - 4*/\
                             37900, 38400,  39100,  39500,  40000,   /* Col Value 5 - 9*/\
                             \
                             "Default Table Name"          /* Table Name */



#define CREEP_DEFAULT_OBJ0 /*SensorId,    slope, offset, sampleCnt, sampleRate */\
                            "Unused", SENSOR_UNUSED, 0.0f,   0.0f,  0,  0, /*Row Sensor */\
                            "Unused", SENSOR_UNUSED, 0.0f,   0.0f,  0,  0, /*Col Sensor */\
                            ENGINERUN_UNUSED,    /* EngRun Id   */\
                            CREEP_TABLE_UNUSED,  /* Creep Tbl Id*/\
                            0,                   /* CPU Offset */\
                            1000,                /* Interval Rate (ms)*/\
                            1000,                   /* erTransFault_ms */\
                            "Unused Creep Object 0"

#define CREEP_DEFAULT_OBJ1 /*SensorId,    slope, offset, sampleCnt, sampleRate */\
                            "Unused", SENSOR_UNUSED, 0.0f,   0.0f,   0,   0, /*Row Sensor */\
                            "Unused", SENSOR_UNUSED, 0.0f,   0.0f,   0,   0, /*Col Sensor */\
                            ENGINERUN_UNUSED,    /* EngRun Id   */\
                            CREEP_TABLE_UNUSED,  /* Creep Tbl Id*/\
                            0,                   /* CPU Offset */\
                            1000,                /* Interval Rate (ms)*/\
                            1000,                   /* erTransFault_ms */\
                            "Unused Creep Object 1"


#define CREEP_DEFAULT_OBJ_T /*SensorId,    slope, offset, sampleCnt, sampleRate */\
                            "Unused", 0,  1.0f,   0.0f,      2,       100, /*Row Sensor */\
                            "Unused", 1,  1.0f,   0.0f,      2,       100, /*Col Sensor */\
                            ENGINERUN_UNUSED,    /* EngRun Id   */\
                            0,                   /* Creep Tbl Id*/\
                            0,                   /* CPU Offset */\
                            1000,                /* Interval Rate (ms) */\
                            1000,                   /* erTransFault_ms */\
                            "Unused Creep Object T"


#define CREEP_DEFAULT_CRC16   0x041E

#define CREEP_DEFAULT_CFG  STA_NORMAL,  /* PBITSysCond */\
                           STA_NORMAL,  /* CBITSysCond */\
                           FALSE,       /* bEnabled */\
                           0, 0, 0,     /* Pads 3 bytes */\
                           /*CREEP_DEFAULT_140A_PER_SEC_1E12,*/ /* Creep Tbl 140A Default */\
                           CREEP_DEFAULT_TBL,   /* Creep Tbl 0 */\
                           CREEP_DEFAULT_TBL,   /* Creep Tbl 1 */\
                           CREEP_DEFAULT_OBJ0,  /* Creep Obj 0 */\
                           CREEP_DEFAULT_OBJ1,  /* Creep Obj 1 */\
                           10000,               /* 10 sec Sensor Start Timeout */\
                           12,                  /* Base Units Creep Damage Counts 1E-12*/\
                           50,                  /* Max SensorLoss Log Recording */\
                           CREEP_DEFAULT_CRC16  /* CRC16, add actual val for default */


/******************************************************************************
                                 Package Typedefs
******************************************************************************/
typedef enum
{
  CREEP_1HZ         =  1,  /*  1Hz Rate */
  CREEP_2HZ         =  2,  /*  2Hz Rate */
  CREEP_4HZ         =  4,  /*  4Hz Rate */
  CREEP_5HZ         =  5,  /*  5Hz Rate */
  CREEP_10HZ        = 10,  /* 10Hz Rate */
  CREEP_20HZ        = 20,  /* 20Hz Rate */
  CREEP_50HZ        = 50,  /* 50Hz Rate */
  CREEP_100HZ       = 100  /* 100Hz Rate */
} CREEP_RATE;

typedef enum
{
  CREEP_STATUS_OK = 0,         // Creep Status is OK
  CREEP_STATUS_FAULTED_PBIT,   // Creep PBIT Faulted.  Creep processing disabled.
  // CREEP_STATUS_FAULTED_CBIT,   // Creep CBIT Faulted.  Current Sensor Failure.
  CREEP_STATUS_OK_DISABLED,    // Creep Status is OK, but processing cfg to be dsb
  CREEP_STATUS_MAX
} CREEP_STATUS_ENUM;

typedef enum
{
  CREEP_STATE_IDLE = 0,
  CREEP_STATE_ENGRUN,
  CREEP_STATE_FAULT_ER,
  CREEP_STATE_FAULT_IDLE,
  CREEP_STATE_MAX
} CREEP_STATE_ENUM;


// NOTE:  This enum list is also used as index into CREEP_PBIT_TYPE_TO_SYSLOG_ID[]
typedef enum {
  PBIT_OK,
  PBIT_FAIL_CFG_CRC,       // Creep Cfg CRC failed
  PBIT_FAIL_CRC_MISMATCH,  // Creep Cfg CRC != EE APP DATA, creep cfg changed ?
  PBIT_FAIL_EE_APP_DATA,   // Creep EE APP DATA restore failed.
  PBIT_FAIL_MAX            //    could be default creep restored as well.
} PBIT_FAIL_TYPE;


// NOTE:  This enum list is also used as index into CREEP_CBIT_TYPE_TO_SYSLOG_ID[]
typedef enum {
  CREEP_CBIT_OK,
  CREEP_FAULT_TYPE_SENSOR_LOSS,  // Creep processing interrupted by sensor loss
  CREEP_FAULT_TYPE_RESET,        // Creep processing interrupted by reset
  CREEP_FAULT_TYPE_MAX
} CREEP_FAULT_TYPE;




// The Configuration needs to be "packed" due to being CRC protected.  Don't
//   want random data in pad bytes to affect CRC calc.
#pragma pack(1)
typedef struct {
  FLOAT64 dCount[CREEP_MAX_ROWS][CREEP_MAX_COLS];  // "Damage/Sec" Count
  FLOAT64 rowVal[CREEP_MAX_ROWS];                // Row Sensor Values (i.e. ITT)
  FLOAT64 colVal[CREEP_MAX_COLS];                // Col Sensor Values (i.e. Np)
  CHAR name[CREEP_MAX_NAME];
} CREEP_TBL, *CREEP_TBL_PTR;

typedef struct {
  CHAR name[CREEP_MAX_NAME];
  UINT16 id;           // ID of sensor used for Row, Col
  FLOAT32 slope;       //  Slope conversion (if used) to convert Sensor
  FLOAT32 offset;      //  Offset conversion (if used) to convert Sensor
  UINT16 sampleCnt;    //  Expected sample count missed (at rate specified)
  UINT16 sampleRate_ms;//      before indicating sensor failure.
} CREEP_SENSOR, *CREEP_SENSOR_PTR;

typedef struct {
  CREEP_SENSOR sensorRow;  // Creep Sensor Row Definition
  CREEP_SENSOR sensorCol;  // Creep Sensor Col Definition
  UINT16 engId;            // Engine Run Id to assoc with this CREEP object
  UINT16 creepTblId;       // Creep Table to use for calculating creep for this obj
  UINT16 cpuOffset_ms;     // CPU Offset for calculating CREEP Interval
  UINT32 intervalRate_ms;     // Creep interval exe frame/rate
  UINT32 erTransFault_ms;     // Timeout used to determine fault if box reset during ER
                              //   or sensors selfed heal during ER.
  CHAR name[CREEP_MAX_NAME];  // Descriptive name for debug purposes
} CREEP_OBJECT, *CREEP_OBJECT_PTR;
// NOTE: For CESNA CARAVAN App, two creep obj will be defined. One for Ct and
//       one for Pt Blade creep monitoring.

typedef struct {
  FLT_STATUS sysCondPBIT;   // PBIT Sys Cond when PBIT fails
  FLT_STATUS sysCondCBIT;   // CBIT Sys Cond when CBIT fails
  BOOLEAN    bEnabled;      // Enable/Dsb of Creep processing. Consistent w/other objects
  UINT8      pads[3];       // creepTbl as to be 4 byte ALIGNED for code to work / Not ASSERT
  CREEP_TBL    creepTbl[CREEP_MAX_TBL];  // Creep Tables
  CREEP_OBJECT creepObj[CREEP_MAX_OBJ];  // Creep Object Definitions
  UINT32     sensorStartTime_ms;   // Sensor Startup timeout, before declaring sensor failure
  UINT16     baseUnits;     // Base units of damage counts in tables (def 1E12)
  UINT16     maxSensorLossRec;   // Limits the # of max SensorLossDetected log recording
  UINT16     crc16;         // CRC 16 of the data in CREEP_CFG, except for crc16 itself
} CREEP_CFG, *CREEP_CFG_PTR;
#pragma pack()



typedef struct {
  FLOAT64 creepLastMissionCnt;   // Last Mission Cnt
  FLOAT64 creepAccumCnt;     // Acc Creep Cnt since last reset
  FLOAT64 creepMissionCnt;  // Current Creep Cnt (Mission)
  FLOAT64 creepAccumCntTrashed;  // Acc Creep Cnt Thrown away due to fault
  UINT32 sensorFailures;   // Acc Total Sensor Failures since last reset (when creep
                           //    was INACTIVE).  Do not know if this is a CREEP FAILURE.
  UINT32 creepFailures;    // Acc Toatl Creep Failures since last reset (when creep
                           //    was ACTIVE only).
} CREEP_DATA, *CREEP_DATA_PTR;


typedef struct {
  CREEP_DATA data[CREEP_MAX_OBJ];
  // BOOLEAN bInUse;  // Flag to indicate if CREEP is in use.
  UINT16 crc16;       // Copy of crc 16 in the cfg.  Use to determine if cfg has changed.
} CREEP_APP_DATA, *CREEP_APP_DATA_PTR;

typedef struct {
  FLOAT64 accumCnt;
  FLOAT64 accumCntTrashed;
  FLOAT64 lastMissionCnt;
} CREEP_DATA_PERCENT, *CREEP_DATA_PERCENT_PTR;

typedef struct {
  FLOAT64 val[CREEP_MAX_SENSOR_VAL];   // Buffer of Sensor Values read
  UINT32 lastUpdate;                   // Time (in Tick) of last updated sensor value
  UINT16 cnt;                          // Cnt of #Sensor Values in buffer
  BOOLEAN bFailed;           // Sensor is current failed (data loss time exceeded)

  FLOAT32 rateTime;          // Estimated input rate of sensor
  UINT32 rateTimeBuff[CREEP_SENSOR_RATE_BUFF_SIZE];  // Buff of sensor update time
                                                     //   used to determine sensor rate
  UINT16 rateIndex;          // Index into rateTimeBuff[]
  UINT32 timeout_ms;         // Timeout of sensor as determined by cfg and input rate
                             //    to declare sensor failure.
  FLOAT64 peakVal;    // Last peak value use
  FLOAT64 currVal;    // Current value
} CREEP_SENSOR_DATA, *CREEP_SENSOR_DATA_PTR;


typedef struct {
  CREEP_SENSOR_DATA row;  // Row Sensor Data Values
  CREEP_SENSOR_DATA col;  // Col Sensor Data Values
  BOOLEAN bFailed;        // TRUE if either the Row or Col sensor has failed
} CREEP_SENSOR_VAL, *CREEP_SENSOR_VAL_PTR;




typedef struct   // Note: CPU Offset is implemented as count dn in other mods thus
{                //       will implement the same.
  INT16 nRateCountDown;
  INT16 nRateCounts;
} CREEP_INTERVAL, *CREEP_INTERVAL_PTR;


typedef struct {
  CREEP_STATUS_ENUM status; // _OK, _FAULT_PBIT, _DSB,
  CREEP_STATE_ENUM state;   // IDLE, ENGRUN, FAULT_ER, FAULT_IDLE
  INT16 nRateCountDown;     // CPU offset for 1 second
  CREEP_DATA data;
  CREEP_DATA_PERCENT data_percent;
  BOOLEAN bCntInterrupted;  // Internal flag used to indicate Cnt Interrupted
                            //   during prev operation
  UINT32 erTime_ms;         // Last ER Elapse Time in msec
  CREEP_INTERVAL interval;  // Creep Interval Setting
  UINT32 lastIdleTime_ms;   // Tick time of when Creep State was IDLE.  Used to determine
                            //   ER going directly to ER on startup or sensor self heal.
  UINT32 creepIncrement;    // Current 1 sec creep increment count, while creep mission is ACTIVE
} CREEP_STATUS, *CREEP_STATUS_PTR;

#pragma pack(1)
typedef struct {
  FLOAT64 creepMissionCnt[CREEP_MAX_OBJ];
  BOOLEAN creepInProgress[CREEP_MAX_OBJ];
} CREEP_APP_DATA_RTC, *CREEP_APP_DATA_RTC_PTR;
#pragma pack()


#pragma pack(1)
typedef struct {
  CREEP_DATA data;

  FLOAT32 rowInRate;          // Input rate determined for row sensor
  FLOAT32 colInRate;          // Input rate determined for col sensor
} CREEP_DIAG, *CREEP_DIAG_PTR;

typedef struct {
  UINT16 index;              // Echo of Index of Creep Object
//UINT8  pad[2];             // Pad to align log to UINT32

  FLOAT64 accumCntPcnt;      // Accumulated count in percent (0% - 200%)
  FLOAT64 accumCnt;          // Accumulated count (E-12 units)

  FLOAT64 lastMissionCntPcnt;    // Last Mission count in percent (0% - 200%)
  FLOAT64 lastMissionCnt;        // Last Mission count (E-12 units)

  CREEP_DIAG diag;   // Diagnostics data
} CREEP_SUMMARY_LOG, *CREEP_SUMMARY_LOG_PTR;

typedef struct {
  UINT16 index;      // Index of creep object (i.e. CT or PT blade)

  CREEP_FAULT_TYPE type;    // _RESET, _SENSOR_LOSS
  FLOAT64 missionCnt;       // Current mission count to be thrown away
  FLOAT64 creepAccumCntTrashed;  // Accum creep cnt thrown away from other Creep Failures
  UINT32 creepFailures;          // Accum creep faults since last reset
  BOOLEAN bRowSensorFailed;      // Failed state of row sensor
  BOOLEAN bColSensorFailed;      // Failed state of col sensor
//UINT8   pad[2];                // Pad to align to UINT32

  FLOAT64 accumCntPcnt;      // Accumulated count in percent (0% - 200%)
  FLOAT64 accumCnt;          // Accumulated count (E-12 units)

  FLOAT64 lastMissionCntPcnt;    // Last Mission count in percent (0% - 200%)
  FLOAT64 lastMissionCnt;        // Last Mission count (E-12 units)
} CREEP_FAULT_OBJ, *CREEP_FAULT_OBJ_PTR;

typedef struct {
  CREEP_FAULT_OBJ obj[CREEP_MAX_OBJ];
} CREEP_FAULT_LOG_SUMMARY, *CREEP_FAULT_LOG_SUMMARY_PTR;


typedef struct {
  UINT16 index;   // Index of creep object (i.e. CT or PT blade)
  BOOLEAN bRowSensorFailed;  // Failed state of row sensor
  BOOLEAN bColSensorFailed;  // Failed state of col sensor
} CREEP_SENSOR_FAIL_LOG, CREEP_SENSOR_FAIL_LOG_PTR;

typedef struct {
  PBIT_FAIL_TYPE type;  // _CFG_CRC_BAD, _CRC_MISMATCH, EE_APP_DATA
  UINT16 expCRC;  // CRC stored in CFG
  UINT16 calcCRC; // Calc CRC when CFG_BAD, or CRC in EE APP when _MISMATCH
  CREEP_SUMMARY_LOG data[CREEP_MAX_OBJ];
} CREEP_PBIT_LOG, *CREEP_PBIT_LOG_PTR;
#pragma pack()


#pragma pack(1)
typedef struct {
  CHAR name[CREEP_MAX_NAME];   // Creep Object Name
  UINT16 rowSensorId;          // Creep Object Row Sensor Id
  UINT16 colSensorId;          // Creep Object Col Sensor Id
} CREEP_OBJ_BINHDR, *CREEP_OBJ_BINHDR_PTR;

typedef struct {
  BOOLEAN bEnabled;                           // Creep Enb/Dsb
  UINT16 crc16;                               // CRC of Creep Cfg
  UINT16 baseUnits;                           // baseUnits from Creep Cfg
  CREEP_OBJ_BINHDR creepObj[CREEP_MAX_OBJ];   // Info on each Creep Obj
} CREEP_BINHDR, *CREEP_BINHDR_PTR;
#pragma pack()


typedef struct {
  UINT16 exp_crc;           //
  UINT16 nSensorFailedCnt;  // # Sensor Failed Cnt Logged
  UINT16 nCreepFailBuffCnt; // # Creep history buff entry
} CREEP_DEBUG, *CREEP_DEBUG_PTR;


typedef struct {
  SYS_APP_ID id;
  TIMESTAMP ts;
} CREEP_FAULT_BUFFER, *CREEP_FAULT_BUFFER_PTR;

#define CREEP_HISTORY_MAX 75

typedef struct {
  UINT16 cnt;      // Cnt of # buff entries
  TIMESTAMP ts;    // TS of most recent issue of creep.clear cmd
  CREEP_FAULT_BUFFER buff[CREEP_HISTORY_MAX];
} CREEP_FAULT_HISTORY;


/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( CREEP_BODY )
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
EXPORT void Creep_Initialize ( BOOLEAN degradedMode );

EXPORT CREEP_STATUS_PTR Creep_GetStatus (UINT8 index);
EXPORT CREEP_SENSOR_VAL_PTR Creep_GetSensor (UINT8 index);
EXPORT CREEP_CFG_PTR Creep_GetCfg (void);
EXPORT BOOLEAN Creep_FileInit ( void );
EXPORT UINT16 Creep_GetBinaryHdr ( void *pDest, UINT16 nMaxByteSize );
EXPORT CREEP_DEBUG_PTR Creep_GetDebug (void);
EXPORT void Creep_UpdateCreepXAppData (UINT16 index_obj);

EXPORT BOOLEAN Creep_FaultFileInit(void);
EXPORT void Creep_ClearFaultFile (TIMESTAMP *pTs);


#endif // CREEP_H


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: Creep.h $
 * 
 * *****************  Version 4  *****************
 * User: Peter Lee    Date: 12-12-09   Time: 6:39p
 * Updated in $/software/control processor/code/application
 * SCR #1195 Items 5,7a,8,10
 *
 * *****************  Version 3  *****************
 * User: Peter Lee    Date: 12-11-10   Time: 4:37p
 * Updated in $/software/control processor/code/application
 * Code Review Updates
 *
 * *****************  Version 2  *****************
 * User: Peter Lee    Date: 12-11-02   Time: 6:14p
 * Updated in $/software/control processor/code/application
 * SCR #1190 Additional Items to complete
 * Code Review Tool Updates
 *
 * *****************  Version 1  *****************
 * User: Peter Lee    Date: 12-10-27   Time: 5:09p
 * Created in $/software/control processor/code/application
 * SCR #1190 Creep Requirements
 *
 *
 *****************************************************************************/

