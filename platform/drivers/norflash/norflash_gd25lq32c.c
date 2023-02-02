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
#include "norflash_gd25lq32c.h"
#include "hal_norflaship.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "norflash_cfg.h"
#include "norflash_drv.h"
#include "plat_types.h"

static void gd25lq32c_write_status_s0_s15(uint16_t status, uint8_t len) {
  norflash_write_reg(GD25LQ32C_CMD_WRITE_STATUS, (uint8_t *)&status, len);
}

static int gd25lq32c_write_status(enum DRV_NORFLASH_W_STATUS_T type,
                                  uint32_t param) {
  uint8_t status_s0_s7;
  uint8_t status_s8_s15;
  uint32_t bp_mask = 0;
  union DRV_NORFLASH_SEC_REG_CFG_T cfg;
  bool has_quad;

  if (type != DRV_NORFLASH_W_STATUS_INIT && type != DRV_NORFLASH_W_STATUS_QE &&
      type != DRV_NORFLASH_W_STATUS_LB && type != DRV_NORFLASH_W_STATUS_BP) {
    return 1;
  }

  has_quad =
      !!(norflash_get_supported_mode() &
         (HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO));

  if (type == DRV_NORFLASH_W_STATUS_INIT) {
    gd25lq32c_write_status_s0_s15(param, (has_quad ? 2 : 1));
    return 0;
  }

  status_s0_s7 = norflash_read_status_s0_s7();

  if (type == DRV_NORFLASH_W_STATUS_BP) {
    bp_mask = norflash_get_block_protect_mask();
    status_s0_s7 = (status_s0_s7 & ~bp_mask) | (param & bp_mask);
  }

  if (has_quad) {
    status_s8_s15 = norflash_read_status_s8_s15();

    if (type == DRV_NORFLASH_W_STATUS_QE) {
      if (param) {
        status_s8_s15 |= GD25LQ32C_QE_BIT_MASK;
      } else {
        status_s8_s15 &= ~(GD25LQ32C_QE_BIT_MASK);
      }
    } else if (type == DRV_NORFLASH_W_STATUS_BP) {
      param >>= 8;
      bp_mask >>= 8;
      status_s8_s15 = (status_s8_s15 & ~bp_mask) | (param & bp_mask);
    } else if (type == DRV_NORFLASH_W_STATUS_LB) {
      cfg = norflash_get_security_register_config();
      if (!cfg.s.enabled) {
        return 2;
      }
      if (cfg.s.lb == SEC_REG_LB_S11_S13) {
        if (param >= 3) {
          return 3;
        }
        status_s8_s15 |= (STATUS_S11_LB1_BIT_MASK << param);
      } else if (cfg.s.lb == SEC_REG_LB_S10) {
        status_s8_s15 |= STATUS_S10_LB_BIT_MASK;
      } else {
        return 4;
      }
    }

    gd25lq32c_write_status_s0_s15(status_s0_s7 | (status_s8_s15 << 8), 2);
  } else {
    gd25lq32c_write_status_s0_s15(status_s0_s7, 1);
  }

  return 0;
}

// ----------------------
// GigaDevice
// ----------------------

const struct NORFLASH_CFG_T gd25lq64c_cfg = {
    .id =
        {
            0xC8,
            0x60,
            0x17,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_6_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_1024,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25LQ64C_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25lq32c_cfg = {
    .id =
        {
            0xC8,
            0x60,
            0x16,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_5_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_1024,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25LQ32C_TOTAL_SIZE,
    .max_speed = 120 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25lq16c_cfg = {
    .id =
        {
            0xC8,
            0x60,
            0x15,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_6_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25LQ16C_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25lq80c_cfg = {
    .id =
        {
            0xC8,
            0x60,
            0x14,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_6_EIGHTH,
                    .others = SPEED_RATIO_6_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25LQ80C_TOTAL_SIZE,
    .max_speed = 104 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25q80c_cfg = {
    .id =
        {
            0xC8,
            0x40,
            0x14,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_5_EIGHTH,
                    .others = SPEED_RATIO_5_EIGHTH,
                },
        },
    .crm_en_bits = 0xA0,
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X0000,
                    .size = SEC_REG_SIZE_256,
                    .offset = SEC_REG_OFFSET_0X0100,
                    .cnt = SEC_REG_CNT_4,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S10,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25Q80C_TOTAL_SIZE,
#ifdef FLASH_HPM
    .max_speed = 120 * 1000 * 1000,
#else
    .max_speed = 104 * 1000 * 1000,
#endif
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
#ifdef FLASH_HPM
         HAL_NORFLASH_OP_MODE_HIGH_PERFORMANCE |
#endif
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25d40c_cfg = {
    .id =
        {
            0xC8,
            0x40,
            0x13,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_6_EIGHTH,
                    .others = SPEED_RATIO_6_EIGHTH,
                },
        },
    .crm_en_bits = 0,
    .crm_dis_bits = 0,
    .block_protect_mask = 0x1C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = false,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25D40C_TOTAL_SIZE,
    .max_speed = 80 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_PAGE_PROGRAM),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T gd25d20c_cfg = {
    .id =
        {
            0xC8,
            0x40,
            0x12,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_6_EIGHTH,
                    .others = SPEED_RATIO_6_EIGHTH,
                },
        },
    .crm_en_bits = 0,
    .crm_dis_bits = 0,
    .block_protect_mask = 0x1C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = false,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = GD25D20C_TOTAL_SIZE,
    .max_speed = 80 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_PAGE_PROGRAM),
    .write_status = gd25lq32c_write_status,
};

// ----------------------
// Puya
// ----------------------

const struct NORFLASH_CFG_T p25q16l_cfg = {
    .id =
        {
            0x85,
            0x60,
            0x15,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_3_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = P25Q16L_TOTAL_SIZE,
    .max_speed =
        70 * 1000 * 1000, // P25Q80L=70M, P25Q80H=104M, P25Q80U=70M/104M
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T p25q80h_cfg = {
    .id =
        {
            0x85,
            0x60,
            0x14,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_3_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = P25Q80H_TOTAL_SIZE,
    .max_speed =
        70 * 1000 * 1000, // P25Q80L=70M, P25Q80H=104M, P25Q80U=70M/104M
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T p25q40h_cfg = {
    .id =
        {
            0x85,
            0x60,
            0x13,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_3_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = P25Q40H_TOTAL_SIZE,
    .max_speed =
        70 * 1000 * 1000, // P25Q21L=70M, P25Q21H=104M, P25Q21U=70M/104M
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

const struct NORFLASH_CFG_T p25q21h_cfg = {
    .id =
        {
            0x85,
            0x40,
            0x12,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_3_EIGHTH,
                    .others = SPEED_RATIO_8_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x407C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X1000,
                    .size = SEC_REG_SIZE_512,
                    .offset = SEC_REG_OFFSET_0X1000,
                    .cnt = SEC_REG_CNT_3,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S11_S13,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = P25Q21H_TOTAL_SIZE,
    .max_speed =
        70 * 1000 * 1000, // P25Q21L=70M, P25Q21H=104M, P25Q21U=70M/104M
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ | HAL_NORFLASH_OP_MODE_READ_WRAP |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_DUAL_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};

// ----------------------
// XTS
// ----------------------

const struct NORFLASH_CFG_T xt25q08b_cfg = {
    .id =
        {
            0x0B,
            0x60,
            0x14,
        },
    .speed_ratio =
        {
            .s =
                {
                    .std_read = SPEED_RATIO_7_EIGHTH,
                    .others = SPEED_RATIO_7_EIGHTH,
                },
        },
    .crm_en_bits = (1 << 5) | (0 << 4),
    .crm_dis_bits = 0,
    .block_protect_mask = 0x403C,
    .sec_reg_cfg =
        {
            .s =
                {
                    .enabled = true,
                    .base = SEC_REG_BASE_0X0000,
                    .size = SEC_REG_SIZE_256,
                    .offset = SEC_REG_OFFSET_0X0100,
                    .cnt = SEC_REG_CNT_4,
                    .pp = SEC_REG_PP_256,
                    .lb = SEC_REG_LB_S10,
                },
        },
    .page_size = GD25LQ32C_PAGE_SIZE,
    .sector_size = GD25LQ32C_SECTOR_SIZE,
    .block_size = GD25LQ32C_BLOCK_SIZE,
    .total_size = XT25Q08B_TOTAL_SIZE,
    .max_speed = 96 * 1000 * 1000,
    .mode =
        (HAL_NORFLASH_OP_MODE_STAND_SPI | HAL_NORFLASH_OP_MODE_FAST_SPI |
         HAL_NORFLASH_OP_MODE_DUAL_OUTPUT | HAL_NORFLASH_OP_MODE_DUAL_IO |
         HAL_NORFLASH_OP_MODE_QUAD_OUTPUT | HAL_NORFLASH_OP_MODE_QUAD_IO |
         HAL_NORFLASH_OP_MODE_CONTINUOUS_READ |
         HAL_NORFLASH_OP_MODE_PAGE_PROGRAM |
         HAL_NORFLASH_OP_MODE_QUAD_PAGE_PROGRAM | HAL_NORFLASH_OP_MODE_SUSPEND),
    .write_status = gd25lq32c_write_status,
};
