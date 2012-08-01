;******************************************************************************
;            Copyright (C) 2010-2011 Pratt & Whitney Engine Services, Inc. 
;                      Altair Engine Diagnostic Solutions
;               All Rights Reserved. Proprietary and Confidential.
;               
;  File:          PBIT.s      
; 
;* Version
;* $Revision: 21 $  $Date: 8/24/11 2:45p $
;
;  Description:   Power on Built In Tests.
;
;                 Performs destructive RAM tests and CRC verification of
;                 the program in flash. If any tests fail, a message is
;                 sent to the GSE port and the processor is halted.
;                 If the tests pass, the stack is filled with a pattern
;                 in preparation for run time stack depth testing.
;
;                 Since this code destructively writes to RAM, it must be
;                 called after processor initialization, but before any
;                 code requiring Ram or the Stack.
;
;*****************************************************************************/


;----------------------------------------------------------------------------
; Software Specific Includes
;----------------------------------------------------------------------------

    INCLUDE MCF548X_GPIO.INC   ;GPIO reg and bit defs
    
    XREF    StackFill
    XREF    RAM_TEST_PATTERN

;*****************************************************************************/
;* Package Defines                                                           */
;*****************************************************************************/

VERBOSE_OUTPUT  .equ 0          ; define as 1 for debug output
HW_VER_1        .equ 0          ; define as 1 for old hardware support
RAM_PATTERN_TEST .equ 0         ; define as 1 to enable Ram Pattern Testing

; Base address of MBAR peripherals area
MBAR_BASE:    .equ  0x80000000

; Memory-mapped registers within MBAR peripherals area
SRAMBEG:      .equ  MBAR_BASE+0x00010000           ; On Chip SRAM Beginning address
SRAMEND:      .equ  MBAR_BASE+0x00017fff           ; On Chip SRAM End address

; Flags at the beginning of SRAM to detect an unexpected program restart
SRAM_INIT_FLAG          .equ    0                       ; SRAMBEG
SRAM_INIT_FLAG_INV      .equ    SRAM_INIT_FLAG+4        ; SRAM_INIT_FLAG+0x04
WATCHDOG_RESTART_FLAG   .equ    SRAM_INIT_FLAG_INV+4    ; SRAM_INIT_FLAG_INV+0x04
ASSERT_COUNT            .equ    WATCHDOG_RESTART_FLAG+4 ; count of assert restarts
UNKNOWN_RESTART_COUNT   .equ    ASSERT_COUNT+4          ; count of watchdog restarts
INIT_RESTART_COUNT      .equ    UNKNOWN_RESTART_COUNT+4 ; count of restarts during init
SRAM_INIT_SAVE          .equ    INIT_RESTART_COUNT+4    ; save the original value at POR
SRAM_INV_SAVE           .equ    SRAM_INIT_SAVE+4        ; save the original value at POR
RESTART_DATA_END        .equ    SRAM_INV_SAVE+4         ; END DATA SET

; following code duplicated in watchdog.h
ASSERT_RESTART_SET      .equ    1           ; Code for watchdog assert restart
UNKNOWN_RESTART_SET     .equ    2           ; Code for unexpected restart
INIT_RESTART_SET        .equ    3           ; Code for unexpected restart during init

SRAM_INIT_SET           .equ    $e1d2c3b4   ; Ram Init Pattern - somewhat random
SRAM_INIT_PROCESS       .equ    $abcd1234   ; Ram pattern to detect restarts during init 

;---------------------------------------------------------------------------
; Watchdog support 
; FPGA Addr
FPGA_BASE_ADDR          .equ    $40000000
FPGA_WD_TIMER_ADDR      .equ    $40005A5A
FPGA_WD_TIMER_INV_ADDR  .equ    $4000A5A5
FPGA_WD_TIMER_VAL       .equ    $AAAA
FPGA_WD_TIMER_INV_VAL   .equ    $5555

.if HW_VER_1==1
WDOG_BIT                .equ    6           ; Legacy Watchdog timer bit position
.endif

WDOG_CTR                EQU     $30000      ; # of bytes before kick watchdog
;---------------------------------------------------------------------------

; CRC function commands (from Utility.h)
CRC_FUNC_SINGLE: .equ   0 
CRC_FUNC_START: .equ    1
CRC_FUNC_NEXT:  .equ    2
CRC_FUNC_END:   .equ    3

; external memory address
SDRAMLINES:     .equ    25                  ; Number of address lines

; Test fail data log layout
MTFAILTYPE:     .equ    0                   ; Test Type
MTFAILADDR:     .equ    MTFAILTYPE+4        ; Test failed address
MTFAILWRITE:    .equ    MTFAILADDR+4        ; Test failed write pattern
MTFAILREAD:     .equ    MTFAILWRITE+4       ; Test failed read back pattern
MTFAILSZ:       .equ    MTFAILREAD+4        ; size of Test log entry

; CRC32 function call stack layout
CRCADDR:        .equ    0                   ; CRC Start Address
CRCBCNT:        .equ    CRCADDR+4           ; Byte count to CRC
CRCDEST:        .equ    CRCBCNT+4           ; CRC Destination Address
CRCFUNC:        .equ    CRCDEST+4           ; CRC Function Code
CRCSTKSZ:       .equ    CRCFUNC+4           ; size of CRC32 call stack

;*****************************************************************************/
;* Constants and strings                                                     */
;*****************************************************************************/
    SECTION ".text","a"

; stack fill code for stack depth check
;StackFill   dc.l    $deadbeef

; error display messages
CR          EQU     13              ; CR serial char
LF          EQU     10              ; LF serial char
CRLF        dc.b    CR,LF,0
StartHeader dc.b    CR,LF,LF,LF,"PBIT Running... ",0
OkMsg       dc.b    "OK",CR,LF,LF,0
RestartMsg  dc.b    "Restarting....",CR,LF,0
FailHeader  dc.b    CR,LF,"PBIT FATAL ERROR - ",0
RamDataFail dc.b    "RAM Data Line at address: 0x",0
RamAddrFail dc.b    "RAM Address Line at address: 0x",0
RamPtrnFail dc.b    "RAM Pattern Test at address: 0x",0
CrcFail     dc.b    "CRC mismatch at address: 0x",0
Expected    dc.b    CR,LF,"  Expected: 0x",0
Actual      dc.b    "  Read: 0x",0
    ALIGN   2

;*****************************************************************************/
;* Code                                                                      */
;*****************************************************************************/

; WEAK xref to __ghs_romend allows program to test if running in ram
    WEAK    __ghs_romend

; __ghsbegin_sdabase marks the end of used ram section, which
; encompasses the uninitialized and initialized ram sections.
; It does not include the stack area.
    WEAK    __ghsbegin_sdabase

    XREF    __ghs_ramstart
    XREF    __ghs_ramend
    XREF    __SP_END            ; bottom of stack
    XREF    __SP_INIT           ; top of stack

    XREF    RamTestData
    XREF    RamTestAddr
    XREF    RamTestPattern
    
    XREF    CRC32
    XREF    __CRC_START
    XREF    __CRC_END

; Display code in exception.s
    XREF    SetXMT
    XREF    PutSerialString
    XREF    PutHexString
    XREF    InitGSEBuf

    XREF    _start              ; main code startup
    
    XDEF    pbit

; Test Points
    XDEF    CRC_CMP
    XDEF    PBIT_FAIL

;---------------------------------------------------------------------------

;    SECTION ".data"
;    SECTION ".text","ax"

;*********************************************
; Run Ram/CRC Tests before Startup
;*********************************************


;*****************************************************************************/
; pbit
;
;Inputs: 
; none
;
;Description:
; 1.0   Check bit patterns in 1st 8 bytes of internal SRAM to determine
;         if this is a cold start (power-on) or warm start (reset)
; 1.1   Test if PBIT interrupted.
; 1.2   If PBIT interrupted, flag Init restart and goto SDRAM test (3.0)
; 1.3   else check if internal ram test interrupted.
; 1.4   If yes, goto internal ram test (2.0)
; 1.5   else check if warm start.
; 1.6   If yes, test and set restart type (Assert or Unknown)
;         and goto external SDRAM test (3.0)
; 1.7   else cold start - goto internal ram test (2.0)
; 2.0   Run Pattern Test on internal Ram.
; 2.1   Set PBIT in progress pattern and clear internal ram
; 2.2   If Pattern Test fail, set fail flag and output fail message
; 3.0   Check if executing in RAM.
; 3.1   If yes, skip to stack ram test (4.0)
; 3.2   Run Data Line Test on SDRAM
; 3.3   If fail, set fail flag, output fail message and goto CRC test (5.0)
; 3.4   Run Address Line Test on SDRAM
; 3.5   If fail, set fail flag, output fail message and goto CRC test (5.0)
; 3.4   Run pattern Test on allocated SDRAM only
; 3.5   If fail, set fail flag, output fail message and goto Stack test (4.0)
; 4.0   Run pattern Test on SDRAM allocated for Stack
; 4.1   If fail, set fail flag and output fail message
; 4.2   Fill stack area with pattern for stack depth test during runtime
; 5.0   Check program CRC
; 5.1   If CRC fail, set fail flag and output fail message
; 6.0   If PBIT fail flag set, output message and enter wait loop for watchdog reset
; 6.1   else set PBIT complete pattern in 1st 8 bytes of internal ram
;         and jump to main program.
;
; Modifies: consider all registers modified after call
;
; Returns:  none
;*****************************************************************************/
pbit:
    lea.l   SRAMEND,a7          ; temp init stack pointer to processor internal ram
    move.l  #0,d7               ; init halt on PBIT fail flag

; Kick watchdog before start of tests
.if HW_VER_1==1
    movea.l #MCF_GPIO_PODR_FEC1L,a0
    bset    #WDOG_BIT,(a0)              ; Toggle watchdog ON
    bclr    #WDOG_BIT,(a0)              ; Toggle watchdog OFF
.endif
    movea.l #FPGA_WD_TIMER_ADDR,a0      ; Move WD_TIMER_VAL to FPGA_WD_TIMER_ADDR
    move.w  #FPGA_WD_TIMER_VAL,(a0)
    movea.l #FPGA_WD_TIMER_INV_ADDR,a0  ; Move WD_TIMER_INV_VAL to FPGA_WD_TIMER_INV_ADDR
    move.w  #FPGA_WD_TIMER_INV_VAL,(a0)

.if VERBOSE_OUTPUT==1
    jsr     InitGSEBuf
    jsr     SetXMT              ; turn on output
    lea.l   StartHeader,a0      ; PBIT Start header
    jsr     PutSerialString     ; output header
.endif

; Only run pattern test on processor internal ram
;   As the address and data lines are internal to the processor,
;   it is assumed the internal ram addressing is OK.
; Then clear the area to store test status.
; At end of startup, any ram failures found here will be logged.
; The first 2 long words are an ID pattern to detect warm restarts.
; The 3rd long word is set by the main program to indicate a forced restart.
; The 4th long word is the count of assert (expected) restarts.
; The 5th long word is the count of unexpected or watchdog restarts.
; A count of 0 means all ram tests have passed.
; NOTE: various ID patterns are used to detect where a restart may have occured.
; d6 - holds the original POR value of SRAM_INIT_FLAG
; d5 - holds the original POR value of SRAM_INIT_FLAG_INV
    lea.l   SRAMBEG,a1
    move.l  SRAM_INIT_FLAG(a1),d6       ; get first location contents
    move.l  SRAM_INIT_FLAG_INV(a1),d5   ; get next location contents

    ; first check if restart occurred during PBIT process
    cmp.l   #SRAM_INIT_PROCESS,d6   ; check init in process pattern set
    beq     RAMTST008               ; match - don't run ram test - flag unexpected restart

    ; check if we were interrupted during internal ram test
    ; if so, re-run test. This 'debounces' the system boot.
    move.l  RAM_TEST_PATTERN,d0     ; get ram test fill pattern
    cmp.l   d0,d6
    beq.s   RAMTST000               ; match - run ram test
    cmp.l   d0,d5                   ; inverse pattern in second word
    beq.s   RAMTST000               ; match - run ram test

    ; now check for warm start
    cmp.l   #SRAM_INIT_SET,d6   ; check init pattern set
    bne.s   RAMTST000           ; it isn't - run ram test
    move.l  d5,d1               ; copy for check
    eor.l   d6,d1               ; eor in first location contents
    add.l   #1,d1               ; if warm start should result in 0
    beq.s   RAMTST004           ; it is - don't run ram test - flag unexpected restart

    ; cold start
RAMTST000:
    ; Run destructive ram test on internal memory
    lea.l   RAMTST001(pc),a0        ; set return address
    move.l  #(SRAMEND-SRAMBEG+1),d1 ; number of bytes to test
    jmp     RamTestPattern          ; test ram

RAMTST001:
    lea.l   SRAMBEG,a2
    ; set first 2 locations which are restart detect flags
    move.l  #SRAM_INIT_PROCESS,d2   ; PBIT in process pattern
    move.l  d2,(a2)+
    not.l   d2
    move.l  d2,(a2)+
    move.l  #((SRAMEND-SRAMBEG+1)/4)-2,d2   ; number of long words to clear
    ; clear internal memory - do not change return status from test
RAMTST002:
    clr.l   (a2)+               ; clear memory for error storage
    sub.l   #1,d2
    bne.s   RAMTST002

    ; check return status of internal ram test
    tst.l   d0
    beq.s   RAMTST010           ; OK - run next test
; save error info
    move.l  #1,d7               ; set fail halt flag
    lea.l   RAMTST010(pc),a0    ; set return address
    lea.l   RamPtrnFail(pc),a2  ; fail message
    bra     OutputFailInfo

; unexpected restart - flag it for degraded mode
RAMTST004:
    move.l  WATCHDOG_RESTART_FLAG(a1),d1    ; get assert flag
    cmp.l   #ASSERT_RESTART_SET,d1          ; check if assert flag set
    bne.s   RAMTST006
    add.l   #1,ASSERT_COUNT(a1)             ; increment assert counter
    bra.s   RAMTST010
RAMTST006:
    move.l  #UNKNOWN_RESTART_SET,WATCHDOG_RESTART_FLAG(a1); set unexpected reset flag
    add.l   #1,UNKNOWN_RESTART_COUNT(a1)    ; inc counter
    bra.s   RAMTST010

; unexpected restart during PBIT - flag it for degraded mode
RAMTST008:
    move.l  #INIT_RESTART_SET,WATCHDOG_RESTART_FLAG(a1) ; set unexpected reset flag
    add.l   #1,INIT_RESTART_COUNT(a1)       ; inc counter

; Check if running in ram and skip ram test if yes
RAMTST010:
    lea.l   SRAMBEG,a1
    move.l  d6,SRAM_INIT_SAVE(a1)
    move.l  d5,SRAM_INV_SAVE(a1)
;
    move.l  #__ghs_romend,d0
    tst.l   d0
    beq     RAMTST100

; run data line ram test
    lea.l   RAMTST020(pc),a0    ; set return address
    lea.l   __ghs_ramstart,a1   ; start of SDRAM
    jmp     RamTestData         ; test data lines
    
RAMTST020:
    tst.l   d0                  ; check return status
    beq.s   RAMTST030           ; OK - run next test
; save error info
; if data line test fails, all other tests will, so skip them
    move.l  #1,d7               ; set fail halt flag
    lea.l   CRCTST010(pc),a0    ; set return address
    lea.l   RamDataFail(pc),a2  ; fail message
    bra     OutputFailInfo
    
; run address line ram test
RAMTST030:
    lea.l   RAMTST040(pc),a0    ; set return address
    lea.l   __ghs_ramstart,a1   ; start of SDRAM
    move.l  #SDRAMLINES,d3      ; number of address lines
    jmp     RamTestAddr         ; test data lines
    
RAMTST040:
    tst.l   d0                  ; check return status
    beq.s   RAMTST050           ; OK - run next test
; save error info
; if address line test fails, all other tests will, so skip them
    move.l  #1,d7               ; set fail halt flag
    lea.l   CRCTST010(pc),a0    ; set return address
    lea.l   RamAddrFail(pc),a2  ; fail message
    bra     OutputFailInfo

; run ram pattern write test    
RAMTST050:
.if RAM_PATTERN_TEST==1
    lea.l   RAMTST060(pc),a0    ; set return address
    lea.l   __ghs_ramstart,a1   ; start of SDRAM
    move.l  #__ghsbegin_sdabase,d1
    sub.l   a1,d1               ; number of bytes to test
    jmp     RamTestPattern      ; test ram
    
RAMTST060:
    tst.l   d0                  ; check return status
    beq.s   RAMTST070           ; OK - run next test
; save error info
    move.l  #1,d7               ; set fail halt flag
    lea.l   RAMTST100(pc),a0    ; set return address
    lea.l   RamPtrnFail(pc),a2  ; fail message
    bra     OutputFailInfo
.endif

; Clear used memory section
RAMTST070:
    move.l  #WDOG_CTR/4,d4      ; watchdog kick timer
.if HW_VER_1==1
    movea.l #MCF_GPIO_PODR_FEC1L,a4
.endif
    movea.l #FPGA_WD_TIMER_ADDR,a5      ; FPGA_WD_TIMER_ADDR
    movea.l #FPGA_WD_TIMER_INV_ADDR,a6  ; FPGA_WD_TIMER_INV_ADDR
    
    lea.l   __ghs_ramstart,a1   ; start of SDRAM
    move.l  #__ghsbegin_sdabase,d1
    sub.l   a1,d1               ; number of bytes to test
    lsr.l   #2,d1               ; convert to number of 32 bit words
    
RAMTST080:
    clr.l   (a1)+               ; clear memory
    subq.l  #1,d4               ; time to kick dog?
    bne.s   RAMTST090           ; no - continue

    ; Kick watchdog 
.if HW_VER_1==1
    bset    #WDOG_BIT,(a4)              ; Toggle watchdog ON
    bclr    #WDOG_BIT,(a4)              ; Toggle watchdog OFF
.endif
    move.w  #FPGA_WD_TIMER_VAL,(a5)     ; Move WD_TIMER_VAL to FPGA_WD_TIMER_ADDR
    move.w  #FPGA_WD_TIMER_INV_VAL,(a6) ; Move WD_TIMER_INV_VAL to FPGA_WD_TIMER_INV_ADDR
    move.l  #WDOG_CTR/4,D4              ; reset watchdog kick timer

RAMTST090:
    subq.l  #1,d1           ; end of test?
    bgt.s   RAMTST080       ; clear next address

; Test Ram in stack area
; Data and Address line tests have already been done.
RAMTST100:
.if RAM_PATTERN_TEST==1
    lea.l   RAMTST110(pc),a0    ; set return address
    lea.l   __SP_END,a1         ; bottom of stack
    move.l  #__SP_INIT,d1
    sub.l   a1,d1               ; number of bytes to test
    jmp     RamTestPattern      ; test stack ram
    
RAMTST110:
    tst.l   d0                  ; check return status
    beq.s   RAMTST120           ; OK - run next test
; save error info
    move.l  #1,d7               ; set fail halt flag
    lea.l   RAMTST120(pc),a0    ; set return address
    lea.l   RamPtrnFail(pc),a2  ; fail type code
    bra     OutputFailInfo
.endif

; Initialize stack to set value so max stack depth can
; be determined during runtime.
RAMTST120:
    lea.l   __SP_END,a2         ; bottom of stack
    move.l  #__SP_INIT,d2       ; top of stack
    sub.l   a2,d2               ; number of bytes
    lsr.l   #2,d2               ; number of 32 bit words
    move.l  StackFill,d1        ; fill pattern
RAMTST130:
    move.l  d1,(a2)+            ; init memory for stack depth test
    sub.l   #1,d2
    bne.s   RAMTST130

; kick watchdog    
CRCTST010:
.if HW_VER_1==1
    movea.l #MCF_GPIO_PODR_FEC1L,a0
    bset    #WDOG_BIT,(a0)              ; Toggle watchdog ON
    bclr    #WDOG_BIT,(a0)              ; Toggle watchdog OFF
.endif
    movea.l #FPGA_WD_TIMER_ADDR,a0      ; Move WD_TIMER_VAL to FPGA_WD_TIMER_ADDR
    move.w  #FPGA_WD_TIMER_VAL,(a0)
    movea.l #FPGA_WD_TIMER_INV_ADDR,a0  ; Move WD_TIMER_INV_VAL to FPGA_WD_TIMER_INV_ADDR
    move.w  #FPGA_WD_TIMER_INV_VAL,(a0)

; Run CRC Check on code
; This check calls C code, so the stack is needed.
    lea.l   __CRC_START,a4      ; start address to CRC'd memory
    move.l  #__CRC_END,d4       ; end of CRC'd memory
    sub.l   a4,d4               ; calc number of bytes to CRC
    move.l  #CRC_FUNC_START,d1  ; initial CRC function
    move.l  #WDOG_CTR/2,d2      ; number of CRC bytes before watchdog kick
    cmp.l   d2,d4               ; Just in case check for CRC block < max bytes
    bgt.s   CRCTST020
    move.l  d4,d2               ; reset byte count

CRCTST020:    
    lea.l   __SP_INIT-4,a7      ; init stack with extra space on top
    move.l  a7,a0               ; copy address - used for return value
    sub.l   #CRCSTKSZ,a7        ; allocate room for call data
    move.l  a0,CRCDEST(a7)      ; crc calc destination storage address

CRCTST030:    
    move.l  d1,CRCFUNC(a7)      ; function code
    move.l  d2,CRCBCNT(a7)      ; save byte count on stack
    move.l  a4,CRCADDR(a7)      ; start address to CRC'd memory
    jsr     CRC32               ; do calc

; kick watchdog    
.if HW_VER_1==1
    movea.l #MCF_GPIO_PODR_FEC1L,a0
    bset    #WDOG_BIT,(a0)              ; Toggle watchdog ON
    bclr    #WDOG_BIT,(a0)              ; Toggle watchdog OFF
.endif
    movea.l #FPGA_WD_TIMER_ADDR,a0      ; Move WD_TIMER_VAL to FPGA_WD_TIMER_ADDR
    move.w  #FPGA_WD_TIMER_VAL,(a0)
    movea.l #FPGA_WD_TIMER_INV_ADDR,a0  ; Move WD_TIMER_INV_VAL to FPGA_WD_TIMER_INV_ADDR
    move.w  #FPGA_WD_TIMER_INV_VAL,(a0)
    
; check for end of CRC 
    move.l  #CRC_FUNC_NEXT,d1   ; next function code
    add.l   d2,a4               ; adjust start for next cycle
    sub.l   d2,d4               ; sub CRC block size from total byte count
    beq.s   CRCTST040           ; done - finish up
    cmp.l   d2,d4               ; Just in case check for CRC block < max bytes
    bgt.s   CRCTST030
    move.l  d4,d2               ; reset byte count
    bra.s   CRCTST030

; done computing CRC
CRCTST040:
    move.l  d4,CRCBCNT(a7)      ; zero byte count on stack
    move.l  d4,CRCADDR(a7)      ; zero start address to CRC'd memory
    move.l  #CRC_FUNC_END,d0    ; end function code
    move.l  d0,CRCFUNC(a7)      ; end function code
    jsr     CRC32               ; finish calc

    adda.l  #CRCSTKSZ,a7        ; remove call data from stack
    move.l  (a7)+,d0            ; get calculated CRC
    move.l  (a4),d1             ; get pre-calculated CRC
CRC_CMP:
    cmp.l   d0,d1
    beq.s   CRCTST050           ; OK - continue
    
; CRC check failed - display info
    cmp.l   #0xFFFFFFFF,d1      ; uninitialized pattern?
.if VERBOSE_OUTPUT==1
    beq     CRCTST045           ; display error, but proceed normally
.else
    beq     CRCTST050           ; no display of error, and proceed normally
.endif
    move.l  #1,d7               ; set fail halt flag
CRCTST045:
    lea.l   CRCTST050(pc),a0    ; set return address
    move.l  a4,a1               ; copy fail address
    lea.l   CrcFail(pc),a2      ; Fail message
    bra     OutputFailInfo

CRCTST050:
    tst.l   d7                  ; test if halt flag set
    beq     CRCTST100           ; not set - start mainprogram
; FAIL - loop until watchdog reset    
    jsr     InitGSEBuf
    jsr     SetXMT              ; turn on output
    lea.l   RestartMsg,a0       ; PBIT Restart msg
    jsr     PutSerialString     ; output header
PBIT_FAIL:
    bra     PBIT_FAIL

CRCTST100:
.if VERBOSE_OUTPUT==1
    jsr     InitGSEBuf
    jsr     SetXMT              ; turn on output
    lea.l   OkMsg,a0            ; PBIT OK msg
    jsr     PutSerialString     ; output header
.endif

    ; set first 2 locations which are restart detect flags
    lea.l   SRAMBEG,a2
    move.l  #SRAM_INIT_SET,d2   ; PBIT complete pattern
    move.l  d2,(a2)+
    not.l   d2
    move.l  d2,(a2)

    jmp     _start              ; start main program

    XDEF    OutputFailInfo
;*********************************************
; OutputFailInfo:
;   Output Test Error Data

; Inputs:
;        a0 = Program Return Address (RTS)
;        a1 = Failing Address
;        a2 = Fail Message Address
;        d0 = Failed Write Pattern
;        d1 = Failed Read Memory Value
; Outputs:
;
; Modifies:     a3,d3,d4
;*********************************************
OutputFailInfo:
    move.l  a0,a3               ; save return address
    move.l  d0,d3               ; save failed write pattern
    move.l  d1,d4               ; save failed read pattern
    jsr     InitGSEBuf
    jsr     SetXMT              ; turn on output
    lea.l   FailHeader,a0       ; failure header
    jsr     PutSerialString     ; output header

    move.l  a2,a0               ; failure message
    jsr     PutSerialString     ; output message
    move.l  a1,d0               ; failure address
    move.l  #32,d1              ; data width
    jsr     PutHexString        ; output failed address

    lea.l   Expected,a0         ; Expected string
    jsr     PutSerialString     ; output message
    move.l  d3,d0               ; expected data pattern
    move.l  #32,d1              ; data width
    jsr     PutHexString        ; output failed data
    
    lea.l   Actual,a0           ; failure header
    jsr     PutSerialString     ; output message
    move.l  d4,d0               ; actual read data pattern
    move.l  #32,d1              ; data width
    jsr     PutHexString        ; output failed data
    
    lea.l   CRLF,a0             ; message ender
    jsr     PutSerialString     ; output message

    jmp     (a3)                ; return

;*********************************************
;*  MODIFICATIONS
;*     $History: PBIT.s $
;*
;******************  Version 21  *****************
;*User: Contractor2  Date: 8/24/11    Time: 2:45p
;*Updated in $/software/control processor/code/drivers
;*SCR #1061 Requirements: PBIT RAM Test
;*
;******************  Version 20  *****************
;*User: John Omalley Date: 8/23/11    Time: 11:52a
;*Updated in $/software/control processor/code/drivers
;*SCR 1061 - Removed RAM Pattern test from DDR SDRAM.
;*
;******************  Version 19  *****************
;*User: Contractor2  Date: 8/22/11    Time: 2:47p
;*Updated in $/software/control processor/code/drivers
;*SCR #1021 Error: Data Manager Buffers Full Fault Detected
;*Fix reboot on init problem.
;*
;******************  Version 18  *****************
;*User: Jim Mood     Date: 11/02/10   Time: 4:53p
;*Updated in $/software/control processor/code/drivers
;*SCR 977 Code Review changes
;*
;******************  Version 17  *****************
;*User: Contractor2  Date: 9/29/10    Time: 1:29p
;*Updated in $/software/control processor/code/drivers
;*SCR #808 Degraded Mode - still instances where degraded mode prompt
;*appears
;*Added module outline comments.
;*Changed code to ignore being interrupted during internal ram test.
;*
;******************  Version 16  *****************
;*User: Contractor2  Date: 9/24/10    Time: 3:02p
;*Updated in $/software/control processor/code/drivers
;*SCR #882 WD - Old Code for pulsing the WD should be removed
;*
;******************  Version 15  *****************
;*User: Contractor2  Date: 8/09/10    Time: 11:40a
;*Updated in $/software/control processor/code/drivers
;*SCR #707 Misc - Modify code for coverage data collection
;*
;******************  Version 14  *****************
;*User: Contractor2  Date: 8/05/10    Time: 12:12p
;*Updated in $/software/control processor/code/drivers
;*SCR #707 Misc - Modify code for coverage data collection
;*Added vss keywords. Added testpoint labels.
;*Corrected display of CRC fail info to GSE port
;*
;*********************************************
