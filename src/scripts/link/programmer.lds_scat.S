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

#define MIN_BURN_BUFFER_SIZE		0x2000
#define BURN_BUFFER_END				0x11000

#define HEAP_SECTION_SIZE			0x40
#define STACK_SECTION_SIZE			0x1000

#define CODE_MSG_OVERHEAD			8

LR_vector (RAMX_BASE + BURN_BUFFER_END) NOCOMPRESS
{
	vector_table RAM_BASE UNINIT VECTOR_SECTION_SIZE
	{
		*(.bss.vector_table, +FIRST)
	}

	reboot_param (ImageBase(vector_table) + VECTOR_SECTION_SIZE) UNINIT REBOOT_PARAM_SECTION_SIZE
	{
		*(.bss.reboot_param, +FIRST)
	}

	burn_buffer (ImageBase(reboot_param) + REBOOT_PARAM_SECTION_SIZE) UNINIT
	{
		/* The size of .burn_buffer should be greater than __rom_HeapLimit. In most cases 8K is enough. */
		*(.bss.burn_buffer)
	}

	ScatterAssert(ImageLimit(burn_buffer) <= (ImageBase(vector_table) + BURN_BUFFER_END))
}

LR_boot_struct +0
{
	boot_struct RAMX_TO_RAM(+0)
	{
		*(.boot_struct, +FIRST)
	}

	exec_struct +0
	{
		*(.exec_struct, +FIRST)
	}

	got_info +0
	{
		*(.got)
		*(.got.plt)
		*(.igot.plt)
		*(.dynamic)
	}
}

LR_text +0
{
	text +0 FIXED
	{
		*(.text*)

#ifndef NOSTD
		*(InRoot$$Sections)
		*armlib*(+TEXT)
#endif
	}

	.ARM.extab +0
	{
		*(.ARM.extab*, .gnu.linkonce.armextab.*)
	}

	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx +0
	{
		*(.ARM.exidx*, .gnu.linkonce.armexidx.*)
	}

	rodata RAMX_TO_RAM(+0)
	{
		*(.rodata*)

#ifndef NOSTD
		*armlib*(+CONST)
		KEEP(*(.eh_frame*))
#endif
		*(.note.gnu.build-id)
		. = ALIGN(4);
	}

	cust_cmd_init_tbl +0
	{
		*(.cust_cmd_init_tbl)
	}

	cust_cmd_hdlr_tbl +0
	{
		*(.cust_cmd_hldr_tbl)
	}

	data +0
	{
		*(.data*)

#ifndef NOSTD
		*armlib*(+DATA)
#endif
	}

	.build_info +0
	{
		*(.build_info, +FIRST)
	}

	/* The following section be the last loaded section */
	code_start_addr +0
	{
		*(.code_start_addr, +LAST)
	}

	bss +0 UNINIT
	{
		*(.bss*)
		*(COMMON)

#ifndef NOSTD
		*armlib*(+BSS)
#endif
	}

	ARM_LIB_HEAP +0 ALIGN 8 EMPTY HEAP_SECTION_SIZE
	{
	}

	ARM_LIB_STACK (RAM_BASE + RAM_SIZE - STACK_SECTION_SIZE) ALIGN 8 EMPTY STACK_SECTION_SIZE
	{
	}

	/* Check if data + heap + stack exceeds RAM limit */
	ScatterAssert(ImageBase(ARM_LIB_STACK) >= ImageLimit(ARM_LIB_HEAP))
	ScatterAssert(ImageLimit(ARM_LIB_STACK) <= (RAM_BASE + RAM_SIZE))

	free_ram ImageLimit(ARM_LIB_HEAP) EMPTY (ImageBase(ARM_LIB_STACK) - ImageLimit(ARM_LIB_HEAP))
	{
	}
}
