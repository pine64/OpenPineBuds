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
#ifdef __ARMCC_VERSION

#include "plat_addr_map.h"
#include "link_sym_armclang.h"

	.syntax  unified

	.text

	.macro def_sym, name:req, val:req
	.global \name
	.set \name, \val
	.endm

	def_sym __export_fn_rom, (ROM_BASE + ROM_SIZE - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE)

	.section .code_start_addr, "a", %progbits
	.word BUILD_INFO_MAGIC
#ifdef PROGRAMMER
	.word __exec_struct_start
#else
	.word __flash_start
#endif

	.end

#endif // __ARMCC_VERSION

