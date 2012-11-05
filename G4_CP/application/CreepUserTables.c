#define CREEP_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        CreepUserTables.c

    Description: Routines to support the user commands for Creep CSC

    VERSION
    $Revision: 2 $  $Date: 12-11-02 6:14p $

******************************************************************************/
#ifndef CREEP_BODY
#error CreepUserTables.c should only be included by Creep.c
#endif

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/


/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/
#include "CfgManager.h"


/*****************************************************************************/
/* Local Defines                                                             */
/*****************************************************************************/
#define CREEP_MAX_STR_LEN 256

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Status(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Sensor(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Cnt(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Val(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Cfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Table(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Object(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT CreepMsg_ObjRow(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT CreepMsg_ObjCol(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

USER_HANDLER_RESULT CreepMsg_Clear(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

USER_HANDLER_RESULT CreepMsg_DCount(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);


USER_HANDLER_RESULT CreepMsg_DebugTbl(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

USER_HANDLER_RESULT CreepMsg_UserMessageRecentAll(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr);

static CHAR *Creep_GetHistoryBuffEntry ( UINT8 index );


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static CREEP_STATUS CreepStatusTemp;
static CREEP_CFG CreepCfgTemp;
static CREEP_TBL CreepCfgTblTemp;
static CREEP_OBJECT CreepCfgObjTemp;
static CREEP_SENSOR CreepCfgObjSenTemp;
static CREEP_DEBUG CreepDebugTemp;
static CREEP_SENSOR_VAL CreepSensorTemp;

// Debug Timing
UINT32 startHSTick, endHSTick, diffHSTick;


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
USER_ENUM_TBL CreepStatusStrs[] =
{
  {"OK",          CREEP_STATUS_OK},
  {"FAULT_PBIT",  CREEP_STATUS_FAULTED_PBIT},
  {"OK_DISABLED", CREEP_STATUS_OK_DISABLED},
  { NULL,          0}
};

USER_ENUM_TBL CreepStateStrs[] =
{
  {"IDLE",         CREEP_STATE_IDLE},
  {"ACTIVE_ER",    CREEP_STATE_ENGRUN},
  {"FAULTED_ER",   CREEP_STATE_FAULT_ER},
  {"FAULTED_IDLE", CREEP_STATE_FAULT_IDLE},
  { NULL,           0}
};

USER_ENUM_TBL CreepRateTypes[] =
{
  { "1HZ"    , CREEP_1HZ    },
  { "2HZ"    , CREEP_2HZ    },
  { "4HZ"    , CREEP_4HZ    },
  { "5HZ"    , CREEP_5HZ    },
  { "10HZ"   , CREEP_10HZ   },
  { "20HZ"   , CREEP_20HZ   },
  { "50HZ"   , CREEP_50HZ   },
  { "100HZ"  , CREEP_100HZ  },
  { NULL     , 0            }
};


// Level 2
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned
USER_MSG_TBL CreepTableTbl[] =
{
  {"NAME", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_STR,  USER_RW,
                  (void *) &CreepCfgTblTemp.name,  0, CREEP_MAX_TBL-1, 0, CREEP_MAX_NAME, NULL},

// Row Values
  {"ROW_VAL0", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[0],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL1", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[1],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL2", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[2],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL3", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[3],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL4", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[4],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL5", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[5],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL6", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[6],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL7", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[7],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL8", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[8],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL9", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[9],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL10", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[10],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL11", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[11],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL12", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[12],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL13", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[13],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL14", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[14],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL15", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[15],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL16", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[16],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL17", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[17],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL18", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[18],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"ROW_VAL19", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.RowVal[19],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

// Column Values
  {"COL_VAL0", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[0],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL1", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[1],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL2", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[2],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL3", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[3],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL4", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[4],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL5", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[5],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL6", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[6],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL7", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[7],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL8", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[8],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"COL_VAL9", NO_NEXT_TABLE, CreepMsg_Table, USER_TYPE_FLOAT64,  USER_RW,
               (void *) &CreepCfgTblTemp.ColVal[9],  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

// Damage Count Values
  {"DCOUNT_R0",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 0,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R1",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 1,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R2",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 2,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R3",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 3,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R4",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 4,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R5",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 5,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R6",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 6,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R7",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 7,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R8",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 8,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R9",   NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 9,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R10",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 10,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R11",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 11,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R12",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 12,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R13",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 13,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R14",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 14,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R15",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 15,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R16",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 16,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R17",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 17,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R18",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 18,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {"DCOUNT_R19",  NO_NEXT_TABLE, CreepMsg_DCount, USER_TYPE_STR,  USER_RW,
                  (void *) 19,  0, CREEP_MAX_TBL-1, NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned
USER_MSG_TBL CreepObjectSenRowTbl[] =
{
  {"NAME",      NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_STR,  USER_RW,
              (void *) &CreepCfgObjSenTemp.name,    0, CREEP_MAX_OBJ-1, 0, CREEP_MAX_NAME, NULL},

  {"ID",      NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_UINT16,  USER_RW,
              (void *) &CreepCfgObjSenTemp.Id,    0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SLOPE",   NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_FLOAT,  USER_RW,
              (void *) &CreepCfgObjSenTemp.slope, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"OFFSET",  NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_FLOAT,  USER_RW,
              (void *) &CreepCfgObjSenTemp.offset, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SAMPLECNT",  NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_UINT16,  USER_RW,
                 (void *) &CreepCfgObjSenTemp.sampleCnt,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SAMPLERATE_MS", NO_NEXT_TABLE, CreepMsg_ObjRow, USER_TYPE_UINT16,  USER_RW,
                    (void *) &CreepCfgObjSenTemp.sampleRate_ms, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned
USER_MSG_TBL CreepObjectSenColTbl[] =
{
  {"NAME",      NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_STR,  USER_RW,
              (void *) &CreepCfgObjSenTemp.name,    0, CREEP_MAX_OBJ-1, 0, CREEP_MAX_NAME, NULL},

  {"ID",      NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_UINT16,  USER_RW,
              (void *) &CreepCfgObjSenTemp.Id,    0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SLOPE",   NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_FLOAT,  USER_RW,
              (void *) &CreepCfgObjSenTemp.slope, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"OFFSET",  NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_FLOAT,  USER_RW,
              (void *) &CreepCfgObjSenTemp.offset, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SAMPLECNT",  NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_UINT16,  USER_RW,
                 (void *) &CreepCfgObjSenTemp.sampleCnt,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SAMPLERATE_MS", NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_UINT16,  USER_RW,
                    (void *) &CreepCfgObjSenTemp.sampleRate_ms, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned
USER_MSG_TBL CreepObjectTbl[] =
{
  {"NAME",    NO_NEXT_TABLE, CreepMsg_Object, USER_TYPE_STR, USER_RW,
              (void *) &CreepCfgObjTemp.name,  0, CREEP_MAX_OBJ-1,  0, CREEP_MAX_NAME, NULL},

  {"ENG_ID",  NO_NEXT_TABLE, CreepMsg_Object, USER_TYPE_UINT16,  USER_RW,
              (void *) &CreepCfgObjTemp.EngId,        0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"TBL_ID",  NO_NEXT_TABLE, CreepMsg_Object, USER_TYPE_UINT16,  USER_RW,
              (void *) &CreepCfgObjTemp.CreepTblId,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"RATEOFFSET_MS", NO_NEXT_TABLE, CreepMsg_Object, USER_TYPE_UINT16,  USER_RW,
              (void *) &CreepCfgObjTemp.CPUOffset_ms, 0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"RATE_ms", NO_NEXT_TABLE, CreepMsg_Object, USER_TYPE_ENUM,  USER_RW,
              (void *) &CreepCfgObjTemp.IntervalRate, 0, CREEP_MAX_OBJ-1, NO_LIMIT, CreepRateTypes},

  {"SensorRow", CreepObjectSenRowTbl, NULL,  NO_HANDLER_DATA},

  {"SensorCol", CreepObjectSenColTbl, NULL,  NO_HANDLER_DATA},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned.


USER_MSG_TBL CreepSensorTbl[] =
{
  {"ROW_FAULTED",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_BOOLEAN,  USER_RO,
                  (void *) &CreepSensorTemp.row.bFailed,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"ROW_RATE_MS",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT,  USER_RO,
                    (void *) &CreepSensorTemp.row.rateTime,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"ROW_TIMEOUT_MS", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_UINT32,  USER_RO,
                    (void *) &CreepSensorTemp.row.timeout_ms,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"ROW_CURR_VAL", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,
                    (void *) &CreepSensorTemp.row.currVal,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"ROW_PEAK_VAL", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,
                    (void *) &CreepSensorTemp.row.peakVal,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},


  {"COL_FAULTED",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_BOOLEAN,  USER_RO,
                  (void *) &CreepSensorTemp.col.bFailed,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"COL_RATE_MS",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT,  USER_RO,
                    (void *) &CreepSensorTemp.col.rateTime,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"COL_TIMEOUT_MS", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_UINT32,  USER_RO,
                    (void *) &CreepSensorTemp.col.timeout_ms,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"COL_CURR_VAL", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,
                    (void *) &CreepSensorTemp.col.currVal,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"COL_PEAK_VAL", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,
                    (void *) &CreepSensorTemp.col.peakVal,   0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL CreepCntTbl[] =
{
  {"MISSION",     NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data.creepMissionCnt,   0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"ACCUM",       NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data.creepAccumCnt,   0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"ACCUM_REJECT",NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data.creepAccumCntTrashed,  0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"L_MISSION",   NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data.creepLastMissionCnt, 0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"SENSOR_FAILURES",  NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_UINT32,  USER_RW,
                  (void *) &CreepStatusTemp.data.sensorFailures,   0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"CREEP_FAILURES",  NO_NEXT_TABLE, CreepMsg_Cnt, USER_TYPE_UINT32,  USER_RW,
                  (void *) &CreepStatusTemp.data.creepFailures,      0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL CreepValTbl[] =
{
  {"ACCUM_PCNT", NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data_percent.AccumCnt,       0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"ACCUM_REJECT_PCNT", NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RW,
                  (void *) &CreepStatusTemp.data_percent.AccumCntTrashed, 0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {"LAST_MISSION_PCNT", NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RO,
                  (void *) &CreepStatusTemp.data_percent.LastMissionCnt, 0, CREEP_MAX_OBJ-1,
                  NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


// Level 1

#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned
USER_MSG_TBL CreepCfgTbl[] =
{
  {"PBITSYSCOND", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_ENUM,  USER_RW,
                (void *) &CreepCfgTemp.PBITSysCond,   -1,   -1,   NO_LIMIT, Flt_UserEnumStatus},

  {"CBITSYSCOND", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_ENUM,  USER_RW,
                (void *) &CreepCfgTemp.CBITSysCond,   -1,   -1,   NO_LIMIT, Flt_UserEnumStatus},

  {"ENABLED", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_BOOLEAN,  USER_RW,
                (void *) &CreepCfgTemp.bEnabled,      -1,   -1,   NO_LIMIT, NULL},

  {"TABLE",  CreepTableTbl,  NULL,  NO_HANDLER_DATA},

  {"OBJECT", CreepObjectTbl, NULL,  NO_HANDLER_DATA},

  {"BASEUNITS", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_UINT16,  USER_RW,
               (void *) &CreepCfgTemp.baseUnits,      -1,   -1,   NO_LIMIT, NULL},

  {"SENSOR_START_MS", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_UINT32,  USER_RW,
                      (void *) &CreepCfgTemp.sensorStartTime_ms,  -1,   -1,   NO_LIMIT, NULL},

  {"CRC", NO_NEXT_TABLE, CreepMsg_Cfg,  USER_TYPE_HEX16,  USER_RW,
                (void *) &CreepCfgTemp.crc16,         -1,   -1,   NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere to ensure if not aligned.


USER_MSG_TBL CreepStatusTbl[] =
{
  {"FAULT",     NO_NEXT_TABLE, CreepMsg_Status, USER_TYPE_ENUM,  USER_RO,
                (void *) &CreepStatusTemp.status,    0, CREEP_MAX_OBJ-1, NO_LIMIT, CreepStatusStrs},

  {"STATE",     NO_NEXT_TABLE, CreepMsg_Status,  USER_TYPE_ENUM,  USER_RO,
                (void *) &CreepStatusTemp.state,     0, CREEP_MAX_OBJ-1, NO_LIMIT, CreepStateStrs},

  {"ER_TIME_ms",  NO_NEXT_TABLE, CreepMsg_Status,  USER_TYPE_UINT32,  USER_RO,
                  (void *) &CreepStatusTemp.erTime_ms,     0, CREEP_MAX_OBJ-1, NO_LIMIT, NULL},

  {"SENSOR",    CreepSensorTbl,  NULL,  NO_HANDLER_DATA},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

USER_MSG_TBL CreepPersistTbl[] =
{
  {"CNT",     CreepCntTbl,     NULL,  NO_HANDLER_DATA},
  {"PCNT",    CreepValTbl,     NULL,  NO_HANDLER_DATA},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


USER_MSG_TBL CreepDebugTbl[] =
{
  {"EXP_CRC", NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_HEX16,  USER_RW,
                  (void *) &CreepDebugTemp.exp_crc, -1,  -1, NO_LIMIT, NULL},

  {"SENSOR_LOG_CNT", NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_UINT16,  USER_RW,
                  (void *) &CreepDebugTemp.nSensorFailedCnt, -1,  -1, NO_LIMIT, NULL},

  {"CREEP_HISTORY_CNT", NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_UINT16,  USER_RW,
                  (void *) &CreepDebugTemp.nCreepFailBuffCnt, -1,  -1, NO_LIMIT, NULL},

  {NULL,NULL,NULL,NO_HANDLER_DATA}
};



// Root Level

USER_MSG_TBL CreepRoot[] =
{
  {"STATUS",  CreepStatusTbl,  NULL,  NO_HANDLER_DATA},
  {"CFG",     CreepCfgTbl,     NULL,  NO_HANDLER_DATA},
//{"CNT",     CreepCntTbl,     NULL,  NO_HANDLER_DATA},
//{"VALUE",   CreepValTbl,     NULL,  NO_HANDLER_DATA},
  {"PERSIST", CreepPersistTbl, NULL,  NO_HANDLER_DATA},
  {"DEBUG",   CreepDebugTbl,   NULL,  NO_HANDLER_DATA},

  {"CLEAR",      NO_NEXT_TABLE,  CreepMsg_Clear,                USER_TYPE_STR,
                 USER_WO,               NULL,   -1, -1, NO_LIMIT,   NULL},
  {"RECENTALL",  NO_NEXT_TABLE,  CreepMsg_UserMessageRecentAll, USER_TYPE_ACTION,
                 (USER_RO|USER_NO_LOG), NULL,   -1, -1, NO_LIMIT,   NULL},

  {NULL,      NULL,            NULL,  NO_HANDLER_DATA}
};


USER_MSG_TBL CreepRootTblPtr = {"CREEP",CreepRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
void CreepMsg_UpdateCfg ( void );


/*****************************************************************************/
/* Public Functions                                                          */
/*****************************************************************************/


/*****************************************************************************/
/* Local Functions                                                           */
/*****************************************************************************/

/******************************************************************************
 * Function:    CreepMsg_Status
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Status
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Status(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  CreepStatusTemp = *Creep_GetStatus(Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Sensor
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Sensor
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Sensor(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  CreepSensorTemp = *Creep_GetSensor(Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Cnt
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Data
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Cnt(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  CreepStatusTemp = *Creep_GetStatus(Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
  // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime version)
  //       is then copied to AppData.  Executing this cmd during creep processing will/could
  //       force early update of App Data. If Fault deteced, then some cnt would have been committed
  //       to App Data. Normally when creep fault detected, the cnt for that current flight is
  //       "thrown" away.
  if (SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Update Status counts only
    Creep_GetStatus(Index)->data = CreepStatusTemp.data;

    // Update EEPROM APP Data
    Creep_UpdateCreepXAppData(Index);

    // Update the Data Percent here.  Normally Data precent update in init(), and @ end of ER.
    //    But if the cnt values are updated, should update % here
    Creep_GetStatus(Index)->data_percent.AccumCnt = CreepStatusTemp.data.creepAccumCnt
                                                    / m_Creep_100_pcnt;

    Creep_GetStatus(Index)->data_percent.AccumCntTrashed =
                          CreepStatusTemp.data.creepAccumCntTrashed / m_Creep_100_pcnt;

    Creep_GetStatus(Index)->data_percent.LastMissionCnt =
                          CreepStatusTemp.data.creepLastMissionCnt / m_Creep_100_pcnt;
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Val
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Data in Percent
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Val(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;
  CREEP_DATA_PERCENT CreepPercentPrev;


  result = USER_RESULT_OK;

  CreepStatusTemp = *Creep_GetStatus(Index);
  CreepPercentPrev = CreepStatusTemp.data_percent;


  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
  // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime version)
  //       is then copied to AppData.  Executing this cmd during creep processing will/could
  //       force early update of App Data. If Fault deteced, then some cnt would have been committed
  //       to App Data. Normally when creep fault detected, the cnt for that current flight is
  //       "thrown" away.
  if (SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Need to convert data_percent to count values, only of the data_percent val that has changed
    if ( fabs (CreepPercentPrev.AccumCnt - CreepStatusTemp.data_percent.AccumCnt) >=
         DBL_EPSILON )
    {
      // Update associated counts here
      CreepStatusTemp.data.creepAccumCnt = CreepStatusTemp.data_percent.AccumCnt
                                           * m_Creep_100_pcnt;

      // Recompute percentage.  Note: this value might be different than the val entered due
      //    to error assoc with DOUBLE representation of real numbers.
      Creep_GetStatus(Index)->data_percent.AccumCnt = CreepStatusTemp.data.creepAccumCnt /
                                                         m_Creep_100_pcnt;
    }

    if ( fabs (CreepPercentPrev.AccumCntTrashed - CreepStatusTemp.data_percent.AccumCntTrashed) >=
         DBL_EPSILON )
    {
      // Update associated counts here
      CreepStatusTemp.data.creepAccumCntTrashed = CreepStatusTemp.data_percent.AccumCntTrashed
                                                  * m_Creep_100_pcnt;

      // Recompute percentage.  Note: this value might be different than the val entered due
      //    to error assoc with DOUBLE representation of real numbers.
      Creep_GetStatus(Index)->data_percent.AccumCntTrashed =
               CreepStatusTemp.data.creepAccumCntTrashed / m_Creep_100_pcnt;
    }

    // Update Status counts only
    Creep_GetStatus(Index)->data = CreepStatusTemp.data;

    // Update EEPROM APP Data
    Creep_UpdateCreepXAppData(Index);
  }

  return result;
}




/******************************************************************************
 * Function:    CreepMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Cfg
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Cfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  CreepCfgTemp = *Creep_GetCfg();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    CreepMsg_UpdateCfg();
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Cfg
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Table
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Table(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  CreepCfgTemp = *Creep_GetCfg();
  CreepCfgTblTemp = *(CREEP_TBL_PTR) &(CreepCfgTemp.creepTbl[Index]);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *(CREEP_TBL_PTR) &(CreepCfgTemp.creepTbl[Index]) = CreepCfgTblTemp;
    CreepMsg_UpdateCfg();
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Object
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Object
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Object(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  CreepCfgTemp = *Creep_GetCfg();
  CreepCfgObjTemp = *(CREEP_OBJECT_PTR) &(CreepCfgTemp.creepObj[Index]);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *(CREEP_OBJECT_PTR) &(CreepCfgTemp.creepObj[Index]) = CreepCfgObjTemp;
    CreepMsg_UpdateCfg();
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_ObjRow
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Object Sensor Row
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_ObjRow(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  CreepCfgTemp = *Creep_GetCfg();
  CreepCfgObjSenTemp = *(CREEP_SENSOR_PTR) &(CreepCfgTemp.creepObj[Index].SensorRow);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *(CREEP_SENSOR_PTR) &(CreepCfgTemp.creepObj[Index].SensorRow) = CreepCfgObjSenTemp;
    CreepMsg_UpdateCfg();
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_ObjCol
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Object Sensor Col
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_ObjCol(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;


  result = USER_RESULT_OK;

  CreepCfgTemp = *Creep_GetCfg();
  CreepCfgObjSenTemp = *(CREEP_SENSOR_PTR) &(CreepCfgTemp.creepObj[Index].SensorCol);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    *(CREEP_SENSOR_PTR) &(CreepCfgTemp.creepObj[Index].SensorCol) = CreepCfgObjSenTemp;
    CreepMsg_UpdateCfg();
  }

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Clear
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Clears the Creep EEPROM APP DATA Cnts.
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_Clear(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result;
  UINT16 i;


  result = USER_RESULT_ERROR;

  if(strncmp(SetPtr,"really",sizeof("really")) == 0)
  {
    result = USER_RESULT_OK;
    GSE_DebugStr(NORMAL,TRUE, "All Creep Data Cleared");

    memset ( (void *) &CreepStatusTemp.data, 0, sizeof(CREEP_DATA) );

    // Clear all Creep Object EEPROM APP Data
    for (i=0;i<CREEP_MAX_OBJ;i++)
    {
      Creep_GetStatus(i)->data = CreepStatusTemp.data;
      Creep_UpdateCreepXAppData(i);    // Commit to EE APP
      // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
      // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime version)
      //       is then copied to AppData.  Executing this cmd during creep processing will/could
      //       force early update of App Data. If Fault deteced, then some cnt would have been committed
      //       to App Data. Normally when creep fault detected, the cnt for that current flight is
      //       "thrown" away.

      // Update the Data Percent here.  Normally Data precent update in init(), and @ end of ER.
      //    But if the cnt values are updated, should update % here
      Creep_GetStatus(i)->data_percent.AccumCnt = 0;
      Creep_GetStatus(i)->data_percent.AccumCntTrashed = 0;
      Creep_GetStatus(i)->data_percent.LastMissionCnt = 0;
    }

    Creep_FaultFileInit();  // Clear Creep Fault Recent buffer
    Creep_GetDebug()->nSensorFailedCnt = 0;
    Creep_GetDebug()->nCreepFailBuffCnt = 0;
  } // End if "really" is enterred

  if(strncmp(SetPtr,"flags",sizeof("flags")) == 0)
  {
    result = USER_RESULT_OK;
    GSE_DebugStr(NORMAL,TRUE, "Creep Flags Cleared");


    // Clear all Creep Object EEPROM APP Data
    for (i=0;i<CREEP_MAX_OBJ;i++)
    {
      // Get Current values
      CreepStatusTemp.data = Creep_GetStatus(i)->data;

      // Update diagnostic counts only
      CreepStatusTemp.data.creepAccumCntTrashed = 0;
      CreepStatusTemp.data.sensorFailures = 0;
      CreepStatusTemp.data.creepFailures = 0;

      // Update modified values
      Creep_GetStatus(i)->data = CreepStatusTemp.data;
      Creep_UpdateCreepXAppData(i);  // Commit to EE APP
    }

    Creep_FaultFileInit();  // Clear Creep Fault Recent buffer
    Creep_GetDebug()->nSensorFailedCnt = 0;
    Creep_GetDebug()->nCreepFailBuffCnt = 0;
  } // End if "flags" is entered

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_DCount
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              View / Update the current Creep Damage Count values
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_DCount(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;
  UINT16 i;
  char *strCount[CREEP_MAX_COLS];  // There should be exactly 10 counts
  char *ptr;
  char msg[CREEP_MAX_STR_LEN];
  static char msg_out[CREEP_MAX_STR_LEN];


  result = USER_RESULT_ERROR;

  CreepCfgTemp = *Creep_GetCfg();
  CreepCfgTblTemp = *(CREEP_TBL_PTR) &(CreepCfgTemp.creepTbl[Index]);

  if(SetPtr != NULL)
  {
    // Update Damage Counts
    // Need to copy from SetPtr as it is const void and strtok below will modified input
    strncpy (msg, SetPtr, CREEP_MAX_STR_LEN);

    ptr = msg;
    i = 0;
    strCount[i++] = strtok( ptr, "," );
    while ( (strCount != NULL) && (i < CREEP_MAX_COLS) )
    {
      strCount[i] = strtok(NULL, ",");
      i++;
    }

    if ( (strtok(NULL, ",") == NULL) && (i == CREEP_MAX_COLS) )
    {
      // We have parse exactly 10 values.  If >10 or <10, then will ret ERR.
      ptr = strCount[0];
      for (i=0;i<CREEP_MAX_COLS;i++)
      {
        if (ptr == strCount[i])
        {
          // Convert the string to double
          CreepCfgTblTemp.dCount[Param.Int][i] = strtod ( (const char* )
                           strCount[i], &ptr );
          // Note: ptr point to NULL between values ("," removed by strtok()),
          //       move ptr by one.
          ptr++;
        }
        else
        {
          // Could not parse the value entirely.  There must have been an error.
          //  strtod() updates 'ptr' to point to
          break;
        }
      } // end for loop thru all 10 col convert

      if (i == CREEP_MAX_COLS)
      {
        // We parse 10 value successfully
        result = USER_RESULT_OK;

        // Update Creep Cfg.  Update damage count fields only.
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
        memcpy ( (void *) &(CreepCfgTemp.creepTbl[Index].dCount[Param.Int][0]),
                 (void *) &CreepCfgTblTemp.dCount[Param.Int][0], sizeof(FLOAT64) *
                  CREEP_MAX_COLS );
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() to ensure if not aligned
        CreepMsg_UpdateCfg();
      }
      // else result will remain USER_RESULT_ERROR
    } // end of if we have exactly 10 values


  }
  else
  {

// Debug Timing
   startHSTick = TTMR_GetHSTickCount();
// Debug Timing

    // Read Damage Counts
    msg_out[0] = NULL;
    for (i=0;i<(CREEP_MAX_COLS-1);i++)
    {
      snprintf(msg,CREEP_MAX_STR_LEN,"%0.0lf,", CreepCfgTblTemp.dCount[Param.Int][i]);
      strcat (msg_out, (const char *) msg);
    }
    // No ',' at the end
    snprintf(msg,CREEP_MAX_STR_LEN,"%0.0lf", CreepCfgTblTemp.dCount[Param.Int][i]);
    strcat (msg_out, (const char *) msg);

    *GetPtr = msg_out;

    result = USER_RESULT_OK;

// Debug Timing
   endHSTick = TTMR_GetHSTickCount();
   diffHSTick = endHSTick - startHSTick;
// Debug Timing


  }

  return result;

}


/******************************************************************************
 * Function:    CreepMsg_UserMessageRecentAll
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep fault history entries
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_UserMessageRecentAll(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr)
{
  UINT8 i;
  static CHAR ResultBuffer[USER_SINGLE_MSG_MAX_SIZE];
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  CHAR *pStr;
  UINT32 nOffset, len;


  pStr = (CHAR *) &ResultBuffer[0];

  // Output # of faults in buffer
  sprintf ( ResultBuffer, "\r\nCreep Faults = %d\r\n", m_creepHistory.cnt );
  len = strlen ( ResultBuffer );
  nOffset = len;

  for (i=0;i<CREEP_HISTORY_MAX;i++)
  {
    pStr = Creep_GetHistoryBuffEntry(i);
    len = strlen (pStr);

    memcpy ( (void *) &ResultBuffer[nOffset], pStr, len );
    nOffset += len;
  }

  ResultBuffer[nOffset] = NULL;
  User_OutputMsgString(ResultBuffer, FALSE);

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_DebugTbl
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the latest Creep Debug
 *
 * Parameters:   [in] DataType:  C type of the data to be read or changed, used
 *                               for casting the data pointers
 *               [in/out] Param: Pointer to the configuration item to be read
 *                               or changed
 *               [in] Index:     Index parameter is used to reference the
 *                               specific sensor to change.  Range is validated
 *                               by the user manager
 *               [in] SetPtr:    For write commands, a pointer to the data to
 *                               write to the configuration.
 *               [out] GetPtr:   For read commands, UserCfg function will set
 *                               this to the location of the data requested.
 *
 * Returns:     USER_RESULT_OK:    Processed sucessfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
USER_HANDLER_RESULT CreepMsg_DebugTbl(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  CreepDebugTemp = *Creep_GetDebug();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}

/******************************************************************************
 * Function:    CreepMsg_UpdateCfg
 *
 * Description: Generic Function to Update the Creep Cfg
 *
 * Parameters:  None
 *
 * Returns:     None
 *
 * Notes:
 *
*****************************************************************************/
void CreepMsg_UpdateCfg ( void )
{
  *Creep_GetCfg() = CreepCfgTemp;     // Update current cfg to new setting

  memcpy(&CfgMgr_ConfigPtr()->CreepConfig, &CreepCfgTemp,
         sizeof(CREEP_CFG));         // Update EEPROM cfg version

  //Store the modified temporary structure in the EEPROM.
  CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(),&CfgMgr_ConfigPtr()->CreepConfig,
                         sizeof(CREEP_CFG));
}


/******************************************************************************
* Function:     Creep_GetHistoryBuffEntry
*
* Description:  Returns the selected Fault Entry from the non-volatile memory
*
* Parameters:   [in] Index:     Index parameter selecting specified entry
*
* Returns:      [out] ptr to RecentLogStr[] containing decoded entry
*
* Notes:        none
*
*****************************************************************************/
static CHAR *Creep_GetHistoryBuffEntry ( UINT8 index )
{
  CHAR *pStr;
  TIMESTRUCT Ts;
  static CHAR  RecentLogStr[256];


  if ( index < m_creepHistory.cnt )
  {
    CM_ConvertTimeStamptoTimeStruct( &m_creepHistory.buff[index].Ts, &Ts);

    // Stringify the Log data
    snprintf( RecentLogStr, 80, "%02d:%s(%x) at %02d:%02d:%02d %02d/%02d/%4d\r\n",
        index+1,
        SystemLogIDString( m_creepHistory.buff[index].Id),
        m_creepHistory.buff[index].Id,
        Ts.Hour,
        Ts.Minute,
        Ts.Second,
        Ts.Month,
        Ts.Day,
        Ts.Year);

    // Return log data string
    pStr = RecentLogStr;
  }
  else
  {
    snprintf( RecentLogStr, 80, "%02d:Fault entry empty\r\n", index );
    pStr = RecentLogStr;
  }

  return pStr;
}


/*****************************************************************************
 *  MODIFICATIONS
 *    $History: CreepUserTables.c $
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
