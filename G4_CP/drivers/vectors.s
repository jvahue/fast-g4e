;/******************************************************************************
;         Copyright (C) 2007 - 2011 Pratt & Whitney Engine Services, Inc. 
;                      Altair Engine Diagnostic Solutions
;               All Rights Reserved. Proprietary and Confidential.
; 
; 
;  File:        vectors.s
; 
; 
;  Description: MCF548x interrupt vector table. This is a hard-coded vector table
;               located at 0x0 in ROM.
; 
;   VERSION
;    $Revision: 6 $  $Date: 6/27/11 1:52p $
; 
;******************************************************************************/

#ifdef  _UNDERSCORE_
#define __SP_INIT         ___SP_INIT
#define asm_startmeup     _asm_startmeup
#define asm_exception_handler   _asm_exception_handler
#define asm_exception_handler _asm_exception_handler
#endif

  .global VECTOR_TABLE
  .global _VECTOR_TABLE
  .global start

  .extern __SP_INIT
; .extern asm_startmeup
; .extern asm_exception_handler
; .extern asm_exception_handler

  SECTION ".vector","ax"
  

; Exception Vector Table

VECTOR_TABLE:
_VECTOR_TABLE:
INITSP:   .long __SP_INIT   ; Initial SP
INITPC:   .long startinit   ; Initial PC
vector002:  .long asm_exception_handler ; Access Error
vector003:  .long asm_exception_handler ; Address Error
vector004:  .long asm_exception_handler ; Illegal Instruction
vector005:  .long asm_exception_handler ; Reserved
vector006:  .long asm_exception_handler ; Reserved
vector007:  .long asm_exception_handler ; Reserved
vector008:  .long asm_exception_handler ; Privilege Violation
vector009:  .long asm_exception_handler ; Trace
vector010:  .long asm_exception_handler ; Unimplemented A-Line
vector011:  .long asm_exception_handler ; Unimplemented F-Line
vector012:  .long asm_exception_handler ; Debug Interrupt
vector013:  .long asm_exception_handler ; Reserved
vector014:  .long asm_exception_handler ; Format Error
vector015:  .long asm_exception_handler ; Unitialized Int.
vector016:  .long asm_exception_handler ; Reserved
vector017:  .long asm_exception_handler ; Reserved
vector018:  .long asm_exception_handler ; Reserved
vector019:  .long asm_exception_handler ; Reserved
vector020:  .long asm_exception_handler ; Reserved
vector021:  .long asm_exception_handler ; Reserved
vector022:  .long asm_exception_handler ; Reserved
vector023:  .long asm_exception_handler ; Reserved
vector024:  .long asm_exception_handler ; Spurious Interrupt
vector025:  .long asm_exception_handler ; Autovector Level 1
vector026:  .long asm_exception_handler ; Autovector Level 2
vector027:  .long asm_exception_handler ; Autovector Level 3
vector028:  .long asm_exception_handler ; Autovector Level 4
vector029:  .long asm_exception_handler ; Autovector Level 5
vector030:  .long asm_exception_handler ; Autovector Level 6
vector031:  .long asm_exception_handler ; Autovector Level 7
vector032:  .long asm_exception_handler ; TRAP #0
vector033:  .long asm_exception_handler ; TRAP #1
vector034:  .long asm_exception_handler ; TRAP #2
vector035:  .long asm_exception_handler ; TRAP #3
vector036:  .long asm_exception_handler ; TRAP #4
vector037:  .long asm_exception_handler ; TRAP #5
vector038:  .long asm_exception_handler ; TRAP #6
vector039:  .long asm_exception_handler ; TRAP #7
vector040:  .long asm_exception_handler ; TRAP #8
vector041:  .long asm_exception_handler ; TRAP #9
vector042:  .long asm_exception_handler ; TRAP #10
vector043:  .long asm_exception_handler ; TRAP #11
vector044:  .long asm_exception_handler ; TRAP #12
vector045:  .long asm_exception_handler ; TRAP #13
vector046:  .long asm_exception_handler ; TRAP #14
vector047:  .long asm_exception_handler ; TRAP #15
vector048:  .long asm_exception_handler ; Reserved
vector049:  .long asm_exception_handler ; Reserved
vector050:  .long asm_exception_handler ; Reserved
vector051:  .long asm_exception_handler ; Reserved
vector052:  .long asm_exception_handler ; Reserved
vector053:  .long asm_exception_handler ; Reserved
vector054:  .long asm_exception_handler ; Reserved
vector055:  .long asm_exception_handler ; Reserved
vector056:  .long asm_exception_handler ; Reserved
vector057:  .long asm_exception_handler ; Reserved
vector058:  .long asm_exception_handler ; Reserved
vector059:  .long asm_exception_handler ; Reserved
vector060:  .long asm_exception_handler ; Reserved
vector061:  .long asm_exception_handler ; Reserved
vector062:  .long asm_exception_handler ; Reserved
vector063:  .long asm_exception_handler ; Reserved
vector064:  .long asm_exception_handler   ; Unused
vector065:  .long asm_exception_handler   ; User int 1 Edge port flag 1
vector066:  .long asm_exception_handler   ; User int 2 Edge port flag 2
vector067:  .long asm_exception_handler   ; User int 3 Edge port flag 3
vector068:  .long asm_exception_handler   ; User int 4 Edge port flag 4
vector069:  .long DPRAM_EP5_ISR           ; User int 5 Edge port flag 5
vector070:  .long FPGA_EP6_ISR            ; User int 6 Edge port flag 6
vector071:  .long PowerFailIsr            ; User int 7 Edge port flag 7
vector072:  .long asm_exception_handler   ; User int 8
vector073:  .long asm_exception_handler   ; User int 9
vector074:  .long asm_exception_handler   ; User int 10 
vector075:  .long asm_exception_handler   ; User int 11
vector076:  .long asm_exception_handler   ; User int 12
vector077:  .long asm_exception_handler   ; User int 13
vector078:  .long asm_exception_handler   ; User int 14
vector079:  .long asm_exception_handler   ; User int 15 USB Endpoint 0
vector080:  .long asm_exception_handler   ; User int 16 USB Endpoint 1
vector081:  .long asm_exception_handler   ; User int 17 USB Endpoint 2
vector082:  .long asm_exception_handler   ; User int 18 USB Endpoint 3
vector083:  .long asm_exception_handler   ; User int 19 USB Endpoint 4
vector084:  .long asm_exception_handler   ; User int 20 USB Endpoint 5
vector085:  .long asm_exception_handler   ; User int 21 USB Endpoint 6
vector086:  .long asm_exception_handler   ; User int 22 USB General Int
vector087:  .long asm_exception_handler   ; User int 23 USB Core Int
vector088:  .long asm_exception_handler   ; User int 24 OR of all USB Int
vector089:  .long asm_exception_handler   ; User int 25 DSPI over/under flow
vector090:  .long asm_exception_handler   ; User int 26 DSPI RX FIFO overflow
vector091:  .long asm_exception_handler   ; User int 27 DSPI RX FIFO drain
vector092:  .long asm_exception_handler   ; User int 28 DSPI TX FIFO underflow
vector093:  .long asm_exception_handler   ; User int 29 DSPI TX Complete
vector094:  .long asm_exception_handler   ; User int 30 DSPI Transfer FIFO fill
vector095:  .long asm_exception_handler   ; User int 31 DSPI End of queue 
vector096:  .long UART_PSC3_ISR           ; User int 32 PSC3
vector097:  .long UART_PSC2_ISR           ; User int 33 PSC2
vector098:  .long UART_PSC1_ISR           ; User int 34 PSC1
vector099:  .long UART_PSC0_ISR           ; User int 35 PSC0
vector100:  .long asm_exception_handler   ; User int 36 TC Combined comm timer
vector101:  .long asm_exception_handler   ; User int 37 SEC
vector102:  .long asm_exception_handler   ; User int 38 FEC1
vector103:  .long asm_exception_handler   ; User int 39 FEC0
vector104:  .long asm_exception_handler   ; User int 40 I2C
vector105:  .long asm_exception_handler   ; User int 41 PCIARB PCI Arbiter
vector106:  .long asm_exception_handler   ; User int 42 CBPCI  Comm bus PCI
vector107:  .long asm_exception_handler   ; User int 43 XLBPCI XLB PCI
vector108:  .long asm_exception_handler   ; User int 44
vector109:  .long asm_exception_handler   ; User int 45
vector110:  .long asm_exception_handler   ; User int 46
vector111:  .long BusTimeoutException     ; User int 47 XLBARB
vector112:  .long asm_exception_handler   ; User int 48 DMA Multichannel
vector113:  .long asm_exception_handler   ; User int 49 CAN0 FlexCAN error
vector114:  .long asm_exception_handler   ; User int 50 CAN0 FlexCAN bus off
vector115:  .long asm_exception_handler   ; User int 51 CAN0 Message buffer ORed  
vector116:  .long asm_exception_handler   ; User int 52
vector117:  .long asm_exception_handler   ; User int 53 Slice Timer 1
vector118:  .long asm_exception_handler   ; User int 54 Slice Timer 2
vector119:  .long asm_exception_handler   ; User int 55 FlexCAN error
vector120:  .long asm_exception_handler   ; User int 56 FlexCan bus off
vector121:  .long asm_exception_handler   ; User int 57 FlexCAN Message buffer ORed
vector122:  .long asm_exception_handler   ; User int 58 
vector123:  .long asm_exception_handler   ; User int 59 GPT3 Interrupt
vector124:  .long asm_exception_handler   ; User int 60 GPT2 Interrupt
vector125:  .long asm_exception_handler   ; User int 61 GPT1 Interrupt 
vector126:  .long TTMR_GPT0ISR            ; User int 62 GPT0 interrupt
vector127:  .long asm_exception_handler   ; User int 63
vector128:  .long asm_exception_handler   ; User int 64
vector129:  .long asm_exception_handler   ; User int 65
vector130:  .long asm_exception_handler   ; User int 66
vector131:  .long asm_exception_handler   ; User int 67
vector132:  .long asm_exception_handler   ; User int 68
vector133:  .long asm_exception_handler   ; User int 69
vector134:  .long asm_exception_handler   ; User int 70
vector135:  .long asm_exception_handler   ; User int 71
vector136:  .long asm_exception_handler   ; User int 72
vector137:  .long asm_exception_handler   ; User int 73
vector138:  .long asm_exception_handler   ; User int 74
vector139:  .long asm_exception_handler   ; User int 75
vector140:  .long asm_exception_handler   ; User int 76
vector141:  .long asm_exception_handler   ; User int 77
vector142:  .long asm_exception_handler   ; User int 78
vector143:  .long asm_exception_handler   ; User int 79
vector144:  .long asm_exception_handler   ; User int 80
vector145:  .long asm_exception_handler   ; User int 81
vector146:  .long asm_exception_handler   ; User int 82
vector147:  .long asm_exception_handler   ; User int 83
vector148:  .long asm_exception_handler   ; User int 84
vector149:  .long asm_exception_handler   ; User int 86
vector150:  .long asm_exception_handler   ; User int 86
vector151:  .long asm_exception_handler   ; User int 87
vector152:  .long asm_exception_handler   ; User int 88
vector153:  .long asm_exception_handler   ; User int 89
vector154:  .long asm_exception_handler   ; User int 90
vector155:  .long asm_exception_handler   ; User int 91
vector156:  .long asm_exception_handler   ; User int 92
vector157:  .long asm_exception_handler   ; User int 93
vector158:  .long asm_exception_handler   ; User int 94
vector159:  .long asm_exception_handler   ; User int 95
vector160:  .long asm_exception_handler   ; User int 96
vector161:  .long asm_exception_handler   ; User int 97
vector162:  .long asm_exception_handler   ; User int 98
vector163:  .long asm_exception_handler   ; User int 99
vector164:  .long asm_exception_handler   ; User int 100
vector165:  .long asm_exception_handler   ; User int 101
vector166:  .long asm_exception_handler   ; User int 102
vector167:  .long asm_exception_handler   ; User int 103
vector168:  .long asm_exception_handler   ; User int 104
vector169:  .long asm_exception_handler   ; User int 105
vector170:  .long asm_exception_handler   ; User int 106
vector171:  .long asm_exception_handler   ; User int 107
vector172:  .long asm_exception_handler   ; User int 108
vector173:  .long asm_exception_handler   ; User int 109
vector174:  .long asm_exception_handler   ; User int 110
vector175:  .long asm_exception_handler   ; User int 111
vector176:  .long asm_exception_handler   ; User int 112
vector177:  .long asm_exception_handler   ; User int 113
vector178:  .long asm_exception_handler   ; User int 114
vector179:  .long asm_exception_handler   ; User int 115
vector180:  .long asm_exception_handler   ; User int 116
vector181:  .long asm_exception_handler   ; User int 117
vector182:  .long asm_exception_handler   ; User int 118
vector183:  .long asm_exception_handler   ; User int 119
vector184:  .long asm_exception_handler   ; User int 120
vector185:  .long asm_exception_handler   ; User int 121
vector186:  .long asm_exception_handler   ; User int 122
vector187:  .long asm_exception_handler   ; User int 123
vector188:  .long asm_exception_handler   ; User int 124
vector189:  .long asm_exception_handler   ; User int 125
vector190:  .long asm_exception_handler   ; User int 126
vector191:  .long asm_exception_handler   ; User int 127
vector192:  .long asm_exception_handler   ; User int 128
vector193:  .long asm_exception_handler   ; User int 129
vector194:  .long asm_exception_handler   ; User int 130
vector195:  .long asm_exception_handler   ; User int 131
vector196:  .long asm_exception_handler   ; User int 132
vector197:  .long asm_exception_handler   ; User int 133
vector198:  .long asm_exception_handler   ; User int 134
vector199:  .long asm_exception_handler   ; User int 135
vector200:  .long asm_exception_handler   ; User int 136
vector201:  .long asm_exception_handler   ; User int 137
vector202:  .long asm_exception_handler   ; User int 138
vector203:  .long asm_exception_handler   ; User int 139
vector204:  .long asm_exception_handler   ; User int 140
vector205:  .long asm_exception_handler   ; User int 141
vector206:  .long asm_exception_handler   ; User int 142
vector207:  .long asm_exception_handler   ; User int 143
vector208:  .long asm_exception_handler   ; User int 144
vector209:  .long asm_exception_handler   ; User int 145
vector210:  .long asm_exception_handler   ; User int 146
vector211:  .long asm_exception_handler   ; User int 147
vector212:  .long asm_exception_handler   ; User int 148
vector213:  .long asm_exception_handler   ; User int 149
vector214:  .long asm_exception_handler   ; User int 150
vector215:  .long asm_exception_handler   ; User int 151
vector216:  .long asm_exception_handler   ; User int 152
vector217:  .long asm_exception_handler   ; User int 153
vector218:  .long asm_exception_handler   ; User int 154
vector219:  .long asm_exception_handler   ; User int 155
vector220:  .long asm_exception_handler   ; User int 156
vector221:  .long asm_exception_handler   ; User int 157
vector222:  .long asm_exception_handler   ; User int 158
vector223:  .long asm_exception_handler   ; User int 159
vector224:  .long asm_exception_handler   ; User int 160
vector225:  .long asm_exception_handler   ; User int 161
vector226:  .long asm_exception_handler   ; User int 162
vector227:  .long asm_exception_handler   ; User int 163
vector228:  .long asm_exception_handler   ; User int 164
vector229:  .long asm_exception_handler   ; User int 165
vector230:  .long asm_exception_handler   ; User int 166
vector231:  .long asm_exception_handler   ; User int 167
vector232:  .long asm_exception_handler   ; User int 168
vector233:  .long asm_exception_handler   ; User int 169
vector234:  .long asm_exception_handler   ; User int 170
vector235:  .long asm_exception_handler   ; User int 171
vector236:  .long asm_exception_handler   ; User int 172
vector237:  .long asm_exception_handler   ; User int 173
vector238:  .long asm_exception_handler   ; User int 174
vector239:  .long asm_exception_handler   ; User int 175
vector240:  .long asm_exception_handler   ; User int 176
vector241:  .long asm_exception_handler   ; User int 177
vector242:  .long asm_exception_handler   ; User int 178
vector243:  .long asm_exception_handler   ; User int 179
vector244:  .long asm_exception_handler   ; User int 180
vector245:  .long asm_exception_handler   ; User int 181
vector246:  .long asm_exception_handler   ; User int 182
vector247:  .long asm_exception_handler   ; User int 183
vector248:  .long asm_exception_handler   ; User int 184
vector249:  .long asm_exception_handler   ; User int 185
vector250:  .long asm_exception_handler   ; User int 186
vector251:  .long asm_exception_handler   ; User int 187
vector252:  .long asm_exception_handler   ; User int 188
vector253:  .long asm_exception_handler   ; User int 189
vector254:  .long asm_exception_handler   ; User int 190
vector255:  .long asm_exception_handler   ; User int 191


startinit:
    move.l  #__SP_INIT,SP
    move.w  #0x2700,SR
    jmp     init_processor

asm_exception_handler:

   jmp      UnexpectedException
  .end
  
  
;/*************************************************************************
; *  MODIFICATIONS
; *    $History: vectors.s $
;*
;******************  Version 6  *****************
;*User: Contractor2  Date: 6/27/11    Time: 1:52p
;*Updated in $/software/control processor/code/drivers
;*SCR #515 Enhancement - sys.get.mem.l halts processor when invalid
;*memory is referenced
;*Corrected vector comments - incorrect order
;*
;*
;******************  Version 5  *****************
;*User: Jim Mood     Date: 11/02/10   Time: 4:48p
;*Updated in $/software/control processor/code/drivers
;*SCR 978 Code Review changes
; *
; *************************************************************************/
  
