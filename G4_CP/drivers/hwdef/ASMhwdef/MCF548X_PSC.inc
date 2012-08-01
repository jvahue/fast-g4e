;File:	mcf548x_psc.INC
;Purpose:	Assembler syntax register and bit definitions for the MCF548X
;Notes:	Generated from the mcf548x_psc.h C header file

;
;
;Programmable Serial Controller (PSC)
;
;
;Register read/write macros
MCF_PSC_MR0            EQU     0x80008600
MCF_PSC_SR0            EQU     0x80008604
MCF_PSC_CSR0           EQU     0x80008604
MCF_PSC_CR0            EQU     0x80008608
MCF_PSC_RB0            EQU     0x8000860C
MCF_PSC_TB0            EQU     0x8000860C
MCF_PSC_TB_8BIT0       EQU     0x8000860C
MCF_PSC_TB_16BIT0      EQU     0x8000860C
MCF_PSC_TB_AC970       EQU     0x8000860C
MCF_PSC_IPCR0          EQU     0x80008610
MCF_PSC_ACR0           EQU     0x80008610
MCF_PSC_ISR0           EQU     0x80008614
MCF_PSC_IMR0           EQU     0x80008614
MCF_PSC_CTUR0          EQU     0x80008618
MCF_PSC_CTLR0          EQU     0x8000861C
MCF_PSC_IP0            EQU     0x80008634
MCF_PSC_OPSET0         EQU     0x80008638
MCF_PSC_OPRESET0       EQU     0x8000863C
MCF_PSC_SICR0          EQU     0x80008640
MCF_PSC_IRCR10         EQU     0x80008644
MCF_PSC_IRCR20         EQU     0x80008648
MCF_PSC_IRSDR0         EQU     0x8000864C
MCF_PSC_IRMDR0         EQU     0x80008650
MCF_PSC_IRFDR0         EQU     0x80008654
MCF_PSC_RFCNT0         EQU     0x80008658
MCF_PSC_TFCNT0         EQU     0x8000865C
MCF_PSC_RFSR0          EQU     0x80008664
MCF_PSC_TFSR0          EQU     0x80008684
MCF_PSC_RFCR0          EQU     0x80008668
MCF_PSC_TFCR0          EQU     0x80008688
MCF_PSC_RFAR0          EQU     0x8000866E
MCF_PSC_TFAR0          EQU     0x8000868E
MCF_PSC_RFRP0          EQU     0x80008672
MCF_PSC_TFRP0          EQU     0x80008692
MCF_PSC_RFWP0          EQU     0x80008676
MCF_PSC_TFWP0          EQU     0x80008696
MCF_PSC_RLRFP0         EQU     0x8000867A
MCF_PSC_TLRFP0         EQU     0x8000869A
MCF_PSC_RLWFP0         EQU     0x8000867E
MCF_PSC_TLWFP0         EQU     0x8000869E
MCF_PSC_MR1            EQU     0x80008700
MCF_PSC_SR1            EQU     0x80008704
MCF_PSC_CSR1           EQU     0x80008704
MCF_PSC_CR1            EQU     0x80008708
MCF_PSC_RB1            EQU     0x8000870C
MCF_PSC_TB1            EQU     0x8000870C
MCF_PSC_TB_8BIT1       EQU     0x8000870C
MCF_PSC_TB_16BIT1      EQU     0x8000870C
MCF_PSC_TB_AC971       EQU     0x8000870C
MCF_PSC_IPCR1          EQU     0x80008710
MCF_PSC_ACR1           EQU     0x80008710
MCF_PSC_ISR1           EQU     0x80008714
MCF_PSC_IMR1           EQU     0x80008714
MCF_PSC_CTUR1          EQU     0x80008718
MCF_PSC_CTLR1          EQU     0x8000871C
MCF_PSC_IP1            EQU     0x80008734
MCF_PSC_OPSET1         EQU     0x80008738
MCF_PSC_OPRESET1       EQU     0x8000873C
MCF_PSC_SICR1          EQU     0x80008740
MCF_PSC_IRCR11         EQU     0x80008744
MCF_PSC_IRCR21         EQU     0x80008748
MCF_PSC_IRSDR1         EQU     0x8000874C
MCF_PSC_IRMDR1         EQU     0x80008750
MCF_PSC_IRFDR1         EQU     0x80008754
MCF_PSC_RFCNT1         EQU     0x80008758
MCF_PSC_TFCNT1         EQU     0x8000875C
MCF_PSC_RFSR1          EQU     0x80008764
MCF_PSC_TFSR1          EQU     0x80008784
MCF_PSC_RFCR1          EQU     0x80008768
MCF_PSC_TFCR1          EQU     0x80008788
MCF_PSC_RFAR1          EQU     0x8000876E
MCF_PSC_TFAR1          EQU     0x8000878E
MCF_PSC_RFRP1          EQU     0x80008772
MCF_PSC_TFRP1          EQU     0x80008792
MCF_PSC_RFWP1          EQU     0x80008776
MCF_PSC_TFWP1          EQU     0x80008796
MCF_PSC_RLRFP1         EQU     0x8000877A
MCF_PSC_TLRFP1         EQU     0x8000879A
MCF_PSC_RLWFP1         EQU     0x8000877E
MCF_PSC_TLWFP1         EQU     0x8000879E
MCF_PSC_MR2            EQU     0x80008800
MCF_PSC_SR2            EQU     0x80008804
MCF_PSC_CSR2           EQU     0x80008804
MCF_PSC_CR2            EQU     0x80008808
MCF_PSC_RB2            EQU     0x8000880C
MCF_PSC_TB2            EQU     0x8000880C
MCF_PSC_TB_8BIT2       EQU     0x8000880C
MCF_PSC_TB_16BIT2      EQU     0x8000880C
MCF_PSC_TB_AC972       EQU     0x8000880C
MCF_PSC_IPCR2          EQU     0x80008810
MCF_PSC_ACR2           EQU     0x80008810
MCF_PSC_ISR2           EQU     0x80008814
MCF_PSC_IMR2           EQU     0x80008814
MCF_PSC_CTUR2          EQU     0x80008818
MCF_PSC_CTLR2          EQU     0x8000881C
MCF_PSC_IP2            EQU     0x80008834
MCF_PSC_OPSET2         EQU     0x80008838
MCF_PSC_OPRESET2       EQU     0x8000883C
MCF_PSC_SICR2          EQU     0x80008840
MCF_PSC_IRCR12         EQU     0x80008844
MCF_PSC_IRCR22         EQU     0x80008848
MCF_PSC_IRSDR2         EQU     0x8000884C
MCF_PSC_IRMDR2         EQU     0x80008850
MCF_PSC_IRFDR2         EQU     0x80008854
MCF_PSC_RFCNT2         EQU     0x80008858
MCF_PSC_TFCNT2         EQU     0x8000885C
MCF_PSC_RFSR2          EQU     0x80008864
MCF_PSC_TFSR2          EQU     0x80008884
MCF_PSC_RFCR2          EQU     0x80008868
MCF_PSC_TFCR2          EQU     0x80008888
MCF_PSC_RFAR2          EQU     0x8000886E
MCF_PSC_TFAR2          EQU     0x8000888E
MCF_PSC_RFRP2          EQU     0x80008872
MCF_PSC_TFRP2          EQU     0x80008892
MCF_PSC_RFWP2          EQU     0x80008876
MCF_PSC_TFWP2          EQU     0x80008896
MCF_PSC_RLRFP2         EQU     0x8000887A
MCF_PSC_TLRFP2         EQU     0x8000889A
MCF_PSC_RLWFP2         EQU     0x8000887E
MCF_PSC_TLWFP2         EQU     0x8000889E
MCF_PSC_MR3            EQU     0x80008900
MCF_PSC_SR3            EQU     0x80008904
MCF_PSC_CSR3           EQU     0x80008904
MCF_PSC_CR3            EQU     0x80008908
MCF_PSC_RB3            EQU     0x8000890C
MCF_PSC_TB3            EQU     0x8000890C
MCF_PSC_TB_8BIT3       EQU     0x8000890C
MCF_PSC_TB_16BIT3      EQU     0x8000890C
MCF_PSC_TB_AC973       EQU     0x8000890C
MCF_PSC_IPCR3          EQU     0x80008910
MCF_PSC_ACR3           EQU     0x80008910
MCF_PSC_ISR3           EQU     0x80008914
MCF_PSC_IMR3           EQU     0x80008914
MCF_PSC_CTUR3          EQU     0x80008918
MCF_PSC_CTLR3          EQU     0x8000891C
MCF_PSC_IP3            EQU     0x80008934
MCF_PSC_OPSET3         EQU     0x80008938
MCF_PSC_OPRESET3       EQU     0x8000893C
MCF_PSC_SICR3          EQU     0x80008940
MCF_PSC_IRCR13         EQU     0x80008944
MCF_PSC_IRCR23         EQU     0x80008948
MCF_PSC_IRSDR3         EQU     0x8000894C
MCF_PSC_IRMDR3         EQU     0x80008950
MCF_PSC_IRFDR3         EQU     0x80008954
MCF_PSC_RFCNT3         EQU     0x80008958
MCF_PSC_TFCNT3         EQU     0x8000895C
MCF_PSC_RFSR3          EQU     0x80008964
MCF_PSC_TFSR3          EQU     0x80008984
MCF_PSC_RFCR3          EQU     0x80008968
MCF_PSC_TFCR3          EQU     0x80008988
MCF_PSC_RFAR3          EQU     0x8000896E
MCF_PSC_TFAR3          EQU     0x8000898E
MCF_PSC_RFRP3          EQU     0x80008972
MCF_PSC_TFRP3          EQU     0x80008992
MCF_PSC_RFWP3          EQU     0x80008976
MCF_PSC_TFWP3          EQU     0x80008996
MCF_PSC_RLRFP3         EQU     0x8000897A
MCF_PSC_TLRFP3         EQU     0x8000899A
MCF_PSC_RLWFP3         EQU     0x8000897E
MCF_PSC_TLWFP3         EQU     0x8000899E
                       
MCF_PSC_MR_BC                 EQU     0x03
MCF_PSC_MR_PT                 EQU    (0x04)
MCF_PSC_MR_PM                 EQU    (0x03<<3)
MCF_PSC_MR_ERR                EQU    (0x20)
MCF_PSC_MR_RXIRQ              EQU    (0x40)
MCF_PSC_MR_RXRTS              EQU    (0x80)
MCF_PSC_MR_SB                 EQU     0x0F
MCF_PSC_MR_TXCTS              EQU    (0x10)
MCF_PSC_MR_TXRTS              EQU    (0x20)
MCF_PSC_MR_CM                 EQU    (0x03<<6)
MCF_PSC_MR_PM_MULTI_ADDR      EQU    (0x1C)
MCF_PSC_MR_PM_MULTI_DATA      EQU    (0x18)
MCF_PSC_MR_PM_NONE            EQU    (0x10)
MCF_PSC_MR_PM_FORCE_HI        EQU    (0x0C)
MCF_PSC_MR_PM_FORCE_LO        EQU    (0x08)
MCF_PSC_MR_PM_ODD             EQU    (0x04)
MCF_PSC_MR_PM_EVEN            EQU    (0x00)
MCF_PSC_MR_BC_5               EQU    (0x00)
MCF_PSC_MR_BC_6               EQU    (0x01)
MCF_PSC_MR_BC_7               EQU    (0x02)
MCF_PSC_MR_BC_8               EQU    (0x03)
MCF_PSC_MR_CM_NORMAL          EQU    (0x00)
MCF_PSC_MR_CM_ECHO            EQU    (0x40)
MCF_PSC_MR_CM_LOCAL_LOOP      EQU    (0x80)
MCF_PSC_MR_CM_REMOTE_LOOP     EQU    (0xC0)
MCF_PSC_MR_SB_STOP_BITS_1     EQU    (0x07)
MCF_PSC_MR_SB_STOP_BITS_15    EQU    (0x08)
MCF_PSC_MR_SB_STOP_BITS_2     EQU    (0x0F)

MCF_PSC_SR_ERR                EQU    (0x0040)
MCF_PSC_SR_CDE_DEOF           EQU    (0x0080)
MCF_PSC_SR_RXRDY              EQU    (0x0100)
MCF_PSC_SR_FU                 EQU    (0x0200)
MCF_PSC_SR_TXRDY              EQU    (0x0400)
MCF_PSC_SR_TXEMP_URERR        EQU    (0x0800)
MCF_PSC_SR_OE                 EQU    (0x1000)
MCF_PSC_SR_PE_CRCERR          EQU    (0x2000)
MCF_PSC_SR_FE_PHYERR          EQU    (0x4000)
MCF_PSC_SR_RB_NEOF            EQU    (0x8000)

MCF_PSC_CSR_TCSEL             EQU    (0x0F<<0)
MCF_PSC_CSR_RCSEL             EQU    (0x0F<<4)
MCF_PSC_CSR_RCSEL_SYS_CLK     EQU    (0xD0)
MCF_PSC_CSR_RCSEL_CTM16       EQU    (0xE0)
MCF_PSC_CSR_RCSEL_CTM         EQU    (0xF0)
MCF_PSC_CSR_TCSEL_SYS_CLK     EQU    (0x0D)
MCF_PSC_CSR_TCSEL_CTM16       EQU    (0x0E)
MCF_PSC_CSR_TCSEL_CTM         EQU    (0x0F)

MCF_PSC_CR_RXC                EQU    (0x03<<0)
MCF_PSC_CR_TXC                EQU    (0x03<<2)
MCF_PSC_CR_MISC               EQU    (0x07<<4)
MCF_PSC_CR_NONE               EQU    (0x00)
MCF_PSC_CR_STOP_BREAK         EQU    (0x70)
MCF_PSC_CR_START_BREAK        EQU    (0x60)
MCF_PSC_CR_BKCHGINT           EQU    (0x50)
MCF_PSC_CR_RESET_ERROR        EQU    (0x40)
MCF_PSC_CR_RESET_TX           EQU    (0x30)
MCF_PSC_CR_RESET_RX           EQU    (0x20)
MCF_PSC_CR_RESET_MR           EQU    (0x10)
MCF_PSC_CR_TX_DISABLED        EQU    (0x08)
MCF_PSC_CR_TX_ENABLED         EQU    (0x04)
MCF_PSC_CR_RX_DISABLED        EQU    (0x02)
MCF_PSC_CR_RX_ENABLED         EQU    (0x01)

MCF_PSC_TB_8BIT_TB3           EQU    0x000000FF
MCF_PSC_TB_8BIT_TB2           EQU    (0x000000FF<<8)
MCF_PSC_TB_8BIT_TB1           EQU    (0x000000FF<<16)
MCF_PSC_TB_8BIT_TB0           EQU    (0x000000FF<<24)

MCF_PSC_TB_16BIT_TB1          EQU    (0x0000FFFF<<0)
MCF_PSC_TB_16BIT_TB0          EQU    (0x0000FFFF<<16)

MCF_PSC_TB_AC97_SOF           EQU    (0x00000800)
MCF_PSC_TB_AC97_TB            EQU    (0x000FFFFF<<12)

MCF_PSC_IPCR_RESERVED         EQU    (0x0C)
MCF_PSC_IPCR_CTS              EQU    (0x0D)
MCF_PSC_IPCR_D_CTS            EQU    (0x1C)
MCF_PSC_IPCR_SYNC             EQU    (0x8C)

MCF_PSC_ACR_IEC0              EQU    (0x01)
MCF_PSC_ACR_CTMS              EQU    (0x07<<4)
MCF_PSC_ACR_BRG               EQU    (0x80)

MCF_PSC_ISR_ERR               EQU    (0x0040)
MCF_PSC_ISR_DEOF              EQU    (0x0080)
MCF_PSC_ISR_TXRDY             EQU    (0x0100)
MCF_PSC_ISR_RXRDY_FU          EQU    (0x0200)
MCF_PSC_ISR_DB                EQU    (0x0400)
MCF_PSC_ISR_IPC               EQU    (0x8000)

MCF_PSC_IMR_ERR               EQU    (0x0040)
MCF_PSC_IMR_DEOF              EQU    (0x0080)
MCF_PSC_IMR_TXRDY             EQU    (0x0100)
MCF_PSC_IMR_RXRDY_FU          EQU    (0x0200)
MCF_PSC_IMR_DB                EQU    (0x0400)
MCF_PSC_IMR_IPC               EQU    (0x8000)

MCF_PSC_IP_CTS                EQU    (0x01)
MCF_PSC_IP_TGL                EQU    (0x40)
MCF_PSC_IP_LWPR_B             EQU    (0x80)

MCF_PSC_OPSET_RTS             EQU    (0x01)

MCF_PSC_OPRESET_RTS           EQU    (0x01)

MCF_PSC_SICR_SIM              EQU    (0x07<<0)
MCF_PSC_SICR_SHDIR            EQU    (0x10)
MCF_PSC_SICR_DTS              EQU    (0x20)
MCF_PSC_SICR_AWR              EQU    (0x40)
MCF_PSC_SICR_ACRB             EQU    (0x80)
MCF_PSC_SICR_SIM_UART         EQU    (0x00)
MCF_PSC_SICR_SIM_MODEM8       EQU    (0x01)
MCF_PSC_SICR_SIM_MODEM16      EQU    (0x02)
MCF_PSC_SICR_SIM_AC97         EQU    (0x03)
MCF_PSC_SICR_SIM_SIR          EQU    (0x04)
MCF_PSC_SICR_SIM_MIR          EQU    (0x05)
MCF_PSC_SICR_SIM_FIR          EQU    (0x06)

MCF_PSC_IRCR1_SPUL            EQU    (0x01)
MCF_PSC_IRCR1_SIPEN           EQU    (0x02)
MCF_PSC_IRCR1_FD              EQU    (0x04)
                              
MCF_PSC_IRCR2_NXTEOF          EQU    (0x01)
MCF_PSC_IRCR2_ABORT           EQU    (0x02)
MCF_PSC_IRCR2_SIPREQ          EQU    (0x04)
                              
MCF_PSC_IRMDR_M_FDIV          EQU    (0x7F<<0)
MCF_PSC_IRMDR_FREQ            EQU    (0x80)
                              
MCF_PSC_IRFDR_F_FDIV          EQU    (0x0F<<0)

MCF_PSC_RFCNT_CNT             EQU    (0x01FF<<0)

MCF_PSC_TFCNT_CNT             EQU    (0x01FF<<0)

MCF_PSC_RFSR_EMT              EQU    (0x0001)
MCF_PSC_RFSR_ALARM            EQU    (0x0002)
MCF_PSC_RFSR_FU               EQU    (0x0004)
MCF_PSC_RFSR_FRMRY            EQU    (0x0008)
MCF_PSC_RFSR_OF               EQU    (0x0010)
MCF_PSC_RFSR_UF               EQU    (0x0020)
MCF_PSC_RFSR_RXW              EQU    (0x0040)
MCF_PSC_RFSR_FAE              EQU    (0x0080)
MCF_PSC_RFSR_FRM              EQU    (0x000F<<8)
MCF_PSC_RFSR_TAG              EQU    (0x1000)
MCF_PSC_RFSR_TXW              EQU    (0x4000)
MCF_PSC_RFSR_IP               EQU    (0x8000)
MCF_PSC_RFSR_FRM_BYTE0        EQU    (0x0800)
MCF_PSC_RFSR_FRM_BYTE1        EQU    (0x0400)
MCF_PSC_RFSR_FRM_BYTE2        EQU    (0x0200)
MCF_PSC_RFSR_FRM_BYTE3        EQU    (0x0100)

MCF_PSC_TFSR_EMT              EQU    (0x0001)
MCF_PSC_TFSR_ALARM            EQU    (0x0002)
MCF_PSC_TFSR_FU               EQU    (0x0004)
MCF_PSC_TFSR_FRMRY            EQU    (0x0008)
MCF_PSC_TFSR_OF               EQU    (0x0010)
MCF_PSC_TFSR_UF               EQU    (0x0020)
MCF_PSC_TFSR_RXW              EQU    (0x0040)
MCF_PSC_TFSR_FAE              EQU    (0x0080)
MCF_PSC_TFSR_FRM              EQU    (0x000F<<8)
MCF_PSC_TFSR_TAG              EQU    (0x1000)
MCF_PSC_TFSR_TXW              EQU    (0x4000)
MCF_PSC_TFSR_IP               EQU    (0x8000)
MCF_PSC_TFSR_FRM_BYTE0        EQU    (0x0800)
MCF_PSC_TFSR_FRM_BYTE1        EQU    (0x0400)
MCF_PSC_TFSR_FRM_BYTE2        EQU    (0x0200)
MCF_PSC_TFSR_FRM_BYTE3        EQU    (0x0100)

MCF_PSC_RFCR_CNTR             EQU    (0x0000FFFF<<0)
MCF_PSC_RFCR_TXW_MSK          EQU    (0x00040000)
MCF_PSC_RFCR_OF_MSK           EQU    (0x00080000)
MCF_PSC_RFCR_UF_MSK           EQU    (0x00100000)
MCF_PSC_RFCR_RXW_MSK          EQU    (0x00200000)
MCF_PSC_RFCR_FAE_MSK          EQU    (0x00400000)
MCF_PSC_RFCR_IP_MSK           EQU    (0x00800000)
MCF_PSC_RFCR_GR               EQU    (0x00000007<<24)
MCF_PSC_RFCR_FRMEN            EQU    (0x08000000)
MCF_PSC_RFCR_TIMER            EQU    (0x10000000)
MCF_PSC_RFCR_WRITETAG         EQU    (0x20000000)
MCF_PSC_RFCR_SHADOW           EQU    (0x80000000)

MCF_PSC_TFCR_CNTR             EQU    (0x0000FFFF<<0)
MCF_PSC_TFCR_TXW_MSK          EQU    (0x00040000)
MCF_PSC_TFCR_OF_MSK           EQU    (0x00080000)
MCF_PSC_TFCR_UF_MSK           EQU    (0x00100000)
MCF_PSC_TFCR_RXW_MSK          EQU    (0x00200000)
MCF_PSC_TFCR_FAE_MSK          EQU    (0x00400000)
MCF_PSC_TFCR_IP_MSK           EQU    (0x00800000)
MCF_PSC_TFCR_GR               EQU    (0x00000007<<24)
MCF_PSC_TFCR_FRMEN            EQU    (0x08000000)
MCF_PSC_TFCR_TIMER            EQU    (0x10000000)
MCF_PSC_TFCR_WRITETAG         EQU    (0x20000000)
MCF_PSC_TFCR_SHADOW           EQU    (0x80000000)

MCF_PSC_RFAR_ALARM            EQU    (0x01FF<<0)

MCF_PSC_TFAR_ALARM            EQU    (0x01FF<<0)

MCF_PSC_RFRP_READ             EQU    (0x01FF<<0)

MCF_PSC_TFRP_READ             EQU    (0x01FF<<0)

MCF_PSC_RFWP_WRITE            EQU    (0x01FF<<0)

MCF_PSC_TFWP_WRITE            EQU    (0x01FF<<0)

MCF_PSC_RLRFP_LFP             EQU    (0x01FF<<0)

MCF_PSC_TLRFP_LFP             EQU    (0x01FF<<0)

MCF_PSC_RLWFP_LFP             EQU    (0x01FF<<0)

MCF_PSC_TLWFP_LFP             EQU    (0x01FF<<0)