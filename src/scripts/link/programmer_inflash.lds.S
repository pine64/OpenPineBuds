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

#define CODE_MSG_OVERHEAD			8

/* TODO: Add FLASH_REGION_OFFSET if there are multiple levels of boot loaders */
#if !defined(OTA_BOOT_SIZE) && defined(OTA_CODE_OFFSET)
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#else
#define FLASH_REGION_BASE			(FLASH_BASE)
#endif

#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif
#define FLASH_NC_REGION_BASE		FLASH_C_TO_NC(FLASH_REGION_BASE)
#define FLASHX_REGION_BASE			FLASH_TO_FLASHX(FLASH_REGION_BASE)
/* Linker script to configure memory regions. */
MEMORY
{
	ROM    (rx)  : ORIGIN = ROM_BASE,    LENGTH = ROM_SIZE	/* see plat_addr_map.h and common.mk */
	FLASH 		(r)		: ORIGIN = FLASH_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
	FLASH_NC	(r)		: ORIGIN = FLASH_NC_REGION_BASE,	LENGTH = FLASH_REGION_SIZE
	FLASHX		(rx)	: ORIGIN = FLASHX_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
	RAM    (rwx) : ORIGIN = RAM_BASE,    LENGTH = RAM_SIZE
	RAMX   (rx)  : ORIGIN = RAMX_BASE,   LENGTH = RAM_SIZE
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
 *   __exec_struct_start
 *   __cust_cmd_init_tbl_start
 *   __cust_cmd_init_tbl_end
 *   __cust_cmd_hldr_tbl_start
 *   __cust_cmd_hldr_tbl_end
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 */

ENTRY(Boot_Loader)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

	.boot_struct (ORIGIN(FLASH)) :
	{
		__flash_start = .;
		KEEP(*(.boot_struct))
		. = ALIGN(4);
	} > FLASH

	. = FLASH_TO_FLASHX(.);

	.boot_text_flash (.) : AT (FLASHX_TO_FLASH(ADDR(.boot_text_flash)))
	{
		*(.boot_loader)
		*(.boot_text_flash*)
		*(.boot_rodata_flash*)
		*:main.o(.text* .rodata*)
		*:hal_bootmode.o(.text* .rodata*)
		*:hal_cmu_*.o(.text.hal_cmu_get_bootmode_addr .rodata.hal_cmu_get_bootmode_addr)
		*:cmsis_nvic.o(.text.NVIC_DisableAllIRQs .rodata.NVIC_DisableAllIRQs)
		. = ALIGN(4);
	} > FLASHX

	.ota_boot_info (ALIGN(4096)) :
	{
		*(.ota_boot_info)
		. = (0x1000);
	} > FLASHX

	.ota_boot_rev (ALIGN(4096)) :
	{
		*(.ota_boot_rev)
		. = (0x1000);
	} > FLASHX

	. = FLASHX_TO_FLASH(.);

	.got_info (.) :
	{
		__got_info_start = .;
		__got_start = .;
		*(.got)
		. = ALIGN(4);
		__got_end = .;
		__got_plt_start = .;
		*(.got.plt)
		. = ALIGN(4);
		__igot_plt_start = .;
		*(.igot.plt)
		. = ALIGN(4);
		__dynamic_start = .;
		*(.dynamic)
		. = ALIGN(4);
		__got_info_end = .;
	} > FLASH

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

	.burn_buffer (.) (NOLOAD) :
	{
		/* The size of .burn_buffer should be greater than __rom_HeapLimit. In most cases 8K is enough. */
		KEEP(*(.burn_buffer))
		. = ALIGN(4);
	} > RAM

/*
	.code_dummy (.) (NOLOAD) :
	{
		. = CODE_MSG_OVERHEAD;
		. = ALIGN(4);
	} > RAM
*/

	__boot_sram_start__ = .;

	. = RAM_TO_RAMX(.);

	__boot_sram_start_flash__ = LOADADDR(.got_info) + SIZEOF(.got_info);

	.text (.) : AT (__boot_sram_start_flash__)
	{
		*(.text*)
		. = ALIGN(4);
	} > RAMX

	.ARM.extab (.) : AT (LOADADDR(.text) + SIZEOF(.text))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > RAMX

	__exidx_start = .;
	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx (.) : AT (LOADADDR(.ARM.extab) + SIZEOF(.ARM.extab))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > RAMX
	__exidx_end = .;

	. = RAMX_TO_RAM(.);

	.rodata (.) : AT (LOADADDR(.ARM.exidx) + SIZEOF(.ARM.exidx))
	{
		*(.rodata*)
		*(.boot_rodata_sram*)

		. = ALIGN(4);
		__cust_cmd_init_tbl_start = .;
		KEEP(*(.cust_cmd_init_tbl))
		__cust_cmd_init_tbl_end = .;

		. = ALIGN(4);
		__cust_cmd_hldr_tbl_start = .;
		KEEP(*(.cust_cmd_hldr_tbl))
		__cust_cmd_hldr_tbl_end = .;

		*(.note.gnu.build-id)
		. = ALIGN(4);
	} > RAM

	__etext = .;

	.data (.) : AT (LOADADDR(.rodata) + SIZEOF(.rodata))
	{
		__data_start__ = .;
		*(.data*)

		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	__boot_sram_end__ = .;

	__boot_sram_end_flash__ = LOADADDR(.data) + SIZEOF(.data);
	
	.build_info (LOADADDR(.data) + SIZEOF(.data)) : AT (LOADADDR(.data) + SIZEOF(.data))
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > FLASH = 0x00000000

	/* The following section be the last loaded section */
	.code_start_addr (.) :
	{
		__app_entry_address__ = .;
		LONG(APP_ENTRY_ADDRESS);
		__download_uart_bandrate__ = .;
		LONG(DOWNLOAD_UART_BANDRATE);
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(__flash_start));
	} > FLASH
	
	__flash_end = .;
	
	.bss (ADDR(.data)+SIZEOF(.data)) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.heap (.) (NOLOAD) :
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. += HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (.) (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;

	__boot_bss_sram_start__ = .;
	__boot_bss_sram_end__ = .;
	__sram_text_data_start_flash__ = .;
	__sram_text_data_end_flash__ = .;
	__sram_text_data_start__ = .;
	__sram_bss_start__ = .;
	__sram_bss_end__ = .;
	__fast_sram_text_data_start_flash__ = .;
	__fast_sram_text_data_end_flash__ = .;
	__fast_sram_text_data_start__ = .;	
}
