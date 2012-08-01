/*
 * File:	mcf548x.h
 * Purpose:	Register and bit definitions for the MCF548X
 *
 * Notes:	
 *	
 */

#ifndef __MCF548X_H__
#define __MCF548X_H__

/*********************************************************************/
//extern unsigned char __MBAR[];
#ifndef WIN32
  #define __MBAR ((const char*)(0x80000000))
#else
  #include "HwSupport.h"
#endif

#include "mcf5xxx.h"
#include "mcf548x_fec.h"
#include "mcf548x_siu.h"
#include "mcf548x_ctm.h"
#include "mcf548x_dspi.h"
#include "mcf548x_eport.h"
#include "mcf548x_fbcs.h"
#include "mcf548x_gpio.h"
#include "mcf548x_gpt.h"
#include "mcf548x_i2c.h"
#include "mcf548x_intc.h"
#include "mcf548x_sdramc.h"
#include "mcf548x_sec.h"
#include "mcf548x_slt.h"
#include "mcf548x_usb.h"
#include "mcf548x_psc.h"
#include "mcf548x_sram.h"
#include "mcf548x_pci.h"
#include "mcf548x_pciarb.h"
#include "mcf548x_dma.h"
#include "mcf548x_can.h"
#include "mcf548x_dma_ereq.h"
#include "mcf548x_xarb.h"

/*********************************************************************/

#endif /* __MCF548X_H__ */
