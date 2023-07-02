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

#define HEAP_SECTION_SIZE			0x40
#define STACK_SECTION_SIZE			0x1000
#define CP_STACK_SECTION_SIZE		0x1000

/* Linker script to configure memory regions. */
MEMORY
{
	ROM    		(rx)  	: ORIGIN = ROM_BASE,    			LENGTH = ROM_SIZE	/* see plat_addr_map.h and common.mk */
	RAM    		(rwx) 	: ORIGIN = RAM_BASE,    			LENGTH = RAM_SIZE
	RAMX   		(rwx) 	: ORIGIN = RAMX_BASE,   			LENGTH = RAM_SIZE
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	RAMCP 		(rwx)	: ORIGIN = RAMCP_BASE,				LENGTH = RAMCP_SIZE
	RAMCPX 		(rwx)	: ORIGIN = RAMCPX_BASE,				LENGTH = RAMCPX_SIZE
#endif
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
		KEEP(*(.vectors))

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

		. = ALIGN(4);
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

	.rom_bss (.) (NOLOAD) : AT (ADDR(.rom_bss))
	{
		. = ALIGN(4);
		__rom_bss_start__ = .;
		*(.bss*)
		*(COMMON)
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
	} > RAM

	.scratch_buf (.) (NOLOAD) :
	{
		. = ALIGN(4);
		*(.scratch_buf)
		. = ALIGN(4);
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
	__StackTop = __rom_StackTop;
	PROVIDE(__stack = __rom_StackTop);
#endif

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__rom_StackLimit >= __rom_HeapLimit, "region RAM overflowed with stack")
	__free_rom_ram = __rom_StackLimit - __rom_HeapLimit;

	__rom_end = __rom_etext + SIZEOF(.rom_data);

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	.cp_ramx_last_dummy (NOLOAD) :
	{
		__cp_ramx_last_dummy_start = .;
	} > RAMCPX

	.cp_ram_last_dummy (NOLOAD) :
	{
		. = ALIGN(8);
		__cp_ram_last_dummy_start = .;
	} > RAMCP

	__cp_stack_top = ORIGIN(RAMCP) + LENGTH(RAMCP);
	__cp_stack_limit = __cp_stack_top - CP_STACK_SECTION_SIZE;

	__free_ramcpx = ORIGIN(RAMCPX) + LENGTH(RAMCPX) - __cp_ramx_last_dummy_start;
	ASSERT(__cp_stack_limit >= __cp_ram_last_dummy_start, "region RAMCP overflowed with stack")
	__free_ramcp = __cp_stack_limit - __cp_ram_last_dummy_start;
#endif

	.rom_export_fn (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE) :
	{
		__export_fn_rom = .;
		KEEP(*(.export_fn))
		. = ROM_EXPORT_FN_SECTION_SIZE;
	} > ROM = 0x00000000

	.rom_build_info (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE) :
	{
		KEEP(*(.build_info))
		. = ROM_BUILD_INFO_SECTION_SIZE;
	} > ROM = 0x00000000

	ASSERT(ADDR(.rom_export_fn) >= __rom_end, "region ROM overflowed with .rom_export_fn")
	__free_rom = ADDR(.rom_export_fn) - __rom_end;
}

