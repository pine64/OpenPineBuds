/* mbed Microcontroller Library
 * CMSIS-style functionality to support dynamic vectors
 *******************************************************************************
 * Copyright (c) 2011 ARM Limited. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of ARM Limited nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */
#ifndef __ARM_ARCH_ISA_ARM

#include "cmsis_nvic.h"
#include "hal_location.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#ifdef __ARMCC_VERSION
#include "link_sym_armclang.h"
#endif

STATIC_ASSERT(NVIC_NUM_VECTORS * 4 <= VECTOR_SECTION_SIZE,
              "ERROR: VECTOR_SECTION_SIZE too small!");

// The vector table must be aligned to NVIC_NUM_VECTORS-word boundary, rounding
// up to the next power of two
// -- 0x100 for 33~64 vectors, and 0x200 for 65~128 vectors
#ifdef __ARMCC_VERSION
#define VECTOR_LOC __attribute__((section(".bss.vector_table")))
#else
#define VECTOR_LOC __attribute__((section(".vector_table")))
#endif

#define FAULT_HANDLER __attribute__((weak, alias("NVIC_default_handler")))

static uint32_t VECTOR_LOC vector_table[NVIC_NUM_VECTORS];

void NVIC_DisableAllIRQs(void) {
  int i;

  for (i = 0; i < (USER_IRQn_QTY + 31) / 32; i++) {
    NVIC->ICER[i] = ~0UL;
  }
  SCB->VTOR = 0;
}

static void NAKED BOOT_TEXT_FLASH_LOC NVIC_default_handler(void) {
  asm volatile("_loop:; nop; nop; nop; nop; b _loop;");
}

void FAULT_HANDLER Reset_Handler(void);
void FAULT_HANDLER NMI_Handler(void);
void FAULT_HANDLER HardFault_Handler(void);
void FAULT_HANDLER MemManage_Handler(void);
void FAULT_HANDLER BusFault_Handler(void);
void FAULT_HANDLER UsageFault_Handler(void);
void FAULT_HANDLER SecureFault_Handler(void);
void FAULT_HANDLER SVC_Handler(void);
void FAULT_HANDLER DebugMon_Handler(void);
void FAULT_HANDLER PendSV_Handler(void);
void FAULT_HANDLER SysTick_Handler(void);
extern uint32_t __rom_stack[];
extern uint32_t __stack[];

static const uint32_t BOOT_RODATA_FLASH_LOC
    fault_handlers[NVIC_USER_IRQ_OFFSET] = {
#if defined(ROM_BUILD) && !defined(ROM_IN_FLASH)
        (uint32_t)__rom_stack,
#else
    (uint32_t)__stack,
#endif
        (uint32_t)Reset_Handler,        (uint32_t)NMI_Handler,
        (uint32_t)HardFault_Handler,    (uint32_t)MemManage_Handler,
        (uint32_t)BusFault_Handler,     (uint32_t)UsageFault_Handler,
        (uint32_t)SecureFault_Handler,  (uint32_t)NVIC_default_handler,
        (uint32_t)NVIC_default_handler, (uint32_t)NVIC_default_handler,
        (uint32_t)SVC_Handler,          (uint32_t)DebugMon_Handler,
        (uint32_t)NVIC_default_handler, (uint32_t)PendSV_Handler,
        (uint32_t)SysTick_Handler,
};

void BOOT_TEXT_FLASH_LOC NVIC_InitVectors(void) {
  int i;

  for (i = 0; i < NVIC_NUM_VECTORS; i++) {
    vector_table[i] = (i < ARRAY_SIZE(fault_handlers))
                          ? fault_handlers[i]
                          : (uint32_t)NVIC_default_handler;
  }

  SCB->VTOR = (uint32_t)vector_table;
  __DSB();
}

void NVIC_SetDefaultFaultHandler(NVIC_DEFAULT_FAULT_HANDLER_T handler) {
  int i;

  for (i = 1; i < ARRAY_SIZE(fault_handlers); i++) {
    if (vector_table[i] == (uint32_t)NVIC_default_handler) {
      vector_table[i] = (uint32_t)handler;
    }
  }
}

IRQn_Type NVIC_GetCurrentActiveIRQ(void) {
  IRQn_Type irq = (__get_IPSR() & IPSR_ISR_Msk) - NVIC_USER_IRQ_OFFSET;
  return irq;
}

#ifdef CORE_SLEEP_POWER_DOWN

void SRAM_TEXT_LOC NVIC_PowerDownSleep(uint32_t *buf, uint32_t cnt) {
  int i;
  uint32_t idx;
  __IO uint32_t *regs;

  idx = 0;
  for (i = 0; i < (NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET + 31) / 32; i++) {
    buf[idx++] = NVIC->ISER[i];
  }
#if (__CORTEX_M <= 4)
  regs = (__IO uint32_t *)&NVIC->IP[0];
#else
  regs = (__IO uint32_t *)&NVIC->IPR[0];
#endif
  for (i = 0; i < (NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET + 3) / 4; i++) {
    buf[idx++] = regs[i];
  }
  buf[idx++] = SCnSCB->ACTLR;
  buf[idx++] = SCB->ICSR;
  buf[idx++] = SCB->AIRCR;
  buf[idx++] = SCB->SCR;
  buf[idx++] = SCB->CCR;
#if (__CORTEX_M <= 4)
  regs = (__IO uint32_t *)&SCB->SHP[0];
#else
  regs = (__IO uint32_t *)&SCB->SHPR[0];
#endif
  buf[idx++] = regs[0];
  buf[idx++] = regs[1];
  buf[idx++] = regs[2];
  buf[idx++] = SCB->SHCSR;
  buf[idx++] = SysTick->CTRL;
  buf[idx++] = SysTick->LOAD;

  if (idx > cnt) {
    do {
      asm volatile("nop \n nop \n nop \n nop");
    } while (1);
  }
}

void SRAM_TEXT_LOC NVIC_PowerDownWakeup(uint32_t *buf, uint32_t cnt) {
  int i;
  uint32_t idx;
  __IO uint32_t *regs;

  idx = 0;
  for (i = 0; i < (NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET + 31) / 32; i++) {
    NVIC->ISER[i] = buf[idx++];
  }
#if (__CORTEX_M <= 4)
  regs = (__IO uint32_t *)&NVIC->IP[0];
#else
  regs = (__IO uint32_t *)&NVIC->IPR[0];
#endif
  for (i = 0; i < (NVIC_NUM_VECTORS - NVIC_USER_IRQ_OFFSET + 3) / 4; i++) {
    regs[i] = buf[idx++];
  }
  SCnSCB->ACTLR = buf[idx++];
  SCB->ICSR = buf[idx++];
  SCB->AIRCR = buf[idx++];
  SCB->SCR = buf[idx++];
  SCB->CCR = buf[idx++];
#if (__CORTEX_M <= 4)
  regs = (__IO uint32_t *)&SCB->SHP[0];
#else
  regs = (__IO uint32_t *)&SCB->SHPR[0];
#endif
  regs[0] = buf[idx++];
  regs[1] = buf[idx++];
  regs[2] = buf[idx++];
  SCB->SHCSR = buf[idx++];
  SysTick->CTRL = buf[idx++];
  SysTick->LOAD = buf[idx++];

  SCB->VTOR = (uint32_t)vector_table;
  SysTick->VAL = 0;
}

#endif

#endif
