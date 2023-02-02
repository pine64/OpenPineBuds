/******************************************************************************
 * @file     system_ARMCA7.c
 * @brief    CMSIS Device System Source File for Arm Cortex-A7 Device Series
 * @version  V1.0.1
 * @date     13. February 2019
 *
 * @note
 *
 ******************************************************************************/
/*
 * Copyright (c) 2009-2019 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ca/system_ARMCA.h"
#include "ca/irq_ctrl.h"
#include "cmsis_nvic.h"
#include "hal_cmu.h"
#include "hal_location.h"

extern uint32_t __sram_text_data_start_load__[];
extern uint32_t __sram_text_data_end_load__[];
extern uint32_t __sram_text_data_start__[];
extern uint32_t __sram_bss_start__[];
extern uint32_t __sram_bss_end__[];
extern uint32_t __bss_start__[];
extern uint32_t __bss_end__[];
extern uint32_t __sync_flags_start[];
extern uint32_t __sync_flags_end[];
extern uint32_t __psramuhs_text_data_start_load__[];
extern uint32_t __psramuhs_text_data_end_load__[];
extern uint32_t __psramuhs_text_start[];

/*----------------------------------------------------------------------------
  System Initialization
 *----------------------------------------------------------------------------*/
void SystemInit(void) {
  uint32_t *dst, *src;

  if (__sram_text_data_start__ != __sram_text_data_start_load__) {
    for (dst = __sram_text_data_start__, src = __sram_text_data_start_load__;
         src < __sram_text_data_end_load__; dst++, src++) {
      *dst = *src;
    }
  }

  hal_cmu_dsp_setup();
  /*psramhus_test load region covers sram_bss, and it needs to be copyed first*/
#if defined(CHIP_HAS_PSRAMUHS) && defined(PSRAMUHS_ENABLE)
  for (dst = __psramuhs_text_start, src = __psramuhs_text_data_start_load__;
       src < __psramuhs_text_data_end_load__; dst++, src++) {
    *dst = *src;
  }
#endif

  for (dst = __sram_bss_start__; dst < __sram_bss_end__; dst++) {
    *dst = 0;
  }

#ifdef NOSTD
  for (dst = __bss_start__; dst < __bss_end__; dst++) {
    *dst = 0;
  }
#endif

  for (dst = __sync_flags_start; dst < __sync_flags_end; dst++) {
    *dst = 0;
  }

  /* do not use global variables because this function is called before
     reaching pre-main. RW section may be overwritten afterwards.          */

  // Init exception vectors
  GIC_InitVectors();

  // Invalidate entire Unified TLB
  __set_TLBIALL(0);

  // Invalidate entire branch predictor array
  __set_BPIALL(0);
  __DSB();
  __ISB();

  //  Invalidate instruction cache and flush branch target cache
  __set_ICIALLU(0);
  __DSB();
  __ISB();

  //  Invalidate data cache
  L1C_InvalidateDCacheAll();

#if ((__FPU_PRESENT == 1) && (__FPU_USED == 1))
  // Enable FPU
  __FPU_Enable();
#endif

  // Create Translation Table
  MMU_CreateTranslationTable();

  // Enable MMU
  MMU_Enable();

  // Enable Caches
  L1C_EnableCaches();
  L1C_EnableBTAC();

#if (__L2C_PRESENT == 1)
  // Enable GIC
  L2C_Enable();
#endif

  // IRQ Initialize
  IRQ_Initialize();
}

uint32_t BOOT_TEXT_SRAM_DEF(get_cpu_id)(void) { return __get_MPIDR() & 3; }
