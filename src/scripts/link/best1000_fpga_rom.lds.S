/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#include "plat_addr_map.h"

#define HEAP_SECTION_SIZE			0x400
#define STACK_SECTION_SIZE			0x1000
#define FAST_XRAM_SECTION_SIZE		0x3000
/* Linker script to configure memory regions. */
MEMORY
{
  ROM    (rx)  : ORIGIN = ROM_BASE,    LENGTH = ROM_SIZE
  RAM    (rwx) : ORIGIN = RAM_BASE,    LENGTH = RAM_SIZE
  RAMX   (rwx) : ORIGIN = RAMX_BASE,   LENGTH = RAM_SIZE
  FRAMX  (rwx) : ORIGIN = RAM_BASE + RAM_SIZE - FAST_XRAM_SECTION_SIZE,	LENGTH = FAST_XRAM_SECTION_SIZE  
}

/* Library configurations */
GROUP(libgcc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exidx_start
 *   __exidx_end
 *   __rom_etext
 *   __rom_data_start__
 *   __rom_data_end__
 *   __rom_bss_start__
 *   __rom_bss_end__
 *   __rom_HeapLimit
 *   __rom_StackLimit
 *   __rom_StackTop
 *   __rom_stack
 *   __Vectors_End
 *   __Vectors_Size
 *   __free_rom_ram
 *   __rom_end
 *   __free_rom
 *   __audio_const_rom_start
 *   __audio_const_rom_end
 *   __audio_const_rom_size
 */
ENTRY(Reset_Handler)

SECTIONS
{
	.rom_text (ORIGIN(ROM)) :
	{
		__vector_table = .;
		KEEP(*(.vectors_table))
		*(.text.Reset_Handler)
		*(.text.System_Init)
		. = ALIGN(4);

		__Vectors_End = .;
		__Vectors_Size = __Vectors_End - __Vectors;

		*(.text*)

#ifndef NOSTD
		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)
#endif

		. = ALIGN(4);

		*(.rodata*)

#ifdef CHIP_HAS_AUDIO_CONST_ROM
		__audio_const_rom_start = .;
		KEEP(*audio_const_rom.o(.audio_const_index))
		KEEP(*audio_const_rom.o(.rodata*))
		__audio_const_rom_end = .;
		__audio_const_rom_size = __audio_const_rom_end - __audio_const_rom_start;
#endif
#ifndef NOSTD
		KEEP(*(.eh_frame*))
#endif
		*(.note.gnu.build-id)

		. = ALIGN(4);
	} > ROM

	.rom_got_info (.) :
	{
		__rom_got_info_start = .;
		__rom_got_start = .;
		*(.got)
		. = ALIGN(4);
		__rom_got_end = .;
		__rom_got_plt_start = .;
		*(.got.plt)
		. = ALIGN(4);
		__rom_igot_plt_start = .;
		*(.igot.plt)
		. = ALIGN(4);
		__rom_dynamic_start = .;
		*(.dynamic)
		. = ALIGN(4);
		__rom_got_info_end = .;
	} > ROM

	.ARM.extab (.) :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ROM

	__exidx_start = .;
	.ARM.exidx (.) :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ROM
	__exidx_end = .;

	. = ALIGN(4);

	.custom.cmd.table (.) :
	{
	. = ALIGN(4);
	__custom_handler_table_start = .;
		KEEP(*(.custom_handler_table))
	__custom_handler_table_end = .;
	} > ROM

	__rom_text0_end = .;

	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.reboot_param (.) (NOLOAD) :
	{
		KEEP(*(.reboot_param))
		. = REBOOT_PARAM_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.rom_ramx (RAM_TO_RAMX(.)) : AT (__rom_text0_end)
	{
		__rom_ramx_start = .;
		*(.rom_ramx)
		. = ALIGN(4);
		__rom_ramx_end = .;
	} > RAMX

	__rom_etext = __rom_text0_end + SIZEOF(.rom_ramx);

	.rom_data (RAMX_TO_RAM(.)) : AT (__rom_etext)
	{
		__rom_data_start__ = .;
		*(.data*)
		. = ALIGN(4);

#ifndef NOSTD
		*(vtable)

		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
#endif

		/* All data end */
		__rom_data_end__ = .;
	} > RAM

	.rom_bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__rom_bss_start__ = .;
		*(.bss*)
		*(COMMON)
		*(AHBSDRAM1)
		. = ALIGN(4);
		__rom_bss_end__ = .;
#ifndef NOSTD
		__bss_start__ = __rom_bss_start__;
		__bss_end__ = __rom_bss_end__;
#endif
	} > RAM

	.rom_heap (.) (NOLOAD) :
	{
		. = ALIGN(8);
		__rom_HeapBase = .;
#ifndef NOSTD
		__end__ = .;
		end = __end__;
		. += HEAP_SECTION_SIZE;
#endif
		. = ALIGN(8);
		__rom_HeapLimit = .;
#ifndef NOSTD
		__HeapLimit = __rom_HeapLimit;
#endif
	} > RAM

	.scratch_buf (.) (NOLOAD) :
	{
		. = ALIGN(4);
		*(.scratch_buf)
		. = ALIGN(4);
	} > RAM
	
	.rodata (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__mixprompt_property_table_start = .;
		KEEP(*(.mixprompt_property_table))
		__mixprompt_property_table_end = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.rom_stack_dummy (.) (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__rom_StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__rom_StackLimit = __rom_StackTop - SIZEOF(.rom_stack_dummy);
	PROVIDE(__rom_stack = __rom_StackTop);
#ifndef NOSTD
	__StackLimit = __rom_StackLimit;
	__StackTop = __rom_StackTop;
	PROVIDE(__stack = __rom_StackTop);
#endif

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__rom_StackLimit >= __rom_HeapLimit, "region RAM overflowed with stack")
	__free_rom_ram = __rom_StackLimit - __rom_HeapLimit;

	__rom_end = __rom_etext + SIZEOF(.rom_data);

	.rom_export_fn (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE
					- CUSTOM_PARAMETER_SECTION_SIZE) :
	{
		__export_fn_rom = .;
		KEEP(*(.export_fn))
		. = ROM_EXPORT_FN_SECTION_SIZE;
	} > ROM = 0x00000000

	.rom_build_info (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE - CUSTOM_PARAMETER_SECTION_SIZE) :
	{
		KEEP(*(.build_info))
		. = ROM_BUILD_INFO_SECTION_SIZE;
	} > ROM = 0x00000000
	.crash_dump (ORIGIN(ROM) + LENGTH(ROM) - CRASH_DUMP_SECTION_SIZE - CUSTOM_PARAMETER_SECTION_SIZE) :
	{
		__crash_dump_start = .;
		. = CRASH_DUMP_SECTION_SIZE;
		__crash_dump_end = .;
	} > ROM = 0x00000000
	.custom_parameter (ORIGIN(ROM) + LENGTH(ROM) - CUSTOM_PARAMETER_SECTION_SIZE) :
	{
		__custom_parameter_start = .;
		. = CUSTOM_PARAMETER_SECTION_SIZE;
		__custom_parameter_end = .;
	} > ROM = 0x00000000

	ASSERT(ADDR(.rom_export_fn) >= __rom_end, "region ROM overflowed with .rom_export_fn")
	__free_rom = ADDR(.rom_export_fn) - __rom_end;
}

