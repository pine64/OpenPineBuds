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
#include "norflash_drv.h"
#include "cmsis.h"
#include "hal_cache.h"
#include "hal_norflash.h"
#include "hal_norflaship.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "norflash_gd25q32c.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "tool_msg.h"

#ifdef PROGRAMMER
#include "task_schedule.h"
#else
#define TASK_SCHEDULE true
#endif

#if (CHIP_FLASH_CTRL_VER >= 2) && !defined(ROM_BUILD) &&                       \
    !defined(PROGRAMMER) &&                                                    \
    !(defined(CHIP_BEST2001) &&                                                \
      (defined(PSRAMUHS_ENABLE) || defined(PSRAM_ENABLE)))
#define FLASH_BURST_WRAP
#endif

#define SAMP_DELAY_PRIO_FALLING_EDGE

#define NORFLASH_UNIQUE_ID_LEN 18

#define NORFLASH_MAX_DIV 0xFF

#define NORFLASH_DEFAULT_MAX_SPEED (104 * 1000 * 1000)

#define NORFLASH_DIV1_MAX_SPEED (99 * 1000 * 1000)

#define NORFLASH_SPEED_RATIO_DENOMINATOR 8

#define NORFLASH_PUYA_ID_PREFIX 0x85

#define NORFLASH_XTS_ID_PREFIX 0x0B

#define XTS_UNIQUE_ID_LEN 16
#define XTS_UNIQUE_ID_CMD 0x5A
#define XTS_UNIQUE_ID_PARAM 0x00019400

// GigaDevice
extern const struct NORFLASH_CFG_T gd25lq64c_cfg;
extern const struct NORFLASH_CFG_T gd25lq32c_cfg;
extern const struct NORFLASH_CFG_T gd25lq16c_cfg;
extern const struct NORFLASH_CFG_T gd25lq80c_cfg;
extern const struct NORFLASH_CFG_T gd25q32c_cfg;
extern const struct NORFLASH_CFG_T gd25q80c_cfg;
extern const struct NORFLASH_CFG_T gd25d40c_cfg;
extern const struct NORFLASH_CFG_T gd25d20c_cfg;

// Puya
extern const struct NORFLASH_CFG_T p25q128l_cfg;
extern const struct NORFLASH_CFG_T p25q64l_cfg;
extern const struct NORFLASH_CFG_T p25q32l_cfg;
extern const struct NORFLASH_CFG_T p25q16l_cfg;
extern const struct NORFLASH_CFG_T p25q80h_cfg;
extern const struct NORFLASH_CFG_T p25q21h_cfg;
extern const struct NORFLASH_CFG_T p25q40h_cfg;

// XTS
extern const struct NORFLASH_CFG_T xt25q08b_cfg;

// EON
extern const struct NORFLASH_CFG_T en25s80b_cfg;

// XMC
extern const struct NORFLASH_CFG_T xm25qh16c_cfg;
extern const struct NORFLASH_CFG_T xm25qh80b_cfg;

static const struct NORFLASH_CFG_T *const flash_list[] = {
// ----------------------
// GigaDevice
// ----------------------
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25LQ64C__)
    &gd25lq64c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25LQ32C__)
    &gd25lq32c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25LQ6C__)
    &gd25lq16c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25LQ80C__)
    &gd25lq80c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25Q32C__)
    &gd25q32c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25Q80C__)
    &gd25q80c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25D40C__)
    &gd25d40c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_GD25D20C__)
    &gd25d20c_cfg,
#endif

// ----------------------
// Puya
// ----------------------
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q128L__)
    &p25q128l_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q64L__)
    &p25q64l_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q32L__)
    &p25q32l_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q16L__)
    &p25q16l_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q80H__)
    &p25q80h_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q21H__)
    &p25q21h_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_P25Q40H__)
    &p25q40h_cfg,
#endif

// ----------------------
// Xinxin
// ----------------------
#if defined(__NORFLASH_XM25QH16C__)
    &xm25qh16c_cfg,
#endif
#if defined(__NORFLASH_ALL__) || defined(__NORFLASH_XM25QH80B__)
    &xm25qh80b_cfg,
#endif

// ----------------------
// XTS
// ----------------------
#if defined(__NORFLASH_XT25Q08B__)
    &xt25q08b_cfg,
#endif

// ----------------------
// EON
// ----------------------
#if defined(__NORFLASH_EN25S80B__)
    &en25s80b_cfg,
#endif

};

// Sample delay will be larger if:
// 1) flash speed is higher (major impact)
// 2) vcore voltage is lower (secondary major impact)
// 3) flash voltage is lower (minor impact)

// Sample delay unit:
// V1: 1/2 source_clk cycle when <= 2, 1 source_clk cycle when >= 2
// V2: 1/2 source_clk cycle when <= 4, 1 source_clk cycle when >= 4

// Flash clock low to output valid delay:
// T_clqv: 7 ns

// Flash IO latency:
// BEST1000/3001/1400: 4 ns
// BEST2000: 5 ns
// BEST2300: 2 ns

// Flash output time: T_clqv + T_io_latency
// Falling edge sample time: one spi_clk cycle (should > flash output time)

#ifdef CHIP_BEST2300
#define FALLING_EDGE_SAMPLE_ADJ_FREQ (110 * 1000 * 1000) // about 9 ns
#else
#define FALLING_EDGE_SAMPLE_ADJ_FREQ (77 * 1000 * 1000) // about 13 ns
#endif

#if (CHIP_FLASH_CTRL_VER <= 1)
#define SAM_EDGE_FALLING (1 << 4)
#define SAM_NEG_PHASE (1 << 5)
#define SAMDLY_MASK (0xF << 0)

#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
#define DIV2_SAMP_DELAY_FALLING_EDGE_IDX 1
#define DIVN_SAMP_DELAY_FALLING_EDGE_IDX 2
// Sample delays: 1, 1.5, 2, 3
static const uint8_t samdly_list_divn[] = {
    1,
    SAM_NEG_PHASE | SAM_EDGE_FALLING | 2,
    2,
    3,
};
#else
// Sample delays: 0, 0.5, 1, 1.5, 2, 3, 4, 5, 6, 7
static const uint8_t samdly_list_divn[] = {/*0,*/ SAM_EDGE_FALLING | 1,
                                           1,
                                           SAM_NEG_PHASE | SAM_EDGE_FALLING | 2,
                                           2,
                                           3,
                                           4,
                                           /*5, 6, 7,*/};
#endif
#else
#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
#ifdef CHIP_BEST1402
#define DIV1_SAMP_DELAY_FALLING_EDGE_IDX 2
#else
#define DIV1_SAMP_DELAY_FALLING_EDGE_IDX 1
#endif
static const uint8_t samdly_list_div1[] = {
    0,
    1,
    2,
    3,
};
#define DIV2_SAMP_DELAY_FALLING_EDGE_IDX 1
#define DIVN_SAMP_DELAY_FALLING_EDGE_IDX 3
static const uint8_t samdly_list_divn[] = {
    2, 3, 4, 5, 6, 7, 8, 9,
};
#else
static const uint8_t samdly_list_divn[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
};
#endif
#endif

static uint8_t flash_idx;
static uint8_t div_read;
static uint8_t div_std_read;
static uint8_t div_others;
static uint32_t norflash_op_mode = 0;
static bool falling_edge_adj;
static uint8_t sample_delay_index;

#ifdef FLASH_SUSPEND
static uint32_t check_irq[(USER_IRQn_QTY + 31) / 32];
static bool specific_irq_check;
#endif

#ifdef FLASH_CALIB_DEBUG
static uint32_t norflash_source_clk;
static uint32_t norflash_speed;
static uint8_t calib_matched_idx[DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_matched_cnt[DRV_NORFLASH_CALIB_QTY];
static uint8_t calib_final_idx[DRV_NORFLASH_CALIB_QTY];
#endif

static void norflash_delay(uint32_t us) {
#ifdef CHIP_BEST1000
  hal_sys_timer_delay(US_TO_TICKS(us));
#else
  hal_sys_timer_delay_us(us);
#endif
}

#ifdef FLASH_HPM
static int norflash_set_hpm(uint8_t on) {
  if (on) {
    norflaship_cmd_addr(GD25Q32C_CMD_HIGH_PERFORMANCE, 0);
  } else {
    norflaship_cmd_addr(GD25Q32C_CMD_RELEASE_FROM_DP, 0);
  }
  norflaship_busy_wait();

  return 0;
}
#endif

#ifdef FLASH_SUSPEND
static int norflash_suspend(void) {
  norflaship_clear_fifos();
  norflaship_ext_tx_cmd(GD25Q32C_CMD_PROGRAM_ERASE_SUSPEND, 0);
  // Suspend time: 20~30 us
  norflash_delay(40);
  return 0;
}

static int norflash_resume(void) {
  norflaship_clear_fifos();
  norflaship_ext_tx_cmd(GD25Q32C_CMD_PROGRAM_ERASE_RESUME, 0);
  if (flash_list[flash_idx]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
    // PUYA flash requires the mean interval of resume to suspend >= 250us
    norflash_delay(250);
  } else {
    // At least resume the work for 100 us to avoid always staying in suspended
    // state
    norflash_delay(100);
  }
  return 0;
}
#endif

uint8_t norflash_read_status_s0_s7(void) {
  uint8_t val;
  norflash_read_reg(GD25Q32C_CMD_READ_STATUS_S0_S7, &val, 1);
  return val;
}

uint8_t norflash_read_status_s8_s15(void) {
  uint8_t val;
  norflash_read_reg(GD25Q32C_CMD_READ_STATUS_S8_S15, &val, 1);
  return val;
}

static int norflash_status_WEL(void) {
  uint32_t status;
  status = norflash_read_status_s0_s7();
  return !!(status & GD25Q32C_WEL_BIT_MASK);
}

static int norflash_status_WIP(void) {
  uint32_t status;
  status = norflash_read_status_s0_s7();
  return !!(status & GD25Q32C_WIP_BIT_MASK);
}

void norflash_status_WEL_0_wait(void) {
  while (norflash_status_WEL() == 0 && TASK_SCHEDULE)
    ;
}

#ifdef FLASH_SUSPEND
int norflash_suspend_check_irq(uint32_t irq_num) {
  uint32_t idx;
  uint32_t offset;

  idx = irq_num / 32;
  offset = irq_num % 32;

  if (idx >= ARRAY_SIZE(check_irq)) {
    return 1;
  }

  check_irq[idx] |= (1 << offset);
  specific_irq_check = true;

  return 0;
}

static int norflash_system_active(void) {
  if (specific_irq_check) {
    return hal_sleep_specific_irq_pending(check_irq, ARRAY_SIZE(check_irq));
  } else {
    return hal_sleep_irq_pending();
  }
}
#endif

enum HAL_NORFLASH_RET_T norflash_status_WIP_1_wait(int suspend) {
  while (norflash_status_WIP() && TASK_SCHEDULE) {
#ifdef FLASH_SUSPEND
    if (suspend && norflash_system_active()) {
      norflash_suspend();
      return HAL_NORFLASH_SUSPENDED;
    }
#endif
  }

  return HAL_NORFLASH_OK;
}

#ifdef FLASH_BURST_WRAP
static int norflash_set_burst_wrap(uint32_t len) {
  uint8_t val;

  if (len == 64) {
    val = (1 << 6) | (1 << 5);
  } else if (len == 32) {
    val = (1 << 6);
  } else if (len == 16) {
    val = (1 << 5);
  } else if (len == 8) {
    val = 0;
  } else if (len == 0) {
    // Disable wrap around
    val = (1 << 4);
  } else {
    return 1;
  }

  norflaship_clear_txfifo();
  norflaship_write_txfifo(&val, 1);
  norflaship_cmd_addr(GD25Q32C_CMD_SET_BURST_WRAP, 0);
  norflash_status_WIP_1_wait(0);

  return 0;
}
#endif

static int norflash_set_continuous_read(uint8_t on) {
  uint8_t cmd;

  if (on) {
    norflaship_continuous_read_on();

    norflaship_continuous_read_mode_bit(flash_list[flash_idx]->crm_en_bits);

    // Continuous Read Mode takes effect after the first read
  } else {
    norflaship_continuous_read_off();

    norflaship_continuous_read_mode_bit(flash_list[flash_idx]->crm_dis_bits);

    if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
      cmd = GD25Q32C_CMD_FAST_QUAD_IO_READ;
    } else {
      norflaship_quad_mode(0);

      if (norflash_op_mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
        cmd = GD25Q32C_CMD_FAST_DUAL_IO_READ;
      } else {
        norflaship_dual_mode(0);

        // Command 0x03 and address 0xFFFFFE will make M4=1 (mode bit 4)
        // at both qual and dual continuous read modes, which will disable it.
        cmd = GD25Q32C_CMD_STANDARD_READ;
      }
    }

    norflaship_clear_rxfifo();
    norflaship_busy_wait();
    norflaship_blksize(2);
    if (cmd == GD25Q32C_CMD_STANDARD_READ) {
      if (div_std_read) {
        norflaship_div(div_std_read);
      }
    }
    norflaship_cmd_addr(cmd, 0xFFFFFE);
    // Now Continuous Read Mode has been disabled
    if (cmd == GD25Q32C_CMD_STANDARD_READ) {
      if (div_others) {
        norflaship_div(div_others);
      }
    }
    norflaship_clear_rxfifo();
  }

  norflaship_busy_wait();

  return 0;
}

static int norflash_set_quad(uint8_t on) {
  if (flash_list[flash_idx]->write_status == NULL) {
    return -1;
  }

  if (on) {
    flash_list[flash_idx]->write_status(DRV_NORFLASH_W_STATUS_QE, 1);
  } else {
    flash_list[flash_idx]->write_status(DRV_NORFLASH_W_STATUS_QE, 0);
  }
  return 0;
}

static int norflash_set_quad_io_mode(uint8_t on) {
  norflash_set_quad(on);
  if (on) {
    norflaship_quad_mode(1);
  } else {
    norflaship_quad_mode(0);
  }
  norflaship_busy_wait();
  return 0;
}

static int norflash_set_quad_output_mode(uint8_t on) {
  norflash_set_quad(on);
  if (on) {
    norflaship_rdcmd(GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ);
  } else {
    norflaship_rdcmd(GD25Q32C_CMD_STANDARD_READ);
  }
  norflaship_busy_wait();
  return 0;
}

static uint8_t norflash_set_dual_io_mode(uint8_t on) {
  if (on) {
    norflaship_dual_mode(1);
  } else {
    norflaship_dual_mode(0);
  }
  norflaship_busy_wait();

  return 0;
}

static uint8_t norflash_set_dual_output_mode(uint8_t on) {
  if (on) {
    norflaship_rdcmd(GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ);
  } else {
    norflaship_rdcmd(GD25Q32C_CMD_STANDARD_READ);
  }
  norflaship_busy_wait();
  return 0;
}

static uint8_t norflash_set_fast_mode(uint8_t on) {
  if (on) {
    norflaship_rdcmd(GD25Q32C_CMD_STANDARD_FAST_READ);
  } else {
    norflaship_rdcmd(GD25Q32C_CMD_STANDARD_READ);
  }
  norflaship_busy_wait();
  return 0;
}

static uint8_t norflash_set_stand_mode(void) {
  norflaship_rdcmd(GD25Q32C_CMD_STANDARD_READ);
  norflaship_busy_wait();
  return 0;
}

uint32_t norflash_get_supported_mode(void) {
  return flash_list[flash_idx]->mode;
}

uint32_t norflash_get_current_mode(void) { return norflash_op_mode; }

union DRV_NORFLASH_SEC_REG_CFG_T norflash_get_security_register_config(void) {
  return flash_list[flash_idx]->sec_reg_cfg;
}

uint32_t norflash_get_block_protect_mask(void) {
  return flash_list[flash_idx]->block_protect_mask;
}

void norflash_reset(void) {
  norflaship_clear_fifos();

  // Disable continuous read mode
  norflaship_continuous_read_off();
  norflaship_busy_wait();
  norflaship_blksize(2);
  // Command 0x03 and address 0xFFFFFE will make M4=1 (mode bit 4)
  // at both qual and dual continuous read modes, which will disable it.
  norflaship_cmd_addr(GD25Q32C_CMD_STANDARD_READ, 0xFFFFFE);

  // Release from deep power-down
  norflaship_cmd_addr(GD25Q32C_CMD_RELEASE_FROM_DP, 0);
  // Wait 20us for flash to finish
  norflash_delay(40);

  // Software reset
  norflaship_ext_tx_cmd(GD25Q32C_CMD_ENABLE_RESET, 0);
  norflaship_ext_tx_cmd(GD25Q32C_CMD_RESET, 0);
  // Reset recovery time: 20~30 us
  norflash_delay(50);

  norflaship_cmd_done();

  // Reset cfg
  flash_idx = 0;
  div_read = 0;
  div_std_read = 0;
  div_others = 0;
  norflash_op_mode = 0;
  falling_edge_adj = false;
}

int norflash_get_size(uint32_t *total_size, uint32_t *block_size,
                      uint32_t *sector_size, uint32_t *page_size) {
  if (total_size) {
    *total_size = flash_list[flash_idx]->total_size;
  }
  if (block_size) {
    *block_size = flash_list[flash_idx]->block_size;
  }
  if (sector_size) {
    *sector_size = flash_list[flash_idx]->sector_size;
  }
  if (page_size) {
    *page_size = flash_list[flash_idx]->page_size;
  }

  return 0;
}

static void norflash_set_cfg_div(void) {
  if (div_others) {
    norflaship_div(div_others);
  }
}

static void norflash_set_read_div(void) {
  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
    if (div_std_read) {
      norflaship_div(div_std_read);
    }
  } else {
    if (div_read) {
      norflaship_div(div_read);
    }
  }
}

int norflash_set_mode(uint32_t op) {
  uint32_t read_mode = 0;
  uint32_t ext_mode = 0;
  uint32_t program_mode = 0;
  uint32_t self_mode;
  uint32_t mode;

  self_mode = norflash_get_supported_mode();
  mode = (self_mode & op);

  if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
    read_mode = HAL_NORFLASH_OP_MODE_QUAD_IO;
  } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
    read_mode = HAL_NORFLASH_OP_MODE_QUAD_OUTPUT;
  } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
    read_mode = HAL_NORFLASH_OP_MODE_DUAL_IO;
  } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
    read_mode = HAL_NORFLASH_OP_MODE_DUAL_OUTPUT;
  } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
    read_mode = HAL_NORFLASH_OP_MODE_FAST_SPI;
  } else if (mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
    read_mode = HAL_NORFLASH_OP_MODE_STAND_SPI;
  } else {
    // Op error
    return 1;
  }

  if (mode & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
    program_mode = HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM;
  } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
    program_mode = HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM;
  } else if (mode & HAL_NORFLASH_OP_MODE_PAGE_PROGRAM) {
    program_mode = HAL_NORFLASH_OP_MODE_PAGE_PROGRAM;
  } else {
    // Op error
    return 1;
  }

#ifdef FLASH_BURST_WRAP
  if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
    ext_mode |= HAL_NORFLASH_OP_MODE_READ_WRAP;
  }
#endif

#ifdef FLASH_HPM
  if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
    ext_mode |= HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE;
  }
#endif

  if (mode & (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) {
    if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
      ext_mode |= HAL_NORFLASH_OP_MODE_CONTINUOUS_READ;
    }
  }

  mode = (read_mode | ext_mode | program_mode);

  if (norflash_op_mode != mode) {
    norflash_op_mode = mode;

    // Continuous read off if flash supported
    if ((self_mode &
         (HAL_NORFLASH_OP_MODE_QUAD_IO | HAL_NORFLASH_OP_MODE_DUAL_IO)) &&
        (self_mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ)) {
      norflash_set_continuous_read(0);
    }

    norflaship_quad_mode(0);

    norflaship_dual_mode(0);

#ifdef FLASH_HPM
    if (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
      // High performance mode on
      norflash_set_hpm(1);
    }
#endif

    if (mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
      // Quad io mode
      norflash_set_quad_io_mode(1);
    } else if (mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
      // Quad output mode
      norflash_set_quad_output_mode(1);
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
      // Dual io mode
      norflash_set_dual_io_mode(1);
    } else if (mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
      // Dual output mode
      norflash_set_dual_output_mode(1);
    } else if (mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
      // Fast mode
      norflash_set_fast_mode(1);
    } else if (mode & HAL_NORFLASH_OP_MODE_STAND_SPI) {
      // Standard spi mode
      norflash_set_stand_mode();
    }

#ifdef FLASH_BURST_WRAP
    if (mode & HAL_NORFLASH_OP_MODE_READ_WRAP) {
      norflash_set_burst_wrap(32);
      hal_cache_wrap_enable(HAL_CACHE_ID_I_CACHE);
      hal_cache_wrap_enable(HAL_CACHE_ID_D_CACHE);
    }
#endif

#ifdef FLASH_HPM
    if ((self_mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) &&
        (mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) == 0) {
      // High performance mode off
      norflash_set_hpm(0);
    }
#endif

    if (mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
      // Continuous read on
      norflash_set_continuous_read(1);
    }

    norflaship_cmd_done();
  }

  norflash_set_read_div();

  return 0;
}

int norflash_pre_operation(void) {
  norflash_set_cfg_div();

  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
    norflash_set_continuous_read(0);
  }

  return 0;
}

int norflash_post_operation(void) {
  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_CONTINUOUS_READ) {
    norflash_set_continuous_read(1);
  }

  norflaship_cmd_done();

  norflash_set_read_div();

  return 0;
}

int norflash_read_reg(uint8_t cmd, uint8_t *val, uint32_t len) {
  int i;

  norflaship_clear_fifos();
#ifdef TRY_EMBEDDED_CMD
  if ((cmd == GD25Q32C_CMD_READ_STATUS_S0_S7 ||
       cmd == GD25Q32C_CMD_READ_STATUS_S8_S15) &&
      len == 1) {
    norflaship_cmd_addr(cmd, 0);
  } else {
    if (cmd == GD25Q32C_CMD_ID) {
      norflaship_blksize(len);
      norflaship_cmd_addr(cmd, 0);
    } else {
      norflaship_ext_rx_cmd(cmd, 0, len);
    }
  }
#else
  norflaship_ext_rx_cmd(cmd, 0, len);
#endif
  norflaship_rxfifo_count_wait(len);
  for (i = 0; i < len; i++) {
    val[i] = norflaship_read_rxfifo();
  }

  return 0;
}

int norflash_read_reg_ex(uint8_t cmd, uint8_t *param, uint32_t param_len,
                         uint8_t *val, uint32_t len) {
  int i;

  norflaship_clear_fifos();
  if (param && param_len > 0) {
    norflaship_write_txfifo(param, param_len);
  } else {
    param_len = 0;
  }
  norflaship_ext_rx_cmd(cmd, param_len, len);
  for (i = 0; i < len; i++) {
    norflaship_rxfifo_empty_wait();
    val[i] = norflaship_read_rxfifo();
  }

  return 0;
}

int norflash_write_reg(uint8_t cmd, const uint8_t *val, uint32_t len) {
  norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, 0);
  norflash_status_WIP_1_wait(0);
  norflash_status_WEL_0_wait();

  norflaship_clear_txfifo();
  norflaship_write_txfifo(val, len);

#ifdef TRY_EMBEDDED_CMD
  if (cmd == GD25Q32C_CMD_WRITE_STATUS_S0_S7) {
    norflaship_cmd_addr(GD25Q32C_CMD_WRITE_STATUS_S0_S7, 0);
  } else {
    norflaship_ext_tx_cmd(cmd, len);
  }
#else
  norflaship_ext_tx_cmd(cmd, len);
#endif

  norflash_status_WIP_1_wait(0);

  return 0;
}

int norflash_get_id(uint8_t *value, uint32_t len) {
  norflash_pre_operation();

  if (len > NORFLASH_ID_LEN) {
    len = NORFLASH_ID_LEN;
  }
  norflash_read_reg(GD25Q32C_CMD_ID, value, len);

  norflash_post_operation();

  return 0;
}

int norflash_get_unique_id(uint8_t *value, uint32_t len) {
  uint32_t param;
  uint8_t cmd;

  norflash_pre_operation();

  if (flash_list[flash_idx]->id[0] == NORFLASH_XTS_ID_PREFIX) {
    if (len > XTS_UNIQUE_ID_LEN) {
      len = XTS_UNIQUE_ID_LEN;
    }
    param = XTS_UNIQUE_ID_PARAM;
    cmd = XTS_UNIQUE_ID_CMD;
  } else {
    if (len > NORFLASH_UNIQUE_ID_LEN) {
      len = NORFLASH_UNIQUE_ID_LEN;
    }
    param = 0;
    cmd = GD25Q32C_CMD_UNIQUE_ID;
  }
  norflash_read_reg_ex(cmd, (uint8_t *)&param, sizeof(param), value, len);

  norflash_post_operation();

  return 0;
}

static void norflash_get_samdly_list(uint32_t div,
                                     const uint8_t **samdly_list_p,
                                     uint32_t *size_p) {
  const uint8_t *samdly_list = NULL;
  uint32_t size = 0;

#if (CHIP_FLASH_CTRL_VER <= 1)
  if (div >= 2) {
    samdly_list = samdly_list_divn;
    size = ARRAY_SIZE(samdly_list_divn);
  }
#else
  if (div >= 1) {
    if (div == 1) {
      samdly_list = samdly_list_div1;
      size = ARRAY_SIZE(samdly_list_div1);
    } else {
      samdly_list = samdly_list_divn;
      size = ARRAY_SIZE(samdly_list_divn);
    }
  }
#endif

  if (samdly_list_p) {
    *samdly_list_p = samdly_list;
  }
  if (size_p) {
    *size_p = size;
  }
}

void norflash_set_sample_delay_index(uint32_t index) {
  const uint8_t *samdly_list;
  uint32_t size;
  uint32_t div;

  sample_delay_index = index;

  div = norflaship_get_div();

  norflash_get_samdly_list(div, &samdly_list, &size);

  if (index < size) {
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflaship_pos_neg(samdly_list[index] & SAM_EDGE_FALLING);
    norflaship_neg_phase(samdly_list[index] & SAM_NEG_PHASE);
    norflaship_samdly(samdly_list[index] & SAMDLY_MASK);
#else
    norflaship_samdly(samdly_list[index]);
#endif
  }
}

uint32_t norflash_get_sample_delay_index(void) { return sample_delay_index; }

static bool norflash_calib_flash_id_valid(void) {
  uint8_t id[HAL_NORFLASH_DEVICE_ID_LEN];
  const uint8_t *cmp_id;

  norflash_get_id(id, sizeof(id));
  cmp_id = flash_list[flash_idx]->id;

  if (id[0] == cmp_id[0] && id[1] == cmp_id[1] && id[2] == cmp_id[2]) {
    return true;
  }
  return false;
}

static bool norflash_calib_magic_word_valid(void) {
  uint32_t magic;

#if (CHIP_FLASH_CTRL_VER <= 1)
  norflash_read(FLASH_NC_BASE, NULL, 1);
#endif
  norflaship_clear_rxfifo();
#if (FLASH_NC_BASE == FLASH_BASE)
  hal_cache_invalidate(HAL_CACHE_ID_D_CACHE, FLASH_BASE, sizeof(magic));
  magic = *(volatile uint32_t *)FLASH_BASE;
#else
  magic = *(volatile uint32_t *)FLASH_NC_BASE;
#endif

  if (magic == BOOT_MAGIC_NUMBER) {
    return true;
  }
  return false;
}

int norflash_sample_delay_calib(enum DRV_NORFLASH_CALIB_T type) {
  int i;
  uint32_t matched_cnt = 0;
  uint32_t matched_idx = 0;
  uint32_t div;
  uint32_t size;
  bool valid;

  if (type >= DRV_NORFLASH_CALIB_QTY) {
    return 1;
  }
#if defined(ROM_BUILD) || defined(PROGRAMMER)
  if (type != DRV_NORFLASH_CALIB_FLASH_ID) {
    return 0;
  }
#endif

  div = norflaship_get_div();

  if (div == 0) {
    return -1;
#if (CHIP_FLASH_CTRL_VER <= 1)
  } else if (div == 1) {
    return -2;
#endif
  }

  norflash_get_samdly_list(div, NULL, &size);

  for (i = 0; i < size; i++) {
    norflaship_busy_wait();

    norflash_set_sample_delay_index(i);

    if (type == DRV_NORFLASH_CALIB_FLASH_ID) {
      valid = norflash_calib_flash_id_valid();
    } else {
      valid = norflash_calib_magic_word_valid();
    }

    if (valid) {
      if (matched_cnt == 0) {
        matched_idx = i;
      }
      matched_cnt++;
    } else if (matched_cnt) {
      break;
    }
  }

#ifdef FLASH_CALIB_DEBUG
  calib_matched_idx[type] = matched_idx;
  calib_matched_cnt[type] = matched_cnt;
#endif

  if (matched_cnt) {
#ifdef SAMP_DELAY_PRIO_FALLING_EDGE
    if (matched_cnt == 2) {
      uint32_t falling_edge_idx;

      if (0) {
#if (CHIP_FLASH_CTRL_VER >= 2)
      } else if (div == 1) {
        falling_edge_idx = DIV1_SAMP_DELAY_FALLING_EDGE_IDX;
#endif
      } else if (div == 2) {
        falling_edge_idx = DIV2_SAMP_DELAY_FALLING_EDGE_IDX;
        if (falling_edge_adj) {
          falling_edge_idx++;
        }
      } else {
        falling_edge_idx = DIVN_SAMP_DELAY_FALLING_EDGE_IDX;
      }
      if (matched_idx <= falling_edge_idx &&
          falling_edge_idx < matched_idx + matched_cnt) {
        matched_idx = falling_edge_idx;
        matched_cnt = 1;
      }
    }
#endif
    matched_idx += matched_cnt / 2;
    norflash_set_sample_delay_index(matched_idx);

#ifdef FLASH_CALIB_DEBUG
    calib_final_idx[type] = matched_idx;
#endif

    return 0;
  }

#ifdef FLASH_CALIB_DEBUG
  calib_final_idx[type] = -1;
#endif

  return 1;
}

void norflash_show_calib_result(void) {
#ifdef FLASH_CALIB_DEBUG
  union DRV_NORFLASH_SPEED_RATIO_T ratio;
  uint32_t div;
  uint32_t size;
  const uint8_t *list;
  int i;

  TRACE(0, "FLASH_CALIB_RESULT:");
  TRACE(0, "<FREQ>\nsource_clk=%u speed=%u flash_max=%u", norflash_source_clk,
        norflash_speed, flash_list[flash_idx]->max_speed);
  ratio = flash_list[flash_idx]->speed_ratio;
  TRACE(0, "<RATIO>\nstd_read=%u/8 others=%u/8", (ratio.s.std_read + 1),
        (ratio.s.others + 1));

  div = norflaship_get_div();
  TRACE(0, "<DIV>\ndiv=%u", div);
  norflash_get_samdly_list(div, &list, &size);
  TRACE(0, "<SAMDLY LIST>");
  if (list == NULL || size == 0) {
    TRACE(0, "NONE");
  } else {
    DUMP8("%02X ", list, size);
  }
  TRACE(0, "<CALIB RESULT>");
  for (i = 0; i < DRV_NORFLASH_CALIB_QTY; i++) {
    TRACE(0, "type=%d idx=%02u cnt=%02u final=%02u", i, calib_matched_idx[i],
          calib_matched_cnt[i], calib_final_idx[i]);
  }
  TRACE(0, "\t");
#endif
}

int norflash_init_sample_delay_by_div(uint32_t div) {
  if (div == 0) {
    return -1;
  }
  if (div == 1) {
#if (CHIP_FLASH_CTRL_VER <= 1)
    return -2;
#else
    norflaship_samdly(1);
#endif
  } else if (div == 2 && !falling_edge_adj) {
    // Set sample delay to clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflaship_pos_neg(1);
    norflaship_neg_phase(1);
    norflaship_samdly(2);
#else
    norflaship_samdly(3);
#endif
  } else {
    // Set sample delay to nearest to but not later than clock falling edge
#if (CHIP_FLASH_CTRL_VER <= 1)
    norflaship_pos_neg(0);
    norflaship_neg_phase(0);
    norflaship_samdly(2);
#else
    norflaship_samdly(4);
#endif
  }

  return 0;
}

int norflash_init_div(const struct HAL_NORFLASH_CONFIG_T *cfg) {
  uint32_t max_speed;
  uint32_t std_read_speed;
  uint32_t others_speed;
  union DRV_NORFLASH_SPEED_RATIO_T ratio;
  uint32_t div;

#ifdef FLASH_CALIB_DEBUG
  norflash_source_clk = cfg->source_clk;
  norflash_speed = cfg->speed;
#endif

  max_speed = flash_list[flash_idx]->max_speed;
  ratio = flash_list[flash_idx]->speed_ratio;

  if (max_speed == 0) {
    max_speed = NORFLASH_DEFAULT_MAX_SPEED;
  }
  if (max_speed > cfg->speed) {
    max_speed = cfg->speed;
  }
  if (max_speed > cfg->source_clk) {
    max_speed = cfg->source_clk;
  }
  std_read_speed =
      max_speed * (1 + ratio.s.std_read) / NORFLASH_SPEED_RATIO_DENOMINATOR;
  others_speed =
      max_speed * (1 + ratio.s.others) / NORFLASH_SPEED_RATIO_DENOMINATOR;

  div = (cfg->source_clk + max_speed - 1) / max_speed;
  div_read = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
  div = (cfg->source_clk + std_read_speed - 1) / std_read_speed;
  div_std_read = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;
  div = (cfg->source_clk + others_speed - 1) / others_speed;
  div_others = (div < NORFLASH_MAX_DIV) ? div : NORFLASH_MAX_DIV;

  if (div_read == 2 && max_speed >= FALLING_EDGE_SAMPLE_ADJ_FREQ) {
    falling_edge_adj = true;
  } else {
    falling_edge_adj = false;
  }

  if (div_read && div_std_read && div_others) {
#if (CHIP_FLASH_CTRL_VER <= 1)
    if (div_read == 1) {
      return -1;
    }
#else
    if (div_read == 1 && max_speed > NORFLASH_DIV1_MAX_SPEED) {
      return -1;
    }
#endif
    // Init sample delay according to div_read
    norflash_init_sample_delay_by_div(div_read);
    // Still in command mode
    norflaship_div(div_others);
    return 0;
  }

  return 1;
}

int norflash_match_chip(const uint8_t *id, uint32_t len) {
  const uint8_t *cmp_id;

  if (len == NORFLASH_ID_LEN) {
    for (flash_idx = 0; flash_idx < ARRAY_SIZE(flash_list); flash_idx++) {
      cmp_id = flash_list[flash_idx]->id;
      if (id[0] == cmp_id[0] && id[1] == cmp_id[1] && id[2] == cmp_id[2]) {
        return true;
      }
    }
  }

  return false;
}

#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
static void norflaship_ext_cmd_addr(uint8_t cmd, uint32_t addr) {
  uint8_t buff[3];

  buff[2] = (uint8_t)(addr & 0xff);
  buff[1] = (uint8_t)((addr >> 8) & 0xff);
  buff[0] = (uint8_t)((addr >> 16) & 0xff);

  norflaship_clear_txfifo();
  norflaship_write_txfifo(buff, 3);
  norflaship_ext_tx_cmd(cmd, 3);
}
#endif

enum HAL_NORFLASH_RET_T norflash_erase(uint32_t start_address,
                                       enum DRV_NORFLASH_ERASE_T type,
                                       int suspend) {
  enum HAL_NORFLASH_RET_T ret;

  if (flash_list[flash_idx]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
    if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
      norflash_set_quad_io_mode(0);
    } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
      norflash_set_quad_output_mode(0);
    }
  }

  norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);
  // Need 1us. Or norflash_status_WEL_0_wait(), which needs 6us.
  switch (type) {
#ifdef PUYA_FLASH_ERASE_PAGE_ENABLE
  case DRV_NORFLASH_ERASE_PAGE:
    norflaship_ext_cmd_addr(PUYA_FLASH_CMD_PAGE_ERASE, start_address);
    break;
#endif
  case DRV_NORFLASH_ERASE_SECTOR:
    norflaship_cmd_addr(GD25Q32C_CMD_SECTOR_ERASE, start_address);
    break;
  case DRV_NORFLASH_ERASE_BLOCK:
    norflaship_cmd_addr(GD25Q32C_CMD_BLOCK_ERASE, start_address);
    break;
  case DRV_NORFLASH_ERASE_CHIP:
    norflaship_cmd_addr(GD25Q32C_CMD_CHIP_ERASE, start_address);
    break;
  }

  norflaship_busy_wait();

#ifdef FLASH_SUSPEND
  // PUYA flash requires the first delay of erase >= 400us
  if (flash_list[flash_idx]->id[0] == NORFLASH_PUYA_ID_PREFIX) {
    norflash_delay(400);
  }
#endif

  ret = norflash_status_WIP_1_wait(suspend);

  if (flash_list[flash_idx]->mode & HAL_NORFLASH_OP_MODE_ERASE_IN_STD) {
    if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
      norflash_set_quad_io_mode(1);
    } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
      norflash_set_quad_output_mode(1);
    }
  }

  norflaship_cmd_done();

  return ret;
}

enum HAL_NORFLASH_RET_T norflash_write(uint32_t start_address,
                                       const uint8_t *buffer, uint32_t len,
                                       int suspend) {
  enum HAL_NORFLASH_RET_T ret;
  uint32_t POSSIBLY_UNUSED remains;

  if (len > GD25Q32C_PAGE_SIZE) {
    return HAL_NORFLASH_ERR;
  }

  norflaship_clear_txfifo();

#if (CHIP_FLASH_CTRL_VER >= 2)
  norflaship_blksize(len);
#endif

  remains = norflaship_write_txfifo(buffer, len);

  norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);
  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM) {
    norflaship_cmd_addr(GD25Q32C_CMD_QUAD_PAGE_PROGRAM, start_address);
  } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM) {
    norflaship_cmd_addr(GD25Q32C_CMD_DUAL_PAGE_PROGRAM, start_address);
  } else {
    norflaship_cmd_addr(GD25Q32C_CMD_PAGE_PROGRAM, start_address);
  }

#if (CHIP_FLASH_CTRL_VER >= 2)
  while (remains > 0) {
    buffer += len - remains;
    len = remains;
    remains = norflaship_write_txfifo(buffer, len);
  }
#endif

  norflaship_busy_wait();

#ifdef FLASH_SUSPEND
  // PUYA flash requires the first delay of byte program >= 450us
  if (flash_list[flash_idx]->id[0] == NORFLASH_PUYA_ID_PREFIX &&
      len < GD25Q32C_PAGE_SIZE) {
    norflash_delay(450);
  }
#endif

  ret = norflash_status_WIP_1_wait(suspend);

  norflaship_cmd_done();

  return ret;
}

#ifdef FLASH_SUSPEND
enum HAL_NORFLASH_RET_T norflash_erase_resume(int suspend) {
  // TODO: Need to check SUS1 bit in status reg?

  enum HAL_NORFLASH_RET_T ret;

  norflash_pre_operation();

  norflash_resume();

  ret = norflash_status_WIP_1_wait(suspend);

  norflash_post_operation();

  return ret;
}

enum HAL_NORFLASH_RET_T norflash_write_resume(int suspend) {
  // TODO: Need to check SUS2 bit in status reg?

  return norflash_erase_resume(suspend);
}
#endif

int norflash_read(uint32_t start_address, uint8_t *buffer, uint32_t len) {
  uint32_t index = 0;
  uint8_t val;

  if (len > NORFLASHIP_RXFIFO_SIZE) {
    return 1;
  }

  norflaship_clear_rxfifo();

  norflaship_busy_wait();

  norflaship_blksize(len);

  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_IO) {
    /* Quad , only fast */
    norflaship_cmd_addr(GD25Q32C_CMD_FAST_QUAD_IO_READ, start_address);
  } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_QUAD_OUTPUT) {
    /* Dual, only fast */
    norflaship_cmd_addr(GD25Q32C_CMD_FAST_QUAD_OUTPUT_READ, start_address);
  } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_DUAL_IO) {
    /* Dual, only fast */
    norflaship_cmd_addr(GD25Q32C_CMD_FAST_DUAL_IO_READ, start_address);
  } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_DUAL_OUTPUT) {
    /* Dual, only fast */
    norflaship_cmd_addr(GD25Q32C_CMD_FAST_DUAL_OUTPUT_READ, start_address);
  } else if (norflash_op_mode & HAL_NORFLASH_OP_MODE_FAST_SPI) {
    /* fast */
    norflaship_cmd_addr(GD25Q32C_CMD_STANDARD_FAST_READ, start_address);
  } else {
    /* normal */
    norflaship_cmd_addr(GD25Q32C_CMD_STANDARD_READ, start_address);
  }

  while (1) {
    norflaship_rxfifo_empty_wait();

    val = norflaship_read_rxfifo();
    if (buffer) {
      buffer[index] = val;
    }

    ++index;
    if (index >= len) {
      break;
    }
  }

  norflaship_cmd_done();

  return 0;
}

void norflash_sleep(void) {
  norflash_pre_operation();
#ifdef FLASH_HPM
  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
    norflash_set_hpm(0);
  }
#endif
  norflaship_cmd_addr(GD25Q32C_CMD_DEEP_POWER_DOWN, 0);
}

void norflash_wakeup(void) {
  norflaship_cmd_addr(GD25Q32C_CMD_RELEASE_FROM_DP, 0);
  // Wait 20us for flash to finish
  norflash_delay(40);
#ifdef FLASH_HPM
  if (norflash_op_mode & HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE) {
    norflash_set_hpm(1);
  }
#endif
  norflash_post_operation();
}

int norflash_init_status(uint32_t status) {
  if (flash_list[flash_idx]->write_status == NULL) {
    return -1;
  }

  flash_list[flash_idx]->write_status(DRV_NORFLASH_W_STATUS_INIT, status);

  return 0;
}

int norflash_set_block_protection(uint32_t bp) {
  if (flash_list[flash_idx]->write_status == NULL) {
    return -1;
  }

  flash_list[flash_idx]->write_status(DRV_NORFLASH_W_STATUS_BP, bp);

  return 0;
}

#ifdef FLASH_SECURITY_REGISTER
int norflash_security_register_lock(uint32_t id) {
  if (flash_list[flash_idx]->write_status == NULL) {
    return -1;
  }

  flash_list[flash_idx]->write_status(DRV_NORFLASH_W_STATUS_LB, id);

  return 0;
}

enum HAL_NORFLASH_RET_T
norflash_security_register_erase(uint32_t start_address) {
  enum HAL_NORFLASH_RET_T ret;

  norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);
  // Need 1us. Or norflash_status_WEL_0_wait(), which needs 6us.

  norflaship_cmd_addr(GD25Q32C_CMD_SECURITY_REGISTER_ERASE, start_address);

  ret = norflash_status_WIP_1_wait(0);

  norflaship_cmd_done();

  return ret;
}

enum HAL_NORFLASH_RET_T norflash_security_register_write(uint32_t start_address,
                                                         const uint8_t *buffer,
                                                         uint32_t len) {
  enum HAL_NORFLASH_RET_T ret;
  uint32_t remains;

  // Security register page size might be larger than normal page size
  // E.g., the size of P25Q32L and P25Q64L is 1024

  norflaship_clear_txfifo();

#if (CHIP_FLASH_CTRL_VER <= 1)
  uint32_t div = 0;

  if (len > NORFLASHIP_TXFIFO_SIZE) {
    div = norflaship_get_div();

    // Slow down to avoid tx fifo underflow (it takes about 10 cpu cycles to
    // fill one byte)
    norflaship_div(16);
  }

  remains = norflaship_v1_write_txfifo_safe(buffer, len);
#else
  norflaship_blksize(len);

  remains = norflaship_write_txfifo(buffer, len);
#endif

  norflaship_cmd_addr(GD25Q32C_CMD_WRITE_ENABLE, start_address);

  norflaship_cmd_addr(GD25Q32C_CMD_SECURITY_REGISTER_PROGRAM, start_address);

#if (CHIP_FLASH_CTRL_VER <= 1)
  if (remains) {
    norflaship_v1_write_txfifo_all(buffer, len);
  }
#else
  while (remains > 0) {
    buffer += len - remains;
    len = remains;
    remains = norflaship_write_txfifo(buffer, len);
  }
#endif

  norflaship_busy_wait();

  ret = norflash_status_WIP_1_wait(0);

#if (CHIP_FLASH_CTRL_VER <= 1)
  if (div) {
    // Restore the old div
    norflaship_div(div);
  }
#endif

  norflaship_cmd_done();

  return ret;
}

int norflash_security_register_read(uint32_t start_address, uint8_t *buffer,
                                    uint32_t len) {
  uint32_t index = 0;

  if (len > NORFLASHIP_RXFIFO_SIZE) {
    return 1;
  }

  norflaship_clear_rxfifo();

  norflaship_busy_wait();

  norflaship_blksize(len);

  norflaship_cmd_addr(GD25Q32C_CMD_SECURITY_REGISTER_READ, start_address);

  while (1) {
    norflaship_rxfifo_empty_wait();

    buffer[index] = norflaship_read_rxfifo();

    ++index;
    if (index >= len) {
      break;
    }
  }

  norflaship_cmd_done();

  return 0;
}

uint32_t norflash_security_register_enable_read(void) {
  uint32_t mode;

  mode = norflash_op_mode;

  norflash_set_mode(0);

  norflaship_busy_wait();
#if (CHIP_FLASH_CTRL_VER <= 1)
  norflash_read(FLASH_NC_BASE, NULL, 1);
#endif
  norflaship_clear_rxfifo();
  norflaship_busy_wait();
  norflaship_rdcmd(GD25Q32C_CMD_SECURITY_REGISTER_READ);
  norflaship_busy_wait();

  return mode;
}

void norflash_security_register_disable_read(uint32_t mode) {
  norflaship_busy_wait();
#if (CHIP_FLASH_CTRL_VER <= 1)
  norflash_read(FLASH_NC_BASE, NULL, 1);
#endif
  norflaship_clear_rxfifo();
  norflaship_busy_wait();
  norflaship_rdcmd(GD25Q32C_CMD_STANDARD_READ);
  norflaship_busy_wait();

  norflash_set_mode(mode);
}

#endif
