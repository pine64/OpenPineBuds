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
#include "cmsis.h"

#if defined(__GIC_PRESENT) && (__GIC_PRESENT)

#include "cmsis_nvic.h"
#include "plat_types.h"
#include "plat_addr_map.h"
#include "hal_location.h"

extern void Reset_Handler(void);
extern void Undef_Handler(void);
extern void SVC_Handler(void);
extern void PAbt_Handler(void);
extern void DAbt_Handler(void);
extern void IRQ_Handler(void);
extern void FIQ_Handler(void);

struct VECTOR_HDLR_T {
    uint32_t reset_hdlr;
    uint32_t undef_hdlr;
    uint32_t svc_hdlr;
    uint32_t pabt_hdlr;
    uint32_t dabt_hdlr;
    uint32_t hyp_hdlr;
    uint32_t irq_hdlr;
    uint32_t fiq_hdlr;
};

static struct VECTOR_HDLR_T * const exc_vector = (struct VECTOR_HDLR_T *)(DSP_BOOT_REG + 0x20);

static IRQn_ID_t cur_act_irq = EXCEPTION_NONE;
static uint32_t cur_irq_level;

static GIC_FAULT_DUMP_HANDLER_T dump_hdlr;

void c_irq_handler(void)
{
    const IRQn_ID_t irqn = IRQ_GetActiveIRQ();
    const IRQn_ID_t old_irq = cur_act_irq;

    cur_irq_level++;
    cur_act_irq = irqn;

    IRQHandler_t const handler = IRQ_GetHandler(irqn);
    if (handler != NULL) {
        __enable_irq();
        handler();
        __disable_irq();
    }
    IRQ_EndOfInterrupt(irqn);

    cur_act_irq = old_irq;
    cur_irq_level--;
}

void c_undef_handler(uint32_t opcode, uint32_t state, const struct FAULT_REGS_T *regs)
{
    const IRQn_ID_t old_irq = cur_act_irq;
    cur_act_irq = EXCEPTION_UNDEF;

    if (dump_hdlr) {
        struct UNDEF_FAULT_INFO_T info;

        info.id = cur_act_irq;
        info.opcode = opcode;
        info.state = state;
        dump_hdlr((uint32_t *)regs, (uint32_t *)&info, sizeof(info));
    }
    //ASSERT(false, "Undefined Instruction!");

    cur_act_irq = old_irq;
}

void c_svc_handler(uint32_t svc_num, const struct FAULT_REGS_T *regs)
{
    const IRQn_ID_t old_irq = cur_act_irq;
    cur_act_irq = EXCEPTION_SVC;

    if (dump_hdlr) {
        struct SVC_FAULT_INFO_T info;

        info.id = cur_act_irq;
        info.svc_num = svc_num;
        dump_hdlr((uint32_t *)regs, (uint32_t *)&info, sizeof(info));
    }
    //ASSERT(false, "SVC!");

    cur_act_irq = old_irq;
}

void c_pabt_handler(uint32_t IFSR, uint32_t IFAR, const struct FAULT_REGS_T *regs)
{
    const IRQn_ID_t old_irq = cur_act_irq;
    cur_act_irq = EXCEPTION_PABT;

    if (dump_hdlr) {
        struct PABT_FAULT_INFO_T info;

        info.id = cur_act_irq;
        info.IFSR = IFSR;
        info.IFAR = IFAR;
        dump_hdlr((uint32_t *)regs, (uint32_t *)&info, sizeof(info));
    }
    //ASSERT(false, "Prefetch Abort!");

    cur_act_irq = old_irq;
}

void c_dabt_handler(uint32_t DFSR, uint32_t DFAR, const struct FAULT_REGS_T *regs)
{
    const IRQn_ID_t old_irq = cur_act_irq;
    cur_act_irq = EXCEPTION_DABT;

    if (dump_hdlr) {
        struct DABT_FAULT_INFO_T info;

        info.id = cur_act_irq;
        info.DFSR = DFSR;
        info.DFAR = DFAR;
        dump_hdlr((uint32_t *)regs, (uint32_t *)&info, sizeof(info));
    }
    //ASSERT(false, "Data Abort!");

    cur_act_irq = old_irq;
}

void c_fiq_handler(void)
{
    c_irq_handler();
}

void GIC_DisableAllIRQs(void)
{
    int i;

    for (i = 0; i < (USER_IRQn_QTY + 31) / 32; i++) {
        GICDistributor->ICENABLER[i] = ~0UL;
    }
}
void NVIC_DisableAllIRQs(void) __attribute__((alias("GIC_DisableAllIRQs")));

void GIC_InitVectors(void)
{
    volatile uint32_t *boot = (volatile uint32_t *)DSP_BOOT_REG;

    // Unlock
    boot[32] = 0xCAFE0001;
    __DMB();

    exc_vector->reset_hdlr = (uint32_t)Reset_Handler;
    exc_vector->undef_hdlr = (uint32_t)Undef_Handler;
    exc_vector->svc_hdlr = (uint32_t)SVC_Handler;
    exc_vector->pabt_hdlr = (uint32_t)PAbt_Handler;
    exc_vector->dabt_hdlr = (uint32_t)DAbt_Handler;
    exc_vector->irq_hdlr = (uint32_t)IRQ_Handler;
    exc_vector->fiq_hdlr = (uint32_t)FIQ_Handler;

    // Lock
    __DMB();
    boot[32] = 0xCAFE0000;
    __DMB();
}
void NVIC_InitVectors(void) __attribute__((alias("GIC_InitVectors")));

void GIC_SetFaultDumpHandler(GIC_FAULT_DUMP_HANDLER_T handler)
{
    dump_hdlr = handler;
}

IRQn_Type IRQ_GetCurrentActiveIRQ(void)
{
    return cur_act_irq;
}
IRQn_Type NVIC_GetCurrentActiveIRQ(void) __attribute__((alias("IRQ_GetCurrentActiveIRQ")));

#endif

