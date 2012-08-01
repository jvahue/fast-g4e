;******************************************************************************
;            Copyright (C) 2007-2011 Pratt & Whitney Engine Services, Inc. 
;                      Altair Engine Diagnostic Solutions
;               All Rights Reserved. Proprietary and Confidential.
;               
;  File:          exception.s      
; 
;* Version
;* $Revision: 13 $  $Date: 6/27/11 1:47p $
;
;  Description:   Exception processes and displays debug data for
;                 unhandled user interrupts, core processor exceptions, and
;                 ASSERT macro failures.
;
;                 Debug information for Exceptions includes the exception stack
;                 frame, registers, and a call trace.
;
;                 The ASSERT information omits the stack frame data because it
;                 only applies to Exceptions.
;
;                 Exception dump format below:

;                 FATAL ERROR: UNHANDLED or CORE EXCEPTION
;                                   Debug Information
;                 Stack Frame:
;                 format:##  vector:##  PC:########  FS:##  SR:####
;
;                 Regs:
;                 D0:########  D1:########  D2:########  D3:########
;                 D4:########  D5:########  D6:########  D7:########
;
;                 A0:########  A1:########  A2:########  A3:########
;                 A4:########  A5:########  A6:########  A7:########
;                 Stack Trace:
;                 RetAddr  CallTo
;                 ######## ########
;                 ######## ########
;                 ######## ########
;                 ######## _main()
;                 Rebooting......................
;
;
;  Requires:
;
;*****************************************************************************/

;*****************************************************************************/
;* Software Specific Includes                                                */
;*****************************************************************************/

    INCLUDE MCF548X_GPIO.INC   ;GPIO reg and bit defs
    INCLUDE MCF548X_PSC.INC    ;PSC (serial controller) reg and bit defs

;*****************************************************************************/
;* Local Variables                                                           */
;*****************************************************************************/
    SECTION ".bss","ab"
    XDEF    ExOutputBuf
    XDEF    ReOutputBuf

ASSERT_MAX_LOG_SIZE EQU 1024        ; This value must match define in Assert.h
MIN_RET_ADDR_CHECK  EQU 0x40000000  ; Min value for valid return address check 
MAX_RET_ADDR_CHECK  EQU 0x7FFFFFFF  ; Max value for valid return address check 

    
DRegSave      DS.L 8
ARegSave      DS.L 8
FormatSave    DS.L 1
VectorSave    DS.L 1
PCSave        DS.L 1
FSSave        DS.L 1
SRSave        DS.L 1
ExOutputBuf   DS.B ASSERT_MAX_LOG_SIZE  ;For storing stack dump output
ReOutputBuf   DS.B ASSERT_MAX_LOG_SIZE  ;For storing exception/reg dump output
OutputEndPtr  DS.L 1
OutputPtr     DS.L 1

;*****************************************************************************/
;* Constants and strings                                                     */
;*****************************************************************************/
;   SECTION ".data","a"
    SECTION ".text","ax"
CR            EQU     13              ; CR serial char
LF            EQU     10              ; LF serial char
CRLF          DC.B    CR,LF,0
HeaderLn1:    DC.B    CR,LF,CR,LF,CR,LF,CR,LF,"FATAL ERROR - UNHANDLED or CORE EXCEPTION ",0
HeaderLn2:    DC.B    CR,LF,"                  Debug Information",0
HeaderStack:  DC.B    CR,LF,"Stack Frame:",0
FormatStr:    DC.B    CR,LF,"format:",0
VectorStr:    DC.B    "  vector:",0
FSStr:        DC.B    "  FS:",0
SRStr:        DC.B    "  SR:",0
HeaderRegs:   DC.B    CR,LF,CR,LF,"Registers:",0
PCStr:        DC.B    CR,LF,"PC:",0
D0Str:        DC.B    CR,LF,"D0:",0    
D1Str:        DC.B    "  D1:",0    
D2Str:        DC.B    "  D2:",0    
D3Str:        DC.B    "  D3:",0    
D4Str:        DC.B    CR,LF,"D4:",0    
D5Str:        DC.B    "  D5:",0    
D6Str:        DC.B    "  D6:",0    
D7Str:        DC.B    "  D7:",0    
A0Str:        DC.B    CR,LF,"A0:",0    
A1Str:        DC.B    "  A1:",0    
A2Str:        DC.B    "  A2:",0    
A3Str:        DC.B    "  A3:",0    
A4Str:        DC.B    CR,LF,"A4:",0    
A5Str:        DC.B    "  A5:",0    
A6Str:        DC.B    "  A6:",0    
A7Str:        DC.B    "  A7:",0
StackTrace:   DC.B    CR,LF,CR,LF,"Stack Trace      size: 0x",0
StackTraceDone: DC.B  CR,LF,"trace done",0
StackTraceError: DC.B CR,LF,"Address out of range, trace stopped",0
StackLabels:  DC.B    CR,LF,"RetAddr  CallTo",CR,LF,0
StackInstErr: DC.B    "decode error",0
StackMainStr: DC.B    "_main",0
RebootingStr: DC.B    CR,LF,"Rebooting......................",0

; The HexTable must remain in Flash for use by PBIT
HexTable      DC.B    "0123456789ABCDEF"
    ALIGN 2


;*****************************************************************************/
;* Code                                                                      */
;*****************************************************************************/
    SECTION ".text","ax"
    XDEF    BusTimeoutException
    XDEF    UnexpectedException
    XDEF    StackWalk
    XDEF    AssertDump
    
    XDEF    SetXMT
    XDEF    PutSerialString
    XDEF    PutHexString

    XDEF    InitGSEBuf
    
;---------------------------------------------------------------------------
; InitGSEBuf: EXPORTED
;             This bit of code initializes the Output Buffer Pointers.
;             This function allows the Power-On Built In Test (PBIT) to
;             reuse the serial port functions to output messages before
;             the system is fully initialized.
;
; Inputs:
;
; Outputs:
;
; Modifies:     A0
;---------------------------------------------------------------------------
InitGSEBuf:
  LEA       ExOutputBuf,A0
  MOVEA.L   A0,OutputPtr
  ADD.L     #ASSERT_MAX_LOG_SIZE,A0
  MOVEA.L   A0,OutputEndPtr 
  rts

;---------------------------------------------------------------------------
; AssertDump: EXPORTED
;             Print the string contained in the ExOutputBuf directly to
;             the serial port.
;             Then, copy stack dump data to ExOutputBuf and also output it to 
;             the GSE port.  The stack dump string overwrites the ExOutputBuf
;
; Inputs:     ExOutputBuf: Contains a string from Assert.c that is to
;                               be printed directly to the Coldfire PSC
;                          Must be < ASSERT_MAX_LOG_SIZE chars
;
; Outputs:    ExOutputBuf: Contains the stack dump output
;
; Modifies:     
;---------------------------------------------------------------------------
AssertDump:
  ;All interrupts off 
  MOVE.W  #0x2700,SR
  ;Set 485 port to transmit.
  JSR     SetXMT  
  LEA     ExOutputBuf,A0               ; Print out caller's string
  JSR     PutSerialString

  LEA     ExOutputBuf,A0

  
AssertDump_1:
  MOVE.B      (A0)+,D0                 ; Find terminator of caller's string
  BNE         AssertDump_1
  SUBA        #1,A0
  MOVEA.L     A0,OutputPtr

  LEA     (ExOutputBuf+ASSERT_MAX_LOG_SIZE),A0  ; Setup end ptr for the chars that will
  MOVEA.L A0,OutputEndPtr                       ; be accessed by StackWalk
  
  ;(SP) has PC return [31-0]
  MOVE.L  (A7),D0
  MOVE.L  D0,PCSave

  JSR     StackWalk

  MOVEA.L OutputPtr,A0                 ;Null terminate the result buffer.
  MOVE.B  #0,(A0)


  RTS
  
;---------------------------------------------------------------------------
; BusTimeoutException: EXPORTED
;
;             Copy processor debug data to a buffer and output it to the
;             GSE port.  This records all the address and data registers,
;             the exception stack data and a stack trace.  
;
; Inputs:       
;
; Outputs:      
;
; Modifies:     
;---------------------------------------------------------------------------
BusTimeoutException:
  ;All interrupts off 
  MOVE.W  #0x2700,SR
  ;save regs, A0 first, then use A0 as an address pointer
  MOVE.L  A0,ARegSave
  LEA     ARegSave+4,A0
  MOVEM.L A1-A7,(A0)
  LEA     DRegSave,A0
  MOVEM.L D0-D7,(A0)

  ;Copy exception stack frame values (See Ref Manual "Exception Stack Frame Definition")

  ;(SP) has Fmt[31-28],FS[27-26],VEC[25-18],FS[17-16],SR[15-0]
  MOVE.L  (SP),D0
  MOVE.L  D0,SRSave
  LSR     #8,D0
  LSR     #8,D0
  MOVE.L  D0,FSSave
  LSR     #2,D0
  MOVE.L  D0,VectorSave
  LSR     #8,D0
  LSR     #2,D0
  MOVE.L  D0,FormatSave

  ;SP + 4 has PC [31-0]
  MOVE.L  4(A7),D0
  MOVE.L  D0,PCSave

  ;Set 485 port to transmit.
  JSR     SetXMT

  ;Set Bus Timeout Header Line
  JSR     BTO_Handler
  MOVE.L  D0,A0
  JSR     PutSerialString
  
  ;Initialize output buffer pointer before putting anything to the GSE
  LEA       ReOutputBuf,A0
  MOVEA.L   A0,OutputPtr
  ADD.L     #ASSERT_MAX_LOG_SIZE,A0
  MOVEA.L   A0,OutputEndPtr   

  ;Print information to the screen - header output by BTO_Handler
  LEA     HeaderLn2,A0
  JSR     PutSerialString
  LEA     HeaderStack,A0
  JSR     PutSerialString

  ;Print exception data to the screen
  JSR     PrintExceptionStackFrame
  JSR     PrintRegisters
  JSR     StackWalk

  ;Null terminate the output buffer.
  MOVEA.L OutputPtr,A0  
  MOVE.B  #0,(A0)

  ;Save data to EE using Assert.c function. (Assert_UnhandledException does
  ;not return)
  JSR     BTO_WriteToEE

  ;In case some one changes the above called function and it does return
BusTimeoutExceptionWait:
  ;wait for watchdog,   
  BRA     BusTimeoutExceptionWait

  

;---------------------------------------------------------------------------
; UnexpectedException: EXPORTED
;
;             Copy processor debug data to a buffer and output it to the
;             GSE port.  This records all the address and data registers,
;             the exception stack data and a stack trace.  
;
; Inputs:       
;
; Outputs:      
;
; Modifies:     
;---------------------------------------------------------------------------
UnexpectedException:
  ;All interrupts off 
  MOVE.W  #0x2700,SR
  ;save regs, A0 first, then use A0 as an address pointer
  MOVE.L  A0,ARegSave
  LEA     ARegSave+4,A0
  MOVEM.L A1-A7,(A0)
  LEA     DRegSave,A0
  MOVEM.L D0-D7,(A0)

  ;Copy exception stack frame values (See Ref Manual "Exception Stack Frame
  ;Definition")

  ;(SP) has Fmt[31-28],FS[27-26],VEC[25-18],FS[17-16],SR[15-0]
  MOVE.L  (SP),D0
  MOVE.L  D0,SRSave
  LSR     #8,D0
  LSR     #8,D0
  MOVE.L  D0,FSSave
  LSR     #2,D0
  MOVE.L  D0,VectorSave
  LSR     #8,D0
  LSR     #2,D0
  MOVE.L  D0,FormatSave

  ;SP + 4 has PC [31-0]
  MOVE.L  4(A7),D0
  MOVE.L  D0,PCSave

  ;Set 485 port to transmit.
  JSR     SetXMT

  ;Initialize output buffer pointer before putting anything to the GSE
  LEA       ReOutputBuf,A0
  MOVEA.L   A0,OutputPtr
  ADD.L     #ASSERT_MAX_LOG_SIZE,A0
  MOVEA.L   A0,OutputEndPtr   
  
  
  ;Print information to the screen
  LEA     HeaderLn1,A0
  JSR     PutSerialString
  LEA     HeaderLn2,A0
  JSR     PutSerialString
  LEA     HeaderStack,A0
  JSR     PutSerialString

  ;Print exception data to the screen
  JSR     PrintExceptionStackFrame
  JSR     PrintRegisters
  JSR     StackWalk

  ;Null terminate the output buffer.
  MOVEA.L OutputPtr,A0  
  MOVE.B  #0,(A0)

  ;Save data to EE using Assert.c function. (Assert_UnhandledException does
  ;not return)
  JSR     Assert_UnhandledException

  ;In case some one changes the above called function and it does return
UnexpectedExceptionWait:
  ;wait for watchdog,   
  BRA     UnexpectedExceptionWait

  

;---------------------------------------------------------------------------
; PrintExceptionStackFrame:
;               Output the value of the Exception or Interrupt stack frame
;               previously stored in FormatSave, VectorSave, PCSave, FSSave,
;               and SRSave.
;
;               Output format:
;
;                 Stack Frame:
;                 format:##  vector:##  PC:########  FS:##  SR:####
;
; Inputs:       FormatSave, VectorSave, PCSave, FSSave,
;               and SRSave.              
;
; Outputs:      None.
;
; Modifies:     A0,D0,D1
;---------------------------------------------------------------------------
PrintExceptionStackFrame:
  ;Stack frame information
  ; Format field
  LEA     FormatStr,A0
  JSR     PutSerialString
  MOVE.L  FormatSave,D0
  ANDI.L  #$F,D0
  MOVE.L  #8,D1
  JSR     PutHexString
  ; Vector field
  LEA     VectorStr,A0
  JSR     PutSerialString
  MOVE.L  VectorSave,D0
  ANDI.L  #$FF,D0
  MOVE.L  #8,D1
  JSR     PutHexString
  ; FS field
  LEA     FSStr,A0
  JSR     PutSerialString
  MOVE.L  FSSave,D0
  ANDI.L  #$FF,D0
  MOVE.L  #8,D1
  JSR     PutHexString
  ; SR field
  LEA     SRStr,A0
  JSR     PutSerialString
  MOVE.L  SRSave,D0
  ANDI.L  #$FFFF,D0
  MOVE.L  #16,D1
  JSR     PutHexString

PrintExceptionStackFrame_0:
  RTS


  
;---------------------------------------------------------------------------
; PrintRegisters:
;               Output the value of the Data and Address registers that were
;               previously stored in the DRegSave and ARegSave arrays.
;               Output format:
;
;                 Regs:
;                 PC:########
;                 D0:########  D1:########  D2:########  D3:########
;                 D4:########  D5:########  D6:########  D7:########
;
;                 A0:########  A1:########  A2:########  A3:########
;                 A4:########  A5:########  A6:########  A7:########
;
;
;
; Inputs:       DRegSave and ARegSave.
;               
;
; Outputs:      None.
;
; Modifies:     A0,D0,D1
;---------------------------------------------------------------------------
PrintRegisters:
  ;Data Registers
  LEA     HeaderRegs,A0
  JSR     PutSerialString
  ; PC
  LEA     PCStr,A0
  JSR     PutSerialString
  MOVE.L  PCSave,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D0
  LEA     D0Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D1
  LEA     D1Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+4,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D2
  LEA     D2Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+8,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D3
  LEA     D3Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+12,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D4
  LEA     D4Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+16,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D5
  LEA     D5Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+20,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D6
  LEA     D6Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+24,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; D7
  LEA     D7Str,A0
  JSR     PutSerialString
  MOVE.L  DRegSave+28,D0
  MOVE.L  #32,D1
  JSR     PutHexString

  ;Address Registers
  ; A0
  LEA     A0Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A1
  LEA     A1Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+4,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A2
  LEA     A2Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+8,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A3
  LEA     A3Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+12,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A4
  LEA     A4Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+16,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A5
  LEA     A5Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+20,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A6
  LEA     A6Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+24,D0
  MOVE.L  #32,D1
  JSR     PutHexString
  ; A7
  LEA     A7Str,A0
  JSR     PutSerialString
  MOVE.L  ARegSave+28,D0
  MOVE.L  #32,D1
  JSR     PutHexString

PrintRegisters_0:
  RTS



;---------------------------------------------------------------------------
; PutHexString: Prints a .b, .w, or .l binary to the console in ASCII-HEX
;
; Inputs:       D0 has the char to send
;               D1 has the data width in bits, 8, 16, or 32
;
; Outputs:      None.

;
;---------------------------------------------------------------------------
PutHexString:
  MOVE.L  D2,-(SP)
  MOVE.L  A0,-(SP)

  MOVE.L  D0,D2                   ; Save word, D0 is used as a working reg

  ;Convert number of nibbles to print to amount of shift needed to align 
  ;MS nibble in bits 0-3 (ex. 4 shifts for 8 bits, 12 shifts for 16 bits, etc) 
  SUBQ    #4,D1                    

  LEA     HexTable,A0             ; Point to the ASCII char table

PHS_1:
  MOVE.L  D2,D0                   ; Get word to print into D0
  LSR.L   D1,D0                   ; Shift to align the nibble into bits 0-3
  ANDI.L  #$F,D0                  
  MOVE.B  (0,A0,D0),D0            ; Use the nibble to index ASCII table and..
  JSR     PutSerialChar           ; print the char
  SUBQ    #4,D1
  BPL     PHS_1
  
PHS_0:
  MOVE.L  (SP)+,A0
  MOVE.L  (SP)+,D2
  RTS                             ; Return to caller

;---------------------------------------------------------------------------
; PutSerialString: Send a string to the serial port.
;
; Inputs:       A0 points to string to send
;
; Outputs:      None.

;
;---------------------------------------------------------------------------
PutSerialString:
  MOVE.L      A0,-(SP)            ; Save original string pointer
  MOVE.L      D0,-(SP)

PSS_Loop:
  MOVE.B      (A0)+,D0            ; Get next char to send
  BEQ         PSS_RTS             ; Br if no more chars to send
  JSR         PutSerialChar       ; Send out char
  BRA         PSS_Loop            ; Br to do more chars

PSS_RTS:
  MOVE.L      (SP)+,D0
  MOVE.L      (SP)+,A0
  RTS                             ; Return to caller

;---------------------------------------------------------------------------
; SetXMT: Set the UART for output
;
; Note: Works for both RS-232 and RS-485 modes.
;
; Inputs:       None.
;
; Outputs:      None.
;
;---------------------------------------------------------------------------
SetXMT:
  MOVE.W      D0,-(SP)            ; Save user's register
  MOVE.B      MCF_GPIO_PODR_PSC1PSC0,D0
  ORI         #$08,D0
  MOVE.B      D0,MCF_GPIO_PODR_PSC1PSC0

Xmt_Done:
  MOVE.W      (SP)+,D0            ; Restore user's register
  RTS


;---------------------------------------------------------------------------
; PutSerialChar: Send a character to the serial port.  
;
;
; Inputs:       D0 contains char to send
;
; Outputs:      None.
;
;---------------------------------------------------------------------------
PutSerialChar:
  MOVE.L  A0,-(SP)
  MOVE.L  D0,-(SP)                    ;Save caller's char

PutSerialChar_1:
  MOVEA.L  OutputPtr,A0                ;Put the char in the output buffer
  CMPA     OutputEndPtr,A0             ;Ptr past buffer limit?
  BGT      PutSerialChar_2             ; Y: Buffer full, char is dropped
  MOVE.B   D0,(A0)+                    ; N: Add char
  MOVEA.L  A0,OutputPtr

PutSerialChar_2:
  MOVE.W  MCF_PSC_SR0,D0              ;Test for room in the TX FIFO
  ANDI.L  #MCF_PSC_SR_TXRDY,D0        ;One byte available?
  BEQ     PutSerialChar_1             ; N: Keep checking
                                      ; Y: Put the byte in the buffer
  MOVE.L  (SP),D0
  MOVE.B  D0,MCF_PSC_TB0

PutSerialChar_0:
  MOVE.W  MCF_PSC_SR0,D0              ;Before exit, wait for the last byte
  ANDI.L  #MCF_PSC_SR_TXEMP_URERR,D0  ;to transmit.  This ensures we won't
  BEQ     PutSerialChar_0             ;switch to RX mode before the last
                                      ;byte is out.
  MOVE.L  (SP)+,D0
  MOVE.L  (SP)+,A0  
  RTS



;---------------------------------------------------------------------------
; StackWalk: 
;
; Inputs:    Current stack state is:
;
;            [Ret Addr      ] //from call to here
;            [Info          ] //exception frame
;            [PC @ exception] //exception frame, return to where exception 
;                             //occured
;            [Stack frame   ]
;            [A6            ]
;            [Ret addr      ]
;            [Param0..n     ]
;
; Outputs:      
;
;---------------------------------------------------------------------------
StackWalk:
  ;Print header and stack size info
  LEA     StackTrace,A0
  JSR     PutSerialString
  
  MOVE.L  #__SP_INIT,D0
  MOVE.L  SP,D1
  SUB.L   D1,D0
  MOVQ    #32,D1
  JSR     PutHexString
  LEA     StackLabels,A0
  JSR     PutSerialString
  
StackWalk_1:
  ;backup to the stack frame before the exception
  ;A6 has the pointer to the last stack frame, so unlink A6
  MOVE.L  A6,A0                ;This perfoms an "UNLK" without modifying SP.
  JSR     IsAddressInStack     ;Validate the address is in the stack range
  BCS     StackWalk_3
  MOVE.L  (A0),A6   
  ADDA    #4,A0                ;Point one long word before stack frame
  BSR     FindAndCheckRetType
  BCS     StackWalk_3          ;error
  BNE     StackWalk_NCall      ;Normal Call 

StakWalk_ExCall:
  MOVE.L  (A0),D0          ;Print out the Return address on the stack
  MOVQ    #32,D1
  JSR     PutHexString
  MOVE.B  #' ',D0
  JSR     PutSerialChar
  MOVE.B  #'v',D0
  JSR     PutSerialChar
  SUBA.L  #4,A0            ;Print out vector number
  MOVE.L  (A0),D0
  LSR     #8,D0
  LSR     #8,D0
  LSR     #2,D0
  MOVEQ   #8,D1
  JSR     PutHexString
  BRA     StackWalk_4
  
StackWalk_NCall:  
  MOVE.L  (A0),D0          ;Print out the Return address on the stack
  MOVQ    #32,D1
  JSR     PutHexString
  MOVE.B  #' ',D0
  JSR     PutSerialChar
  BSR     StackWalk_DecodeCallInstruction
  CMP.L   #0,D0
  BEQ     StackWalk_2

StackWalk_4:
  LEA     CRLF,A0
  JSR     PutSerialString
  BRA     StackWalk_1

StackWalk_3:
  LEA     StackTraceError,A0
  JSR     PutSerialString

StackWalk_2:
  LEA     StackTraceDone,A0
  JSR     PutSerialString
  
  RTS

 

;---------------------------------------------------------------------------
; StackWalk_DecodeCallInstruction:
; Determines the address of the entry point of the current subroutine by
; attempting to decode the BSR or JSR instruction preceding the return address
; on the stack.
; For the BSR opcode, the routine searches -2,-4,-6 bytes from the 
; return address looking for the BSR opcode (0x6100) If found, the size of
; the displacement is determined and the subroutine address is computed.
;
; For the JSR opcode, the routine searches -2,-4,-6 bytes from the return 
; address and attempts to decode the Mode and Register fields of the 
;
; Inputs:   A0: Pointer to the address of the instruction immediatly following 
;               the subroutine call.
;           
;
; Outputs: Prints address to serial port
;          D0: 0: This instruction was the call to _main
;              1: This instruction was not the call to _main
;
; Modifies A0,D0,D1
;---------------------------------------------------------------------------
StackWalk_DecodeCallInstruction:
  ;Save A6, it is used as a working register
  MOVE.L  A6,-(SP)                    

  ;Dereference the pointer to the return address and store the return address
  ;in A6
  MOVE.L  A0,A6
  ;ADDA    #8,A6
  MOVE.L  (A6),A6

  ;Backup one word and see if the "BSR" or "JSR" instruction exists there
  SUBA    #2,A6             
  MOVE.W  (A6),D0
  ANDI    #0xFF00,D0
  CMP.W   #0x6100,D0
  BEQ     StackWalk_DCI_8bitDisp
  MOVE.W  (A6),D0
  ANDI.L  #0xFFC0,D0;
  CMP.W   #0x4E80,D0
  BEQ     StackWalk_DCI_JSR

  ;Backup two words and see of the "BSR" or "JSR" instruction exists there
  SUBA    #2,A6
  MOVE.W  (A6),D0  
  CMP.W   #0x6100,D0
  BEQ     StackWalk_DCI_16bitDisp
  ANDI.L  #0xFFC0,D0
  CMP.W   #0x4E80,D0
  BEQ     StackWalk_DCI_JSR

  ;Backup three words and see of the "BSR" or "JSR" instruction exists there
  SUBA    #2,A6
  MOVE.W  (A6),D0  
  CMP.W   #0x6100,D0
  BEQ     StackWalk_DCI_32bitDisp
  ANDI.L  #0xFFC0,D0
  CMP.W   #0x4E80,D0
  BEQ     StackWalk_DCI_JSR

  ;Else, cannot find instruction the stack is incoherent here.
  BRA     StackWalk_DCI_PrintError
  
;JSR, decode 3-bit "Mode" field bits [5:3]
StackWalk_DCI_JSR:
  MOVE.W  (A6),D0
  ANDI    #0x38,D0
  LSR     #3,D0
  CMP.W   #2,D0
  BEQ     StackWalk_DCI_JSRAy
  CMP.W   #7,D0
  BEQ     StackWalk_DCI_JSRConst
  BRA     StackWalk_DCI_PrintError

StackWalk_DCI_JSRAy:
  ;Call by pointer, print out address register used to call
  MOVE.B  #'(',D0
  JSR     PutSerialChar
  MOVE.B  #'A',D0
  JSR     PutSerialChar
  MOVE.W  (A6),D0
  ANDI    #0x7,D0
  MOVE.L  #4,D1
  JSR     PutHexString
  MOVE.B  #')',D0
  JSR     PutSerialChar
  BRA     StackWalk_DCI_0     

StackWalk_DCI_JSRConst:
  MOVE.W  (A6),D0
  ANDI    #0x7,D0
  CMP.W   #0,D0
  BEQ     StackWalk_DCI_JSRConstW
  CMP.W   #1,D0
  BEQ     StackWalk_DCI_JSRConstL
  CMP.W   #2,D0
  BEQ     StackWalk_DCI_16bitDisp
  BRA     StackWalk_DCI_PrintError
  
StackWalk_DCI_JSRConstW:
  MVS.W   (2,A6),D0
  BRA     StackWalk_DCI_PrintAddr
  
StackWalk_DCI_JSRConstL:
  MOVE.L  (2,A6),D0
  BRA     StackWalk_DCI_PrintAddr
  
;8-bit BSR displacement
StackWalk_DCI_8bitDisp: 
  MOVE.L  A6,D0
  MVS.B   (1,A6),D1
  ADD     D1,D0
  BRA     StackWalk_DCI_PrintAddr

;16-bit displacement
StackWalk_DCI_16bitDisp: 
  MOVE.L  A6,D0
  MVS.W   (2,A6),D1
  ADD     D1,D0
  ADDQ    #2,D0
  BRA     StackWalk_DCI_PrintAddr

;32-bit displacement  
StackWalk_DCI_32bitDisp: 
  MOVE.L  A6,D0
  MOVE.L  (2,A6),D1
  ADD     D1,D0
;  BRA     StackWalk_DCI_PrintAddr

StackWalk_DCI_PrintAddr:
  CMP.L   #main,D0
  BEQ     StackWalk_DCI_PrintMain
  MOVE.L  #32,D1
  JSR     PutHexString
  BRA     StackWalk_DCI_0 

StackWalk_DCI_PrintMain:
  LEA     StackMainStr,A0
  JSR     PutSerialString
  BRA     StackWalk_DCI_0_main

StackWalk_DCI_PrintError:
  LEA     StackInstErr,A0
  JSR     PutSerialString  
;  BRA     StackWalk_DCI_0 

;Set return value as 0
StackWalk_DCI_0_main:
  CLR.L   D0
  
  MOVE.L  (SP)+,A6
  RTS

;Set return value as 1
StackWalk_DCI_0:
  MOVEQ   #1,D0
  MOVE.L  (SP)+,A6
  RTS

 
;---------------------------------------------------------------------------
; IsAddressInStack: 
;
; Inputs:    Checks the address passed in A0 is within the range of the
;            stack section
;            A0: Address to check
;
; Outputs:   Carry Clear: Address is within stack range
;            Carry Set:   Address is not within stack range
;
;---------------------------------------------------------------------------
IsAddressInStack:
  ;must be less than initial stack address
  CMPA.L  #__SP_INIT,A0  
  BLT     IsAddressInStack_1
  BRA     IsAddressInStack_NO
IsAddressInStack_1:
  ;and greater than the end address
  CMPA.L  #__SP_END,A0  
  BGT     IsAddressInStack_YES
  ;BRA     IsAddressInStack_NO
    
IsAddressInStack_NO:
  MOVE.B  #1,CCR
  RTS

IsAddressInStack_YES:
  MOVE.B  #0,CCR  
  RTS



;---------------------------------------------------------------------------
; FindAndCheckRetType: 
;
; Inputs:    Checks the address pointed to by A0 and determines if it looks
;            like a valid return address.  The location pointed to is likely
;            to be a return address in the case of a normal call, or it is
;            the CPU state in the case of an interrupt.
;            The CPU state is identified when the high
;            nibble is either a 4,5,6 or 7.  If the high nibble of the 
;            address pointed to is 4,5,6, or 7, it is assumed this is the saved
;            CPU state, and A0 is advanced 4 bytes check the previous longword. 
;            
;            For the FAST system, the normal address range for a return address
;            is 0x1000 0000 to 0x11FF FFFF.  Any address with the high nibble
;            != 4,5,6, or 7 is assumed to be a normal call.
;
;            A0: Pointer to address to check
;
; Outputs:   Carry Clear:  Address appears to be a valid return address. Z flag
;                          indicates the return type
;              Zero Clear: Normal call
;              Zero Set:   Exception
;            Carry Set:   Address is not a return address
;
;---------------------------------------------------------------------------
FindAndCheckRetType:
  MOVE.L    (A0),D0
  ;Check if address pointed to is between 0x40000000 and 0x7FFFFFFF
  CMPI.L    #MIN_RET_ADDR_CHECK,D0
  BLT       FindAndCheckRetType_1
  CMPI.L    #MAX_RET_ADDR_CHECK,D0
  BGT       FindAndCheckRetType_1
  
  ;Return address not found. If this was an interrupt, the return address
  ;will be found 4 bytes after the CPU state save.  Check that location. 
  ADDA      #4,A0
  MOVE.L    (A0),D0
  CMPI.L    #MIN_RET_ADDR_CHECK,D0
  BLT       FindAndCheckRetType_2
  CMPI.L    #MAX_RET_ADDR_CHECK,D0
  BGT       FindAndCheckRetType_2

  ;Not a valid return address
  MOVE.B    #1,CCR
  BRA       FindAndCheckRetType_0

  ;Type is a normal return from call
FindAndCheckRetType_1:
  MOVE.B    #0,CCR
  BRA       FindAndCheckRetType_0

  ;Type is a return from exception 
FindAndCheckRetType_2:
  MOVE.B    #4,CCR
  
FindAndCheckRetType_0:
  
  RTS

;*********************************************
;*  MODIFICATIONS
;*     $History: exception.s $
;*
;******************  Version 13  *****************
;*User: Contractor2  Date: 6/27/11    Time: 1:47p
;*Updated in $/software/control processor/code/drivers
;*SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
;*memory is referenced
;*
;******************  Version 12  *****************
;*User: Jim Mood     Date: 11/08/10   Time: 4:06p
;*Updated in $/software/control processor/code/drivers
;*SCR 979 Unhandled exception log missing data
;*
;******************  Version 11  *****************
;*User: Contractor2  Date: 9/27/10    Time: 2:55p
;*Updated in $/software/control processor/code/drivers
;*SCR #795 Code Review Updates
;*
;******************  Version 10  *****************
;*User: Contractor2  Date: 8/09/10    Time: 12:12p
;*Updated in $/software/control processor/code/drivers
;*SCR #707 Misc - Modify code for coverage data collection
;*Add Source Control tags
;*
;*
;*********************************************
