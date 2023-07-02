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
#if (CHIP_FLASH_CTRL_VER >= 2)

#include "cmsis.h"
#include "hal_norflaship.h"
#include "plat_addr_map.h"
#include "plat_types.h"
#include "reg_norflaship_v2.h"
#ifdef PROGRAMMER
#include "task_schedule.h"
#else
#define TASK_SCHEDULE true
#endif

//====================================================================================
// Flash IP Operations
//====================================================================================

/*
 Supported Command List (Based on GD25Q32C):

 parameter WRSR1     = 8'h01 ;
 parameter PP        = 8'h02 ;
 parameter READ      = 8'h03 ;
 parameter WRDI      = 8'h04 ;
 parameter RDSR1     = 8'h05 ;
 parameter WREN      = 8'h06 ;
 parameter FREAD     = 8'h0B ;
 parameter WRSR3     = 8'h11 ;
 parameter RDSR3     = 8'h15 ;
 parameter SE        = 8'h20 ;
 parameter WRSR2     = 8'h31 ;
 parameter QPP       = 8'h32 ;
 parameter RDSR2     = 8'h35 ;
 parameter DOR       = 8'h3B ;
 parameter PSR       = 8'h42 ;
 parameter ESR       = 8'h44 ;
 parameter RSR       = 8'h48 ;
 parameter WEVSR     = 8'h50 ;
 parameter BE32      = 8'h52 ;
 parameter RSFDP     = 8'h5A ;
 parameter CE        = 8'h60 ;
 parameter RSTE      = 8'h66 ;
 parameter QOR       = 8'h6B ;
 parameter PES       = 8'h75 ;
 parameter SBW       = 8'h77 ;
 parameter PER       = 8'h7A ;
 parameter REMS      = 8'h90 ;
 parameter DREAD_ID  = 8'h92 ;
 parameter QREAD_ID  = 8'h94 ;
 parameter RST       = 8'h99 ;
 parameter RDID      = 8'h9F ;
 parameter HPM       = 8'hA3 ;
 parameter RDI       = 8'hAB ;
 parameter DP        = 8'hB9 ;
 parameter DIOR      = 8'hBB ;
 parameter BE64      = 8'hD8 ;
 parameter QIOWR     = 8'hE7 ;
 parameter QIOR      = 8'hEB ;
 parameter FPP       = 8'hF2 ;
*/

static struct NORFLASH_CTRL_T *const norflash =
    (struct NORFLASH_CTRL_T *)FLASH_CTRL_BASE;

#if 0
static const uint8_t line_mode[8] = {
    NEW_CMD_LINE_1X, NEW_CMD_LINE_1X, NEW_CMD_LINE_2X, NEW_CMD_LINE_1X,
    NEW_CMD_LINE_4X, NEW_CMD_LINE_1X, NEW_CMD_LINE_1X, NEW_CMD_LINE_1X,
};
#endif

uint8_t norflaship_continuous_read_mode_bit(uint8_t mode_bit) {
  norflaship_busy_wait();
  norflash->REG_004 =
      SET_BITFIELD(norflash->REG_004, REG_004_MODEBIT, mode_bit);
  return 0;
}
uint8_t norflaship_continuous_read_off(void) {
  norflaship_busy_wait();
  norflash->REG_004 &= ~REG_004_CONTINUOUS_MODE;
  return 0;
}
uint8_t norflaship_continuous_read_on(void) {
  norflaship_busy_wait();
  norflash->REG_004 |= REG_004_CONTINUOUS_MODE;
  return 0;
}
uint8_t norflaship_read_txfifo_empty_count(void) {
  return GET_BITFIELD(norflash->REG_00C, REG_00C_TXFIFO_EMPCNT);
}
uint32_t norflaship_write_txfifo(const uint8_t *val, uint32_t len) {
  uint32_t avail = norflaship_read_txfifo_empty_count();
  while (len > 0 && avail > 0) {
#if 0
        if (len >= 4 && ((uint32_t)val & 3) == 0) {
            norflash->REG_008.TXWORD = *(uint32_t *)val;
            val += 4;
            len -= 4;
        } else if (len >= 2 && ((uint32_t)val & 1) == 0) {
            norflash->REG_008.TXHALFWORD = *(uint16_t *)val;
            val += 2;
            len -= 2;
        } else
#endif
    {
      norflash->REG_008.TXBYTE = *(uint8_t *)val;
      val += 1;
      len -= 1;
    }
    avail -= 1;
  }
  return len;
}
uint8_t norflaship_read_rxfifo_count(void) {
  return GET_BITFIELD(norflash->REG_00C, REG_00C_RXFIFO_COUNT);
}
uint8_t norflaship_read_rxfifo(void) { return norflash->REG_010; }
void norflaship_blksize(uint32_t blksize) {
  norflash->REG_004 =
      SET_BITFIELD(norflash->REG_004, REG_004_BLOCK_SIZE, blksize);
}
void norflaship_cmd_addr(uint8_t cmd, uint32_t address) {
  norflaship_busy_wait();
  norflash->REG_004 &= ~REG_004_NEW_CMD_EN;
  norflash->REG_000 = REG_000_CMD(cmd) | REG_000_ADDR(address);
}
void norflaship_ext_tx_cmd(uint8_t cmd, uint32_t tx_len) {
  norflaship_busy_wait();
  norflash->REG_004 =
      SET_BITFIELD(norflash->REG_004, REG_004_BLOCK_SIZE, tx_len) |
      REG_004_NEW_CMD_EN;
  norflash->REG_000 = REG_000_CMD(cmd);
}
void norflaship_ext_rx_cmd(uint8_t cmd, uint32_t tx_len, uint32_t rx_len) {
  norflaship_busy_wait();
  norflash->REG_004 =
      SET_BITFIELD(norflash->REG_004, REG_004_BLOCK_SIZE, tx_len) |
      REG_004_NEW_CMD_EN;
  norflash->REG_000 =
      REG_000_CMD(cmd) | REG_000_NEW_CMD_RX_EN | REG_000_NEW_CMD_RX_LEN(rx_len);
}
void norflaship_cmd_done(void) {
  norflaship_busy_wait();
  norflash->REG_004 &= ~REG_004_NEW_CMD_EN;
}
void norflaship_rxfifo_count_wait(uint8_t cnt) {
  while ((norflaship_read_rxfifo_count() < cnt) && TASK_SCHEDULE)
    ;
}
void norflaship_rxfifo_empty_wait(void) {
  while ((norflash->REG_00C & REG_00C_RXFIFO_EMPTY) && TASK_SCHEDULE)
    ;
}
int norflaship_txfifo_is_full(void) {
  return !!(norflash->REG_00C & REG_00C_TXFIFO_FULL);
}
void norflaship_busy_wait(void) {
  while ((norflash->REG_00C & REG_00C_BUSY) && TASK_SCHEDULE)
    ;
}
int norflaship_is_busy(void) { return !!(norflash->REG_00C & REG_00C_BUSY); }
void norflaship_clear_fifos(void) {
  norflaship_busy_wait();
  norflash->REG_018 = REG_018_TXFIFOCLR | REG_018_RXFIFOCLR;
  norflaship_busy_wait();
}
void norflaship_clear_rxfifo(void) {
  norflaship_busy_wait();
  norflash->REG_018 = REG_018_RXFIFOCLR;
  norflaship_busy_wait();
}
void norflaship_clear_txfifo(void) {
  norflaship_busy_wait();
  norflash->REG_018 = REG_018_TXFIFOCLR;
  norflaship_busy_wait();
}
void norflaship_div(uint32_t div) {
  norflaship_busy_wait();
#ifdef CHIP_BEST2300P
  norflash->REG_014 =
      SET_BITFIELD(norflash->REG_014, REG_014_CLKDIV, div) | EXTRA_TCHSH_EN_O;
#else
  norflash->REG_014 = SET_BITFIELD(norflash->REG_014, REG_014_CLKDIV, div);
#endif
}
uint32_t norflaship_get_div(void) {
  return GET_BITFIELD(norflash->REG_014, REG_014_CLKDIV);
}
void norflaship_cmdquad(uint32_t v) {
  norflaship_busy_wait();
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_CMDQUAD | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_CMDQUAD) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_CMDQUAD;
  } else {
    norflash->REG_014 &= ~REG_014_CMDQUAD;
  }
#endif
}
uint32_t norflaship_get_samdly(void) {
  return GET_BITFIELD(norflash->REG_014, REG_014_SAMPLESEL);
}
void norflaship_samdly(uint32_t v) {
  norflaship_busy_wait();
#ifdef CHIP_BEST2300P
  norflash->REG_014 =
      SET_BITFIELD(norflash->REG_014, REG_014_SAMPLESEL, v) | EXTRA_TCHSH_EN_O;
#else
  norflash->REG_014 = SET_BITFIELD(norflash->REG_014, REG_014_SAMPLESEL, v);
#endif
}
void norflaship_dual_mode(uint32_t v) {
  norflaship_busy_wait();
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_RAM_DUALMODE | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_RAM_DUALMODE) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_RAM_DUALMODE;
  } else {
    norflash->REG_014 &= ~REG_014_RAM_DUALMODE;
  }
#endif
}
void norflaship_hold_pin(uint32_t v) {
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_HOLDPIN | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_HOLDPIN) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_HOLDPIN;
  } else {
    norflash->REG_014 &= ~REG_014_HOLDPIN;
  }
#endif
}
void norflaship_wpr_pin(uint32_t v) {
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_WPROPIN | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_WPROPIN) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_WPROPIN;
  } else {
    norflash->REG_014 &= ~REG_014_WPROPIN;
  }
#endif
}
void norflaship_quad_mode(uint32_t v) {
  norflaship_busy_wait();
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_RAM_QUADMODE | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_RAM_QUADMODE) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_RAM_QUADMODE;
  } else {
    norflash->REG_014 &= ~REG_014_RAM_QUADMODE;
  }
#endif
}
void norflaship_addrbyte4(uint32_t v) {
#ifdef CHIP_BEST2300P
  if (v) {
    norflash->REG_014 |= REG_014_FOUR_BYTE_ADDR_EN | EXTRA_TCHSH_EN_O;
  } else {
    norflash->REG_014 =
        (norflash->REG_014 & ~REG_014_FOUR_BYTE_ADDR_EN) | EXTRA_TCHSH_EN_O;
  }
#else
  if (v) {
    norflash->REG_014 |= REG_014_FOUR_BYTE_ADDR_EN;
  } else {
    norflash->REG_014 &= ~REG_014_FOUR_BYTE_ADDR_EN;
  }
#endif
}
void norflaship_ruen(uint32_t v) {
  norflash->REG_034 = SET_BITFIELD(norflash->REG_034, REG_034_SPI_RUEN, v);
}
void norflaship_rden(uint32_t v) {
  norflash->REG_034 = SET_BITFIELD(norflash->REG_034, REG_034_SPI_RDEN, v);
}
void norflaship_dualiocmd(uint32_t v) {
  norflash->REG_020 = SET_BITFIELD(norflash->REG_020, REG_020_DUALCMD, v);
}
void norflaship_rdcmd(uint32_t v) {
  norflash->REG_020 = SET_BITFIELD(norflash->REG_020, REG_020_READCMD, v);
}
void norflaship_frdcmd(uint32_t v) {
  norflash->REG_020 = SET_BITFIELD(norflash->REG_020, REG_020_FREADCMD, v);
}
void norflaship_qrdcmd(uint32_t v) {
  norflash->REG_020 = SET_BITFIELD(norflash->REG_020, REG_020_QUADCMD, v);
}
uint32_t norflaship_get_rdcmd(void) {
  return GET_BITFIELD(norflash->REG_020, REG_020_READCMD);
}
void norflaship_set_idle_io_dir(uint32_t v) {
#ifdef NORFLASHIP_HAS_IDLE_IO_CTRL
  norflash->REG_034 = SET_BITFIELD(norflash->REG_034, REG_034_SPI_IOEN, v);
#endif
}
void norflaship_sleep(void) {
#ifndef NORFLASHIP_HAS_IDLE_IO_CTRL
  norflash->REG_034 |= REG_034_SPI_IORES(1 << 2);
#endif
}
void norflaship_wakeup(void) {
#ifndef NORFLASHIP_HAS_IDLE_IO_CTRL
  norflash->REG_034 &= ~REG_034_SPI_IORES(1 << 2);
#endif
}

void norflaship_dec_index(uint32_t idx) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_058 = SET_BITFIELD(norflash->REG_058, REG_058_IDX, idx);
#endif
}

void norflaship_dec_saddr(uint32_t addr) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_060 = ((addr & 0x000000ff) << 24) | ((addr & 0x0000ff00) << 8) |
                      ((addr & 0x00ff0000) >> 8) | ((addr & 0xff000000) >> 24);
#endif
}

void norflaship_dec_eaddr(uint32_t addr) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_064 = ((addr & 0x000000ff) << 24) | ((addr & 0x0000ff00) << 8) |
                      ((addr & 0x00ff0000) >> 8) | ((addr & 0xff000000) >> 24);
#endif
}

void norflaship_dec_enable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_06C |= REG_06C_DEC_ENABLE;
#endif
}

void norflaship_dec_disable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_06C &= ~REG_06C_DEC_ENABLE;
#endif
}

void norflaship_man_wrap_width(uint32_t width) {
#ifdef NORFLASHIP_HAS_SECURITY
  int bits = 0;

  if (width == 8)
    bits = 0;
  else if (width == 16)
    bits = 1;
  else if (width == 32)
    bits = 2;
  else if (width == 64)
    bits = 3;
  else
    bits = 1;
  norflash->REG_038 =
      SET_BITFIELD(norflash->REG_038, REG_038_MAN_WRAP_BITS, bits);
#endif
}

void norflaship_man_wrap_enable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_038 |= REG_038_MAN_WRAP_ENABLE;
#endif
}

void norflaship_man_wrap_disable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_038 &= ~REG_038_MAN_WRAP_ENABLE;
#endif
}

void norflaship_auto_wrap_cmd(uint32_t cmd) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_038 =
      SET_BITFIELD(norflash->REG_038, REG_038_AUTO_WRAP_CMD, cmd);
#endif
}

void norflaship_man_mode_enable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_038 |= REG_038_WRAP_MODE_SEL;
#endif
}

void norflaship_man_mode_disable(void) {
#ifdef NORFLASHIP_HAS_SECURITY
  norflash->REG_038 &= ~REG_038_WRAP_MODE_SEL;
#endif
}

int norflaship_config_remap_section(uint32_t id, uint32_t addr, uint32_t len,
                                    uint32_t new_addr) {
#ifdef NORFLASHIP_HAS_REMAP
  __IO uint32_t *start, *end, *to;

  if (id >= 4) {
    return 1;
  }
  if (len == 0) {
    norflash->REG_0B0 &= ~(REG_0B0_ADDR0_REMAP_EN << id);
    return 0;
  }
  if (addr & (REMAP_SECTOR_SIZE - 1)) {
    return 2;
  }
  if (new_addr & (REMAP_SECTOR_SIZE - 1)) {
    return 3;
  }
  if (len < REMAP_SECTOR_SIZE) {
    return 4;
  }

#ifdef CHIP_BEST2300P
  if (len & (len - 1)) {
    return 5;
  }

  uint32_t len_idx;

  len_idx = __CLZ(__RBIT(len)) - 9;

  norflash->REG_0A0 =
      (norflash->REG_0A0 &
       ~(REG_0A0_LEN_WIDTH0_MASK << (id * REG_0A0_LEN_WIDTH1_SHIFT))) |
      (REG_0A0_LEN_WIDTH0(len_idx) << (id * REG_0A0_LEN_WIDTH1_SHIFT));
#else
  if (len & (REMAP_SECTOR_SIZE - 1)) {
    return 5;
  }
#endif

  start = &norflash->REG_070 + id;
  end = &norflash->REG_080 + id;
  to = &norflash->REG_090 + id;

  *start = addr;
  *end = addr + len - 1;
  *to = new_addr;

  norflash->REG_0B0 |= (REG_0B0_ADDR0_REMAP_EN << id);

  return 0;
#else
  return -1;
#endif
}

void norflaship_enable_remap(void) {
#ifdef NORFLASHIP_HAS_REMAP
  norflash->REG_0B0 |= REG_0B0_GLB_REMAP_EN;
#endif
}

void norflaship_disable_remap(void) {
#ifdef NORFLASHIP_HAS_REMAP
  norflash->REG_0B0 &= ~REG_0B0_GLB_REMAP_EN;
#endif
}

int norflaship_get_remap_status(void) {
#ifdef NORFLASHIP_HAS_REMAP
  return (norflash->REG_0B0 & REG_0B0_GLB_REMAP_EN) ? true : false;
#else
  return false;
#endif
}

#if (CHIP_FLASH_CTRL_VER >= 3)
void norflaship_dummy_others(uint32_t v) {
  norflash->REG_150 = REG_150_DUMMY_OTHERS(v);
}
#endif

void norflaship_fetch_disable() { norflash->REG_02C &= ~REG_02C_FETCH_EN; }

#endif
