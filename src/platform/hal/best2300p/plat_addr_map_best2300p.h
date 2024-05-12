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
#ifndef __PLAT_ADDR_MAP_BEST2300P_H__
#define __PLAT_ADDR_MAP_BEST2300P_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ROM_BASE                                0x00000000

#ifndef ROM_SIZE
#define ROM_SIZE                                0x0000C000
#endif

#define RAMRET_BASE                             0x200D8000
#define RAMXRET_BASE                            0x002D8000

#ifndef RAMRET_SIZE
#define RAMRET_SIZE                             0x00020000
#endif

#define RAM0_BASE                               0x20000000
#define RAMX0_BASE                              0x00200000
#define RAM1_BASE                               0x20020000
#define RAMX1_BASE                              0x00220000
#define RAM2_BASE                               0x20040000
#define RAMX2_BASE                              0x00240000
#define RAM3_BASE                               0x20060000
#define RAMX3_BASE                              0x00260000
#define RAM4_BASE                               0x20080000
#define RAMX4_BASE                              0x00280000
#define RAM5_BASE                               0x200A0000
#define RAMX5_BASE                              0x002A0000
#define RAM6_BASE                               0x200C0000
#define RAMX6_BASE                              0x002C0000
#define RAM_BASE                                RAM0_BASE
#define RAMX_BASE                               RAMX0_BASE

#ifdef CHIP_HAS_CP
#define RAMCP_TOP                               (RAMRET_BASE + RAMRET_SIZE - 0x20)

#ifndef RAMCP_SIZE
#define RAMCP_SIZE                              (RAMRET_SIZE - 0x20)
#endif
#define RAMCP_BASE                              (RAMCP_TOP - RAMCP_SIZE)

#ifndef RAMCPX_SIZE
#define RAMCPX_SIZE                             (RAMXRET_BASE - RAMX6_BASE)
#endif
#ifndef RAMCPX_BASE
#define RAMCPX_BASE                             (RAM_TO_RAMX(RAMCP_BASE) - RAMCPX_SIZE)
#endif
#endif

#ifndef RAM_SIZE
#ifdef CHIP_HAS_CP
#define RAM_SIZE                                (RAMCPX_BASE - RAMX_BASE) // 0X000C0000
#else
#ifdef LARGE_RAM
#define RAM_SIZE                                (RAMRET_BASE + RAMRET_SIZE - RAM_BASE) // 0x000F8000
#else
#define RAM_SIZE                                (RAMRET_BASE - RAM_BASE) // 0x000D8000
#endif
#endif /* !CHIP_HAS_CP */
#endif /* !RAM_SIZE */

#if defined(ROM_BUILD) && defined(CORE_SLEEP_POWER_DOWN)
#undef RAM_BASE
#undef RAMX_BASE
#undef RAM_SIZE
#define RAM_BASE                                RAMRET_BASE
#define RAMX_BASE                               RAMXRET_BASE
#define RAM_SIZE                                RAMRET_SIZE
#endif

#define FLASH_BASE                              0x3C000000
#define FLASH_NC_BASE                           0x38000000
#define FLASHX_BASE                             0x0C000000
#define FLASHX_NC_BASE                          0x08000000

#define ICACHE_CTRL_BASE                        0x07FFE000
#define ICACHECP_CTRL_BASE                      0x07FFA000
/* No data cache */

#define CMU_BASE                                0x40000000
#define MCU_WDT_BASE                            0x40001000
#define MCU_TIMER0_BASE                         0x40002000
#define MCU_TIMER1_BASE                         0x40003000
#define MCU_TIMER2_BASE                         0x40004000
#define I2C0_BASE                               0x40005000
#define I2C1_BASE                               0x40006000
#define SPI_BASE                                0x40007000
#define SPILCD_BASE                             0x40008000
#define ISPI_BASE                               0x40009000
#define SPIPHY_BASE                             0x4000A000
#define UART0_BASE                              0x4000B000
#define UART1_BASE                              0x4000C000
#define UART2_BASE                              0x4000D000
#define BTPCM_BASE                              0x4000E000
#define I2S0_BASE                               0x4000F000
#define SPDIF0_BASE                             0x40010000
#define I2S1_BASE                               0x40011000
#define SEC_ENG_BASE                            0x40020000

#define AON_CMU_BASE                            0x40080000
#define AON_GPIO_BASE                           0x40081000
#define AON_WDT_BASE                            0x40082000
#define AON_PWM_BASE                            0x40083000
#define AON_TIMER_BASE                          0x40084000
#define AON_PSC_BASE                            0x40085000
#define AON_IOMUX_BASE                          0x40086000

#define SDMMC_BASE                              0x40110000
#define AUDMA_BASE                              0x40120000
#define GPDMA_BASE                              0x40130000
#define FLASH_CTRL_BASE                         0x40140000
#define BTDUMP_BASE                             0x40150000
#define I2C_SLAVE_BASE                          0x40160000
#define SENSOR_ENG_BASE                         0x40170000
#define USB_BASE                                0x40180000
#define SEDMA_BASE                              0x401D0000

#define CODEC_BASE                              0x40300000

#define BT_SUBSYS_BASE                          0xA0000000
#define BT_RAM_BASE                             0xC0000000
#define BT_RAM_SIZE                             0x00008000
#define BT_EXCH_MEM_BASE                        0xD0210000
#define BT_EXCH_MEM_SIZE                        0x00008000
#define BT_UART_BASE                            0xD0300000
#define BT_CMU_BASE                             0xD0330000

#define IOMUX_BASE                              AON_IOMUX_BASE
#define GPIO_BASE                               AON_GPIO_BASE
#define PWM_BASE                                AON_PWM_BASE
#ifdef CORE_SLEEP_POWER_DOWN
#define TIMER0_BASE                             AON_TIMER_BASE
#else
#define TIMER0_BASE                             MCU_TIMER0_BASE
#endif
#define TIMER1_BASE                             MCU_TIMER1_BASE
#define WDT_BASE                                AON_WDT_BASE

/* For linker scripts */

#define VECTOR_SECTION_SIZE                     320
#define REBOOT_PARAM_SECTION_SIZE               64
#define ROM_BUILD_INFO_SECTION_SIZE             40
#define ROM_EXPORT_FN_SECTION_SIZE              128
#define BT_INTESYS_MEM_OFFSET                   0x00000000

/* For boot struct version */
#ifndef SECURE_BOOT_VER
#define SECURE_BOOT_VER                         2
#endif

#ifdef __cplusplus
}
#endif

#endif
