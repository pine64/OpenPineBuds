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

/*
=========================================================================
	MCU Sleep Power Down and Wakeup Power Up Helper Functions
=========================================================================
*/

    .syntax	unified

#ifdef MCU_SLEEP_POWER_DOWN

#ifdef ROM_BUILD
    .section .bss.mcu_sleep_stack
#else
    .section .sram_bss.mcu_sleep_stack
#endif
    .align    2
mcu_sleep_stack:
    .space 4

#ifdef ROM_BUILD
    .section .text.hal_sleep_mcu_power_down, "ax", %progbits
#else
    .section .sram_text.hal_sleep_mcu_power_down, "ax", %progbits
#endif
    .thumb
    .thumb_func
    .align    2
    .globl    hal_sleep_mcu_power_down
    .type    hal_sleep_mcu_power_down, %function
hal_sleep_mcu_power_down:

    ldr r3, =mcu_sleep_stack
    push {r4-r12,lr}
    /* TODO: Save FPU registers? */
    str sp, [r3]
	bl hal_psc_mcu_auto_power_down
.pd_loop:
	b .pd_loop

    .pool
    .size    hal_sleep_mcu_power_down, . - hal_sleep_mcu_power_down


#ifdef ROM_BUILD
    .section .text.hal_sleep_mcu_power_up, "ax", %progbits
#else
    .section .sram_text.hal_sleep_mcu_power_up, "ax", %progbits
#endif
    .thumb
    .thumb_func
    .align    2
    .globl    hal_sleep_mcu_power_up
    .type    hal_sleep_mcu_power_up, %function
hal_sleep_mcu_power_up:

    ldr r3, =mcu_sleep_stack
    ldr sp, [r3]
    isb
    /* TODO: Restore FPU registers? */
    pop {r4-r12,lr}
    bx lr

    .pool
    .size    hal_sleep_mcu_power_up, . - hal_sleep_mcu_power_up

#endif

    .end
