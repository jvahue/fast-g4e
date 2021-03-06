#define CREEP_USERTABLES_BODY
/******************************************************************************
            Copyright (C) 2007-2015 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.
    
    ECCN:        9D991

    File:        CreepUserTables.c

    Description: Routines to support the user commands for Creep CSC

    VERSION
    $Revision: 14 $  $Date: 1/28/15 2:11p $

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
#define CREEP_MAX_RECENT_DATE_TIME 20  // "HH:MM:SS MM/DD/YYYY"
#define CREEP_MAX_VALUE_CHAR_ARRAY 32
#define CREEP_MAX_LABEL_CHAR_ARRAY 128

/*****************************************************************************/
/* Local Typedefs                                                            */
/*****************************************************************************/
static
USER_HANDLER_RESULT CreepMsg_Status(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Sensor(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Cnt(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Val(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Cfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Table(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Object(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_ObjRow(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_ObjCol(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_Clear(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_DCount(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_DebugTbl(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_UserMessageRecentAll(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr);

static
USER_HANDLER_RESULT CreepMsg_UserMessageRecentUpdate(USER_DATA_TYPE DataType,
                                                     USER_MSG_PARAM Param,
                                                     UINT32 Index,
                                                     const void *SetPtr,
                                                     void **GetPtr);

static CHAR *Creep_GetHistoryBuffEntry ( UINT8 index );

static USER_HANDLER_RESULT Creep_ShowConfig(USER_DATA_TYPE DataType,
                                     USER_MSG_PARAM Param,
                                     UINT32 Index,
                                     const void *SetPtr,
                                     void **GetPtr);


/*****************************************************************************/
/* Local Variables                                                           */
/*****************************************************************************/
static CREEP_STATUS creepStatusTemp;
static CREEP_CFG creepCfgTemp;
static CREEP_TBL creepCfgTblTemp;
static CREEP_OBJECT creepCfgObjTemp;
static CREEP_SENSOR creepCfgObjSenTemp;
static CREEP_DEBUG creepDebugTemp;
static CREEP_SENSOR_VAL creepSensorTemp;

// Debug Timing
static UINT32 startHSTick, endHSTick, diffHSTick;


/*****************************************************************************/
/* Local Constants                                                           */
/*****************************************************************************/
static
USER_ENUM_TBL creepStatusStrs[] =
{
  {"OK",          CREEP_STATUS_OK},
  {"FAULT_PBIT",  CREEP_STATUS_FAULTED_PBIT},
  {"OK_DISABLED", CREEP_STATUS_OK_DISABLED},
  { NULL,          0}
};

static
USER_ENUM_TBL creepStateStrs[] =
{
  {"IDLE",         CREEP_STATE_IDLE},
  {"ACTIVE_ER",    CREEP_STATE_ENGRUN},
  {"FAULTED_ER",   CREEP_STATE_FAULT_ER},
  {"FAULTED_IDLE", CREEP_STATE_FAULT_IDLE},
  { NULL,           0}
};

static
USER_ENUM_TBL creepTableIdEnum[] =
{
    {"0", CREEP_TBL_0},
    {"1", CREEP_TBL_1},
    {"UNUSED", CREEP_TBL_UNUSED },
  { NULL     , 0            }
};

// Level 2
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned
static
USER_MSG_TBL creepTableTbl[] =
{
   /*Str           Next Tbl Ptr   Handler Func.       Data Type           Access          Parameter                          IndexRange       DataLimit          EnumTbl*/

  {"NAME",         NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_STR,      USER_RW,   (void *) &creepCfgTblTemp.name[0],     0, CREEP_MAX_TBL-1,  0, CREEP_MAX_NAME, NULL},
// Row Values
  {"ROW_VAL0",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[0],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL1",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[1],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL2",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[2],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL3",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[3],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL4",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[4],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL5",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[5],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL6",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[6],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL7",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[7],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL8",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[8],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL9",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[9],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL10",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[10],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL11",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[11],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL12",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[12],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL13",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[13],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL14",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[14],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL15",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[15],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL16",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[16],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL17",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[17],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL18",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[18],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"ROW_VAL19",    NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.rowVal[19],  0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
// Column Values
  {"COL_VAL0",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[0],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL1",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[1],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL2",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[2],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL3",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[3],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL4",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[4],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL5",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[5],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL6",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[6],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL7",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[7],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL8",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[8],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"COL_VAL9",     NO_NEXT_TABLE, CreepMsg_Table,   USER_TYPE_FLOAT64,   USER_RW,  (void *) &creepCfgTblTemp.colVal[9],   0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
// Damage Count Values
  {"DCOUNT_R0",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 0,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R1",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 1,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R2",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 2,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R3",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 3,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R4",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 4,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R5",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 5,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R6",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 6,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R7",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 7,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R8",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 8,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R9",    NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 9,                            0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R10",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 10,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R11",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 11,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R12",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 12,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R13",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 13,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R14",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 14,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R15",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 15,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R16",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 16,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R17",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 17,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R18",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 18,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  {"DCOUNT_R19",   NO_NEXT_TABLE, CreepMsg_DCount,  USER_TYPE_STR,       USER_RW,  (void *) 19,                           0, CREEP_MAX_TBL-1,  NO_LIMIT,          NULL},
  { NULL,          NULL,          NULL,             NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned
static
USER_MSG_TBL creepObjectSenRowTbl[] =
{ /*Str              Next Tbl Ptr    Handler Func.        Data Type        Access         Parameter                               IndexRange           DataLimit            EnumTbl*/
  {"NAME",           NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_STR,     USER_RW,  (void *) &creepCfgObjSenTemp.name[0],        0, CREEP_MAX_OBJ-1,   0, CREEP_MAX_NAME,   NULL},
  {"ID",             NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_ENUM,    USER_RW,  (void *) &creepCfgObjSenTemp.id,             0, CREEP_MAX_OBJ-1,   NO_LIMIT,            SensorIndexType},
  {"SLOPE",          NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_FLOAT,   USER_RW,  (void *) &creepCfgObjSenTemp.slope,          0, CREEP_MAX_OBJ-1,   NO_LIMIT,            NULL},
  {"OFFSET",         NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_FLOAT,   USER_RW,  (void *) &creepCfgObjSenTemp.offset,         0, CREEP_MAX_OBJ-1,   NO_LIMIT,            NULL},
  {"SAMPLECNT",      NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_UINT16,  USER_RW,  (void *) &creepCfgObjSenTemp.sampleCnt,      0, CREEP_MAX_OBJ-1,   NO_LIMIT,            NULL},
  {"SAMPLERATE_MS",  NO_NEXT_TABLE,  CreepMsg_ObjRow,   USER_TYPE_UINT16,  USER_RW,  (void *) &creepCfgObjSenTemp.sampleRate_ms,  0, CREEP_MAX_OBJ-1,   NO_LIMIT,            NULL},
  {NULL,             NULL,           NULL,              NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned
static
USER_MSG_TBL creepObjectSenColTbl[] =
{  /*Str            Next Tbl Ptr    Handler Func.        Data Type      Access         Parameter                                 IndexRange           DataLimit          EnumTbl*/
  {"NAME",          NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_STR,     USER_RW,    (void *) &creepCfgObjSenTemp.name[0],       0, CREEP_MAX_OBJ-1,    0, CREEP_MAX_NAME, NULL},
  {"ID",            NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_ENUM,    USER_RW,    (void *) &creepCfgObjSenTemp.id,            0, CREEP_MAX_OBJ-1,    NO_LIMIT,          SensorIndexType},
  {"SLOPE",         NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_FLOAT,   USER_RW,    (void *) &creepCfgObjSenTemp.slope,         0, CREEP_MAX_OBJ-1,    NO_LIMIT,          NULL},
  {"OFFSET",        NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_FLOAT,   USER_RW,    (void *) &creepCfgObjSenTemp.offset,        0, CREEP_MAX_OBJ-1,    NO_LIMIT,          NULL},
  {"SAMPLECNT",     NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_UINT16,  USER_RW,    (void *) &creepCfgObjSenTemp.sampleCnt,     0, CREEP_MAX_OBJ-1,    NO_LIMIT,          NULL},
  {"SAMPLERATE_MS", NO_NEXT_TABLE, CreepMsg_ObjCol, USER_TYPE_UINT16,  USER_RW,    (void *) &creepCfgObjSenTemp.sampleRate_ms, 0, CREEP_MAX_OBJ-1,    NO_LIMIT,          NULL},
  {NULL,            NULL,          NULL,            NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned.


#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned
static
USER_MSG_TBL creepObjectTbl[] =
{  /*Str            Next Tbl Ptr          Handler Func.      Data Type          Access         Parameter                               IndexRange          DataLimit         EnumTbl*/
  {"NAME",          NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_STR,     USER_RW,  (void *) &creepCfgObjTemp.name[0],         0, CREEP_MAX_OBJ-1,  0, CREEP_MAX_NAME, NULL},
  {"ENG_ID",        NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_ENUM,    USER_RW,  (void *) &creepCfgObjTemp.engId,           0, CREEP_MAX_OBJ-1,  NO_LIMIT,          engRunIdEnum},
  {"TBL_ID",        NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_ENUM,    USER_RW,  (void *) &creepCfgObjTemp.creepTblId,      0, CREEP_MAX_OBJ-1,  NO_LIMIT,          creepTableIdEnum},
  {"RATEOFFSET_MS", NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_UINT16,  USER_RW,  (void *) &creepCfgObjTemp.cpuOffset_ms,    0, CREEP_MAX_OBJ-1,  NO_LIMIT,          NULL},
  {"RATE_ms",       NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_UINT32,  USER_RW,  (void *) &creepCfgObjTemp.intervalRate_ms, 0, CREEP_MAX_OBJ-1,  10,1000,           NULL},
  {"ER_TRANS_ms",   NO_NEXT_TABLE,        CreepMsg_Object,   USER_TYPE_UINT32,  USER_RW,  (void *) &creepCfgObjTemp.erTransFault_ms, 0, CREEP_MAX_OBJ-1,  NO_LIMIT,          NULL},
  {"SensorRow",     creepObjectSenRowTbl, NULL,              NO_HANDLER_DATA},
  {"SensorCol",     creepObjectSenColTbl, NULL,              NO_HANDLER_DATA},
  {NULL,            NULL,                 NULL,              NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned.

static
USER_MSG_TBL creepSensorTbl[] =
{  /*Str             Next Tbl Ptr    Handler Func.      Data Type        Access          Parameter                                 IndexRange         DataLimit    EnumTbl*/
  {"ROW_FAULTED",    NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_BOOLEAN,  USER_RO,   (void *) &creepSensorTemp.row.bFailed,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"ROW_RATE_MS",    NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT,    USER_RO,   (void *) &creepSensorTemp.row.rateTime,       0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"ROW_TIMEOUT_MS", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_UINT32,   USER_RO,   (void *) &creepSensorTemp.row.timeout_ms,     0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"ROW_CURR_VAL",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,   (void *) &creepSensorTemp.row.currVal,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"ROW_PEAK_VAL",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,   (void *) &creepSensorTemp.row.peakVal,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"COL_FAULTED",    NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_BOOLEAN,  USER_RO,   (void *) &creepSensorTemp.col.bFailed,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"COL_RATE_MS",    NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT,    USER_RO,   (void *) &creepSensorTemp.col.rateTime,       0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"COL_TIMEOUT_MS", NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_UINT32,   USER_RO,   (void *) &creepSensorTemp.col.timeout_ms,     0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"COL_CURR_VAL",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,   (void *) &creepSensorTemp.col.currVal,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {"COL_PEAK_VAL",   NO_NEXT_TABLE, CreepMsg_Sensor, USER_TYPE_FLOAT64,  USER_RO,   (void *) &creepSensorTemp.col.peakVal,        0, CREEP_MAX_OBJ-1, NO_LIMIT,    NULL},
  {NULL,             NULL,          NULL,            NO_HANDLER_DATA}
};

static
USER_MSG_TBL creepCntTbl[] =
{   /*Str              Next Tbl Ptr    Handler Func.     Data Type        Access              Parameter                                    IndexRange          DataLimit   EnumTbl*/
  {"MISSION",          NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_FLOAT64,  USER_RW,   (void *) &creepStatusTemp.data.creepMissionCnt,      0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {"ACCUM",            NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_FLOAT64,  USER_RW,   (void *) &creepStatusTemp.data.creepAccumCnt,        0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {"ACCUM_REJECT",     NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_FLOAT64,  USER_RW,   (void *) &creepStatusTemp.data.creepAccumCntTrashed, 0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {"L_MISSION",        NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_FLOAT64,  USER_RW,   (void *) &creepStatusTemp.data.creepLastMissionCnt,  0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {"SENSOR_FAILURES",  NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_UINT32,   USER_RW,   (void *) &creepStatusTemp.data.sensorFailures,       0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {"CREEP_FAILURES",   NO_NEXT_TABLE, CreepMsg_Cnt,   USER_TYPE_UINT32,   USER_RW,   (void *) &creepStatusTemp.data.creepFailures,        0, CREEP_MAX_OBJ-1,  NO_LIMIT,   NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};

static
USER_MSG_TBL creepValTbl[] =
{  /*Str                Next Tbl Ptr   Handler Func.   Data Type         Access       Parameter                                              IndexRange         DataLimit    EnumTbl*/
  {"ACCUM_PCNT",        NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RW,  (void *) &creepStatusTemp.data_percent.accumCnt,        0, CREEP_MAX_OBJ-1, 0,0x43480000, NULL},
  {"ACCUM_REJECT_PCNT", NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RW,  (void *) &creepStatusTemp.data_percent.accumCntTrashed, 0, CREEP_MAX_OBJ-1, 0,0x43480000, NULL},
  {"LAST_MISSION_PCNT", NO_NEXT_TABLE, CreepMsg_Val, USER_TYPE_FLOAT64,  USER_RO,  (void *) &creepStatusTemp.data_percent.lastMissionCnt,  0, CREEP_MAX_OBJ-1, NO_LIMIT,     NULL},
  {NULL,                NULL,          NULL,         NO_HANDLER_DATA}
};

// Level 1
#define TABLE_STRING  "TABLE"
#define OBJECT_STRING "OBJECT"
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned
static
USER_MSG_TBL creepCfgTbl[] =
{  /*Str                Next Tbl Ptr     Handler Func.   Data Type         Access       Parameter                               IndexRange    DataLimit  EnumTbl*/
  {"PBITSYSCOND",       NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_ENUM,    USER_RW,  (void *) &creepCfgTemp.sysCondPBIT,        -1,-1,        NO_LIMIT,   Flt_UserEnumStatus},
  {"CBITSYSCOND",       NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_ENUM,    USER_RW,  (void *) &creepCfgTemp.sysCondCBIT,        -1,-1,        NO_LIMIT,   Flt_UserEnumStatus},
  {"ENABLED",           NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_BOOLEAN, USER_RW,  (void *) &creepCfgTemp.bEnabled,           -1,-1,        NO_LIMIT,   NULL},
  {TABLE_STRING,        creepTableTbl,   NULL,          NO_HANDLER_DATA},
  {OBJECT_STRING,       creepObjectTbl,  NULL,          NO_HANDLER_DATA},
  {"BASEUNITS",         NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_UINT16,  USER_RW,  (void *) &creepCfgTemp.baseUnits,          -1,-1,        9, 15,      NULL},
  {"SENSOR_START_MS",   NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_UINT32,  USER_RW,  (void *) &creepCfgTemp.sensorStartTime_ms, -1,-1,        NO_LIMIT,   NULL},
  {"MAX_SENSOR_LOG",    NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_UINT16,  USER_RW,  (void *) &creepCfgTemp.maxSensorLossRec,   -1,-1,        NO_LIMIT,   NULL},
  {"CRC",               NO_NEXT_TABLE,   CreepMsg_Cfg,  USER_TYPE_HEX16,   USER_RW,  (void *) &creepCfgTemp.crc16,              -1,-1,        NO_LIMIT,   NULL},
  {NULL,                NULL,            NULL,          NO_HANDLER_DATA}
};
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() elsewhere if not aligned.

static
USER_MSG_TBL creepStatusTbl[] =
{  /*Str             Next Tbl Ptr    Handler Func.    Data Type           Access       Parameter                                IndexRange          DataLimit  EnumTbl*/
  {"FAULT",        NO_NEXT_TABLE,   CreepMsg_Status,  USER_TYPE_ENUM,    USER_RO,     (void *) &creepStatusTemp.status,         0, CREEP_MAX_OBJ-1, NO_LIMIT,  creepStatusStrs},
  {"STATE",        NO_NEXT_TABLE,   CreepMsg_Status,  USER_TYPE_ENUM,    USER_RO,     (void *) &creepStatusTemp.state,          0, CREEP_MAX_OBJ-1, NO_LIMIT,  creepStateStrs},
  {"ER_TIME_ms",   NO_NEXT_TABLE,   CreepMsg_Status,  USER_TYPE_UINT32,  USER_RO,     (void *) &creepStatusTemp.erTime_ms,      0, CREEP_MAX_OBJ-1, NO_LIMIT,  NULL},
  {"INTERVAL_cnt", NO_NEXT_TABLE,   CreepMsg_Status,  USER_TYPE_UINT32,  USER_RO,     (void *) &creepStatusTemp.creepIncrement, 0, CREEP_MAX_OBJ-1, NO_LIMIT,  NULL},
  {"SENSOR",       creepSensorTbl,  NULL,             NO_HANDLER_DATA},
  {NULL,           NULL,            NULL,             NO_HANDLER_DATA}
};

static
USER_MSG_TBL creepPersistTbl[] =
{ /*Str        Next Tbl Ptr    Handler Func.    Data Type           Access Parameter IndexRange DataLimit  EnumTbl*/
  {"CNT",     creepCntTbl,     NULL,            NO_HANDLER_DATA},
  {"PCNT",    creepValTbl,     NULL,            NO_HANDLER_DATA},
  {NULL,      NULL,            NULL,            NO_HANDLER_DATA}
};

static
USER_MSG_TBL creepDebugTbl[] =
{  /*Str                 Next Tbl Ptr    Handler Func.      Data Type           Access       Parameter                                  IndexRange   DataLimit  EnumTbl*/
  {"EXP_CRC",            NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_HEX16,    USER_RO,    (void *) &creepDebugTemp.exp_crc,            -1,  -1,     NO_LIMIT,  NULL},
  {"SENSOR_LOG_CNT",     NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_UINT16,   USER_RO,    (void *) &creepDebugTemp.nSensorFailedCnt,   -1,  -1,     NO_LIMIT,  NULL},
  {"CREEP_HISTORY_CNT",  NO_NEXT_TABLE, CreepMsg_DebugTbl, USER_TYPE_UINT16,   USER_RO,    (void *) &creepDebugTemp.nCreepFailBuffCnt,  -1,  -1,     NO_LIMIT,  NULL},
  {NULL,NULL,NULL,NO_HANDLER_DATA}
};


// Root Level
static
USER_MSG_TBL creepRoot[] =
{  /*Str             Next Tbl Ptr    Handler Func.                     Data Type            Access                       Parameter   IndexRange   DataLimit  EnumTbl*/
  {"STATUS",        creepStatusTbl,  NULL,                             NO_HANDLER_DATA},
  {"CFG",           creepCfgTbl,     NULL,                             NO_HANDLER_DATA},
//{"CNT",           CreepCntTbl,     NULL,                             NO_HANDLER_DATA},
//{"VALUE",         CreepValTbl,     NULL,                             NO_HANDLER_DATA},
  {"PERSIST",       creepPersistTbl, NULL,                             NO_HANDLER_DATA},
  {"DEBUG",         creepDebugTbl,   NULL,                             NO_HANDLER_DATA},
  {"CLEAR",         NO_NEXT_TABLE,   CreepMsg_Clear,                   USER_TYPE_STR,      USER_WO,                        NULL,       -1, -1,      NO_LIMIT,    NULL},
  {"RECENTALL",     NO_NEXT_TABLE,   CreepMsg_UserMessageRecentAll,    USER_TYPE_ACTION,   (USER_RO|USER_NO_LOG),          NULL,       -1, -1,      NO_LIMIT,    NULL},
  {"RECENTUPDATE",  NO_NEXT_TABLE,   CreepMsg_UserMessageRecentUpdate, USER_TYPE_STR,      USER_RO,                        NULL,       -1, -1,      NO_LIMIT,    NULL},
  { DISPLAY_CFG,    NO_NEXT_TABLE,   Creep_ShowConfig,                 USER_TYPE_ACTION,   (USER_RO|USER_NO_LOG|USER_GSE), NULL,        -1,-1,      NO_LIMIT,    NULL},
  {NULL,            NULL,            NULL,                             NO_HANDLER_DATA}
};

static
USER_MSG_TBL creepRootTblPtr = {"CREEP",creepRoot,NULL,NO_HANDLER_DATA};


/*****************************************************************************/
/* Local Function Prototypes                                                 */
/*****************************************************************************/
static void CreepMsg_UpdateCfg (void* pNVRAMShadow, void* pTemp, UINT16 nSize );


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
static
USER_HANDLER_RESULT CreepMsg_Status(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  creepStatusTemp = *Creep_GetStatus( (UINT8) Index);

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
static
USER_HANDLER_RESULT CreepMsg_Sensor(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  creepSensorTemp = *Creep_GetSensor( (UINT8) Index);

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
static
USER_HANDLER_RESULT CreepMsg_Cnt(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  creepStatusTemp = *Creep_GetStatus( (UINT8) Index);

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
  // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime version)
  //       is then copied to AppData.  Executing this cmd during creep processing will/could
  //       force early update of App Data. If Fault deteced, then some cnt would have been
  //       committed to App Data. Normally when creep fault detected, the cnt for that
  //       current flight is "thrown" away.
  if (SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Update Status counts only
    Creep_GetStatus( (UINT8) Index)->data = creepStatusTemp.data;

    // Update EEPROM APP Data
    Creep_UpdateCreepXAppData( (UINT16) Index);

    // Update the Data Percent here.  Normally Data precent update in init(), and @ end of ER.
    //    But if the cnt values are updated, should update % here
    Creep_GetStatus( (UINT8) Index)->data_percent.accumCnt = creepStatusTemp.data.creepAccumCnt
                                                    / m_Creep_100_pcnt;

    Creep_GetStatus( (UINT8) Index)->data_percent.accumCntTrashed =
                          creepStatusTemp.data.creepAccumCntTrashed / m_Creep_100_pcnt;

    Creep_GetStatus( (UINT8) Index)->data_percent.lastMissionCnt =
                          creepStatusTemp.data.creepLastMissionCnt / m_Creep_100_pcnt;
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
static
USER_HANDLER_RESULT CreepMsg_Val(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result ;
  CREEP_DATA_PERCENT creepPercentPrev;


  result = USER_RESULT_OK;

  creepStatusTemp = *Creep_GetStatus( (UINT8) Index);
  creepPercentPrev = creepStatusTemp.data_percent;


  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
  // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime version)
  //       is then copied to AppData.  Executing this cmd during creep processing will/could
  //       force early update of App Data. If Fault deteced, then some cnt would have been
  //       committed to App Data. Normally when creep fault detected, the cnt for that current
  //       flight is "thrown" away.
  if (SetPtr != NULL && USER_RESULT_OK == result)
  {
    // Need to convert data_percent to count values, only of the data_percent val that
    //   has changed
    if ( fabs (creepPercentPrev.accumCnt - creepStatusTemp.data_percent.accumCnt) >=
         DBL_EPSILON )
    {
      // Update associated counts here
      creepStatusTemp.data.creepAccumCnt = creepStatusTemp.data_percent.accumCnt
                                           * m_Creep_100_pcnt;

      // Recompute percentage.  Note: this value might be different than the val entered due
      //    to error assoc with DOUBLE representation of real numbers.
      Creep_GetStatus( (UINT8) Index)->data_percent.accumCnt =
                        creepStatusTemp.data.creepAccumCnt / m_Creep_100_pcnt;
    }

    if ( fabs (creepPercentPrev.accumCntTrashed -
               creepStatusTemp.data_percent.accumCntTrashed) >= DBL_EPSILON )
    {
      // Update associated counts here
      creepStatusTemp.data.creepAccumCntTrashed = creepStatusTemp.data_percent.accumCntTrashed
                                                  * m_Creep_100_pcnt;

      // Recompute percentage.  Note: this value might be different than the val entered due
      //    to error assoc with DOUBLE representation of real numbers.
      Creep_GetStatus( (UINT8) Index)->data_percent.accumCntTrashed =
               creepStatusTemp.data.creepAccumCntTrashed / m_Creep_100_pcnt;
    }

    // Update Status counts only
    Creep_GetStatus( (UINT8) Index)->data = creepStatusTemp.data;

    // Update EEPROM APP Data
    Creep_UpdateCreepXAppData((UINT16) Index);
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
 * Returns:     USER_RESULT_OK:    Processed successfully
 *              USER_RESULT_ERROR: Could not be processed, value at GetPtr not
 *                                 set.
 *
 * Notes:
 *
*****************************************************************************/
static
USER_HANDLER_RESULT CreepMsg_Cfg(USER_DATA_TYPE DataType,
                                 USER_MSG_PARAM Param,
                                 UINT32 Index,
                                 const void *SetPtr,
                                 void **GetPtr)
{
  USER_HANDLER_RESULT result;
  result = USER_RESULT_OK;

  memcpy(&creepCfgTemp,
         &CfgMgr_ConfigPtr()->CreepConfig,
         sizeof( CREEP_CFG ) );

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {    
    CreepMsg_UpdateCfg( (void*)&CfgMgr_ConfigPtr()->CreepConfig, (void*)&creepCfgTemp, sizeof(creepCfgTemp) );    
  }
  return result;
}


/******************************************************************************
 * Function:    CreepMsg_Table
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
static
USER_HANDLER_RESULT CreepMsg_Table(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result ;
  result = USER_RESULT_OK;

  memcpy(&creepCfgTblTemp,
         &CfgMgr_ConfigPtr()->CreepConfig.creepTbl[Index],
         sizeof( creepCfgTblTemp) );  

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    
    CreepMsg_UpdateCfg((void*)&CfgMgr_ConfigPtr()->CreepConfig.creepTbl[Index],
                       (void*)&creepCfgTblTemp,
                       sizeof(creepCfgTblTemp) );    
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
static
USER_HANDLER_RESULT CreepMsg_Object(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;
  result = USER_RESULT_OK;  
  
  memcpy(&creepCfgObjTemp,
         &CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index],
         sizeof( creepCfgObjTemp) );  

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    CreepMsg_UpdateCfg( (void*)&CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index],
                        (void*)&creepCfgObjTemp,
                         sizeof(creepCfgObjTemp) );
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
static
USER_HANDLER_RESULT CreepMsg_ObjRow(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;
  result = USER_RESULT_OK;

  memcpy(&creepCfgObjSenTemp,
          &CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index].sensorRow,
          sizeof(creepCfgObjSenTemp));

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    CreepMsg_UpdateCfg(&CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index].sensorRow,
                       &creepCfgObjSenTemp,
                       sizeof(creepCfgObjSenTemp));
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
static
USER_HANDLER_RESULT CreepMsg_ObjCol(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;
  result = USER_RESULT_OK;

  memcpy(&creepCfgObjSenTemp,
          &CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index].sensorCol,
          sizeof(creepCfgObjSenTemp));

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  if(SetPtr != NULL && USER_RESULT_OK == result)
  {
    CreepMsg_UpdateCfg(&CfgMgr_ConfigPtr()->CreepConfig.creepObj[Index].sensorCol,
                       &creepCfgObjSenTemp,
                       sizeof(creepCfgObjSenTemp));
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
static
USER_HANDLER_RESULT CreepMsg_Clear(USER_DATA_TYPE DataType,
                                   USER_MSG_PARAM Param,
                                   UINT32 Index,
                                   const void *SetPtr,
                                   void **GetPtr)
{
  USER_HANDLER_RESULT result;
  UINT16 i;
  TIMESTAMP ts;


  result = USER_RESULT_ERROR;

  if(strncmp(SetPtr,"really",sizeof("really")) == 0)
  {
    result = USER_RESULT_OK;
    GSE_DebugStr(NORMAL,TRUE, "All Creep Data Cleared");

    memset ( (void *) &creepStatusTemp.data, 0, sizeof(CREEP_DATA) );

    // Clear all Creep Object EEPROM APP Data
    for (i=0;i<CREEP_MAX_OBJ;i++)
    {
      Creep_GetStatus( (UINT8) i)->data = creepStatusTemp.data;
      Creep_UpdateCreepXAppData(i);    // Commit to EE APP
      // Writes, need to update EEPROM APP Data.  Should exe this cmd  when CREEP not active.
      // NOTE: Normally m_Creep_AppData is updated after ER where Data in Status (RunTime ver)
      //      is then copied to AppData. Executing this cmd during creep processing will/could
      //      force early update of App Data. If Fault deteced, then some cnt would have been
      //      committed to App Data. Normally when creep fault detected, the cnt for that
      //      current flight is "thrown" away.

      // Update the Data Percent.  Normally Data precent update in init(), and @ end of ER.
      //    But if the cnt values are updated, should update % here
      Creep_GetStatus((UINT8) i)->data_percent.accumCnt = 0;
      Creep_GetStatus((UINT8) i)->data_percent.accumCntTrashed = 0;
      Creep_GetStatus((UINT8) i)->data_percent.lastMissionCnt = 0;
    }

    CM_GetTimeAsTimestamp((TIMESTAMP *) &ts);
    Creep_ClearFaultFile((TIMESTAMP *) &ts);  // Clear Creep Fault Recent buffer
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
      creepStatusTemp.data = Creep_GetStatus( (UINT8) i)->data;

      // Update diagnostic counts only
      creepStatusTemp.data.creepAccumCntTrashed = 0;
      creepStatusTemp.data.sensorFailures = 0;
      creepStatusTemp.data.creepFailures = 0;

      // Update modified values
      Creep_GetStatus( (UINT8) i)->data = creepStatusTemp.data;
      Creep_UpdateCreepXAppData(i);  // Commit to EE APP
    }

    CM_GetTimeAsTimestamp((TIMESTAMP *) &ts);
    Creep_ClearFaultFile((TIMESTAMP *) &ts);  // Clear Creep Fault Recent buffer
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
static
USER_HANDLER_RESULT CreepMsg_DCount(USER_DATA_TYPE DataType,
                                    USER_MSG_PARAM Param,
                                    UINT32 Index,
                                    const void *SetPtr,
                                    void **GetPtr)
{
  USER_HANDLER_RESULT result;
  UINT16 i;
  CHAR *strCount[CREEP_MAX_COLS];  // There should be exactly 10 counts
  CHAR *ptr;
  CHAR msg[CREEP_MAX_STR_LEN];
  static CHAR msg_out[CREEP_MAX_STR_LEN];

  result = USER_RESULT_ERROR;  
  
  // Get the entire crepp cfg
  memcpy(&creepCfgTemp,
         &CfgMgr_ConfigPtr()->CreepConfig,
         sizeof( creepCfgTemp));

  // Fetch the specific row from cfg into its own temp structure
  creepCfgTblTemp = *(CREEP_TBL_PTR) &(creepCfgTemp.creepTbl[Index]);
  
  if(SetPtr != NULL)
  {
    // Update Damage Counts
    // Need to copy from SetPtr as it is const void and strtok below will modified input
    strncpy (msg, SetPtr, CREEP_MAX_STR_LEN);

    ptr = msg;
    i = 0;
    strCount[i++] = strtok( ptr, "," );
    while ( (i < CREEP_MAX_COLS) && (strCount[i-1] != NULL) )
    {
      strCount[i] = strtok(NULL, ",");
      i++;
    }

    if ( (strtok(NULL, ",") == NULL) && (i == CREEP_MAX_COLS) )
    {
      // We have parse exactly 10 values.  If >10 or <10, then will ret ERR.
      ptr = strCount[0];
      for (i=0; i < CREEP_MAX_COLS; i++)
      {
        if (ptr == strCount[i])
        {
          // Convert the string to double
          creepCfgTblTemp.dCount[Param.Int][i] = strtod ( (const char* )
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
#pragma ghs nowarning 1545  // Ignore alignment in Cfg.  ASSERT() if not aligned
        memcpy ( (void *) &(creepCfgTemp.creepTbl[Index].dCount[Param.Int][0]),
                 (void *) &creepCfgTblTemp.dCount[Param.Int][0], sizeof(FLOAT64) *
                  CREEP_MAX_COLS );

        CreepMsg_UpdateCfg( (void *) &(CfgMgr_ConfigPtr()->CreepConfig.creepTbl[Index].dCount[Param.Int][0]),
                            (void *) &(creepCfgTemp.creepTbl[Index].dCount[Param.Int][0]),
                            (sizeof(FLOAT64) * CREEP_MAX_COLS) );
#pragma ghs endnowarning // Ignore alignment in Cfg.  ASSERT() if not aligned	
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
      snprintf(msg,CREEP_MAX_STR_LEN,"%0.0lf,", creepCfgTblTemp.dCount[Param.Int][i]);
      strcat (msg_out, (const char *) msg);
    }
    // No ',' at the end
    snprintf(msg,CREEP_MAX_STR_LEN,"%0.0lf", creepCfgTblTemp.dCount[Param.Int][i]);
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
static
USER_HANDLER_RESULT CreepMsg_UserMessageRecentAll(USER_DATA_TYPE DataType,
                                                  USER_MSG_PARAM Param,
                                                  UINT32 Index,
                                                  const void *SetPtr,
                                                  void **GetPtr)
{
  UINT8 i;
  static CHAR resultBuffer[USER_SINGLE_MSG_MAX_SIZE];
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  CHAR *pStr;
  UINT32 nOffset, len;
  CHAR strDateTime[CREEP_MAX_RECENT_DATE_TIME];
  TIMESTRUCT ts;


  pStr = (CHAR *) &resultBuffer[0];

  // Output # of faults in buffer
  CM_ConvertTimeStamptoTimeStruct( &m_creepHistory.ts, &ts);

  snprintf ( strDateTime, CREEP_MAX_RECENT_DATE_TIME, "%02d:%02d:%02d %02d/%02d/%4d",
             ts.Hour,
             ts.Minute,
             ts.Second,
             ts.Month,
             ts.Day,
             ts.Year );

  snprintf ( resultBuffer, USER_SINGLE_MSG_MAX_SIZE,
             "\r\nCreep Faults = %d (of %d Max), Last Update =%s\r\n",
             m_creepHistory.cnt, CREEP_HISTORY_MAX, strDateTime);
  len = strlen ( resultBuffer );
  nOffset = len;

  for (i=0;i<m_creepHistory.cnt;i++)
  {
    pStr = Creep_GetHistoryBuffEntry(i);
    len = strlen (pStr);

    memcpy ( (void *) &resultBuffer[nOffset], pStr, len );
    nOffset += len;
  }

  resultBuffer[nOffset] = NULL;
  User_OutputMsgString(resultBuffer, FALSE);

  return result;
}


/******************************************************************************
 * Function:    CreepMsg_UserMessageRecentUpdate
 *
 * Description: Called by the User.c module from the reference to this fucntion
 *              in the user message tables above.
 *              Retrieves the Creep Clear update time
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
static
USER_HANDLER_RESULT CreepMsg_UserMessageRecentUpdate(USER_DATA_TYPE DataType,
                                                     USER_MSG_PARAM Param,
                                                     UINT32 Index,
                                                     const void *SetPtr,
                                                     void **GetPtr)
{
  USER_HANDLER_RESULT result = USER_RESULT_OK;
  TIMESTRUCT ts;
  static CHAR strDateTime[CREEP_MAX_RECENT_DATE_TIME];


  if(SetPtr == NULL)
  {
    CM_ConvertTimeStamptoTimeStruct( &m_creepHistory.ts, &ts);

    snprintf ( strDateTime, CREEP_MAX_RECENT_DATE_TIME, "%02d/%02d/%04d %02d:%02d:%02d",
               ts.Month,
               ts.Day,
               ts.Year,
               ts.Hour,
               ts.Minute,
               ts.Second );

    *GetPtr = strDateTime;
  }

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
static USER_HANDLER_RESULT CreepMsg_DebugTbl(USER_DATA_TYPE DataType,
                                             USER_MSG_PARAM Param,
                                             UINT32 Index,
                                             const void *SetPtr,
                                             void **GetPtr)
{
  USER_HANDLER_RESULT result ;

  result = USER_RESULT_OK;

  creepDebugTemp = *Creep_GetDebug();

  result = User_GenericAccessor (DataType, Param, Index, SetPtr, GetPtr);

  return result;
}

/******************************************************************************
 * Function:    CreepMsg_UpdateCfg
 *
 * Description: Copy the modified cfg info in the NVRAM shadow and the EEPROM 
 *              shadow
 *
 * Parameters:  [in] pNVRAMShadow:  Offset into NVRAM Shadow for config info
 *              [in] pTemp:         Address of the local temp buffer containing
 *                                  modified data.
 *              [in] nSize:         size of the data pointed to by pTemp
 *
 * Returns:     None
 *
 * Notes:
 *
*****************************************************************************/
static void CreepMsg_UpdateCfg ( void* pNVRAMShadow, void* pTemp, UINT16 nSize )
{
  // copy localtemp -> NVRAM Cfg Shadow...
  memcpy(pNVRAMShadow, pTemp,  nSize);

  // and Copy NVRAM Cfg Shadow to Config EEPROM
  CfgMgr_StoreConfigItem(CfgMgr_ConfigPtr(), pNVRAMShadow, nSize );
}


/******************************************************************************
* Function:     Creep_GetHistoryBuffEntry
*
* Description:  Returns the selected Fault Entry from the non-volatile memory
*
* Parameters:   [in] index_obj: Index parameter selecting specified entry
*
* Returns:      [out] ptr to RecentLogStr[] containing decoded entry
*
* Notes:        none
*
*****************************************************************************/
static CHAR *Creep_GetHistoryBuffEntry ( UINT8 index_obj )
{
  CHAR *pStr;
  TIMESTRUCT ts;
  static CHAR  recentLogStr[256];


  if ( index_obj < m_creepHistory.cnt )
  {
    CM_ConvertTimeStamptoTimeStruct( &m_creepHistory.buff[index_obj].ts, &ts);

    // Stringify the Log data
    snprintf( recentLogStr, 80, "%02d:%s(%x) at %02d:%02d:%02d %02d/%02d/%4d\r\n",
        index_obj+1,
        SystemLogIDString( m_creepHistory.buff[index_obj].id),
        m_creepHistory.buff[index_obj].id,
        ts.Hour,
        ts.Minute,
        ts.Second,
        ts.Month,
        ts.Day,
        ts.Year);

    // Return log data string
    pStr = recentLogStr;
  }
  else
  {
    snprintf( recentLogStr, 80, "%02d:Fault entry empty\r\n", index_obj );
    pStr = recentLogStr;
  }

  return pStr;
}

/******************************************************************************
* Function:     Creep_ShowConfig
*
* Description:  Handles User Manager requests to retrieve the configuration
*               settings.
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
* Returns:     USER_RESULT_OK:    Processed successfully
*              USER_RESULT_ERROR: Error processing command.
*
* Notes:
*****************************************************************************/
static
USER_HANDLER_RESULT Creep_ShowConfig(USER_DATA_TYPE DataType,
                                      USER_MSG_PARAM Param,
                                      UINT32 Index,
                                      const void *SetPtr,
                                      void **GetPtr)
{
   // Local Data
   USER_HANDLER_RESULT result;
   UINT32              i;
   CHAR                labelStem[CREEP_MAX_LABEL_CHAR_ARRAY] = "\r\n\r\nCREEP.CFG";
   CHAR                label[USER_MAX_MSG_STR_LEN * 3];
   USER_MSG_TBL*       pCfgTable;
   CHAR                value[CREEP_MAX_VALUE_CHAR_ARRAY];
   CHAR                branchName[USER_MAX_MSG_STR_LEN] = " ";
   CHAR                nextBranchName[USER_MAX_MSG_STR_LEN] = "  ";

   // Declare a GetPtr to be used locally since
   // the GetPtr passed in is "NULL" because this funct is a user-action.
   UINT32              tempInt;
   void*               localPtr = &tempInt;

   // Local Initialization
   pCfgTable = creepCfgTbl;     // Re-set the pointer to beginning of CFG table
   result    = USER_RESULT_OK;

   User_OutputMsgString (labelStem, FALSE);

   while ( pCfgTable->MsgStr  != NULL             &&
           pCfgTable->MsgType != USER_TYPE_ACTION &&
           result             == USER_RESULT_OK )
   {
      if (0 == strncmp(pCfgTable->MsgStr, TABLE_STRING, sizeof(pCfgTable->MsgStr)))
      {
         for ( i = 0; i < CREEP_MAX_TBL && result == USER_RESULT_OK; i++ )
         {
            // Substruct name was the first entry to terminate the while-loop above
            strncpy_safe(labelStem, sizeof(labelStem), pCfgTable->MsgStr, _TRUNCATE);
            // Display element info above each set of data.
            snprintf(label, sizeof(label), "\r\n\r\n%s%s[%d]",branchName, labelStem, i);
            // Assume the worse, to terminate this for-loop
            result = USER_RESULT_ERROR;
            if ( User_OutputMsgString( label, FALSE ) )
            {
               result = User_DisplayConfigTree(nextBranchName, creepTableTbl, i, 0, NULL);
            }
         }
      }
      else if (0 == strncmp(pCfgTable->MsgStr, OBJECT_STRING, sizeof(pCfgTable->MsgStr)))
      {
         for ( i = 0; i < CREEP_MAX_OBJ && result == USER_RESULT_OK; i++ )
         {
            // Substruct name was the first entry to terminate the while-loop above
            strncpy_safe(labelStem, sizeof(labelStem), pCfgTable->MsgStr, _TRUNCATE);
            // Display element info above each set of data.
            snprintf(label, sizeof(label), "\r\n\r\n%s%s[%d]",branchName, labelStem, i);
            // Assume the worse, to terminate this for-loop
            result = USER_RESULT_ERROR;
            if ( User_OutputMsgString( label, FALSE ) )
            {
               result = User_DisplayConfigTree(nextBranchName, creepObjectTbl, i, 0, NULL);
            }
         }
      }
      else
      {
         snprintf ( label, sizeof(label), "\r\n%s%s =", branchName, pCfgTable->MsgStr );
         User_OutputMsgString( label, FALSE );
         result = USER_RESULT_ERROR;
         // Call user function for the table entry
         if( USER_RESULT_OK != pCfgTable->MsgHandler(pCfgTable->MsgType, pCfgTable->MsgParam,
                 0, NULL, &localPtr))
         {
            User_OutputMsgString( USER_MSG_INVALID_CMD_FUNC, FALSE);
         }
         else
         {
            // Convert the param to string.
            if( User_CvtGetStr(pCfgTable->MsgType, value, sizeof(value),
                               localPtr, pCfgTable->MsgEnumTbl))
            {
               if (User_OutputMsgString( value, FALSE ))
               {
                 result = USER_RESULT_OK;
               }
            }
            else
            {
               //Conversion error
               User_OutputMsgString( USER_MSG_RSP_CONVERSION_ERR, FALSE );
            }
         }
      }
      pCfgTable++;
   }

   return result;
}

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: CreepUserTables.c $
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 1/28/15    Time: 2:11p
 * Updated in $/software/control processor/code/application
 * SCR 1234 - Code Review Updates
 * 
 * *****************  Version 13  *****************
 * User: Contractor V&v Date: 10/06/14   Time: 2:14p
 * Updated in $/software/control processor/code/application
 * SCR #1234 - Configuration update of certain modules overwrites default
 * settings. Creep rework.
 *
 * *****************  Version 12  *****************
 * User: Contractor V&v Date: 9/03/14    Time: 5:08p
 * Updated in $/software/control processor/code/application
 * SCR #1234 - Code Review cleanup
 *
 * *****************  Version 11  *****************
 * User: Contractor V&v Date: 8/14/14    Time: 4:02p
 * Updated in $/software/control processor/code/application
 * SCR #1234 - Configuration update of certain modules overwrites
 *
 * *****************  Version 10  *****************
 * User: Jeff Vahue   Date: 1/22/13    Time: 1:47p
 * Updated in $/software/control processor/code/application
 * SCR# 1219 - GSE Cleanup
 *
 * *****************  Version 9  *****************
 * User: John Omalley Date: 13-01-21   Time: 3:46p
 * Updated in $/software/control processor/code/application
 * SCR 1219 - Misc Creep User Table Updates
 *
 * *****************  Version 8  *****************
 * User: Peter Lee    Date: 13-01-17   Time: 8:31p
 * Updated in $/software/control processor/code/application
 * SCR #1195 Items 14, 18, 19
 *
 * *****************  Version 7  *****************
 * User: Peter Lee    Date: 13-01-16   Time: 9:38a
 * Updated in $/software/control processor/code/application
 * SCR #1195.  Item #13.  "creep.cfg.pbitsyscond" pointing to wrong
 * internal var.
 *
 * *****************  Version 6  *****************
 * User: John Omalley Date: 13-01-14   Time: 3:39p
 * Updated in $/software/control processor/code/application
 * SCR 1195 - Item #12 - Add showcfg
 *
 * *****************  Version 5  *****************
 * User: Peter Lee    Date: 12-12-13   Time: 7:20p
 * Updated in $/software/control processor/code/application
 * Code Review Updates
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
