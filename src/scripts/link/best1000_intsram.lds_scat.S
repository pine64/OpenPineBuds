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

#define STACK_SECTION_SIZE			0xD00
#define CP_STACK_SECTION_SIZE		0x1000

#define OVERLAY_DATA_SECTION_SIZE	0x1000

#define FAST_XRAM_SECTION_SIZE		0x0010

#ifndef OTA_CODE_OFFSET
#define OTA_CODE_OFFSET				0
#endif
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif
#define FLASH_NC_REGION_BASE		FLASH_C_TO_NC(FLASH_REGION_BASE)
#define FLASHX_REGION_BASE			FLASH_TO_FLASHX(FLASH_REGION_BASE)

#define RESERVED_SECTION_SIZE		0x1000

#define MAX(a, b)					((a) > (b) ? (a) : (b))

LR_boot_struct FLASHX_REGION_BASE NOCOMPRESS
{
	boot_struct FLASH_REGION_BASE
	{
		*(.boot_struct, +FIRST)
	}
}

LR_boot_flash +0
{
	boot_text_flash +0 FIXED
	{
		*(.boot_loader, +FIRST)
		*(.boot_text_flash*)
		*(.boot_rodata*)
	}

	vector_table RAM_BASE UNINIT VECTOR_SECTION_SIZE
	{
		*(.bss.vector_table, +FIRST)
	}

	reboot_param (ImageBase(vector_table) + VECTOR_SECTION_SIZE) UNINIT REBOOT_PARAM_SECTION_SIZE
	{
		*(.bss.reboot_param, +FIRST)
	}

	userdata_pool (ImageBase(reboot_param) + REBOOT_PARAM_SECTION_SIZE) UNINIT
	{
		*(.bss.userdata_pool)
	}
}

LR_boot_sram +0
{
	boot_text_sram RAM_TO_RAMX(ImageLimit(userdata_pool))
	{
		*armlib/c_*(:gdef:*memcpy*)
		*armlib/c_*(:gdef:*memset*)
		memcpy.o(.text*)
		memset.o(.text*)
		libc_rom.o(.text*)

		hal_norflash*.o(.text*)
		norflash_*.o(.text*)

		*(.boot_text_sram*)

		/* All the codes that are run both in boot and non-boot */
		hal_*(.text*)
		system_ARMCM.o(.text*)
		system_utils.o(.text*)
		cmsis_nvic.o(.text*)
		/* For 64-bit div in boot, e.g., __aeabi_uldivmod */
		/* *libgcc.a:((.text*)) */
	}

	boot_data_sram RAMX_TO_RAM(+0)
	{
		hal_norflash*.o(.data*, .rodata*)
		norflash_*.o(.data*, .rodata*)
		hal_psram.o(.data*, .rodata*)
		*(.boot_data*)
		/* All the data that is accessed both in boot and non-boot */
		hal_*(.rodata*)
		system_ARMCM.o(.rodata*)
		system_utils.o(.rodata*)
		cmsis_nvic.o(.rodata*)
		/* *libgcc.a:(.rodata*) */
	}

	boot_bss_sram +0 UNINIT
	{
		hal_norflash*.o(+BSS)
		norflash_*.o(+BSS)
		hal_psram.o(+BSS)
		*(.bss.boot_bss*)
	}
}

LR_fast_sram +0
{
	fast_text_sram (RAMX_BASE + RAM_SIZE - FAST_XRAM_SECTION_SIZE)
	{
		*(.fast_text_sram)
	}

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	cp_text_sram RAMCPX_BASE
	{
		*(.cp_text_sram*)
	}

	cp_text_sram_start RAMX_TO_RAM(ImageBase(cp_text_sram)) EMPTY ImageLength(cp_text_sram)
	{
	}

	vector_table_cp RAMCP_BASE UNINIT VECTOR_SECTION_SIZE
	{
		*(.vector_table_cp, +FIRST)
	}

	cp_data_sram (ImageBase(vector_table_cp) + VECTOR_SECTION_SIZE)
	{
		*(.cp_data*)
	}

	cp_bss_sram +0 UNINIT
	{
		*(.bss.cp_bss*)
	}

	cp_stack (RAMCP_BASE + RAMCP_SIZE - CP_STACK_SECTION_SIZE) EMPTY CP_STACK_SECTION_SIZE
	{
	}

	ScatterAssert(ImageLimit(cp_stack) <= RAMCP_BASE + RAMCP_SIZE)

#define OVERLAY_TEXT_EXEC_START			ImageLimit(cp_text_sram)
#define OVERLAY_DATA_START				ImageLimit(cp_bss_sram)
#define SRAM_TEXT_START					RAM_TO_RAMX(ImageLimit(boot_bss_sram))
#else
#define OVERLAY_TEXT_EXEC_START			ImageLimit(fast_text_sram)
#define OVERLAY_DATA_START				ImageLimit(boot_bss_sram)
#define SRAM_TEXT_START					RAM_TO_RAMX(ImageLimit(overlay_data_end))
#endif
#define OVERLAY_TEXT_START				RAMX_TO_RAM(OVERLAY_TEXT_EXEC_START)
}

LR_overlay +0
{
	overlay_start OVERLAY_TEXT_START EMPTY 0
	{
	}

	overlay_text0 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text0)
	}
	overlay_text1 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text1)
	}
	overlay_text2 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text2)
	}
	overlay_text3 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text3)
	}
	overlay_text4 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text4)
	}
	overlay_text5 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text5)
	}
	overlay_text6 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text6)
	}
	overlay_text7 OVERLAY_TEXT_EXEC_START OVERLAY
	{
		*(.overlay_text7)
	}

	overlay_text_end (MAX(ImageLimit(overlay_text0), \
						MAX(ImageLimit(overlay_text1), \
						MAX(ImageLimit(overlay_text2), \
						MAX(ImageLimit(overlay_text3), \
						MAX(ImageLimit(overlay_text4), \
						MAX(ImageLimit(overlay_text5), \
						MAX(ImageLimit(overlay_text6), \
							ImageLimit(overlay_text7))))))))) EMPTY 0
	{
	}
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	ScatterAssert(ImageLimit(overlay_text_end) <= RAMCPX_BASE + RAMCPX_SIZE)
#else
	ScatterAssert(ImageLimit(overlay_text_end) <= ImageBase(fast_text_sram) + FAST_XRAM_SECTION_SIZE)
#endif

	overlay_data0 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data0, .overlay_rodata0)
	}
	overlay_data1 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data1, .overlay_rodata1)
	}
	overlay_data2 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data2, .overlay_rodata2)
	}
	overlay_data3 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data3, .overlay_rodata3)
	}
	overlay_data4 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data4, .overlay_rodata4)
	}
	overlay_data5 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data5, .overlay_rodata5)
	}
	overlay_data6 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data6, .overlay_rodata6)
	}
	overlay_data7 OVERLAY_DATA_START OVERLAY
	{
		*(.overlay_data7, .overlay_rodata7)
	}

	overlay_data_end (MAX(ImageLimit(overlay_data0), \
						MAX(ImageLimit(overlay_data1), \
						MAX(ImageLimit(overlay_data2), \
						MAX(ImageLimit(overlay_data3), \
						MAX(ImageLimit(overlay_data4), \
						MAX(ImageLimit(overlay_data5), \
						MAX(ImageLimit(overlay_data6), \
							ImageLimit(overlay_data7))))))))) EMPTY 0
	{
	}
	ScatterAssert(ImageLimit(overlay_data_end) <= ImageBase(overlay_data0) + OVERLAY_DATA_SECTION_SIZE)
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	ScatterAssert(ImageLimit(overlay_data_end) <= ImageBase(cp_stack))
#endif
}

LR_sram +0
{
	sram_text AlignExpr(SRAM_TEXT_START, 8)
	{
		*(InRoot$$Sections)
		*armlib*(+TEXT)

		*(.sram_text*)
		filter_process.o(.text*)

		*(.fast_text_sram*)

		*(.text*)
		*(.flash_text*)

	}

	sram_data RAMX_TO_RAM(+0)
	{
		*armlib*(+DATA +CONST)

		*(.rodata*)
		*(.sram_data*)
	}

	sram_bss +0 UNINIT
	{
		*armlib*(+BSS)

		*(.bss.sram_bss*)
	}
}

LR_text +0
{
	text +0 EMPTY 0
	{
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

	rodata FLASHX_TO_FLASH(+0) :
	{
		*(.flash_rodata*)
		*(.eh_frame*)
		*(.note.gnu.build-id)
	}
}

LR_data +0
{
#ifdef DATA_BUF_START
#define DATA_SEC_START 					DATA_BUF_START
#else
#define DATA_SEC_START 					ImageLimit(sram_bss)
#endif

	data DATA_SEC_START
	{
		*(vtable)
		*(.data*)
	}

	bss +0 UNINIT
	{
		*(.bss*)
		*(COMMON)
	}

	ARM_LIB_HEAP +0 ALIGN 8 EMPTY HEAP_SECTION_SIZE
	{
	}

	ARM_LIB_STACK (RAM_BASE + RAM_SIZE - FAST_XRAM_SECTION_SIZE - STACK_SECTION_SIZE) ALIGN 8 EMPTY STACK_SECTION_SIZE
	{
	}

	/* Check if data + heap + stack exceeds RAM limit */
	ScatterAssert(ImageBase(ARM_LIB_STACK) >= ImageLimit(ARM_LIB_HEAP))
	ScatterAssert(ImageLimit(ARM_LIB_STACK) <= (RAM_BASE + RAM_SIZE - FAST_XRAM_SECTION_SIZE))

	free_ram ImageLimit(ARM_LIB_HEAP) EMPTY (ImageBase(ARM_LIB_STACK) - ImageLimit(ARM_LIB_HEAP))
	{
	}
}

LR_tail +0
{
	build_info FLASHX_TO_FLASH(+0)
	{
		*(.build_info, +FIRST)
	}

	/* The following section be the last loaded section */
	code_start_addr +0
	{
		*(.code_start_addr, +LAST)
	}

	custom_parameter (FLASH_NC_REGION_BASE + FLASH_REGION_SIZE - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE) EMPTY CUSTOM_PARAMETER_SECTION_SIZE
	{
	}

	userdata (FLASH_NC_REGION_BASE + FLASH_REGION_SIZE - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2) EMPTY (USERDATA_SECTION_SIZE*2)
	{
	}

	audio (FLASH_NC_REGION_BASE + FLASH_REGION_SIZE - FACTORY_SECTION_SIZE - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE) EMPTY AUD_SECTION_SIZE
	{
	}

	reserved (FLASH_NC_REGION_BASE + FLASH_REGION_SIZE - FACTORY_SECTION_SIZE - RESERVED_SECTION_SIZE) EMPTY RESERVED_SECTION_SIZE
	{
	}

	factory (FLASH_NC_REGION_BASE + FLASH_REGION_SIZE - FACTORY_SECTION_SIZE) EMPTY FACTORY_SECTION_SIZE
	{
	}

	ScatterAssert(FLASH_C_TO_NC(ImageLimit(code_start_addr)) <= ImageBase(custom_parameter))
}

