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

#ifdef NOSTD
#define HEAP_SECTION_SIZE			0
#else
#define HEAP_SECTION_SIZE			0x40
#endif
#define STACK_SECTION_SIZE			0x1000
#define CP_STACK_SECTION_SIZE		0x1000

LR_rom ROM_BASE NOCOMPRESS
{
	rom_vectors +0
	{
		*(.vectors, +FIRST)
	}

	rom_text +0 FIXED
	{
		*(.text*)

#ifndef NOSTD
		*(InRoot$$Sections)
		*armlib*(+TEXT)
		*armlib*(+CONST)
#endif

		*(.rodata*)
	}

#ifdef CHIP_HAS_AUDIO_CONST_ROM
	rom_audio_const +0
	{
		*audio_const_rom.o(.audio_const_index)
		*audio_const_rom.o(.rodata*)
	}
#endif

	rom_build_id +0
	{
#ifndef NOSTD
		*(.eh_frame*)
#endif
		*(.note.gnu.build-id)
	}

	rom_got_info +0
	{
		*(.got)
		*(.got.plt)
		*(.igot.plt)
		*(.dynamic)
	}

	.ARM.extab +0
	{
		*(.ARM.extab*, .gnu.linkonce.armextab.*)
	}

	.ARM.exidx +0
	{
		*(.ARM.exidx*, .gnu.linkonce.armexidx.*)
	}

	rom_text0_end +0 EMPTY 0
	{
	}

	vector_table RAM_BASE UNINIT VECTOR_SECTION_SIZE
	{
		*(.bss.vector_table, +FIRST)
	}

	reboot_param (ImageBase(vector_table) + VECTOR_SECTION_SIZE) UNINIT REBOOT_PARAM_SECTION_SIZE
	{
		*(.bss.reboot_param, +FIRST)
	}

	rom_ramx RAM_TO_RAMX(ImageBase(reboot_param) + REBOOT_PARAM_SECTION_SIZE)
	{
		*(.rom_ramx)
	}

	rom_data RAMX_TO_RAM(+0)
	{
		*(.data*)

#ifndef NOSTD
		*armlib*(+DATA)
#endif
	}

	rom_bss +0 UNINIT
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

	scratch_buf +0 UNINIT
	{
		*(.bss.scratch_buf)
	}

	ARM_LIB_STACK (RAM_BASE + RAM_SIZE - STACK_SECTION_SIZE) ALIGN 8 EMPTY STACK_SECTION_SIZE
	{
	}

	/* Check if data + heap + stack exceeds RAM limit */
	ScatterAssert(ImageBase(ARM_LIB_STACK) >= ImageLimit(ARM_LIB_HEAP))
	ScatterAssert(ImageLimit(ARM_LIB_STACK) <= (RAM_BASE + RAM_SIZE))

	free_ram ImageLimit(scratch_buf) EMPTY (ImageBase(ARM_LIB_STACK) - ImageLimit(scratch_buf))
	{
	}

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	cp_ramx_last_dummy RAMCPX_BASE EMPTY 0
	{
	}

	cp_ram_last_dummy RAMCP_BASE EMPTY 0
	{
	}

	cp_stack (RAMCP_BASE + RAMCP_SIZE - CP_STACK_SECTION_SIZE) EMPTY CP_STACK_SECTION_SIZE
	{
	}

	ScatterAssert(ImageBase(cp_stack) >= ImageLimit(cp_ram_last_dummy))
	ScatterAssert(ImageLimit(cp_stack) <= RAMCP_BASE + RAMCP_SIZE)
#endif
}

LR_build_info +0
{
	lr_build_info_start +0 FIXED EMPTY 0
	{
	}

	rom_export_fn (ROM_BASE + ROM_SIZE - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE) FIXED
	{
		*(.export_fn)
	}

	rom_build_info (ROM_BASE + ROM_SIZE - ROM_BUILD_INFO_SECTION_SIZE) FIXED
	{
		*(.build_info, +FIRST)
	}

	free_rom ImageLimit(lr_build_info_start) EMPTY (ImageBase(rom_export_fn) - ImageLimit(lr_build_info_start))
	{
	}

	ScatterAssert(LoadLimit(rom_build_info) <= (ROM_BASE + ROM_SIZE))
}

