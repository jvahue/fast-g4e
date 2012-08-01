/*---------------------------------------------------------------------------------------------
            Copyright (C) 2007 - 2010 Pratt & Whitney Engine Services, Inc. 
                      Altair Engine Diagnostic Solutions
               All Rights Reserved. Proprietary and Confidential.


 File:        Debugger RAM RUN at 0x30000000.ld

            dram_memory
              Location for 32MB of SDRAM for the FAST II processor board.  It is IMPORTANT
              to note that this memory is not accessable until the SDRAM periperhal in initialized


	      SECTIONS:
		In SDRAM:
	      .data = initialized data memory
	      .bss = uninitialized data memory
	      .pbss = data memory excluded from CBIT Ram Test

	      .zdata/.zbss NOT IMPLIMENTED - This special section is used by GHS to define
	      		   a special 64k or less, chunk of data based at 0x0 for faster access
			   Because this map puts ROM at 0x0, zdata is meaningless
	      .sdabase, .sdadata, .sdabss  = Small data areas, this defines a 64k or
	      		 less chunk of data memory that can be accessed more efficiently
			 using 16-bit offset addressing.
          .stack = reserved section at the end of other dram data for STACK
          
---------------------------------------------------------------------------------------------*/

CONSTANTS
{
	stack_reserve = 1M
}

MEMORY
{
	dram_memory    : ORIGIN = 0x30000000,	LENGTH = 32M
	sram_memory    : ORIGIN = 0x80010000,	LENGTH = 32K
}

SECTIONS
{
  .vector ALIGN(4)                       : > dram_memory
  .text ALIGN(4)                         : > .

  .rodata                                : > .
  .secinfo                               : > .
  .fixaddr                               : > .
  .fixtype                               : > .
  .sdata                                 : > .
  .syscall                               : > .
  .crcDefault                            : > .
  .buildDate                             : > .

  .bss ALIGN(4)                          : > .
  .data                                  : > .
  .pbss                                  : > .
  .sdabase ALIGN(4)                      : > .
  .sbss                                  : > .

  __SP_END = .;
  .stack ALIGN(4) PAD (stack_reserve)    : > .
  __SP_INIT = .;


   __CRC_START = ADDR(.vector);
   __CRC_END =   ADDR(.crcDefault);

	__ghs_cframstart = 0;
	__ghs_ramstart = ADDR(.bss);
	__ghs_ramend = MEMENDADDR(dram_memory);
	__ghs_allocatedramend = ENDADDR(.data);
	__ghs_pramstart = ADDR(.pbss);
	__ghs_pramend = ENDADDR(.pbss);

//	__ghs_romstart = 0;
//	__ghs_romend = MEMENDADDR(pflash_memory);
//	__ghs_rambootcodestart = 0;
//	__ghs_rambootcodeend = 0;
//	__ghs_rombootcodestart = ADDR(.text);
//	__ghs_rombootcodeend = ENDADDR(.syscall);

}
