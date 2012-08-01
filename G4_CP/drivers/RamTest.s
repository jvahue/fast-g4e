;******************************************************************************
;            Copyright (C) 2010-2011 Pratt & Whitney Engine Services, Inc. 
;                      Altair Engine Diagnostic Solutions
;               All Rights Reserved. Proprietary and Confidential.
;               
;  File:          RamTest.s      
; 
;* Version
;* $Revision: 8 $  $Date: 8/22/11 2:47p $
;
;  Description:   Ram Test Memory
;                 This module contains routines that destructively tests RAM
;                 including the stack if in the specified memory range.
;                 No local ram storage is used during testing.
;                 
;

;----------------------------------------------------------------------------
; Software Specific Includes
;----------------------------------------------------------------------------

    INCLUDE MCF548X_GPIO.INC   ;GPIO reg and bit defs

;---------------------------------------------------------------------------
; Watchdog support 
;---------------------------------------------------------------------------                            
HW_VER_1    .equ 0      ; define as 1 for old hardware support

; FPGA Addr
FPGA_BASE_ADDR          .equ    $40000000
FPGA_WD_TIMER_ADDR      .equ    $40005A5A
FPGA_WD_TIMER_INV_ADDR  .equ    $4000A5A5
FPGA_WD_TIMER_VAL       .equ    $AAAA
FPGA_WD_TIMER_INV_VAL   .equ    $5555

.if HW_VER_1==1
WDOG_BIT                .equ    6           ; Legacy Watchdog timer bit position
.endif

WDOG_CTR                .equ    $30000      ; # of bytes before kick watchdog

;*****************************************************************************/
;* Code                                                                      */
;*****************************************************************************/
    SECTION ".text","ax"

    XDEF    RamTestData
    XDEF    RamTestAddr
    XDEF    RamTestPattern

; Test Points
    XDEF    RTD_TP
    XDEF    RTA_TP
    XDEF    RTP_TP1
    XDEF    RTP_TP2

; This pattern & inverse used in test - ref in PBIT
    XDEF    RAM_TEST_PATTERN
RAM_TEST_PATTERN    dc.l    $55555555

;---------------------------------------------------------------------------
; RamTestData:
;       Test the data lines by using a walking 1 test.
;       Test assumes 32 bit data bus. Smaller busses (8/16) just duplicates
;       test on additional addresses.
;
; Inputs:
;        A0 = Program Return Address (RTS)
;        A1 = Beginning Address (Must be even address)
; Outputs:
;        D0 = Zero if Success or Failed write pattern
;        D1 = Failed data read value
;        A1 = Failing Address
;
; Modifies:     D0,D1
;---------------------------------------------------------------------------
RamTestData:
    moveq.l     #1,D0       ; initial test pattern
    
RTD20:
    move.l      D0,(A1)     ; write pattern to memory
    move.l      (A1),D1     ; Read back memory
RTD_TP:
    cmp.l       D0,D1       ; compare contents
    bne.s       RTDExit     ; error - return
    lsl.l       #1,D0       ; walk 1
    bcc.s       RTD20       ; write next

RTDExit:
    jmp         (A0)        ; return to caller


;---------------------------------------------------------------------------
; RamTestAddr:
;       Test the Address lines
;       This test puts a value into the first location in RAM, and then,
;       by shifting a bit and adding that to the base address, to other
;       locations separated by an address line.
;
; Inputs:
;        A0 = Program Return Address (RTS)
;        A1 = Beginning Address (Must be even address)
;        D3 = Number of Address Lines
; Outputs:
;        D0 = Zero if Success or Failed write Pattern
;        D1 = Failed read value
;        A1 = Failing Address
;
; Modifies:     D1,D2,D3,D4,A2
;---------------------------------------------------------------------------
RamTestAddr:
    move.l      D3,D4               ; save copy of number of address lines
    moveq.l     #1,D1               ; initial address offset

    moveq.l     #1,D0               ; initial write pattern
    move.b      D0,(A1)             ; write pattern to base address
RTA20:
    addq.l      #2,D0               ; inc write pattern
    move.b      D0,0(A1,D1)         ; write pattern to offset address
    
    lsl.l       #1,D1               ; next offset
    subq.l      #1,D3               ; dec address line counter
    bne.s       RTA20               ; write to next offset

; Initial write is done - now check written values    
    moveq.l     #1,D0               ; initial write pattern
    moveq.l     #1,D2               ; initial address offset
    moveq.l     #0,D1               ; init for return
    move.l      A1,A2               ; copy base address
    move.b      (A1),D1             ; read back data
    cmp.b       D0,D1               ; check pattern
    bne.s       RTAExit             ; error exit
    move.b      #0,(A1)             ; reset byte to 0

RTA30:
    addq.l      #2,D0               ; inc compare pattern
    lea.l       0(A2,D2),A1         ; offset address
    move.b      (A1),D1             ; read back from offset address
RTA_TP:
    cmp.b       D0,D1               ; check pattern
    bne.s       RTAExit             ; error exit
    
    lsl.l       #1,D2               ; next offset
    subq.l      #1,D4               ; dec address line counter
    bne.s       RTA30               ; check next offset

; Test Passed - set return code
    moveq.l     #0,D0               ; test passed code
    
RTAExit:
    jmp         (A0)                ; return
    
;---------------------------------------------------------------------------
; RamTestPattern:
;       Test Memory by writing alternating patterns.
;       Memory set to 0 on exit
;
; Inputs:
;        A0 = Program Return Address (RTS)
;        A1 = Beginning Address (Must be even address)
;        D1 = Number of bytes to test.
; Outputs:
;        D0 = Zero if Success or Fail Pattern
;        D1 = Failed read value
;        A1 = Failing Address
;
; Modifies:     D0,D1,D3,D4,A3,A4,A5
;---------------------------------------------------------------------------
RamTestPattern:
    move.l      #WDOG_CTR/4,D4  ; watchdog kick timer
.if HW_VER_1==1
    movea.l     #MCF_GPIO_PODR_FEC1L,A3
.endif
    movea.l     #FPGA_WD_TIMER_ADDR,A4      ; FPGA_WD_TIMER_ADDR
    movea.l     #FPGA_WD_TIMER_INV_ADDR,A5  ; FPGA_WD_TIMER_INV_ADDR

    move.l      D1,D3           ; copy number of bytes to test
    lsr.l       #2,D3           ; convert to number of 32 bit words
    move.l      RAM_TEST_PATTERN(PC),D0 ; initial test pattern
    
RTP20:
    move.l      D0,(A1)     ; write pattern to memory
    move.l      (A1),D1     ; Read back memory
RTP_TP1:
    cmp.l       D0,D1       ; compare contents
    bne.s       RTPExit     ; error - return

    not.l       D0          ; inverse of pattern
    move.l      D0,(A1)     ; write pattern to memory
    move.l      (A1),D1     ; Read back memory
RTP_TP2:
    cmp.l       D1,D0       ; compare contents
    bne.s       RTPExit     ; error - return
    
    add.l       #4,A1       ; next address
    subq.l      #1,D4       ; time to kick dog?
    bne.s       RTP30       ; no - continue

    ; Kick watchdog 
.if HW_VER_1==1
    bset        #WDOG_BIT,(A3)              ; Toggle watchdog ON
    bclr        #WDOG_BIT,(A3)              ; Toggle watchdog OFF
.endif
    move.w      #FPGA_WD_TIMER_VAL,(A4)     ; Move WD_TIMER_VAL to FPGA_WD_TIMER_ADDR
    move.w      #FPGA_WD_TIMER_INV_VAL,(A5) ; Move WD_TIMER_INV_VAL to FPGA_WD_TIMER_INV_ADDR
    move.l      #WDOG_CTR/4,D4              ; reset watchdog kick timer

RTP30:
    subq.l      #1,D3       ; end of test?
    bgt.s       RTP20       ; test next address
    
    moveq.l     #0,D0       ; Test Pass code

RTPExit:
    jmp         (A0)        ; return to caller


;*********************************************
;*  MODIFICATIONS
;*     $History: RamTest.s $
;*
;******************  Version 8  *****************
;*User: Contractor2  Date: 8/22/11    Time: 2:47p
;*Updated in $/software/control processor/code/drivers
;*SCR #1021 Error: Data Manager Buffers Full Fault Detected
;*Fix reboot on init problem. Improved speed of ram test.
;*
;******************  Version 7  *****************
;*User: Contractor2  Date: 9/24/10    Time: 3:02p
;*Updated in $/software/control processor/code/drivers
;*SCR #882 WD - Old Code for pulsing the WD should be removed
;*
;******************  Version 6  *****************
;*User: Contractor2  Date: 8/09/10    Time: 11:40a
;*Updated in $/software/control processor/code/drivers
;*SCR #707 Misc - Modify code for coverage data collection
;*
;******************  Version 5  *****************
;*User: Contractor2  Date: 8/05/10    Time: 12:11p
;*Updated in $/software/control processor/code/drivers
;*SCR #707 Misc - Modify code for coverage data collection
;*Added vss keywords. Added testpoint labels.
;*
;*********************************************
