/**************************************************************************//**
 * @file     best3003.h
 * @brief    CMSIS Core Peripheral Access Layer Header File for
 *           ARMCM33 Device (configured for ARMCM33 with FPU, with DSP extension)
 * @version  V5.3.1
 * @date     09. July 2018
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 Arm Limited. All rights reserved.
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


#ifndef __BEST3003_H__
#define __BEST3003_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
/* -------------------------  Interrupt Number Definition  ------------------------ */

typedef enum IRQn
{
/* -------------------  Cortex-M33 Processor Exceptions Numbers  ------------------ */
  NonMaskableInt_IRQn           = -14,      /*!<  2 Non Maskable Interrupt          */
  HardFault_IRQn                = -13,      /*!<  3 HardFault Interrupt             */
  MemoryManagement_IRQn         = -12,      /*!<  4 Memory Management Interrupt     */
  BusFault_IRQn                 = -11,      /*!<  5 Bus Fault Interrupt             */
  UsageFault_IRQn               = -10,      /*!<  6 Usage Fault Interrupt           */
  SVCall_IRQn                   =  -5,      /*!< 11 SV Call Interrupt               */
  DebugMonitor_IRQn             =  -4,      /*!< 12 Debug Monitor Interrupt         */
  PendSV_IRQn                   =  -2,      /*!< 14 Pend SV Interrupt               */
  SysTick_IRQn                  =  -1,      /*!< 15 System Tick Interrupt           */

/* ----------------------  Chip Specific Interrupt Numbers  ----------------------- */
  FPU_IRQn                      =   0,      /*!< FPU Interrupt                      */
  RESERVED01_IRQn               =   1,      /*!< Reserved Interrupt                 */
  RESERVED02_IRQn               =   2,      /*!< Reserved Interrupt                 */
  GPDMA_IRQn                    =   3,      /*!< General Purpose DMA Interrupt      */
  AUDMA_IRQn                    =   4,      /*!< Audio DMA Interrupt                */
  MCU_TIMER1_IRQ2n              =   5,      /*!< MCU Timer1 Interrupt1              */
  MCU_TIMER1_IRQ1n              =   6,      /*!< MCU Timer1 Interrupt2              */
  USB_IRQn                      =   7,      /*!< USB Interrupt                      */
  WAKEUP_IRQn                   =   8,      /*!< Wakeup Interrupt                   */
  GPIO_IRQn                     =   9,      /*!< GPIO Interrupt                     */
  WDT_IRQn                      =  10,      /*!< Watchdog Timer Interrupt           */
  RTC_IRQn                      =  11,      /*!< RTC Interrupt                      */
  MCU_TIMER00_IRQn              =  12,      /*!< MCU Timer0 Interrupt               */
  MCU_TIMER01_IRQn              =  13,      /*!< MCU Timer0 Interrupt               */
  I2C0_IRQn                     =  14,      /*!< I2C0 Interrupt                     */
  SPI0_IRQn                     =  15,      /*!< SPI0 Interrupt                     */
  FMDUMP0_IRQn                  =  16,      /*!< FM DUMP0 Interrupt                 */
  UART0_IRQn                    =  17,      /*!< UART0 Interrupt                    */
  UART1_IRQn                    =  18,      /*!< UART1 Interrupt                    */
  CODEC_IRQn                    =  19,      /*!< CODEC Interrupt                    */
  FMDUMP1_IRQn                  =  20,      /*!< FM DUMP1 Interrupt                 */
  I2S0_IRQn                     =  21,      /*!< I2S0 Interrupt                     */
  SPDIF0_IRQn                   =  22,      /*!< SPDIF0 Interrupt                   */
  SPI_ITN_IRQn                  =  23,      /*!< Internal SPI Interrupt             */
  RESERVED24_IRQn               =  24,      /*!< Reserved Interrupt                 */
  GPADC_IRQn                    =  25,      /*!< GPADC Interrupt                    */
  RESERVED26_IRQn               =  26,      /*!< Reserved Interrupt                 */
  USB_PIN_IRQn                  =  27,      /*!< PMU USB Interrupt                  */
  RESERVED28_IRQn               =  28,      /*!< Reserved Interrupt                 */
  RESERVED29_IRQn               =  29,      /*!< Reserved Interrupt                 */
  USB_CALIB_IRQn                =  30,      /*!< Reserved Interrupt                 */
  USB_SOF_IRQn                  =  31,      /*!< Reserved Interrupt                 */
  CHARGER_IRQn                  =  32,      /*!< Charger Interrupt                  */
  PWRKEY_IRQn                   =  33,      /*!< POWER KEY Interrupt                */
  EARDET_IRQn                   =  34,      /*!< EAR DET Interrupt                  */
  RESERVED35_IRQn               =  35,      /*!< Reserved Interrupt                 */
  RESERVED36_IRQn               =  36,      /*!< Reserved Interrupt                 */

  USER_IRQn_QTY,
  INVALID_IRQn                  = USER_IRQn_QTY,
} IRQn_Type;

#define TIMER00_IRQn            MCU_TIMER00_IRQn
#define TIMER01_IRQn            MCU_TIMER01_IRQn

#endif

/* ================================================================================ */
/* ================      Processor and Core Peripheral Section     ================ */
/* ================================================================================ */

/* --------  Configuration of Core Peripherals  ----------------------------------- */
#define __CM33_REV                0x0000U   /* Core revision r0p1 */
#define __SAUREGION_PRESENT       0U        /* SAU regions present */
#define __MPU_PRESENT             1U        /* MPU present */
#define __VTOR_PRESENT            1U        /* VTOR present */
#define __NVIC_PRIO_BITS          3U        /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig    0U        /* Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT             1U        /* FPU present */
#define __DSP_PRESENT             1U        /* DSP extension present */

#include "core_cm33.h"                      /* Processor and core peripherals */

#ifndef __ASSEMBLER__

#include "system_ARMCM.h"                  /* System Header                                     */

#endif

/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */

/* -------------------  Start of section using anonymous unions  ------------------ */
#if   defined (__CC_ARM)
  #pragma push
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

/* --------------------  End of section using anonymous unions  ------------------- */
#if   defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050))
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Not supported compiler type
#endif

#ifdef __cplusplus
}
#endif

#endif
