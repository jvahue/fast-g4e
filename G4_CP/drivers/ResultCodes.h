#ifndef DRV_RESULTCODES_H
#define DRV_RESULTCODES_H
/******************************************************************************
 *         Copyright (C) 2007-2012 Pratt & Whitney Engine Services, Inc. 
 *              All Rights Reserved. Proprietary and Confidential.
 *
 * File:          ResultCodes.h
 *
 * Description:   This file defines results codes, the return values for all
 *                driver/system modules.  In order for the higher level code
 *                units to receive detailed diagnostic feedback, all possible
 *                driver/system return values are defined here.
 *
 *                All driver functions should return a result of type RESULT
 *                that can be tested to determine the sucess or failure of
 *                a driver operation.  By having a "catch-all" for the results
 *                all driver functions, it can be simpler to impliment
 *                detailed logging of driver error codes without managing 
 *                different types of error reporting from different drivers
 *
 *  VERSION
 *    $Revision: 57 $  $Date: 6/27/11 1:31p $ 
 *                
 *
******************************************************************************/

/*****************************************************************************/
/* Compiler Specific Includes                                               */
/*****************************************************************************/

/*****************************************************************************/
/* Software Specific Includes                                                 */
/*****************************************************************************/

/******************************************************************************
                                   Package Defines                             
******************************************************************************/
#define RESULTCODES_MAX_STR_LEN  64

#undef RESULT_CODE
#define RESULT_CODE(label,value) label = value,

// length of result code prefix when stringified
#define RESULT_CODE_PREFIX 4

#define RESULT_CODES \
RESULT_CODE(DRV_OK                                     , 0)\
RESULT_CODE(SYS_OK                                     , 0)\
RESULT_CODE(DRV_TTMR_REG_INIT_FAIL                     , 0x00000080)\
RESULT_CODE(DRV_SPI_TIMEOUT                            , 0x000000C0)\
RESULT_CODE(DRV_SPI_REG_INIT_FAIL                      , 0x000000C1)\
RESULT_CODE(DRV_RTC_NVRAM_OFFSET_INVALID               , 0x00000100)\
RESULT_CODE(DRV_RTC_TIME_NOT_VALID                     , 0x00000101)\
RESULT_CODE(DRV_RTC_SETTINGS_RESET_NOT_VALID           , 0x00000102)\
RESULT_CODE(DRV_RTC_ERR_CANNOT_SET_TIME                , 0x00000103)\
RESULT_CODE(DRV_RTC_OSC_STOPPED_FLAG                   , 0x00000106)\
RESULT_CODE(DRV_RTC_CLK_NOT_INC                        , 0x00000109)\
RESULT_CODE(DRV_RTC_NOT_DS3234                         , 0x0000010A)\
RESULT_CODE(DRV_DIO_REG_INIT_FAIL                      , 0x00000140)\
RESULT_CODE(DRV_DIO_INIT_ERR_BAD_DATA_REG_PTR          , 0x00000141)\
RESULT_CODE(DRV_EEPROM_ERR_CS3_NOT_DETECTED            , 0x00000180)\
RESULT_CODE(DRV_EERPOM_ERR_CS5_NOT_DETECTED            , 0x00000181)\
RESULT_CODE(DRV_EEPROM_ERR_CS3_CS5_NOT_DETECTED        , 0x00000182)\
RESULT_CODE(DRV_EEPROM_NOT_DETECTED                    , 0x00000183)\
RESULT_CODE(DRV_EEPROM_ERR_WRITE_IN_PROGRESS           , 0x00000184)\
RESULT_CODE(DRV_EEPROM_ERR_WRITE_CROSSES_PAGE_BOUNDARY , 0x00000185)\
RESULT_CODE(DRV_FLASH_TYP_WRITE_TIME_NOT_VALID         , 0x00000204)\
RESULT_CODE(DRV_FLASH_TYP_BUF_TIME_NOT_VALID           , 0x00000205)\
RESULT_CODE(DRV_FLASH_TYP_SECTOR_ERASE_NOT_VALID       , 0x00000206)\
RESULT_CODE(DRV_FLASH_TYP_CHIP_ERASE_NOT_VALID         , 0x00000207)\
RESULT_CODE(DRV_FLASH_MAX_WRITE_NOT_VALID              , 0x00000208)\
RESULT_CODE(DRV_FLASH_MAX_BUF_TIME_NOT_VALID           , 0x00000209)\
RESULT_CODE(DRV_FLASH_MAX_SECTOR_ERASE_NOT_VALID       , 0x0000020A)\
RESULT_CODE(DRV_FLASH_MAX_CHIP_ERASE_NOT_VALID         , 0x0000020B)\
RESULT_CODE(DRV_FLASH_SECTOR_DATA_NOT_VALID            , 0x00000210)\
RESULT_CODE(DRV_FLASH_GEOMETRY_NOT_VALID               , 0x00000211)\
RESULT_CODE(DRV_FLASH_CANNOT_FIND_CMD_SET              , 0x00000212)\
RESULT_CODE(DRV_FLASH_BUF_WR_SW_TIMEOUT                , 0x00000221)\
RESULT_CODE(DRV_FLASH_BUF_WR_DEV_TIMEOUT               , 0x00000222)\
RESULT_CODE(DRV_FLASH_BUF_WR_ABORT                     , 0x00000223)\
RESULT_CODE(DRV_FLASH_BYPASS_WR_COUNT_INVALID          , 0x00000230)\
RESULT_CODE(DRV_FLASH_BYPASS_WR_SW_TIMEOUT             , 0x00000231)\
RESULT_CODE(DRV_FLASH_BYPASS_WR_DEV_TIMEOUT            , 0x00000232)\
RESULT_CODE(DRV_FLASH_BYPASS_WR_SUSPENDED              , 0x00000233)\
RESULT_CODE(DRV_FLASH_WORD_WR_SW_TIMEOUT               , 0x00000241)\
RESULT_CODE(DRV_FLASH_WORD_WR_DEV_TIMEOUT              , 0x00000242)\
RESULT_CODE(DRV_FLASH_CHIP_ERASE_FAIL                  , 0x00000250)\
RESULT_CODE(DRV_FLASH_ERASE_SUSPEND_FAIL               , 0x00000252)\
RESULT_CODE(DRV_FLASH_PROGRAM_SUSPEND_FAIL             , 0x00000253)\
RESULT_CODE(DRV_FLASH_ERASE_RESUME_FAIL                , 0x00000254)\
RESULT_CODE(DRV_FLASH_PROGRAM_RESUME_FAIL              , 0x00000255)\
RESULT_CODE(DRV_DPRAM_PBIT_REG_INIT_FAIL               , 0x00000280)\
RESULT_CODE(DRV_DPRAM_PBIT_RAM_TEST_FAILED             , 0x00000281)\
RESULT_CODE(DRV_DPRAM_TX_FULL                          , 0x00000282)\
RESULT_CODE(DRV_DPRAM_ERR_RX_OVERFLOW                  , 0x00000283)\
RESULT_CODE(DRV_DPRAM_INTR_ERROR                       , 0x00000284)\
RESULT_CODE(DRV_ADC_PBIT_REG_INIT_FAIL                 , 0x000002C1)\
RESULT_CODE(DRV_A429_FPGA_BAD_STATE                    , 0x00000300)\
RESULT_CODE(DRV_A429_RX_LFR_TEST_FAIL                  , 0x00000301)\
RESULT_CODE(DRV_A429_RX_FIFO_TEST_FAIL                 , 0x00000302)\
RESULT_CODE(DRV_A429_TX_FIFO_TEST_FAIL                 , 0x00000304)\
RESULT_CODE(DRV_A429_RX_LFR_FIFO_TEST_FAIL             , 0x00000307)\
RESULT_CODE(DRV_A429_TX_RX_LOOP_FAIL                   , 0x00000308)\
RESULT_CODE(DRV_UART_PBIT_REG_INIT_FAIL                , 0x00000480)\
RESULT_CODE(DRV_UART_ERR_PORT_ALREADY_OPEN             , 0x00000490)\
RESULT_CODE(DRV_UART_ERR_PORT_NOT_OPEN                 , 0x00000491)\
RESULT_CODE(DRV_UART_ERR_PORT_IDX_OUT_OF_RANGE         , 0x00000492)\
RESULT_CODE(DRV_UART_TX_OVERFLOW                       , 0x00000493)\
RESULT_CODE(DRV_UART_RX_OVERFLOW                       , 0x00000494)\
RESULT_CODE(DRV_UART_RX_HW_ERROR                       , 0x00000495)\
RESULT_CODE(DRV_UART_INTR_ERROR                        , 0x00000496)\
RESULT_CODE(DRV_UART_PORT_NOT_OPEN                     , 0x00000497)\
RESULT_CODE(DRV_UART_0_CBIT_TXOV_CNT                   , 0x00000498)\
RESULT_CODE(DRV_UART_0_CBIT_RXOV_CNT                   , 0x00000499)\
RESULT_CODE(DRV_UART_0_CBIT_FRAMING_ERR_CNT            , 0x0000049A)\
RESULT_CODE(DRV_UART_0_CBIT_PARITY_ERR_CNT             , 0x0000049B)\
RESULT_CODE(DRV_UART_1_CBIT_TXOV_CNT                   , 0x0000049C)\
RESULT_CODE(DRV_UART_1_CBIT_RXOV_CNT                   , 0x0000049D)\
RESULT_CODE(DRV_UART_1_CBIT_FRAMING_ERR_CNT            , 0x0000049E)\
RESULT_CODE(DRV_UART_1_CBIT_PARITY_ERR_CNT             , 0x0000049F)\
RESULT_CODE(DRV_UART_2_CBIT_TXOV_CNT                   , 0x000004A0)\
RESULT_CODE(DRV_UART_2_CBIT_RXOV_CNT                   , 0x000004A1)\
RESULT_CODE(DRV_UART_2_CBIT_FRAMING_ERR_CNT            , 0x000004A2)\
RESULT_CODE(DRV_UART_2_CBIT_PARITY_ERR_CNT             , 0x000004A3)\
RESULT_CODE(DRV_UART_3_CBIT_TXOV_CNT                   , 0x000004A4)\
RESULT_CODE(DRV_UART_3_CBIT_RXOV_CNT                   , 0x000004A5)\
RESULT_CODE(DRV_UART_3_CBIT_FRAMING_ERR_CNT            , 0x000004A6)\
RESULT_CODE(DRV_UART_3_CBIT_PARITY_ERR_CNT             , 0x000004A7)\
RESULT_CODE(DRV_UART_0_PBIT_FAILED                     , 0x000004A8)\
RESULT_CODE(DRV_UART_1_PBIT_FAILED                     , 0x000004A9)\
RESULT_CODE(DRV_UART_2_PBIT_FAILED                     , 0x000004AA)\
RESULT_CODE(DRV_UART_3_PBIT_FAILED                     , 0x000004AB)\
RESULT_CODE(DRV_FPGA_BAD_VERSION                       , 0x000004C0)\
RESULT_CODE(DRV_FPGA_NOT_CONFIGURED                    , 0x000004C1)\
RESULT_CODE(DRV_FPGA_CONFIGURED_FAIL                   , 0x000004C2)\
RESULT_CODE(DRV_FPGA_INT_FAIL                          , 0x000004C3)\
RESULT_CODE(DRV_FPGA_MCF_SETUP_FAIL                    , 0x000004C4)\
RESULT_CODE(DRV_FPGA_WD_FAIL                           , 0x000004C5)\
RESULT_CODE(DRV_BTO_REG_INIT_FAIL                      , 0x000004D0)\
RESULT_CODE(SYS_CM_RTC_PBIT_FAIL                       , 0x00060000)\
RESULT_CODE(SYS_NV_FILE_OPEN                           , 0x000A0000)\
RESULT_CODE(SYS_NV_FILE_NOT_OPEN                       , 0x000A0800)\
RESULT_CODE(SYS_NV_FILE_OPEN_ERR                       , 0x000A1000)\
RESULT_CODE(SYS_NV_WRITE_PENDING                       , 0x000A1800)\
RESULT_CODE(SYS_NV_FILE_END_EXCEEDED                   , 0x000A2000)\
RESULT_CODE(SYS_NV_WRITE_DRIVER_FAIL                   , 0x000A2800)\
RESULT_CODE(SYS_LOG_FIND_NEXT_WRITE_FAIL               , 0x000C0000)\
RESULT_CODE(SYS_LOG_REQUEST_INVALID                    , 0x000C0800)\
RESULT_CODE(SYS_LOG_QUEUE_FULL                         , 0x000C1000)\
RESULT_CODE(SYS_LOG_WRITE_TO_LARGE                     , 0x000C1800)\
RESULT_CODE(SYS_LOG_ERASE_CMD_FAILED                   , 0x000C2000)\
RESULT_CODE(SYS_MEM_INIT_FAIL                          , 0x000E0000)\
RESULT_CODE(SYS_MEM_ADDRESS_RANGE_INVALID              , 0x000E0800)\
RESULT_CODE(SYS_MEM_ERASE_TIMEOUT                      , 0x000E1800)\
RESULT_CODE(SYS_MSI_SEND_BUF_FULL                      , 0x00120000)\
RESULT_CODE(SYS_MSI_MS_PACKET_BAD_SIZE                 , 0x00120800)\
RESULT_CODE(SYS_MSI_MS_PACKET_BAD_CHKSUM               , 0x00121000)\
RESULT_CODE(SYS_MSI_CMD_HANDLER_NOT_ADDED              , 0x00121800)\
RESULT_CODE(SYS_MSI_INTERFACE_NOT_READY                , 0x00122000)\
RESULT_CODE(SYS_A429_PBIT_FAIL                         , 0x00160000)\
RESULT_CODE(SYS_A429_STARTUP_TIMEOUT                   , 0x00160800)\
RESULT_CODE(SYS_A429_DATA_LOSS_TIMEOUT                 , 0x00161000)\
RESULT_CODE(SYS_A429_SSM_FAIL                          , 0x00161800)\
RESULT_CODE(SYS_QAR_PBIT_LOOPBACK                      , 0x00180000)\
RESULT_CODE(SYS_QAR_PBIT_INTERNAL_DPRAM0               , 0x00180800)\
RESULT_CODE(SYS_QAR_PBIT_INTERNAL_DPRAM1               , 0x00181000)\
RESULT_CODE(SYS_QAR_PBIT_DPRAM                         , 0x00181800)\
RESULT_CODE(SYS_QAR_FPGA_BAD_STATE                     , 0x00182000)\
RESULT_CODE(SYS_QAR_STARTUP_TIMEOUT                    , 0x00182800)\
RESULT_CODE(SYS_QAR_DATA_LOSS_TIMEOUT                  , 0x00183000)\
RESULT_CODE(SYS_QAR_WORD_TIMEOUT                       , 0x00183800)\
RESULT_CODE(SYS_QAR_PBIT_FAIL                          , 0x00184000)\
RESULT_CODE(SYS_FPGA_CRC_FAIL                          , 0x00240000)\
RESULT_CODE(SYS_FPGA_FORCE_RELOAD_FAIL                 , 0x00240800)\
RESULT_CODE(SYS_FPGA_FORCE_RELOAD_SUCCESS              , 0x00241000)\
RESULT_CODE(SYS_FPGA_CBIT_REG_FAIL_SW                  , 0x00241800)\
RESULT_CODE(SYS_FPGA_CBIT_REG_FAIL_HW                  , 0x00242000)\
RESULT_CODE(SYS_BOX_POWER_ON_COUNT_RETRIEVE_FAIL       , 0x00260000)\
RESULT_CODE(SYS_GSE_PUT_LINE_TOO_LONG                  , 0x00280000)\
RESULT_CODE(SYS_GSE_GET_LINE_TOO_LONG                  , 0x00280800)\
RESULT_CODE(SYS_UART_STARTUP_TIMEOUT                   , 0x002A0800)\
RESULT_CODE(SYS_UART_DATA_LOSS_TIMEOUT                 , 0x002A1000)\
RESULT_CODE(SYS_UART_WORD_TIMEOUT                      , 0x002A4000)\
RESULT_CODE(SYS_UART_F7X_NEW_DL_FOUND                  , 0x002A8000)\
RESULT_CODE(SYS_UART_F7X_DL_NOT_RECOGNIZED             , 0x002A9000)\
RESULT_CODE(SYS_UART_PBIT_UART_OPEN_FAIL               , 0x002AF000)

/******************************************************************************
                                   Package Typedefs                          
******************************************************************************/

//Enumerate the result codes here.  ResultCodes.c provides stringification
//of each code.

typedef enum{
  RESULT_CODES
  MAX_RESULT_CODE
}RESULT;

/******************************************************************************
                                   Package Exports                             
******************************************************************************/
#undef EXPORT

#if defined ( DRV_RESULTCODES_BODY )
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
EXPORT const CHAR* RcGetResultCodeString(RESULT Result, CHAR* outstr);


#endif // DRV_RESULTCODES_H 
/*************************************************************************
 *  MODIFICATIONS
 *    $History: ResultCodes.h $
 * 
 * *****************  Version 57  *****************
 * User: Contractor2  Date: 6/27/11    Time: 1:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
 * memory is referenced
 * 
 * *****************  Version 56  *****************
 * User: Contractor2  Date: 5/10/11    Time: 2:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #891 Enhancement: Incomplete FPGA initialization functionality
 * 
 * *****************  Version 55  *****************
 * User: Jim Mood     Date: 10/26/10   Time: 5:12p
 * Updated in $/software/control processor/code/drivers
 * SCR 958 Result Codes reentrancy
 * 
 * *****************  Version 54  *****************
 * User: John Omalley Date: 10/25/10   Time: 6:04p
 * Updated in $/software/control processor/code/drivers
 * SCR 961 - Code Review Updates
 * 
 * *****************  Version 53  *****************
 * User: John Omalley Date: 10/18/10   Time: 5:45p
 * Updated in $/software/control processor/code/drivers
 * SCR 954 - Fixed the Result Codes number duplications and made all
 * others correct based on original design.
 * 
 * *****************  Version 52  *****************
 * User: John Omalley Date: 10/04/10   Time: 10:49a
 * Updated in $/software/control processor/code/drivers
 * SCR 895 - Removed Dead Code
 * 
 * *****************  Version 51  *****************
 * User: John Omalley Date: 10/01/10   Time: 5:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 893 - Code Coverage Updates
 * 
 * *****************  Version 50  *****************
 * User: John Omalley Date: 9/30/10    Time: 1:24p
 * Updated in $/software/control processor/code/drivers
 * SCR 894 - LogManager Code Coverage / Code Review Updates
 * 
 * *****************  Version 49  *****************
 * User: John Omalley Date: 9/10/10    Time: 10:01a
 * Updated in $/software/control processor/code/drivers
 * SCR 858 - Removed RESULT codes per fault default cases being added
 * 
 * *****************  Version 48  *****************
 * User: Contractor2  Date: 7/30/10    Time: 6:27p
 * Updated in $/software/control processor/code/drivers
 * SCR #243 Implementation: CBIT of mssim
 * Block MS Interface usage if not ready
 * 
 * *****************  Version 47  *****************
 * User: John Omalley Date: 7/23/10    Time: 10:14a
 * Updated in $/software/control processor/code/drivers
 * SCR 639 - Added a Mem Erase code
 * 
 * *****************  Version 46  *****************
 * User: Contractor2  Date: 7/09/10    Time: 4:31p
 * Updated in $/software/control processor/code/drivers
 * SCR #8 Implementation: External Interrupt Monitors
 * 
 * *****************  Version 45  *****************
 * User: Peter Lee    Date: 6/17/10    Time: 5:37p
 * Updated in $/software/control processor/code/drivers
 * SCR #635 Create log to indicate "unrecognized" dump list detected.
 * 
 * *****************  Version 44  *****************
 * User: Contractor2  Date: 6/14/10    Time: 1:03p
 * Updated in $/software/control processor/code/drivers
 * Function Names must begin with the CSC it belongs with.
 * 
 * *****************  Version 43  *****************
 * User: Peter Lee    Date: 6/08/10    Time: 5:07p
 * Updated in $/software/control processor/code/drivers
 * SCR #635 Add Uart PBIT result code
 * 
 * *****************  Version 42  *****************
 * User: Peter Lee    Date: 5/31/10    Time: 6:42p
 * Updated in $/software/control processor/code/drivers
 * SCR #618 F7X and UartMgr Requirements Implementation
 * 
 * *****************  Version 41  *****************
 * User: Contractor3  Date: 5/27/10    Time: 10:56a
 * Updated in $/software/control processor/code/drivers
 * SCR #617 - Changes made to meet coding standards as a result of code
 * review.
 * 
 * *****************  Version 40  *****************
 * User: Jeff Vahue   Date: 3/23/10    Time: 3:36p
 * Updated in $/software/control processor/code/drivers
 * SCR# 496 - Move GSE from driver to sys, make StatusStr variadic
 * 
 * *****************  Version 39  *****************
 * User: Contractor V&v Date: 3/04/10    Time: 3:52p
 * Updated in $/software/control processor/code/drivers
 * SCR #67 Interrupted SPI Access Added SPI timeout error code
 * 
 * *****************  Version 38  *****************
 * User: Contractor2  Date: 3/02/10    Time: 12:22p
 * Updated in $/software/control processor/code/drivers
 * SCR# 472 - Fix file/function header
 * 
 * *****************  Version 37  *****************
 * User: Peter Lee    Date: 2/01/10    Time: 3:27p
 * Updated in $/software/control processor/code/drivers
 * SCR #432 Add Arinc429 Tx-Rx LoopBack PBIT
 * 
 * *****************  Version 36  *****************
 * User: Peter Lee    Date: 1/28/10    Time: 9:19a
 * Updated in $/software/control processor/code/drivers
 * SCR #427 ClockMgr.c Issues and SRS-3090
 * 
 * *****************  Version 35  *****************
 * User: Jeff Vahue   Date: 12/17/09   Time: 12:10p
 * Updated in $/software/control processor/code/drivers
 * SCR# 364 - change INT8 -> CHAR add version info
 * 
 * *****************  Version 34  *****************
 * User: Peter Lee    Date: 12/16/09   Time: 2:17p
 * Updated in $/software/control processor/code/drivers
 * QAR Req 3110, 3041, 3114 and 3118
 * 
 * *****************  Version 33  *****************
 * User: Peter Lee    Date: 12/10/09   Time: 9:55a
 * Updated in $/software/control processor/code/drivers
 * SCR #358 Updated Arinc429 Tx Flag Processing 
 * 
 * *****************  Version 32  *****************
 * User: Peter Lee    Date: 12/04/09   Time: 5:22p
 * Updated in $/software/control processor/code/drivers
 * SCR #353 FPGA new QARFRAM1 and QARFRAM2 DPRAM FPGA Reg Support
 * 
 * *****************  Version 31  *****************
 * User: Contractor V&v Date: 11/18/09   Time: 3:45p
 * Updated in $/software/control processor/code/drivers
 * Implementing SCR 123  - DIO_ReadPin() returns RESULT
 * 
 * *****************  Version 30  *****************
 * User: Peter Lee    Date: 10/15/09   Time: 9:59a
 * Updated in $/software/control processor/code/drivers
 * FPGA CBIT Requirements
 * 
 * *****************  Version 29  *****************
 * User: Peter Lee    Date: 10/05/09   Time: 1:10p
 * Updated in $/software/control processor/code/drivers
 * SCR #287 QAR_SensorTest() records log and debug msg. 
 * 
 * *****************  Version 28  *****************
 * User: Peter Lee    Date: 9/28/09    Time: 4:19p
 * Updated in $/software/control processor/code/drivers
 * SCR #275 Box Power On Usage Req.   Result code for if box power on
 * usage data can not be retrieved. 
 * 
 * *****************  Version 27  *****************
 * User: Peter Lee    Date: 9/15/09    Time: 6:06p
 * Updated in $/software/control processor/code/drivers
 * SCR #256 Misc Arinc429 PBIT Log Updates
 * 
 * *****************  Version 26  *****************
 * User: Peter Lee    Date: 9/14/09    Time: 3:48p
 * Updated in $/software/control processor/code/drivers
 * SCR #94 Updated Init for SET_CHECK() and return ERR CODE Struct
 * 
 * *****************  Version 25  *****************
 * User: John Omalley Date: 8/06/09    Time: 3:47p
 * Updated in $/software/control processor/code/drivers
 * SCR 179
 *    Added missing driver result codes for the flash.
 * 
 * *****************  Version 24  *****************
 * User: Jim Mood     Date: 2/05/09    Time: 11:26a
 * Updated in $/control processor/code/drivers
 * Added additional result codes for the EEPROM driver and RTC driver
 * 
 * *****************  Version 23  *****************
 * User: Peter Lee    Date: 2/03/09    Time: 5:18p
 * Updated in $/control processor/code/drivers
 * Updates Arinc429, QAR and FPGA result codes
 * 
 * *****************  Version 22  *****************
 * User: Peter Lee    Date: 1/30/09    Time: 4:43p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates 
 * Add SSM Failure Detection to SensorTest()
 * 
 * *****************  Version 21  *****************
 * User: Peter Lee    Date: 1/21/09    Time: 11:26a
 * Updated in $/control processor/code/drivers
 * SCR #131 Add PBIT LFR Test 
 * 
 * *****************  Version 20  *****************
 * User: Peter Lee    Date: 1/20/09    Time: 6:26p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc429 Updates
 * Add Startup and Ch Data Loss TimeOut Result code for CBIT
 * 
 * *****************  Version 19  *****************
 * User: Peter Lee    Date: 1/14/09    Time: 5:11p
 * Updated in $/control processor/code/drivers
 * SCR #131 Arinc 429 Updates.  
 * 
 * *****************  Version 18  *****************
 * User: Peter Lee    Date: 1/06/09    Time: 2:52p
 * Updated in $/control processor/code/drivers
 * SCR 129 Misc FPGA Updates
 * 
 * *****************  Version 17  *****************
 * User: Peter Lee    Date: 11/24/08   Time: 2:12p
 * Updated in $/control processor/code/drivers
 * SCR #97 QAR SW Updates
 * 
 * *****************  Version 16  *****************
 * User: Jim Mood     Date: 10/08/08   Time: 10:40a
 * Updated in $/control processor/code/drivers
 * 
 * *****************  Version 15  *****************
 * User: Jim Mood     Date: 9/25/08    Time: 11:32a
 * Updated in $/control processor/code/drivers
 * Added result codes for NVMgr.c system module.  Updated comments 
 * 
 * *****************  Version 14  *****************
 * User: John Omalley Date: 6/10/08    Time: 11:15a
 * Updated in $/control processor/code/drivers
 * SCR-0026 - Added Result Codes for Log Verify Deleted
 * 
 *
 *
 ***************************************************************************/

