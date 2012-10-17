#ifndef SYSTEMLOG_H
#define SYSTEMLOG_H

/******************************************************************************
            Copyright (C) 2009-2012 Pratt & Whitney Engine Services, Inc.
               All Rights Reserved. Proprietary and Confidential.

    File:        SystemLog.h

    Description: This file defines the enumeration for all possible
                 System type logs. These logs include System events,
                 fault events and application events. It also defines
                 log write counts, limiting the number of log writes that
                 can be made for a particular ID.

   VERSION
      $Revision: 103 $  $Date: 12-09-27 9:14a $
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                                */
/*****************************************************************************/
/*****************************************************************************/
/* Software Specific Includes                                                */
/*****************************************************************************/

#include "alt_stdtypes.h"
#include "ResultCodes.h"

/******************************************************************************
                                 Package Defines
******************************************************************************/
#define SYSTEM_LOG_DATA_SIZE    1024

/******************************************************************************
                                 Package Typedefs
******************************************************************************/
// A log write count of 0 specifies an unlimited number of log writes allowed.
#undef  SYS_LOG_ID
#define SYS_LOG_ID(label,value,count) label = value,

#define SYS_LOG_IDS \
SYS_LOG_ID(SYS_ID_NULL_LOG                          ,0      ,0)\
SYS_LOG_ID(SYS_APP_ID_DONT_CARE                     ,0xFFFFFFFF ,0)\
\
SYS_LOG_ID(APP_ID_END_OF_FLIGHT                     ,0xA200 ,0)\
SYS_LOG_ID(APP_ID_START_OF_RECORDING                ,0xA201 ,0)\
SYS_LOG_ID(APP_ID_END_OF_RECORDING                  ,0xA202 ,0)\
SYS_LOG_ID(APP_ID_FAST_WOWSTART                     ,0xA203 ,0)\
SYS_LOG_ID(APP_ID_FAST_WOWEND                       ,0xA204 ,0)\
SYS_LOG_ID(APP_ID_START_OF_DOWNLOAD                 ,0xA205 ,0)\
SYS_LOG_ID(APP_ID_END_OF_DOWNLOAD                   ,0xA206 ,0)\
SYS_LOG_ID(APP_ID_FSM_STATE_CHANGE                  ,0xA207 ,0)\
SYS_LOG_ID(APP_ID_FSM_CRC_MISMATCH                  ,0xA208 ,0)\
SYS_LOG_ID(APP_ID_EVENT_STARTED                     ,0xB000 ,0)\
SYS_LOG_ID(APP_ID_EVENT_ENDED                       ,0xB001 ,0)\
SYS_LOG_ID(APP_ID_EVENT_TABLE_SUMMARY               ,0xB100 ,0)\
SYS_LOG_ID(APP_ID_EVENT_TABLE_TRANSITION            ,0xB101 ,0)\
SYS_LOG_ID(APP_ID_TIMEHISTORY                       ,0xB200 ,0)\
SYS_LOG_ID(APP_ID_TIMEHISTORY_ETM_LOG               ,0xB210 ,0)\
\
SYS_LOG_ID(APP_ID_ENGINERUN_STARTED                 ,0xB300 ,0)\
SYS_LOG_ID(APP_ID_ENGINERUN_ENDED                   ,0xB301 ,0)\
SYS_LOG_ID(APP_ID_ENGINE_INFO_CRC_FAIL              ,0xB310 ,0)\
\
SYS_LOG_ID(APP_ID_TREND_MANUAL                      ,0xB500 ,0)\
SYS_LOG_ID(APP_ID_TREND_AUTO                        ,0xB501 ,0)\
SYS_LOG_ID(APP_ID_TREND_END                         ,0xB502 ,0)\
SYS_LOG_ID(APP_ID_TREND_AUTO_FAILED                 ,0xB503 ,0)\
SYS_LOG_ID(APP_ID_TREND_AUTO_NOT_DETECTED           ,0xB504 ,0)\
\
SYS_LOG_ID(DRV_ID_PRC_PBIT_BTO_REG_INIT_FAIL        ,0x0500 ,0)\
SYS_LOG_ID(DRV_ID_TTMR_PBIT_REG_INIT_FAIL           ,0x0700 ,0)\
SYS_LOG_ID(DRV_ID_SPI_PBIT_REG_INIT_FAIL            ,0x0801 ,0)\
SYS_LOG_ID(DRV_ID_RTC_PBIT_REG_INIT_FAIL            ,0x0902 ,0)\
SYS_LOG_ID(DRV_ID_RTC_PBIT_TIME_NOT_VALID           ,0x0903 ,0)\
SYS_LOG_ID(DRV_ID_RTC_PBIT_CLK_NOT_INC              ,0x0904 ,0)\
SYS_LOG_ID(DRV_ID_DIO_PBIT_REG_INIT_FAIL            ,0x0A01 ,0)\
SYS_LOG_ID(DRV_ID_EEPROM_PBIT_DEVICE_NOT_DETECTED   ,0x0B01 ,0)\
SYS_LOG_ID(DRV_ID_DF_PBIT_CFI_BAD                   ,0x0D01 ,0)\
SYS_LOG_ID(DRV_ID_DPRAM_PBIT_REG_INIT_FAIL          ,0x0F01 ,0)\
SYS_LOG_ID(DRV_ID_DPRAM_PBIT_RAM_TEST_FAIL          ,0x0F02 ,0)\
SYS_LOG_ID(DRV_ID_DPRAM_INTERRUPT_FAIL              ,0x0F13 ,0)\
SYS_LOG_ID(DRV_ID_ADC_PBIT_RET_INIT_FAIL            ,0x1001 ,0)\
SYS_LOG_ID(DRV_ID_A429_PBIT_FPGA_FAIL               ,0x1101 ,0)\
SYS_LOG_ID(DRV_ID_A429_PBIT_TEST_FAIL               ,0x1102 ,0)\
SYS_LOG_ID(DRV_ID_QAR_PBIT_LOOPBACK_FAIL            ,0x1201 ,0)\
SYS_LOG_ID(DRV_ID_QAR_PBIT_DPRAM_FAIL               ,0x1202 ,0)\
SYS_LOG_ID(DRV_ID_QAR_PBIT_FPGA_FAIL                ,0x1203 ,0)\
SYS_LOG_ID(DRV_ID_PSC_PBIT_FAIL                     ,0x1700 ,0)\
SYS_LOG_ID(DRV_ID_PSC_PBIT_REG_INIT_FAIL            ,0x1701 ,0)\
SYS_LOG_ID(DRV_ID_PSC_INTERRUPT_FAIL                ,0x1712 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_PBIT_BAD_FPGA_VER            ,0x1801 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_PBIT_NOT_CONFIGURED          ,0x1802 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_PBIT_CONFIGURED_FAIL         ,0x1803 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_PBIT_INT_FAIL                ,0x1804 ,0) /* Limit in FPGA.c */\
SYS_LOG_ID(DRV_ID_FPGA_MCF_SETUP_FAIL               ,0x1805 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_INTERRUPT_FAIL               ,0x1816 ,0)\
SYS_LOG_ID(DRV_ID_FPGA_PBIT_WD_FAIL                 ,0x1807 ,0)\
\
SYS_LOG_ID(SYS_TASK_MAN_RMT_OVERRUN                 ,0x4620 ,0)\
SYS_LOG_ID(SYS_TASK_MAN_DT_OVERRUN                  ,0x4621 ,0)\
SYS_LOG_ID(SYS_CBIT_ASSERT_LOG                      ,0x4710 ,0)\
SYS_LOG_ID(SYS_CBIT_RESET_LOG                       ,0x4711 ,0)\
SYS_LOG_ID(SYS_CBIT_REG_CHECK_FAIL                  ,0x4714 ,0) /* Limit in CBITManager.c */\
SYS_LOG_ID(SYS_CBIT_CRC_CHECK_FAIL                  ,0x4715 ,0) /* Limit in CBITManager.c */\
SYS_LOG_ID(SYS_CBIT_RAM_CHECK_FAIL                  ,0x4716 ,0)\
SYS_LOG_ID(SYS_CBIT_PWEH_SEU_COUNT_LOG              ,0x4720 ,0)\
\
SYS_LOG_ID(SYS_CFG_PBIT_OPEN_CFG_FAIL               ,0x4800 ,0)\
SYS_LOG_ID(SYS_CFG_PBIT_REINIT_CFG_FAIL             ,0x4801 ,0)\
SYS_LOG_ID(SYS_CFG_RESET_TO_DEFAULT                 ,0x48C0 ,0)\
\
SYS_LOG_ID(SYS_CM_PBIT_RTC_BAD                      ,0x4900 ,0)\
SYS_LOG_ID(SYS_CM_CBIT_TIME_DRIFT_LIMIT_EXCEEDED    ,0x4910 ,0)\
SYS_LOG_ID(SYS_CM_CBIT_TIME_INVALID_LOG             ,0x4912 ,0)\
SYS_LOG_ID(SYS_CM_CBIT_SET_RTC_FAILED               ,0x4913 ,5)\
SYS_LOG_ID(SYS_CM_CLK_UPDATE_RTC                    ,0x4920 ,0)\
SYS_LOG_ID(SYS_CM_CLK_UPDATE_CPU                    ,0x4921 ,0)\
SYS_LOG_ID(SYS_CM_MS_TIME_DRIFT_LIMIT_EXCEEDED      ,0x4922 ,0)\
\
SYS_LOG_ID(SYS_DIO_DISCRETE_WRAP_FAIL               ,0x4A10 ,0) /* Limit in DIO.c */\
\
SYS_LOG_ID(SYS_NVM_PRIMARY_FILE_BAD                 ,0x4B20 ,0)\
SYS_LOG_ID(SYS_NVM_BACKUP_FILE_BAD                  ,0x4B21 ,0)\
SYS_LOG_ID(SYS_NVM_PRIMARY_BACKUP_FILES_BAD         ,0x4B22 ,0)\
SYS_LOG_ID(SYS_NVM_PRIMARY_NEQ_BACKUP_FILE          ,0x4B23 ,0)\
\
SYS_LOG_ID(SYS_LOG_MEM_CORRUPTED_ERASED             ,0x4C00 ,0)\
SYS_LOG_ID(SYS_LOG_MEM_ERRORS                       ,0x4C01 ,0)\
SYS_LOG_ID(SYS_LOG_MEM_READ_FAIL                    ,0x4C12 ,0)\
SYS_LOG_ID(SYS_LOG_MEM_ERASE_FAIL                   ,0x4C14 ,0)\
SYS_LOG_ID(SYS_LOG_85_PERCENT_FULL                  ,0x4C20 ,0)\
\
SYS_LOG_ID(SYS_ID_PM_POWER_ON_LOG                   ,0x5020 ,0)\
SYS_LOG_ID(SYS_ID_PM_POWER_FAILURE_LOG              ,0x5021 ,0)\
SYS_LOG_ID(SYS_ID_PM_POWER_FAILURE_NO_BATTERY_LOG   ,0x5022 ,0)\
SYS_LOG_ID(SYS_ID_PM_POWER_INTERRUPT_LOG            ,0x5023 ,0)\
SYS_LOG_ID(SYS_ID_PM_POWER_RECONNECT_LOG            ,0x5024 ,0)\
SYS_LOG_ID(SYS_ID_PM_POWER_OFF_LOG                  ,0x5025 ,0)\
SYS_LOG_ID(SYS_ID_PM_FORCED_SHUTDOWN_LOG            ,0x5026 ,0)\
SYS_LOG_ID(SYS_ID_PM_RESTORE_DATA_FAILED            ,0x5027 ,0)\
\
SYS_LOG_ID(SYS_ID_A429_PBIT_DRV_FAIL                ,0x5101 ,0)\
SYS_LOG_ID(SYS_ID_A429_CBIT_STARTUP_FAIL            ,0x5111 ,0)\
SYS_LOG_ID(SYS_ID_A429_CBIT_DATA_LOSS_FAIL          ,0x5112 ,0)\
SYS_LOG_ID(SYS_ID_A429_CBIT_SSM_FAIL                ,0x5113 ,0)\
SYS_LOG_ID(SYS_ID_A429_BCD_DIGITS                   ,0x5114 ,0) /* Limit in Arinc429Mgr.c */\
\
SYS_LOG_ID(SYS_ID_QAR_PBIT_DRV_FAIL                 ,0x5204 ,0)\
SYS_LOG_ID(SYS_ID_QAR_CBIT_STARTUP_FAIL             ,0x5211 ,0)\
SYS_LOG_ID(SYS_ID_QAR_CBIT_DATA_LOSS_FAIL           ,0x5212 ,0)\
SYS_LOG_ID(SYS_ID_QAR_CBIT_WORD_TIMEOUT_FAIL        ,0x5213 ,0)\
SYS_LOG_ID(SYS_ID_QAR_DATA_PRESENT                  ,0x5220 ,10)\
SYS_LOG_ID(SYS_ID_QAR_DATA_LOSS                     ,0x5221 ,10)\
SYS_LOG_ID(SYS_ID_QAR_SYNC                          ,0x5222 ,0)\
SYS_LOG_ID(SYS_ID_QAR_SYNC_LOSS                     ,0x5223 ,0)\
\
SYS_LOG_ID(SYS_ID_SENSOR_CBIT_FAIL                  ,0x5310 ,0)\
SYS_LOG_ID(SYS_ID_SENSOR_RANGE_FAIL                 ,0x5311 ,0)\
SYS_LOG_ID(SYS_ID_SENSOR_RATE_FAIL                  ,0x5312 ,0)\
SYS_LOG_ID(SYS_ID_SENSOR_SIGNAL_FAIL                ,0x5313 ,0)\
\
SYS_LOG_ID(SYS_ID_UART_PBIT_FAIL                    ,0x5C00 ,0)\
SYS_LOG_ID(SYS_ID_UART_CBIT_STARTUP_FAIL            ,0x5C11 ,0)\
SYS_LOG_ID(SYS_ID_UART_CBIT_DATA_LOSS_FAIL          ,0x5C12 ,0)\
SYS_LOG_ID(SYS_ID_UART_CBIT_WORD_TIMEOUT_FAIL       ,0x5C13 ,0)\
SYS_LOG_ID(SYS_ID_UART_F7X_NEW_DL_FOUND             ,0x5C20 ,0)\
SYS_LOG_ID(SYS_ID_UART_F7X_DL_NOT_RECOGNIZED        ,0x5C21 ,0)\
SYS_LOG_ID(SYS_ID_UART_EMU150_STATUS                ,0x5C30 ,0)\
\
SYS_LOG_ID(SYS_ID_INFO_TRIGGER_STARTED              ,0x5420 ,0)\
SYS_LOG_ID(SYS_ID_INFO_TRIGGER_ENDED                ,0x5421 ,0)\
\
SYS_LOG_ID(SYS_ID_MS_HEARTBEAT_FAIL                 ,0x4F10 ,0) /* Cfg Limit in MSStsCtl.c */\
SYS_LOG_ID(SYS_ID_MS_RECONNECT                      ,0x4F20 ,0) /* Cfg Limit in MSStsCtl.c */\
SYS_LOG_ID(SYS_ID_MS_VERSION_MISMATCH               ,0x4F21 ,0)\
\
SYS_LOG_ID(SYS_ID_FPGA_CBIT_CRC_FAIL                ,0x5811 ,0)\
SYS_LOG_ID(SYS_ID_FPGA_CBIT_FORCE_RELOAD_RESULT     ,0x5812 ,0) /* Limit in FPGAManager.c */\
SYS_LOG_ID(SYS_ID_FPGA_CBIT_REG_CHECK_FAIL          ,0x5813 ,0)\
\
SYS_LOG_ID(SYS_ID_BOX_POWER_ON_COUNT_RETRIEVE_FAIL  ,0x5901 ,0)\
SYS_LOG_ID(SYS_ID_BOX_SYS_INFO_CRC_FAIL             ,0x5902 ,0)\
SYS_LOG_ID(SYS_ID_BOX_INFO_SYS_STATUS_UPDATE        ,0x5903 ,0)\
\
SYS_LOG_ID(SYS_ID_SPIMGR_EEPROM_WRITE_FAIL          ,0x5A10 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_EEPROM_READ_FAIL           ,0x5A11 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_RTC_WRITE_FAIL             ,0x5A12 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_RTC_READ_FAIL              ,0x5A13 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_RTC_NVRAM_WRITE_FAIL       ,0x5A14 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_BUS_VOLT_READ_FAIL         ,0x5A15 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_BAT_VOLT_READ_FAIL         ,0x5A16 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_LITBAT_VOLT_READ_FAIL      ,0x5A17 ,0)\
SYS_LOG_ID(SYS_ID_SPIMGR_BOARD_TEMP_READ_FAIL       ,0x5A18 ,0)\
\
SYS_LOG_ID(SYS_ID_GSE_PBIT_REG_INIT_FAIL            ,0x5B01 ,0)\
\
SYS_LOG_ID(SYS_ID_CYCLES_PERSIST_FILES_INVALID      ,0x6020 ,0)/* Cycles */\
SYS_LOG_ID(SYS_ID_CYCLES_CHECKID_CHANGED            ,0x6021 ,0)/* Cycles */\
\
SYS_LOG_ID(SYS_ID_ACTION_PERSIST_CLR                ,0x6120 ,0)\
SYS_LOG_ID(SYS_ID_ACTION_PERSIST_RESET              ,0x6121 ,0)\
\
SYS_LOG_ID(APP_UI_USER_PARAM_CHANGE_LOG             ,0x8800 ,0)\
SYS_LOG_ID(APP_UI_USER_ACTION_COMMAND_LOG           ,0x8801 ,0)\
\
SYS_LOG_ID(APP_AT_INFO_CFG_STATUS                   ,0xA020 ,0)\
SYS_LOG_ID(APP_AT_INFO_AC_ID_MISMATCH               ,0xA023 ,0)\
SYS_LOG_ID(APP_AT_INFO_AUTO_ERROR_DESC              ,0xA024 ,0)\
\
SYS_LOG_ID(APP_DM_SINGLE_BUFFER_OVERFLOW            ,0xA111 ,0)\
SYS_LOG_ID(APP_DM_DOWNLOAD_STARTED                  ,0xA130 ,0)\
SYS_LOG_ID(APP_DM_DOWNLOAD_STATISTICS               ,0xA120 ,0)\
\
SYS_LOG_ID(APP_UPM_INFO_VFY_TBL_ROWS                ,0xA322 ,0)\
SYS_LOG_ID(APP_UPM_INFO_UPLOAD_START                ,0xA323 ,0)\
SYS_LOG_ID(APP_UPM_INFO_UPLOAD_FILE_FAIL            ,0xA324 ,0)\
SYS_LOG_ID(APP_UPM_GS_TRANSMIT_SUCCESS              ,0xA325 ,0)\
SYS_LOG_ID(APP_UPM_GS_CRC_FAIL                      ,0xA326 ,0)\
SYS_LOG_ID(APP_UPM_MS_CRC_FAIL                      ,0xA327 ,0)\
\
SYS_LOG_ID(APP_FM_INFO_USER_UPDATED_CLOCK           ,0xA226 ,0)

//SYS_LOG_ID(SYS_ID_PBIT_WATCHDOG_RESET               ,0x9 ,0)\
//SYS_LOG_ID(SYS_ID_PBIT_QUART                        ,0xA ,0)\
//SYS_LOG_ID(SYS_ID_PBIT_LAMP                         ,0xB ,0)\
//SYS_LOG_ID(SYS_ID_PBIT_WALLCLOCK                    ,0xC ,0)\
//SYS_LOG_ID(SYS_ID_CBIT_PFLASH_CRC                   ,0xD ,0)\
//SYS_LOG_ID(SYS_ID_CBIT_RAM                          ,0xE ,0)\
//SYS_LOG_ID(SYS_ID_CBIT_TPU_RAM                      ,0xF ,0)\
//SYS_LOG_ID(SYS_ID_CBIT_UART                         ,0x10 ,0)\
//SYS_LOG_ID(SYS_ID_CBIT_WALLCLOCK                    ,0x11 ,0)\
//SYS_LOG_ID(SYS_ID_ASSERT                            ,0x12 ,0)\
//SYS_LOG_ID(SYS_ID_TM_MIF_OVERRUN                    ,0x13 ,0)\
//SYS_LOG_ID(SYS_ID_TM_RMT_OVERRUN                    ,0x14 ,0)\
//SYS_LOG_ID(SYS_ID_UNHANDLED_EXCEPTION               ,0x15 ,0)\
//SYS_LOG_ID(SYS_ID_GPIO_OVERLAP                      ,0x16 ,0)\
//SYS_LOG_ID(SYS_ID_GPIO_WIDTH                        ,0x17 ,0)\
//SYS_LOG_ID(SYS_ID_GPIO_DIN_EXCEEDED                 ,0x18 ,0)\
//SYS_LOG_ID(SYS_ID_GPIO_DOUT_EXCEEDED                ,0x19 ,0)\
//SYS_LOG_ID(SYS_ID_INIT_POT                          ,0x1A ,0)\
//SYS_LOG_ID(SYS_ID_INVALID_WC                        ,0x1B ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_REG                      ,0x1C ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_SRAM                     ,0x1D ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_DMT                      ,0x1E ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_LOOP_GRP0                ,0x1F ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_LOOP_GRP1                ,0x20 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_ERR_INT                  ,0x21 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_DATA_LOSS                ,0x22 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_SSM_FAIL_00              ,0x23 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_SSM_NO_COMP              ,0x24 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_SSM_TEST                 ,0x25 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_SSM_FAIL_11              ,0x26 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_PARITY                   ,0x27 ,0)\
//SYS_LOG_ID(SYS_ID_ARINC429_NO_COMM                  ,0x28 ,0)\
//SYS_LOG_ID(APP_ID                                   ,0x3E8 ,0)\
//SYS_LOG_ID(APP_ID_ACS_BUF_OVERFLOW                  ,0x3E9 ,0)\
//SYS_LOG_ID(APP_ID_ACS_DATA_RETRIEVE_STARTED         ,0x3EA ,0)\
//SYS_LOG_ID(APP_ID_ACS_DATA_RETRIEVE_FAILED          ,0x3EB ,0)\
//SYS_LOG_ID(APP_ID_ACS_DATA_ERROR                    ,0x3EC ,0)\
//SYS_LOG_ID(APP_ID_ACS_DATA_LOSS                     ,0x3ED ,0)\
//SYS_LOG_ID(APP_ID_MS_TX_SUCCESS                     ,0x3EE ,0)\
//SYS_LOG_ID(APP_ID_MS_TX_FAIL                        ,0x3EF ,0)\
//SYS_LOG_ID(APP_ID_MS_SETUP_SUCCESS                  ,0x3F0 ,0)\
//SYS_LOG_ID(APP_ID_MS_SETUP_FAILED                   ,0x3F1 ,0)\
//SYS_LOG_ID(APP_ID_MS_GSM_CONNECT                    ,0x3F2 ,0)\
//SYS_LOG_ID(APP_ID_MS_FAIL                           ,0x3F3 ,0)\
//SYS_LOG_ID(APP_ID_LOG_RETRIEVAL                     ,0x3F4 ,0)\
//SYS_LOG_ID(APP_ID_BUF_OVERFLOW                      ,0x3F5 ,0)\
//SYS_LOG_ID(APP_ID_MS_PWR_FAULT                      ,0x3F6 ,0)\
//SYS_LOG_ID(APP_ID_GSM_PWR_FAULT                     ,0x3F7 ,0)\
//SYS_LOG_ID(APP_ID_ACS_UART_OPEN_FAIL                ,0x3F8 ,0)\
//SYS_LOG_ID(APP_ID_MS_UART_OPEN_FAIL                 ,0x3F9 ,0)\
//SYS_LOG_ID(APP_ID_AIRCRAFT_CONFIG_UPDATE            ,0x3FE ,0)\
//SYS_LOG_ID(APP_ID_AIRCRAFT_CONFIG_FAULT             ,0x3FF ,0)\
//SYS_LOG_ID(DRV_ID_SPI_PBIT_FAIL                     ,0x0800 ,0)\
//SYS_LOG_ID(DRV_ID_RTC_PBIT_FAIL_LOG                 ,0x0900 ,0)\
//SYS_LOG_ID(DRV_ID_DIO_PBIT_FAIL                     ,0x0A00 ,0)\
//SYS_LOG_ID(DRV_ID_EEPROM_PBIT_FAIL                  ,0x0B00 ,0)\
//SYS_LOG_ID(DRV_ID_DF_PBIT_FAIL                      ,0x0D00 ,0)\
//SYS_LOG_ID(DRV_ID_DPRAM_PBIT_FAIL                   ,0x0F00 ,0)\
//SYS_LOG_ID(DRV_ID_ADC_PBIT_FAIL                     ,0x1000 ,0)\
//SYS_LOG_ID(DRV_ID_A429_PBIT_FAIL                    ,0x1100 ,0)\
//SYS_LOG_ID(DRV_ID_FPGA_PBIT_FAIL                    ,0x1800 ,0)\
//SYS_LOG_ID(SYS_CFG_PBIT_CFG_FMT_FAIL                ,0x4802 ,0)\
//SYS_LOG_ID(SYS_CFG_CBIT_FAIL_LOG                    ,0x4810 ,0)\
//SYS_LOG_ID(SYS_NVM_PBIT_FAIL_LOG                    ,0x4B00 ,0)\
//SYS_LOG_ID(SYS_NVM_CBIT_FAIL_LOG                    ,0x4B10 ,0)\
//SYS_LOG_ID(SYS_LOG_100_PERCENT_FULL                 ,0x4C11 ,0)\
//SYS_LOG_ID(SYS_ID_A429_PBIT_FAIL                    ,0x5100 ,0)\
//SYS_LOG_ID(SYS_ID_A429_CBIT_FAIL                    ,0x5110 ,0)\
//SYS_LOG_ID(SYS_ID_QAR_PBIT_FAIL                     ,0x5200 ,0)\
//SYS_LOG_ID(SYS_ID_QAR_CBIT_FAIL                     ,0x5210 ,0)\
//SYS_LOG_ID(SYS_ID_SENSOR_PBIT_FAIL                  ,0x5300 ,0)\
//SYS_LOG_ID(SYS_ID_SENSOR_INTERFACE_FAIL             ,0x5314 ,0)\
//SYS_LOG_ID(SYS_ID_FPGA_CBIT_FAIL                    ,0x5810 ,0)\
//SYS_LOG_ID(SYS_ID_BOX_PBIT_FAIL_LOG                 ,0x5900 ,0)\
//SYS_LOG_ID(SYS_ID_GSE_PBIT_FAIL                     ,0x5B00 ,0)\
//

typedef enum {
  SYS_LOG_IDS
  SYS_APP_ID_MAX
} SYS_APP_ID;


#pragma pack(1)
typedef struct
{
   UINT32 nChecksum;   //Checksum of the log and header, excluding checksum
   // UINT32 nTimestamp;  //Altair timestamp format
   TIMESTAMP ts;       //Altair timestamp format
   UINT16 nID;         //Log ID
   UINT16 nSize;       //Size of data, excluding the header size.
} SYS_LOG_HEADER;
#pragma pack()

typedef struct
{
  SYS_LOG_HEADER Hdr;
  UINT8          Data[SYSTEM_LOG_DATA_SIZE];
} SYS_LOG_PAYLOAD;

/******************************************************************************
                                 Package Exports
******************************************************************************/
#undef EXPORT

#if defined ( SYSTEMLOG_BODY )
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
EXPORT const CHAR* SystemLogIDString(SYS_APP_ID LogID);

EXPORT void SystemLogInitLimitCheck(void);
EXPORT void SystemLogResetLimitCheck(SYS_APP_ID LogID);
EXPORT BOOLEAN SystemLogLimitCheck(SYS_APP_ID LogID);

#endif // SYSTEMLOG_H

/*****************************************************************************
 *  MODIFICATIONS
 *    $History: SystemLog.h $
 *
 * *****************  Version 103  *****************
 * User: John Omalley Date: 12-09-27   Time: 9:14a
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Added Engine Identification Fields to software and file
 * header
 *
 * *****************  Version 102  *****************
 * User: Contractor V&v Date: 9/14/12    Time: 4:47p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Trend log types
 *
 * *****************  Version 101  *****************
 * User: Jim Mood     Date: 9/06/12    Time: 6:04p
 * Updated in $/software/control processor/code/system
 * SCR #1107 Time History implementation changes
 *
 * *****************  Version 100  *****************
 * User: Jeff Vahue   Date: 8/28/12    Time: 1:43p
 * Updated in $/software/control processor/code/system
 * SCR #1142 Code Review Findings
 *
 * *****************  Version 99  *****************
 * User: John Omalley Date: 12-08-14   Time: 2:22p
 * Updated in $/software/control processor/code/system
 * SCR 1076 - Code Review Update
 *
 * *****************  Version 98  *****************
 * User: Contractor V&v Date: 8/08/12    Time: 3:30p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Change Cycle log # offset to +0x20
 *
 * *****************  Version 97  *****************
 * User: John Omalley Date: 12-07-27   Time: 3:02p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Action Manager Persistent updates
 *
 * *****************  Version 96  *****************
 * User: Jim Mood     Date: 7/19/12    Time: 11:07a
 * Updated in $/software/control processor/code/system
 * SCR 1107: Data Offload changes for 2.0.0
 *
 * *****************  Version 95  *****************
 * User: Contractor V&v Date: 7/11/12    Time: 4:36p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2  Cycle impl and persistence
 *
 * *****************  Version 94  *****************
 * User: Contractor V&v Date: 6/25/12    Time: 2:32p
 * Updated in $/software/control processor/code/system
 * SCR #1107 FAST 2 Changed names start and run log sys id
 *
 * *****************  Version 93  *****************
 * User: Contractor V&v Date: 5/10/12    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * FAST 2  Cycle  Added 0xB300 APP_ID_ENGINERUN and 0xB400 APP_ID_
 *
 * *****************  Version 92  *****************
 * User: John Omalley Date: 4/24/12    Time: 12:00p
 * Updated in $/software/control processor/code/system
 * SCR 1107 - Cessna Caravan Updates
 *
 * *****************  Version 91  *****************
 * User: Jim Mood     Date: 2/24/12    Time: 10:39a
 * Updated in $/software/control processor/code/system
 * SCR 1114 - Re labeled after v1.1.1 release
 *
 * *****************  Version 89  *****************
 * User: Contractor V&v Date: 12/14/11   Time: 6:50p
 * Updated in $/software/control processor/code/system
 * SCR #307 Add System Status to Fault / Event / etc logs
 *
 * *****************  Version 88  *****************
 * User: Contractor2  Date: 8/15/11    Time: 1:48p
 * Updated in $/software/control processor/code/system
 * SCR #1021 Error: Data Manager Buffers Full Fault Detected
 *
 * *****************  Version 87  *****************
 * User: Jim Mood     Date: 7/20/11    Time: 10:57a
 * Updated in $/software/control processor/code/system
 * SCR 575: GSM Enable when engine status is lost.  (Part of changes for
 * the Fast State Machine)
 *
 * *****************  Version 86  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:47p
 * Updated in $/software/control processor/code/system
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 *
 * *****************  Version 85  *****************
 * User: Contractor2  Date: 6/20/11    Time: 11:31a
 * Updated in $/software/control processor/code/system
 * SCR #113 Enhancement - QAR bad baud rate setting and continuous
 * DATA_PRES trans ind
 *
 * *****************  Version 84  *****************
 * User: Contractor2  Date: 6/14/11    Time: 11:41a
 * Updated in $/software/control processor/code/system
 * SCR #473 Enhancement: Restriction of System Logs
 *
 * *****************  Version 83  *****************
 * User: Contractor2  Date: 6/01/11    Time: 1:45p
 * Updated in $/software/control processor/code/system
 * SCR #473 Enhancement: Restriction of System Logs
 *
 * *****************  Version 82  *****************
 * User: Contractor2  Date: 5/10/11    Time: 2:33p
 * Updated in $/software/control processor/code/system
 * SCR #891 Enhancement: Incomplete FPGA initialization functionality
 *
 * *****************  Version 81  *****************
 * User: Peter Lee    Date: 4/11/11    Time: 10:32a
 * Updated in $/software/control processor/code/system
 * SCR #1029 EMU150 Download
 *
 * *****************  Version 80  *****************
 * User: John Omalley Date: 11/05/10   Time: 11:16a
 * Updated in $/software/control processor/code/system
 * SCR 985 - Data Manager Code Coverage Updates
 *
 * *****************  Version 79  *****************
 * User: Peter Lee    Date: 9/03/10    Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR #806 Code Review Updates
 *
 * *****************  Version 78  *****************
 * User: Contractor2  Date: 8/02/10    Time: 3:42p
 * Updated in $/software/control processor/code/system
 * SCR #762 Error: Micro-Server Interface Logic doesn't work
 *
 * *****************  Version 77  *****************
 * User: Contractor2  Date: 7/30/10    Time: 2:17p
 * Updated in $/software/control processor/code/system
 * SCR #90;243;252 Implementation: CBIT of mssim
 *
 * *****************  Version 76  *****************
 * User: John Omalley Date: 7/30/10    Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR 730 - Fixed problems with the IDs
 *
 * *****************  Version 75  *****************
 * User: John Omalley Date: 7/27/10    Time: 4:43p
 * Updated in $/software/control processor/code/system
 * SCR 730 - Removed Log Write Failed log
 *
 * *****************  Version 74  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:17a
 * Updated in $/software/control processor/code/system
 * SCR 306 / SCR 639 - Added Log Manager System Logs
 *
 * *****************  Version 73  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #8 Implementation: External Interrupt Monitors
 *
 * *****************  Version 72  *****************
 * User: Contractor2  Date: 7/09/10    Time: 1:44p
 * Updated in $/software/control processor/code/system
 * SCR #687 Implementation: Watchdog Reset Log
 *
 * *****************  Version 71  *****************
 * User: Contractor V&v Date: 7/07/10    Time: 6:19p
 * Updated in $/software/control processor/code/system
 * SCR #681 The system shall set SC to FAULT if both copies of sys
 *
 * *****************  Version 70  *****************
 * User: Jim Mood     Date: 6/29/10    Time: 6:05p
 * Updated in $/software/control processor/code/system
 * SCR 623  Updated system log IDs for aircraft reconfiguration.
 *
 * *****************  Version 69  *****************
 * User: Contractor2  Date: 6/18/10    Time: 11:34a
 * Updated in $/software/control processor/code/system
 * SCR #611 Write Logs  on CRC Pass/Fail
 * Added sys log for SRS-2465.
 *
 * *****************  Version 68  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 5:36p
 * Updated in $/software/control processor/code/system
 * SCR #635 Create log to indicate "unrecognized" dump list detected.
 *
 * *****************  Version 67  *****************
 * User: Peter Lee    Date: 6/08/10    Time: 5:08p
 * Updated in $/software/control processor/code/system
 * SCR #635 Uart PBIT System Log Id
 *
 * *****************  Version 66  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:09p
 * Updated in $/software/control processor/code/system
 * SCR #618 F7X and UartMgr Requirements Implementation
 *
 * *****************  Version 65  *****************
 * User: Contractor2  Date: 5/25/10    Time: 3:04p
 * Updated in $/software/control processor/code/system
 * SCR #611 Write Logs  on CRC Pass/Fail
 *
 * *****************  Version 64  *****************
 * User: Contractor2  Date: 5/06/10    Time: 3:58p
 * Updated in $/software/control processor/code/system
 * SCR #530 - Error: Trigger NO_COMPARE infinite logs
 * Removed SYS_ID_CBIT_START_TRIG_BAD_COMPARE and
 * SYS_ID_CBIT_END_TRIG_BAD_COMPARE
 * Related to obsolete requirements SRS-3307 and SRS-3308
 *
 * *****************  Version 63  *****************
 * User: Contractor V&v Date: 4/28/10    Time: 4:54p
 * Updated in $/software/control processor/code/system
 * SCR #559 Check clock diff CP-MS on startup
 *
 * *****************  Version 62  *****************
 * User: Jeff Vahue   Date: 4/23/10    Time: 6:52p
 * Updated in $/software/control processor/code/system
 * SCR #563 - clean up System Log ID values and insert comment into
 * TaskManager.h
 *
 * *****************  Version 61  *****************
 * User: Contractor2  Date: 4/23/10    Time: 2:09p
 * Updated in $/software/control processor/code/system
 * Implementation: Req SRS-1811,1813,1814,1815,3162 - CBIT RAM Tests
 *
 * *****************  Version 60  *****************
 * User: Contractor V&v Date: 4/22/10    Time: 6:42p
 * Updated in $/software/control processor/code/system
 * SCR #453 Misc fix
 *
 * *****************  Version 59  *****************
 * User: Jeff Vahue   Date: 4/13/10    Time: 11:59a
 * Updated in $/software/control processor/code/system
 * SCR #470 - remove unused system logs - moved unused to after the list
 * and commented out for now.
 *
 * *****************  Version 58  *****************
 * User: Contractor V&v Date: 4/12/10    Time: 7:22p
 * Updated in $/software/control processor/code/system
 * SCR #540 Log reason for cfg reset
 *
 * *****************  Version 57  *****************
 * User: Contractor V&v Date: 4/07/10    Time: 5:11p
 * Updated in $/software/control processor/code/system
 * SCR #533 DOUT wrap-around
 *
 * *****************  Version 56  *****************
 * User: Contractor2  Date: 4/06/10    Time: 3:56p
 * Updated in $/software/control processor/code/system
 * SCR 525 - Implement periodic CBIT CRC test.
 *
 * *****************  Version 55  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/system
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 *
 * *****************  Version 54  *****************
 * User: Contractor V&v Date: 3/19/10    Time: 4:32p
 * Updated in $/software/control processor/code/system
 * SCR #492 Log SPI timeouts events
 *
 * *****************  Version 53  *****************
 * User: Contractor V&v Date: 3/10/10    Time: 4:41p
 * Updated in $/software/control processor/code/system
 * SCR #248 Parameter Log Change
 *
 * *****************  Version 52  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 4:04p
 * Updated in $/software/control processor/code/system
 * SCR #67 Interrupted SPI Access-  Added log code for SpiManager
 *
 * *****************  Version 51  *****************
 * User: Contractor2  Date: 3/02/10    Time: 1:58p
 * Updated in $/software/control processor/code/system
 * SCR# 472 - Fix file/function header
 *
 * *****************  Version 50  *****************
 * User: Jeff Vahue   Date: 2/09/10    Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR# 405 - Add DT overrun Log on each occurance, save actual MIF of max
 * Duty Cycle vs. 0-32, so we can tell if issues happen at startup.
 *
 * *****************  Version 49  *****************
 * User: Peter Lee    Date: 2/01/10    Time: 3:28p
 * Updated in $/software/control processor/code/system
 * SCR #432 Add Arinc429 Tx-Rx LoopBack PBIT
 *
 * *****************  Version 48  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 9:20a
 * Updated in $/software/control processor/code/system
 * SCR #427 ClockMgr.c Issues and SRS-3090
 *
 * *****************  Version 47  *****************
 * User: Peter Lee    Date: 1/27/10    Time: 4:31p
 * Updated in $/software/control processor/code/system
 * SCR #427 Misc Clock Updates
 *
 * *****************  Version 46  *****************
 * User: Jeff Vahue   Date: 12/22/09   Time: 2:11p
 * Updated in $/software/control processor/code/system
 * SCR# 326
 *
 * *****************  Version 45  *****************
 * User: Jeff Vahue   Date: 12/18/09   Time: 7:32p
 * Updated in $/software/control processor/code/system
 * SCR# 378
 *
 * *****************  Version 44  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:13p
 * Updated in $/software/control processor/code/system
 * SCR# 364 INT8 -> CHAR
 *
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 12/16/09   Time: 2:17p
 * Updated in $/software/control processor/code/system
 * QAR Req 3110, 3041, 3114 and 3118
 *
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 12/01/09   Time: 5:17p
 * Updated in $/software/control processor/code/system
 * SCR #347 Time Sync Requirements
 *
 * *****************  Version 41  *****************
 * User: John Omalley Date: 11/24/09   Time: 2:44p
 * Updated in $/software/control processor/code/system
 * SCR 348
 * * Updated the system log size to 512 bytes and sized the LogWriteSystem
 * table to 128 entries.
 *
 * *****************  Version 40  *****************
 * User: Peter Lee    Date: 11/23/09   Time: 2:04p
 * Updated in $/software/control processor/code/system
 * Updates to support WIN32 version
 *
 * *****************  Version 39  *****************
 * User: Peter Lee    Date: 11/19/09   Time: 11:22a
 * Updated in $/software/control processor/code/system
 * SCR #315 Fix bug with maintaining SEU counts across power cycle
 *
 * *****************  Version 38  *****************
 * User: Peter Lee    Date: 10/30/09   Time: 3:45p
 * Updated in $/software/control processor/code/system
 * SCR #315 CBIT Reg Check
 *
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 10/21/09   Time: 2:45p
 * Updated in $/software/control processor/code/system
 * SCR #312 Add msec value to logs
 *
 * *****************  Version 36  *****************
 * User: John Omalley Date: 10/20/09   Time: 3:03p
 * Updated in $/software/control processor/code/system
 * SCR 308
 * - Added Trigger Fault System IDs
 *
 * *****************  Version 35  *****************
 * User: John Omalley Date: 10/19/09   Time: 3:29p
 * Updated in $/software/control processor/code/system
 * SCR 23
 * - Added the Log Manager 85% Full informational log.
 *
 * *****************  Version 34  *****************
 * User: John Omalley Date: 10/19/09   Time: 1:36p
 * Updated in $/software/control processor/code/system
 * SCR 283
 * - Added logic to record a record a fail log if the buffers are full
 * when data recording is started.
 * - Updated the logic to record an error only once when the full
 * condition is detected.
 *
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:58a
 * Updated in $/software/control processor/code/system
 *
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:10p
 * Updated in $/software/control processor/code/system
 * SCR #287 QAR_SensorTest() records log
 *
 * *****************  Version 31  *****************
 * User: John Omalley Date: 10/01/09   Time: 4:28p
 * Updated in $/software/control processor/code/system
 * Added Sensor Fail IDs
 *
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:18p
 * Updated in $/software/control processor/code/system
 * SCR #275 Box Power On Usage Requirements.  System log when box power on
 * usage data can not be retrieved.
 *
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 9/17/09    Time: 11:41a
 * Updated in $/software/control processor/code/system
 * Temporary Fix to increase system log que from 16 to 32 to support SCR
 * #94 and #95 (Recording of PBIT logs)
 *
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:24p
 * Updated in $/software/control processor/code/system
 * SCR #94, #95 return PBIT log structure to Init Mgr
 *
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 10:42a
 * Updated in $/software/control processor/code/system
 * SCR #94 and update initialization for SET_CHECK() functionality
 *
 * *****************  Version 25  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:59p
 * Updated in $/software/control processor/code/system
 * SCR 179
 * Removed Unused variables.
 *
 * *****************  Version 24  *****************
 * User: Peter Lee    Date: 6/19/09    Time: 9:10a
 * Updated in $/software/control processor/code/system
 * Update PM System Log ID
 *
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 4/29/09    Time: 10:13a
 * Updated in $/software/control processor/code/system
 * SCR #168 Preliminary code review (jv) updates
 *
 * *****************  Version 22  *****************
 * User: Jim Mood     Date: 4/22/09    Time: 3:29p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 21  *****************
 * User: Jim Mood     Date: 4/01/09    Time: 11:36a
 * Updated in $/control processor/code/system
 * Added Aircraft Tail Number application logs
 *
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 1/30/09    Time: 4:45p
 * Updated in $/control processor/code/system
 * SCR #131 Arinc429 Updates
 * Add SSM Failure Detection to SensorTest()
 *
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:27p
 * Updated in $/control processor/code/system
 * SCR #131 Arinc429 Updates
 * Add Arinc429 CBIT and PBIT System Log ID
 *
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 3:04p
 * Updated in $/control processor/code/system
 * SCR 129 Misc FPGA Updates.
 *
 * *****************  Version 16  *****************
 * User: Peter Lee    Date: 11/12/08   Time: 1:20p
 * Updated in $/control processor/code/system
 * Fix compile issue with macro.  Remove "//" from macro.
 *
 * *****************  Version 15  *****************
 * User: Peter Lee    Date: 11/05/08   Time: 4:17p
 * Updated in $/control processor/code/system
 * Updated QAR System ID's
 *
 * *****************  Version 14  *****************
 * User: Jim Mood     Date: 10/16/08   Time: 4:25p
 * Updated in $/control processor/code/system
 * Added configuration manager PBIT logs
 *
 * *****************  Version 13  *****************
 * User: Jim Mood     Date: 10/13/08   Time: 2:31p
 * Updated in $/control processor/code/system
 * Moved the "DONT_CARE" type log to the top, after "NULL" so that the
 * "SYS_APP_ID_MAX" would indicate the max ID in the SYS_APP_ID
 * enumeration, ignoring the "DONT_CARE" type.
 *
 * *****************  Version 12  *****************
 * User: Peter Lee    Date: 10/08/08   Time: 11:46a
 * Updated in $/control processor/code/system
 * Additional PM System Log ID
 *
 * *****************  Version 11  *****************
 * User: Jim Mood     Date: 10/07/08   Time: 2:54p
 * Updated in $/control processor/code/system
 *
 * *****************  Version 10  *****************
 * User: Jim Mood     Date: 10/02/08   Time: 9:51a
 * Updated in $/control processor/code/system
 * Moved SysLogWrite function to Log Manager.  Provided stringify macros
 * for each log id.
 *
 * *****************  Version 9  *****************
 * User: Jim Mood     Date: 8/25/08    Time: 11:19a
 * Updated in $/control processor/code/system
 * Changed Sys Log enum list to a macro that can be stringified
 *
 * *****************  Version 8  *****************
 * User: John Omalley Date: 6/18/08    Time: 2:52p
 * Updated in $/control processor/code/system
 * SCR-0044 - Add System Logs
 *
 *
 ***************************************************************************/
