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
#define TO_STRING_A(s)                  # s
#define TO_STRING(s)                    TO_STRING_A(s)

.globl __slave_code_start
.globl __slave_code_end
.section .slave_code_flash_rodata, "a", %progbits
.balign 4

__slave_code_start:
	.incbin TO_STRING(SLAVE_BIN_NAME)
__slave_code_end:

