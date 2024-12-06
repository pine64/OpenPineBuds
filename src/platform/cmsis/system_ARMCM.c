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
#ifndef __ARM_ARCH_ISA_ARM

#include "system_ARMCM.h"
#include "cmsis.h"
#include "hal_location.h"

void BOOT_TEXT_FLASH_LOC SystemInit(void) {
#if (__FPU_USED == 1)
  SCB->CPACR |= ((3UL << 10 * 2) | /* set CP10 Full Access */
                 (3UL << 11 * 2)); /* set CP11 Full Access */
#endif

  SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#ifdef __ARM_ARCH_8M_MAIN__
  // Disable stack limit check on hard fault, NMI and reset
  // (The check will generate STKOF usage fault)
  SCB->CCR |= SCB_CCR_STKOFHFNMIGN_Msk;
#endif
#ifdef UNALIGNED_ACCESS
  SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;
#else
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif
#ifdef USAGE_FAULT
  SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
  NVIC_SetPriority(UsageFault_IRQn, IRQ_PRIORITY_REALTIME);
#else
  SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
#endif
#ifdef BUS_FAULT
  SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
  NVIC_SetPriority(BusFault_IRQn, IRQ_PRIORITY_REALTIME);
#else
  SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTENA_Msk;
#endif
#ifdef MEM_FAULT
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
  NVIC_SetPriority(MemoryManagement_IRQn, IRQ_PRIORITY_REALTIME);
#else
  SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
#endif

#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
  TZ_SAU_Setup();
#endif
}

#ifndef UNALIGNED_ACCESS

bool get_unaligned_access_status(void) {
  return !(SCB->CCR & SCB_CCR_UNALIGN_TRP_Msk);
}

bool config_unaligned_access(bool enable) {
  bool en;

  en = !(SCB->CCR & SCB_CCR_UNALIGN_TRP_Msk);

  if (enable) {
    SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;
  } else {
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
  }

  return en;
}

#endif

// -----------------------------------------------------------
// CPU ID
// -----------------------------------------------------------

uint32_t BOOT_TEXT_SRAM_DEF(get_cpu_id)(void) {
#ifdef CHIP_HAS_CP
#ifdef __ARM_ARCH_8M_MAIN__
  return (SCB->ID_ADR & 3);
#else  /*__ARM_ARCH_8M_MAIN__*/
  return (SCB->ADR & 3);
#endif /*__ARM_ARCH_8M_MAIN__*/
#else
  return 0;
#endif
}

#endif
