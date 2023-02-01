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
#include "hal_spi.h"
#include "cmsis.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_location.h"
#include "hal_trace.h"
#include "plat_addr_map.h"
#include "reg_spi.h"
#include "string.h"

// TODO:
// 1) Add transfer timeout control

#ifdef SPI_ROM_ONLY
#define SPI_ASSERT(c, ...) ASSERT_NODUMP(c)
#else
#define SPI_ASSERT(c, ...) ASSERT(c, ##__VA_ARGS__)
#endif

enum HAL_SPI_ID_T {
  HAL_SPI_ID_INTERNAL,
#ifdef CHIP_HAS_SPI
  HAL_SPI_ID_0,
#endif
#ifdef CHIP_HAS_SPILCD
  HAL_SPI_ID_SLCD,
#endif
#ifdef CHIP_HAS_SPIPHY
  HAL_SPI_ID_PHY,
#endif
#ifdef CHIP_HAS_SPIDPD
  HAL_SPI_ID_DPD,
#endif

  HAL_SPI_ID_QTY
};

enum HAL_SPI_CS_T {
  HAL_SPI_CS_0,
#if (CHIP_SPI_VER >= 2)
  HAL_SPI_CS_1,
  HAL_SPI_CS_2,
#if (CHIP_SPI_VER >= 3)
  HAL_SPI_CS_3,
#if (CHIP_SPI_VER >= 4)
// HAL_SPI_CS_4,
#endif
#endif
#endif

  HAL_SPI_CS_QTY
};

enum HAL_SPI_XFER_TYPE_T {
  HAL_SPI_XFER_TYPE_SEND,
  HAL_SPI_XFER_TYPE_RECV,

  HAL_SPI_XFER_TYPE_QTY
};

struct HAL_SPI_MOD_NAME_T {
  enum HAL_CMU_MOD_ID_T mod;
  enum HAL_CMU_MOD_ID_T apb;
};

static struct SPI_T *const spi[HAL_SPI_ID_QTY] = {
    (struct SPI_T *)ISPI_BASE,
#ifdef CHIP_HAS_SPI
    (struct SPI_T *)SPI_BASE,
#endif
#ifdef CHIP_HAS_SPILCD
    (struct SPI_T *)SPILCD_BASE,
#endif
#ifdef CHIP_HAS_SPIPHY
    (struct SPI_T *)SPIPHY_BASE,
#endif
#ifdef CHIP_HAS_SPIDPD
    (struct SPI_T *)SPIDPD_BASE,
#endif
};

static const struct HAL_SPI_MOD_NAME_T spi_mod[HAL_SPI_ID_QTY] = {
    {
        .mod = HAL_CMU_MOD_O_SPI_ITN,
        .apb = HAL_CMU_MOD_P_SPI_ITN,
    },
#ifdef CHIP_HAS_SPI
    {
        .mod = HAL_CMU_MOD_O_SPI,
        .apb = HAL_CMU_MOD_P_SPI,
    },
#endif
#ifdef CHIP_HAS_SPILCD
    {
        .mod = HAL_CMU_MOD_O_SLCD,
        .apb = HAL_CMU_MOD_P_SLCD,
    },
#endif
#ifdef CHIP_HAS_SPIPHY
    {
        .mod = HAL_CMU_MOD_O_SPI_PHY,
        .apb = HAL_CMU_MOD_P_SPI_PHY,
    },
#endif
#ifdef CHIP_HAS_SPIDPD
    {
        .mod = HAL_CMU_MOD_O_SPI_DPD,
        .apb = HAL_CMU_MOD_P_SPI_DPD,
    },
#endif
};

#ifndef SPI_ROM_ONLY
#ifdef CHIP_HAS_SPI
static struct HAL_SPI_CTRL_T spi0_ctrl[HAL_SPI_CS_QTY];
#if (CHIP_SPI_VER >= 2)
static enum HAL_SPI_CS_T spi0_cs = HAL_SPI_CS_0;
#else
static const enum HAL_SPI_CS_T spi0_cs = HAL_SPI_CS_0;
#endif
#endif
#ifdef CHIP_HAS_SPILCD
static struct HAL_SPI_CTRL_T spilcd_ctrl[HAL_SPI_CS_QTY];
#if (CHIP_SPI_VER >= 2)
static enum HAL_SPI_CS_T spilcd_cs = HAL_SPI_CS_0;
#else
static const enum HAL_SPI_CS_T spilcd_cs = HAL_SPI_CS_0;
#endif
#endif
#ifdef CHIP_HAS_SPIDPD
static struct HAL_SPI_CTRL_T spidpd_ctrl;
#endif

static uint8_t BOOT_BSS_LOC spi_cs_map[HAL_SPI_ID_QTY];
STATIC_ASSERT(sizeof(spi_cs_map[0]) * 8 >= HAL_SPI_CS_QTY,
              "spi_cs_map size too small");

static bool BOOT_BSS_LOC in_use[HAL_SPI_ID_QTY] = {
    false,
};

static HAL_SPI_DMA_HANDLER_T BOOT_BSS_LOC spi_txdma_handler[HAL_SPI_ID_QTY];
static HAL_SPI_DMA_HANDLER_T BOOT_BSS_LOC spi_rxdma_handler[HAL_SPI_ID_QTY];
static uint8_t BOOT_BSS_LOC spi_txdma_chan[HAL_SPI_ID_QTY];
static uint8_t BOOT_BSS_LOC spi_rxdma_chan[HAL_SPI_ID_QTY];

static enum HAL_SPI_MOD_CLK_SEL_T clk_sel[HAL_SPI_ID_QTY];

static bool BOOT_BSS_LOC spi_init_done = false;

static int hal_spi_activate_cs_id(enum HAL_SPI_ID_T id, uint32_t cs);
#endif

// static const char *invalid_id = "Invalid SPI ID: %d";

static inline uint8_t get_frame_bytes(enum HAL_SPI_ID_T id) {
  uint8_t bits, cnt;

  bits = GET_BITFIELD(spi[id]->SSPCR0, SPI_SSPCR0_DSS) + 1;
  if (bits <= 8) {
    cnt = 1;
  } else if (bits <= 16) {
    cnt = 2;
  } else {
    cnt = 4;
  }

  return cnt;
}

static inline void copy_frame_from_bytes(uint32_t *val, const uint8_t *data,
                                         uint8_t cnt) {
#ifdef UNALIGNED_ACCESS
  if (cnt == 1) {
    *val = *(const uint8_t *)data;
  } else if (cnt == 2) {
    *val = *(const uint16_t *)data;
  } else {
    *val = *(const uint32_t *)data;
  }
#else
  if (cnt == 1) {
    *val = data[0];
  } else if (cnt == 2) {
    *val = data[0] | (data[1] << 8);
  } else {
    *val = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
  }
#endif
}

static inline void copy_bytes_from_frame(uint8_t *data, uint32_t val,
                                         uint8_t cnt) {
#ifdef UNALIGNED_ACCESS
  if (cnt == 1) {
    *(uint8_t *)data = (uint8_t)val;
  } else if (cnt == 2) {
    *(uint16_t *)data = (uint16_t)val;
  } else {
    *(uint32_t *)data = (uint32_t)val;
  }
#else
  data[0] = (uint8_t)val;
  if (cnt == 1) {
    return;
  } else if (cnt == 2) {
    data[1] = (uint8_t)(val >> 8);
  } else {
    data[1] = (uint8_t)(val >> 8);
    data[2] = (uint8_t)(val >> 16);
    data[3] = (uint8_t)(val >> 24);
  }
#endif
}

int hal_spi_init_ctrl(const struct HAL_SPI_CFG_T *cfg,
                      struct HAL_SPI_CTRL_T *ctrl) {
  uint32_t div;
  uint16_t cpsdvsr, scr;
  uint32_t mod_clk;

#ifdef SPI_ROM_ONLY
  // Assume default crystal -- Never access global versatile data to ensure
  // reentrance
  mod_clk = HAL_CMU_DEFAULT_CRYSTAL_FREQ;
#else // !SPI_ROM_ONLY
  mod_clk = 0;
#ifdef PERIPH_PLL_FREQ
  if (PERIPH_PLL_FREQ / 2 > 2 * hal_cmu_get_crystal_freq()) {
    // Init to OSC_X2
    mod_clk = 2 * hal_cmu_get_crystal_freq();
    if (cfg->rate * 2 > mod_clk) {
      mod_clk = PERIPH_PLL_FREQ / 2;
      ctrl->clk_sel = HAL_SPI_MOD_CLK_SEL_PLL;
    } else {
      mod_clk = 0;
    }
  }
#endif
  if (mod_clk == 0) {
    // Init to OSC
    mod_clk = hal_cmu_get_crystal_freq();
    if (cfg->rate * 2 > mod_clk) {
      mod_clk *= 2;
      ctrl->clk_sel = HAL_SPI_MOD_CLK_SEL_OSC_X2;
    } else {
      ctrl->clk_sel = HAL_SPI_MOD_CLK_SEL_OSC;
    }
  }
#endif // !SPI_ROM_ONLY

  SPI_ASSERT(cfg->rate <= mod_clk / (MIN_CPSDVSR * (1 + MIN_SCR)),
             "SPI rate too large: %u", cfg->rate);
  SPI_ASSERT(cfg->rate >= mod_clk / (MAX_CPSDVSR * (1 + MAX_SCR)),
             "SPI rate too small: %u", cfg->rate);
  SPI_ASSERT(cfg->tx_bits <= MAX_DATA_BITS && cfg->tx_bits >= MIN_DATA_BITS,
             "Invalid SPI TX bits: %d", cfg->tx_bits);
  SPI_ASSERT(cfg->rx_bits <= MAX_DATA_BITS && cfg->rx_bits >= MIN_DATA_BITS,
             "Invalid SPI RX bits: %d", cfg->rx_bits);
  SPI_ASSERT(cfg->rx_frame_bits <= MAX_DATA_BITS &&
                 (cfg->rx_frame_bits == 0 || cfg->rx_frame_bits > cfg->rx_bits),
             "Invalid SPI RX FRAME bits: %d", cfg->rx_frame_bits);
  SPI_ASSERT(cfg->cs < HAL_SPI_CS_QTY, "SPI cs bad: %d", cfg->cs);

  div = (mod_clk + cfg->rate - 1) / cfg->rate;
  cpsdvsr = (div + MAX_SCR) / (MAX_SCR + 1);
  if (cpsdvsr < 2) {
    cpsdvsr = 2;
  } else {
    if (cpsdvsr & 0x1) {
      cpsdvsr += 1;
    }
    if (cpsdvsr > MAX_CPSDVSR) {
      cpsdvsr = MAX_CPSDVSR;
    }
  }
  scr = (div + cpsdvsr - 1) / cpsdvsr;
  if (scr > 0) {
    scr -= 1;
  }
  if (scr > MAX_SCR) {
    scr = MAX_SCR;
  }

  ctrl->sspcr0_tx =
      SPI_SSPCR0_SCR(scr) | (cfg->clk_delay_half ? SPI_SSPCR0_SPH : 0) |
      (cfg->clk_polarity ? SPI_SSPCR0_SPO : 0) |
      SPI_SSPCR0_FRF(0) | // Only support Motorola SPI frame format
      SPI_SSPCR0_DSS(cfg->tx_bits - 1);
  ctrl->sspcr1 = (cfg->rx_sep_line ? SPI_RX_SEL_EN : 0) |
                 SPI_SLAVE_ID(cfg->cs) | SPI_SSPCR1_SOD |
                 (cfg->slave ? SPI_SSPCR1_MS : 0) | SPI_SSPCR1_SSE;
  ctrl->sspcpsr = SPI_SSPCPSR_CPSDVSR(cpsdvsr);
  ctrl->sspdmacr = (cfg->dma_tx ? SPI_SSPDMACR_TXDMAE : 0) |
                   (cfg->dma_rx ? SPI_SSPDMACR_RXDMAE : 0);
  ctrl->ssprxcr_tx = 0;
  if (cfg->rx_frame_bits > 0) {
    ctrl->sspcr0_rx =
        SET_BITFIELD(ctrl->sspcr0_tx, SPI_SSPCR0_DSS, cfg->rx_frame_bits - 1);
    ctrl->ssprxcr_rx = SPI_SSPRXCR_EN | SPI_SSPRXCR_OEN_POLARITY |
                       SPI_SSPRXCR_RXBITS(cfg->rx_bits - 1);
  } else {
    ctrl->sspcr0_rx =
        SET_BITFIELD(ctrl->sspcr0_tx, SPI_SSPCR0_DSS, cfg->rx_bits - 1);
    ctrl->ssprxcr_rx = 0;
  }

  return 0;
}

static void NOINLINE POSSIBLY_UNUSED hal_spi_set_xfer_type_id(
    enum HAL_SPI_ID_T id, const struct HAL_SPI_CTRL_T *ctrl,
    enum HAL_SPI_XFER_TYPE_T type) {
  uint32_t sspcr0;
  uint32_t ssprxcr;

  if (type == HAL_SPI_XFER_TYPE_SEND) {
    sspcr0 = ctrl->sspcr0_tx;
    ssprxcr = ctrl->ssprxcr_tx;
  } else {
    sspcr0 = ctrl->sspcr0_rx;
    ssprxcr = ctrl->ssprxcr_rx;
  }

  spi[id]->SSPCR0 = sspcr0;
  spi[id]->SSPRXCR = ssprxcr;
}

static void NOINLINE hal_spi_enable_id(enum HAL_SPI_ID_T id,
                                       const struct HAL_SPI_CTRL_T *ctrl,
                                       enum HAL_SPI_XFER_TYPE_T type) {
  hal_spi_set_xfer_type_id(id, ctrl, type);

  spi[id]->SSPCR1 = ctrl->sspcr1;
  spi[id]->SSPCPSR = ctrl->sspcpsr;
  spi[id]->SSPDMACR = ctrl->sspdmacr;

#ifdef SPI_ROM_ONLY
  if (id == HAL_SPI_ID_INTERNAL) {
    hal_cmu_ispi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    hal_cmu_spi_set_freq(HAL_CMU_PERIPH_FREQ_26M);
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    hal_cmu_slcd_set_freq(HAL_CMU_PERIPH_FREQ_26M);
#endif
  }
#else // !SPI_ROM_ONLY
  if (clk_sel[id] != ctrl->clk_sel) {
    clk_sel[id] = ctrl->clk_sel;
    if (ctrl->clk_sel == HAL_SPI_MOD_CLK_SEL_PLL) {
#ifdef PERIPH_PLL_FREQ
      if (0) {
#ifdef CHIP_HAS_SPI
      } else if (id == HAL_SPI_ID_0) {
        hal_cmu_spi_set_div(2);
#endif
#ifdef CHIP_HAS_SPILCD
      } else if (id == HAL_SPI_ID_SLCD) {
        hal_cmu_slcd_set_div(2);
#endif
      }
      // ISPI cannot use PLL clock
#endif
    } else {
      enum HAL_CMU_PERIPH_FREQ_T periph_freq;

      if (ctrl->clk_sel == HAL_SPI_MOD_CLK_SEL_OSC_X2) {
        periph_freq = HAL_CMU_PERIPH_FREQ_52M;
      } else {
        periph_freq = HAL_CMU_PERIPH_FREQ_26M;
      }

      if (id == HAL_SPI_ID_INTERNAL) {
        hal_cmu_ispi_set_freq(periph_freq);
#ifdef CHIP_HAS_SPI
      } else if (id == HAL_SPI_ID_0) {
        hal_cmu_spi_set_freq(periph_freq);
#endif
#ifdef CHIP_HAS_SPILCD
      } else if (id == HAL_SPI_ID_SLCD) {
        hal_cmu_slcd_set_freq(periph_freq);
#endif
      }
    }
  }
#endif // !SPI_ROM_ONLY
}

static void hal_spi_disable_id(enum HAL_SPI_ID_T id) {
  spi[id]->SSPCR1 &= ~SPI_SSPCR1_SSE;
}

static void POSSIBLY_UNUSED hal_spi_get_ctrl_id(enum HAL_SPI_ID_T id,
                                                struct HAL_SPI_CTRL_T *ctrl) {
  ctrl->sspcr0_tx = spi[id]->SSPCR0;
  ctrl->sspcr1 = spi[id]->SSPCR1;
  ctrl->sspcpsr = spi[id]->SSPCPSR;
  ctrl->sspdmacr = spi[id]->SSPDMACR;
  ctrl->ssprxcr_tx = spi[id]->SSPRXCR;
}

static int NOINLINE hal_spi_open_id(enum HAL_SPI_ID_T id,
                                    const struct HAL_SPI_CFG_T *cfg,
                                    struct HAL_SPI_CTRL_T *ctrl) {
  int ret;
  struct HAL_SPI_CTRL_T ctrl_regs;
  bool cfg_clk = true;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);

  if (ctrl == NULL) {
    ctrl = &ctrl_regs;
  }

  ret = hal_spi_init_ctrl(cfg, ctrl);
  if (ret) {
    return ret;
  }

#ifndef SPI_ROM_ONLY
  if (!spi_init_done) {
    spi_init_done = true;
    for (int i = HAL_SPI_ID_INTERNAL; i < HAL_SPI_ID_QTY; i++) {
      spi_txdma_chan[i] = HAL_DMA_CHAN_NONE;
      spi_rxdma_chan[i] = HAL_DMA_CHAN_NONE;
    }
  }

  if (spi_cs_map[id]) {
    cfg_clk = false;
  }
  spi_cs_map[id] |= (1 << cfg->cs);
#endif

  if (cfg_clk) {
    hal_cmu_clock_enable(spi_mod[id].mod);
    hal_cmu_clock_enable(spi_mod[id].apb);
    hal_cmu_reset_clear(spi_mod[id].mod);
    hal_cmu_reset_clear(spi_mod[id].apb);
  }

  hal_spi_enable_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);

  return 0;
}

static int POSSIBLY_UNUSED hal_spi_close_id(enum HAL_SPI_ID_T id, uint32_t cs) {
  int ret = 0;
  bool cfg_clk = true;

#ifndef SPI_ROM_ONLY
  if (spi_cs_map[id] & (1 << cs)) {
    spi_cs_map[id] &= ~(1 << cs);
#if (CHIP_SPI_VER >= 2)
    if (spi_cs_map[id]) {
      cfg_clk = false;
    }
#endif
  } else {
    ret = 1;
    cfg_clk = false;
  }
#endif

  if (cfg_clk) {
    hal_spi_disable_id(id);

    hal_cmu_reset_set(spi_mod[id].apb);
    hal_cmu_reset_set(spi_mod[id].mod);
    hal_cmu_clock_disable(spi_mod[id].apb);
    hal_cmu_clock_disable(spi_mod[id].mod);
  }

  return ret;
}

static void POSSIBLY_UNUSED hal_spi_set_cs_id(enum HAL_SPI_ID_T id,
                                              uint32_t cs) {
  spi[id]->SSPCR1 = SET_BITFIELD(spi[id]->SSPCR1, SPI_SLAVE_ID, cs);
}

static bool hal_spi_busy_id(enum HAL_SPI_ID_T id) {
  return ((spi[id]->SSPCR1 & SPI_SSPCR1_SSE) &&
          (spi[id]->SSPSR & SPI_SSPSR_BSY));
}

static void hal_spi_enable_slave_output_id(enum HAL_SPI_ID_T id) {
  if (spi[id]->SSPCR1 & SPI_SSPCR1_MS) {
    spi[id]->SSPCR1 &= ~SPI_SSPCR1_SOD;
  }
}

static void hal_spi_disable_slave_output_id(enum HAL_SPI_ID_T id) {
  if (spi[id]->SSPCR1 & SPI_SSPCR1_MS) {
    spi[id]->SSPCR1 |= SPI_SSPCR1_SOD;
  }
}

static int hal_spi_send_id(enum HAL_SPI_ID_T id, const void *data,
                           uint32_t len) {
  uint8_t cnt;
  uint32_t sent, value;
  int ret;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);
  SPI_ASSERT((spi[id]->SSPDMACR & SPI_SSPDMACR_TXDMAE) == 0,
             "TX-DMA configured on SPI %d", id);

  cnt = get_frame_bytes(id);

  if (len == 0 || (len & (cnt - 1)) != 0) {
    return -1;
  }

  sent = 0;

  hal_spi_enable_slave_output_id(id);

  while (sent < len) {
    if ((spi[id]->SSPCR1 & SPI_SSPCR1_SSE) == 0) {
      break;
    }
    if (spi[id]->SSPSR & SPI_SSPSR_TNF) {
      value = 0;
      copy_frame_from_bytes(&value, (uint8_t *)data + sent, cnt);
      spi[id]->SSPDR = value;
      sent += cnt;
    }
  }

  if (sent >= len) {
    ret = 0;
  } else {
    ret = 1;
  }

  while (hal_spi_busy_id(id))
    ;
  hal_spi_disable_slave_output_id(id);

  return ret;
}

static int hal_spi_recv_id(enum HAL_SPI_ID_T id, const void *cmd, void *data,
                           uint32_t len) {
  uint8_t cnt;
  uint32_t sent, recv, value;
  int ret;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);
  SPI_ASSERT(
      (spi[id]->SSPDMACR & (SPI_SSPDMACR_TXDMAE | SPI_SSPDMACR_RXDMAE)) == 0,
      "RX/TX-DMA configured on SPI %d", id);

  cnt = get_frame_bytes(id);

  if (len == 0 || (len & (cnt - 1)) != 0) {
    return -1;
  }

  // Rx transaction should start from idle state
  if (spi[id]->SSPSR & SPI_SSPSR_BSY) {
    return -11;
  }

  sent = 0;
  recv = 0;

  // Flush the RX FIFO by reset or CPU read
  while (spi[id]->SSPSR & SPI_SSPSR_RNE) {
    spi[id]->SSPDR;
  }
  spi[id]->SSPICR = ~0UL;

  hal_spi_enable_slave_output_id(id);

  while (recv < len || sent < len) {
    if ((spi[id]->SSPCR1 & SPI_SSPCR1_SSE) == 0) {
      break;
    }
    if (sent < len && (spi[id]->SSPSR & SPI_SSPSR_TNF)) {
      value = 0;
      copy_frame_from_bytes(&value, (uint8_t *)cmd + sent, cnt);
      spi[id]->SSPDR = value;
      sent += cnt;
    }
    if (recv < len && (spi[id]->SSPSR & SPI_SSPSR_RNE)) {
      value = spi[id]->SSPDR;
      copy_bytes_from_frame((uint8_t *)data + recv, value, cnt);
      recv += cnt;
    }
  }

  if (recv >= len && sent >= len) {
    ret = 0;
  } else {
    ret = 1;
  }

  while (hal_spi_busy_id(id))
    ;
  hal_spi_disable_slave_output_id(id);

  return ret;
}

#ifdef SPI_ROM_ONLY

//------------------------------------------------------------
// ISPI ROM functions
//------------------------------------------------------------

int hal_ispi_rom_open(const struct HAL_SPI_CFG_T *cfg) {
  SPI_ASSERT(cfg->tx_bits == cfg->rx_bits && cfg->rx_frame_bits == 0,
             "ISPI_ROM: Bad bits cfg");

  return hal_spi_open_id(HAL_SPI_ID_INTERNAL, cfg, NULL);
}

void hal_ispi_rom_activate_cs(uint32_t cs) {
  SPI_ASSERT(cs < HAL_SPI_CS_QTY, "ISPI_ROM: SPI cs bad: %d", cs);

  hal_spi_set_cs_id(HAL_SPI_ID_INTERNAL, cs);
}

int hal_ispi_rom_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_INTERNAL); }

int hal_ispi_rom_send(const void *data, uint32_t len) {
  int ret;

  ret = hal_spi_send_id(HAL_SPI_ID_INTERNAL, data, len);

  return ret;
}

int hal_ispi_rom_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  ret = hal_spi_recv_id(HAL_SPI_ID_INTERNAL, cmd, data, len);

  return ret;
}

#ifdef CHIP_HAS_SPIPHY
//------------------------------------------------------------
// SPI PHY ROM functions
//------------------------------------------------------------

int hal_spiphy_rom_open(const struct HAL_SPI_CFG_T *cfg) {
  SPI_ASSERT(cfg->tx_bits == cfg->rx_bits && cfg->rx_frame_bits == 0,
             "SPIPHY_ROM: Bad bits cfg");

  return hal_spi_open_id(HAL_SPI_ID_PHY, cfg, NULL);
}

int hal_spiphy_rom_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_PHY); }

int hal_spiphy_rom_send(const void *data, uint32_t len) {
  int ret;

  ret = hal_spi_send_id(HAL_SPI_ID_PHY, data, len);

  return ret;
}

int hal_spiphy_rom_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  ret = hal_spi_recv_id(HAL_SPI_ID_PHY, cmd, data, len);

  return ret;
}
#endif

#else // !SPI_ROM_ONLY

static int hal_spi_activate_cs_id(enum HAL_SPI_ID_T id, uint32_t cs) {
  struct HAL_SPI_CTRL_T *ctrl = NULL;

  SPI_ASSERT(cs < HAL_SPI_CS_QTY, "SPI cs bad: %d", cs);
  SPI_ASSERT(spi_cs_map[id] & (1 << cs), "SPI cs not opened: %d", cs);

#if (CHIP_SPI_VER >= 2)
  if (0) {
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    spi0_cs = cs;
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    spilcd_cs = cs;
#endif
  }
#endif

  if (0) {
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    ctrl = &spi0_ctrl[spi0_cs];
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    ctrl = &spilcd_ctrl[spilcd_cs];
#endif
  }
  if (ctrl) {
    hal_spi_enable_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);
  }

  return 0;
}

static int POSSIBLY_UNUSED hal_spi_enable_and_send_id(
    enum HAL_SPI_ID_T id, const struct HAL_SPI_CTRL_T *ctrl, const void *data,
    uint32_t len) {
  int ret;
  struct HAL_SPI_CTRL_T saved;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);

  if (set_bool_flag(&in_use[id])) {
    return -31;
  }

  hal_spi_get_ctrl_id(id, &saved);
  hal_spi_enable_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);
  ret = hal_spi_send_id(id, data, len);
  hal_spi_enable_id(id, &saved, HAL_SPI_XFER_TYPE_SEND);

  clear_bool_flag(&in_use[id]);

  return ret;
}

static int POSSIBLY_UNUSED hal_spi_enable_and_recv_id(
    enum HAL_SPI_ID_T id, const struct HAL_SPI_CTRL_T *ctrl, const void *cmd,
    void *data, uint32_t len) {
  int ret;
  struct HAL_SPI_CTRL_T saved;

  if (set_bool_flag(&in_use[id])) {
    return -31;
  }

  hal_spi_get_ctrl_id(id, &saved);
  hal_spi_enable_id(id, ctrl, HAL_SPI_XFER_TYPE_RECV);
  ret = hal_spi_recv_id(id, cmd, data, len);
  hal_spi_enable_id(id, &saved, HAL_SPI_XFER_TYPE_SEND);

  clear_bool_flag(&in_use[id]);

  return ret;
}

static void hal_spi_txdma_handler(uint8_t chan, uint32_t remains,
                                  uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_SPI_ID_T id;
  uint32_t lock;

  lock = int_lock();
  for (id = HAL_SPI_ID_INTERNAL; id < HAL_SPI_ID_QTY; id++) {
    if (spi_txdma_chan[id] == chan) {
      spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
      break;
    }
  }
  int_unlock(lock);

  if (id >= HAL_SPI_ID_QTY) {
    return;
  }

  hal_gpdma_free_chan(chan);

  clear_bool_flag(&in_use[id]);

  if (spi_txdma_handler[id]) {
    spi_txdma_handler[id](error);
  }
}

static int hal_spi_dma_send_id(enum HAL_SPI_ID_T id, const void *data,
                               uint32_t len, HAL_SPI_DMA_HANDLER_T handler) {
  uint8_t cnt;
  enum HAL_DMA_RET_T ret;
  struct HAL_DMA_CH_CFG_T dma_cfg;
  enum HAL_DMA_WDITH_T dma_width;
  uint32_t lock;
  enum HAL_DMA_PERIPH_T dst_periph;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);
  SPI_ASSERT((spi[id]->SSPDMACR & SPI_SSPDMACR_TXDMAE),
             "TX-DMA not configured on SPI %d", id);

  spi_txdma_handler[id] = handler;

  cnt = get_frame_bytes(id);

  if ((len & (cnt - 1)) != 0) {
    return -1;
  }
  if (((uint32_t)data & (cnt - 1)) != 0) {
    return -2;
  }

  // Tx transaction should start from idle state for SPI mode 1 and 3 (SPH=1)
  if ((spi[id]->SSPCR0 & SPI_SSPCR0_SPH) && (spi[id]->SSPSR & SPI_SSPSR_BSY)) {
    return -11;
  }

  if (id == HAL_SPI_ID_INTERNAL) {
    dst_periph = HAL_GPDMA_ISPI_TX;
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    dst_periph = HAL_GPDMA_SPI_TX;
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    dst_periph = HAL_GPDMA_SPILCD_TX;
#endif
  } else {
    return -12;
  }

  lock = int_lock();
  if (spi_txdma_chan[id] != HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return -3;
  }
  spi_txdma_chan[id] = hal_gpdma_get_chan(dst_periph, HAL_DMA_HIGH_PRIO);
  if (spi_txdma_chan[id] == HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return -4;
  }
  int_unlock(lock);

  if (cnt == 1) {
    dma_width = HAL_DMA_WIDTH_BYTE;
  } else if (cnt == 2) {
    dma_width = HAL_DMA_WIDTH_HALFWORD;
  } else {
    dma_width = HAL_DMA_WIDTH_WORD;
  }

  memset(&dma_cfg, 0, sizeof(dma_cfg));
  dma_cfg.ch = spi_txdma_chan[id];
  dma_cfg.dst = 0; // useless
  dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
  dma_cfg.dst_periph = dst_periph;
  dma_cfg.dst_width = dma_width;
  dma_cfg.handler = handler ? hal_spi_txdma_handler : NULL;
  dma_cfg.src = (uint32_t)data;
  dma_cfg.src_bsize = HAL_DMA_BSIZE_16;
  // dma_cfg.src_periph = HAL_GPDMA_PERIPH_QTY; // useless
  dma_cfg.src_tsize = len / cnt;
  dma_cfg.src_width = dma_width;
  dma_cfg.try_burst = 0;
  dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;

  hal_spi_enable_slave_output_id(id);

  ret = hal_gpdma_start(&dma_cfg);
  if (ret != HAL_DMA_OK) {
    hal_spi_disable_slave_output_id(id);
    return -5;
  }

  if (handler == NULL) {
    while ((spi[id]->SSPCR1 & SPI_SSPCR1_SSE) &&
           hal_gpdma_chan_busy(spi_txdma_chan[id]))
      ;
    hal_gpdma_free_chan(spi_txdma_chan[id]);
    spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
    while (hal_spi_busy_id(id))
      ;
    hal_spi_disable_slave_output_id(id);
  }

  return 0;
}

void hal_spi_stop_dma_send_id(enum HAL_SPI_ID_T id) {
  uint32_t lock;
  uint8_t tx_chan;

  lock = int_lock();
  tx_chan = spi_txdma_chan[id];
  spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
  int_unlock(lock);

  if (tx_chan != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(tx_chan);
    hal_gpdma_free_chan(tx_chan);
  }

  clear_bool_flag(&in_use[id]);
}

static void hal_spi_rxdma_handler(uint8_t chan, uint32_t remains,
                                  uint32_t error, struct HAL_DMA_DESC_T *lli) {
  enum HAL_SPI_ID_T id;
  uint32_t lock;
  uint8_t tx_chan = HAL_DMA_CHAN_NONE;
  struct HAL_SPI_CTRL_T *ctrl = NULL;

  lock = int_lock();
  for (id = HAL_SPI_ID_INTERNAL; id < HAL_SPI_ID_QTY; id++) {
    if (spi_rxdma_chan[id] == chan) {
      tx_chan = spi_txdma_chan[id];
      spi_rxdma_chan[id] = HAL_DMA_CHAN_NONE;
      spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
      break;
    }
  }
  int_unlock(lock);

  if (id >= HAL_SPI_ID_QTY) {
    return;
  }

  hal_gpdma_free_chan(chan);
  hal_gpdma_cancel(tx_chan);
  hal_gpdma_free_chan(tx_chan);

  if (0) {
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    ctrl = &spi0_ctrl[spi0_cs];
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    ctrl = &spilcd_ctrl[spilcd_cs];
#endif
  }
  if (ctrl) {
    hal_spi_set_xfer_type_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);
  }
  clear_bool_flag(&in_use[id]);

  if (spi_rxdma_handler[id]) {
    spi_rxdma_handler[id](error);
  }
}

static int hal_spi_dma_recv_id(enum HAL_SPI_ID_T id, const void *cmd,
                               void *data, uint32_t len,
                               HAL_SPI_DMA_HANDLER_T handler) {
  uint8_t cnt;
  enum HAL_DMA_RET_T ret;
  struct HAL_DMA_CH_CFG_T dma_cfg;
  enum HAL_DMA_WDITH_T dma_width;
  uint32_t lock;
  int result;
  enum HAL_DMA_PERIPH_T dst_periph, src_periph;
  struct HAL_SPI_CTRL_T *ctrl = NULL;

  // SPI_ASSERT(id < HAL_SPI_ID_QTY, invalid_id, id);
  SPI_ASSERT(
      (spi[id]->SSPDMACR & (SPI_SSPDMACR_TXDMAE | SPI_SSPDMACR_RXDMAE)) ==
          (SPI_SSPDMACR_TXDMAE | SPI_SSPDMACR_RXDMAE),
      "RX/TX-DMA not configured on SPI %d", id);

  spi_rxdma_handler[id] = handler;

  result = 0;

  cnt = get_frame_bytes(id);

  if ((len & (cnt - 1)) != 0) {
    return -1;
  }
  if (((uint32_t)data & (cnt - 1)) != 0) {
    return -2;
  }

  // Rx transaction should start from idle state
  if (spi[id]->SSPSR & SPI_SSPSR_BSY) {
    return -11;
  }

  if (id == HAL_SPI_ID_INTERNAL) {
    src_periph = HAL_GPDMA_ISPI_RX;
    dst_periph = HAL_GPDMA_ISPI_TX;
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    src_periph = HAL_GPDMA_SPI_RX;
    dst_periph = HAL_GPDMA_SPI_TX;
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    src_periph = HAL_GPDMA_SPILCD_RX;
    dst_periph = HAL_GPDMA_SPILCD_TX;
#endif
  } else {
    return -12;
  }

  lock = int_lock();
  if (spi_txdma_chan[id] != HAL_DMA_CHAN_NONE ||
      spi_rxdma_chan[id] != HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return -3;
  }
  spi_txdma_chan[id] = hal_gpdma_get_chan(dst_periph, HAL_DMA_HIGH_PRIO);
  if (spi_txdma_chan[id] == HAL_DMA_CHAN_NONE) {
    int_unlock(lock);
    return -4;
  }
  spi_rxdma_chan[id] = hal_gpdma_get_chan(src_periph, HAL_DMA_HIGH_PRIO);
  if (spi_rxdma_chan[id] == HAL_DMA_CHAN_NONE) {
    hal_gpdma_free_chan(spi_txdma_chan[id]);
    spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
    int_unlock(lock);
    return -5;
  }
  int_unlock(lock);

  if (cnt == 1) {
    dma_width = HAL_DMA_WIDTH_BYTE;
  } else if (cnt == 2) {
    dma_width = HAL_DMA_WIDTH_HALFWORD;
  } else {
    dma_width = HAL_DMA_WIDTH_WORD;
  }

  memset(&dma_cfg, 0, sizeof(dma_cfg));
  dma_cfg.ch = spi_rxdma_chan[id];
  dma_cfg.dst = (uint32_t)data;
  dma_cfg.dst_bsize = HAL_DMA_BSIZE_16;
  // dma_cfg.dst_periph = HAL_GPDMA_PERIPH_QTY; // useless
  dma_cfg.dst_width = dma_width;
  dma_cfg.handler = handler ? hal_spi_rxdma_handler : NULL;
  dma_cfg.src = 0; // useless
  dma_cfg.src_periph = src_periph;
  dma_cfg.src_bsize = HAL_DMA_BSIZE_4;
  dma_cfg.src_tsize = len / cnt;
  dma_cfg.src_width = dma_width;
  dma_cfg.try_burst = 0;
  dma_cfg.type = HAL_DMA_FLOW_P2M_DMA;

  // Flush the RX FIFO by reset or DMA read (CPU read is forbidden when DMA is
  // enabled)
  if (spi[id]->SSPSR & SPI_SSPSR_RNE) {
    // Reset SPI MODULE might cause the increment of the FIFO pointer
    hal_cmu_reset_pulse(spi_mod[id].mod);
    // Reset SPI APB will reset the FIFO pointer
    hal_cmu_reset_pulse(spi_mod[id].apb);
    if (0) {
#ifdef CHIP_HAS_SPI
    } else if (id == HAL_SPI_ID_0) {
      ctrl = &spi0_ctrl[spi0_cs];
#endif
#ifdef CHIP_HAS_SPILCD
    } else if (id == HAL_SPI_ID_SLCD) {
      ctrl = &spilcd_ctrl[spilcd_cs];
#endif
    }
    if (ctrl) {
      // hal_spi_set_xfer_type_id() is not enough, for all the registers have
      // been reset by APB reset
      hal_spi_enable_id(id, ctrl, HAL_SPI_XFER_TYPE_RECV);
    }
  }
  spi[id]->SSPICR = ~0UL;

  ret = hal_gpdma_start(&dma_cfg);
  if (ret != HAL_DMA_OK) {
    result = -8;
    goto _exit;
  }

  dma_cfg.ch = spi_txdma_chan[id];
  dma_cfg.dst = 0; // useless
  dma_cfg.dst_bsize = HAL_DMA_BSIZE_4;
  dma_cfg.dst_periph = dst_periph;
  dma_cfg.dst_width = dma_width;
  dma_cfg.handler = NULL;
  dma_cfg.src = (uint32_t)cmd;
  dma_cfg.src_bsize = HAL_DMA_BSIZE_16;
  // dma_cfg.src_periph = HAL_GPDMA_PERIPH_QTY; // useless
  dma_cfg.src_tsize = len / cnt;
  dma_cfg.src_width = dma_width;
  dma_cfg.try_burst = 0;
  dma_cfg.type = HAL_DMA_FLOW_M2P_DMA;

  hal_spi_enable_slave_output_id(id);

  ret = hal_gpdma_start(&dma_cfg);
  if (ret != HAL_DMA_OK) {
    result = -9;
    goto _exit;
  }

  if (handler == NULL) {
    while ((spi[id]->SSPCR1 & SPI_SSPCR1_SSE) &&
           hal_gpdma_chan_busy(spi_rxdma_chan[id]))
      ;
  }

_exit:
  if (result || handler == NULL) {
    hal_gpdma_cancel(spi_txdma_chan[id]);
    hal_gpdma_free_chan(spi_txdma_chan[id]);
    spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;

    while (hal_spi_busy_id(id))
      ;
    hal_spi_disable_slave_output_id(id);

    hal_gpdma_cancel(spi_rxdma_chan[id]);
    hal_gpdma_free_chan(spi_rxdma_chan[id]);
    spi_rxdma_chan[id] = HAL_DMA_CHAN_NONE;

    if (ctrl) {
      hal_spi_set_xfer_type_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);
    }
  }

  return 0;
}

void hal_spi_stop_dma_recv_id(enum HAL_SPI_ID_T id) {
  uint32_t lock;
  uint8_t rx_chan, tx_chan;
  struct HAL_SPI_CTRL_T *ctrl = NULL;

  lock = int_lock();
  rx_chan = spi_rxdma_chan[id];
  spi_rxdma_chan[id] = HAL_DMA_CHAN_NONE;
  tx_chan = spi_txdma_chan[id];
  spi_txdma_chan[id] = HAL_DMA_CHAN_NONE;
  int_unlock(lock);

  if (rx_chan == HAL_DMA_CHAN_NONE && tx_chan == HAL_DMA_CHAN_NONE) {
    return;
  }

  if (rx_chan != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(rx_chan);
    hal_gpdma_free_chan(rx_chan);
  }
  if (tx_chan != HAL_DMA_CHAN_NONE) {
    hal_gpdma_cancel(tx_chan);
    hal_gpdma_free_chan(tx_chan);
  }

  if (0) {
#ifdef CHIP_HAS_SPI
  } else if (id == HAL_SPI_ID_0) {
    ctrl = &spi0_ctrl[spi0_cs];
#endif
#ifdef CHIP_HAS_SPILCD
  } else if (id == HAL_SPI_ID_SLCD) {
    ctrl = &spilcd_ctrl[spilcd_cs];
#endif
  }
  if (ctrl) {
    hal_spi_set_xfer_type_id(id, ctrl, HAL_SPI_XFER_TYPE_SEND);
  }
  clear_bool_flag(&in_use[id]);
}

//------------------------------------------------------------
// ISPI functions
//------------------------------------------------------------

int hal_ispi_open(const struct HAL_SPI_CFG_T *cfg) {
  SPI_ASSERT(cfg->tx_bits == cfg->rx_bits && cfg->rx_frame_bits == 0,
             "ISPI: Bad bits cfg");

  return hal_spi_open_id(HAL_SPI_ID_INTERNAL, cfg, NULL);
}

int hal_ispi_close(uint32_t cs) {
  return hal_spi_close_id(HAL_SPI_ID_INTERNAL, cs);
}

void hal_ispi_activate_cs(uint32_t cs) {
  SPI_ASSERT(cs < HAL_SPI_CS_QTY, "ISPI: SPI cs bad: %d", cs);

  hal_spi_set_cs_id(HAL_SPI_ID_INTERNAL, cs);
}

int hal_ispi_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_INTERNAL); }

int hal_ispi_send(const void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_INTERNAL])) {
    return -31;
  }

  ret = hal_spi_send_id(HAL_SPI_ID_INTERNAL, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_INTERNAL]);

  return ret;
}

int hal_ispi_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_INTERNAL])) {
    return -31;
  }

  ret = hal_spi_recv_id(HAL_SPI_ID_INTERNAL, cmd, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_INTERNAL]);

  return ret;
}

int hal_ispi_dma_send(const void *data, uint32_t len,
                      HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_INTERNAL])) {
    return -31;
  }

  ret = hal_spi_dma_send_id(HAL_SPI_ID_INTERNAL, data, len, handler);

  if (ret || handler == NULL) {
    clear_bool_flag(&in_use[HAL_SPI_ID_INTERNAL]);
  }

  return ret;
}

int hal_ispi_dma_recv(const void *cmd, void *data, uint32_t len,
                      HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_INTERNAL])) {
    return -31;
  }

  ret = hal_spi_dma_recv_id(HAL_SPI_ID_INTERNAL, cmd, data, len, handler);

  if (ret || handler == NULL) {
    clear_bool_flag(&in_use[HAL_SPI_ID_INTERNAL]);
  }

  return ret;
}

void hal_ispi_stop_dma_send(void) {
  hal_spi_stop_dma_send_id(HAL_SPI_ID_INTERNAL);
}

void hal_ispi_stop_dma_recv(void) {
  hal_spi_stop_dma_recv_id(HAL_SPI_ID_INTERNAL);
}

#ifdef CHIP_HAS_SPI
//------------------------------------------------------------
// SPI peripheral functions
//------------------------------------------------------------

int hal_spi_open(const struct HAL_SPI_CFG_T *cfg) {
  int ret;
  uint32_t lock;

  if (cfg->cs >= HAL_SPI_CS_QTY) {
    return -1;
  }

  lock = int_lock();

  ret = hal_spi_open_id(HAL_SPI_ID_0, cfg, &spi0_ctrl[cfg->cs]);

#if (CHIP_SPI_VER >= 2)
  if (ret == 0) {
    spi0_cs = cfg->cs;
  }
#endif

  int_unlock(lock);

  return ret;
}

int hal_spi_close(uint32_t cs) {
  int ret;
  uint32_t lock;

  lock = int_lock();

  ret = hal_spi_close_id(HAL_SPI_ID_0, cs);

#if (CHIP_SPI_VER >= 2)
  if (ret == 0 && spi0_cs == cs) {
    uint32_t lowest_cs;

    lowest_cs = __CLZ(__RBIT(spi_cs_map[HAL_SPI_ID_0]));
    if (lowest_cs < HAL_SPI_CS_QTY) {
      hal_spi_activate_cs_id(HAL_SPI_ID_0, lowest_cs);
    } else {
      lowest_cs = HAL_SPI_CS_0;
    }
    spi0_cs = lowest_cs;
  }
#endif

  int_unlock(lock);

  return ret;
}

int hal_spi_activate_cs(uint32_t cs) {
  return hal_spi_activate_cs_id(HAL_SPI_ID_0, cs);
}

int hal_spi_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_0); }

int hal_spi_send(const void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_0])) {
    return -31;
  }

  ret = hal_spi_send_id(HAL_SPI_ID_0, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_0]);

  return ret;
}

int hal_spi_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_0])) {
    return -31;
  }

  hal_spi_set_xfer_type_id(HAL_SPI_ID_0, &spi0_ctrl[spi0_cs],
                           HAL_SPI_XFER_TYPE_RECV);
  ret = hal_spi_recv_id(HAL_SPI_ID_0, cmd, data, len);
  hal_spi_set_xfer_type_id(HAL_SPI_ID_0, &spi0_ctrl[spi0_cs],
                           HAL_SPI_XFER_TYPE_SEND);

  clear_bool_flag(&in_use[HAL_SPI_ID_0]);

  return ret;
}

int hal_spi_dma_send(const void *data, uint32_t len,
                     HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_0])) {
    return -31;
  }

  ret = hal_spi_dma_send_id(HAL_SPI_ID_0, data, len, handler);

  if (ret || handler == NULL) {
    clear_bool_flag(&in_use[HAL_SPI_ID_0]);
  }

  return ret;
}

int hal_spi_dma_recv(const void *cmd, void *data, uint32_t len,
                     HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_0])) {
    return -31;
  }

  hal_spi_set_xfer_type_id(HAL_SPI_ID_0, &spi0_ctrl[spi0_cs],
                           HAL_SPI_XFER_TYPE_RECV);

  ret = hal_spi_dma_recv_id(HAL_SPI_ID_0, cmd, data, len, handler);

  if (ret || handler == NULL) {
    hal_spi_set_xfer_type_id(HAL_SPI_ID_0, &spi0_ctrl[spi0_cs],
                             HAL_SPI_XFER_TYPE_SEND);

    clear_bool_flag(&in_use[HAL_SPI_ID_0]);
  }

  return ret;
}

void hal_spi_stop_dma_send(void) { hal_spi_stop_dma_send_id(HAL_SPI_ID_0); }

void hal_spi_stop_dma_recv(void) { hal_spi_stop_dma_recv_id(HAL_SPI_ID_0); }

int hal_spi_enable_and_send(const struct HAL_SPI_CTRL_T *ctrl, const void *data,
                            uint32_t len) {
  return hal_spi_enable_and_send_id(HAL_SPI_ID_0, ctrl, data, len);
}

int hal_spi_enable_and_recv(const struct HAL_SPI_CTRL_T *ctrl, const void *cmd,
                            void *data, uint32_t len) {
  return hal_spi_enable_and_recv_id(HAL_SPI_ID_0, ctrl, cmd, data, len);
}
#endif // CHIP_HAS_SPI

#ifdef CHIP_HAS_SPILCD
//------------------------------------------------------------
// SPI LCD functions
//------------------------------------------------------------

int hal_spilcd_open(const struct HAL_SPI_CFG_T *cfg) {
  int ret;
  uint32_t lock;

  if (cfg->cs >= HAL_SPI_CS_QTY) {
    return -1;
  }

  lock = int_lock();

  ret = hal_spi_open_id(HAL_SPI_ID_SLCD, cfg, &spilcd_ctrl[cfg->cs]);

#if (CHIP_SPI_VER >= 2)
  if (ret == 0) {
    spilcd_cs = cfg->cs;
  }
#endif

  int_unlock(lock);

  return ret;
}

int hal_spilcd_close(uint32_t cs) {
  int ret;
  uint32_t lock;

  lock = int_lock();

  ret = hal_spi_close_id(HAL_SPI_ID_SLCD, cs);

#if (CHIP_SPI_VER >= 2)
  if (ret == 0 && spilcd_cs == cs) {
    uint32_t lowest_cs;

    lowest_cs = __CLZ(__RBIT(spi_cs_map[HAL_SPI_ID_SLCD]));
    if (lowest_cs < HAL_SPI_CS_QTY) {
      hal_spi_activate_cs_id(HAL_SPI_ID_SLCD, lowest_cs);
    } else {
      lowest_cs = HAL_SPI_CS_0;
    }
    spilcd_cs = lowest_cs;
  }
#endif

  int_unlock(lock);

  return ret;
}

int hal_spilcd_activate_cs(uint32_t cs) {
  return hal_spi_activate_cs_id(HAL_SPI_ID_SLCD, cs);
}

int hal_spilcd_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_SLCD); }

int hal_spilcd_send(const void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  ret = hal_spi_send_id(HAL_SPI_ID_SLCD, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);

  return ret;
}

int hal_spilcd_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  hal_spi_set_xfer_type_id(HAL_SPI_ID_SLCD, &spilcd_ctrl[spilcd_cs],
                           HAL_SPI_XFER_TYPE_RECV);
  ret = hal_spi_recv_id(HAL_SPI_ID_SLCD, cmd, data, len);
  hal_spi_set_xfer_type_id(HAL_SPI_ID_SLCD, &spilcd_ctrl[spilcd_cs],
                           HAL_SPI_XFER_TYPE_SEND);

  clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);

  return ret;
}

int hal_spilcd_dma_send(const void *data, uint32_t len,
                        HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  ret = hal_spi_dma_send_id(HAL_SPI_ID_SLCD, data, len, handler);

  if (ret || handler == NULL) {
    clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);
  }

  return ret;
}

int hal_spilcd_dma_recv(const void *cmd, void *data, uint32_t len,
                        HAL_SPI_DMA_HANDLER_T handler) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  hal_spi_set_xfer_type_id(HAL_SPI_ID_SLCD, &spilcd_ctrl[spilcd_cs],
                           HAL_SPI_XFER_TYPE_RECV);

  ret = hal_spi_dma_recv_id(HAL_SPI_ID_SLCD, cmd, data, len, handler);

  if (ret || handler == NULL) {
    hal_spi_set_xfer_type_id(HAL_SPI_ID_SLCD, &spilcd_ctrl[spilcd_cs],
                             HAL_SPI_XFER_TYPE_SEND);

    clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);
  }

  return ret;
}

void hal_spilcd_stop_dma_send(void) {
  hal_spi_stop_dma_send_id(HAL_SPI_ID_SLCD);
}

void hal_spilcd_stop_dma_recv(void) {
  hal_spi_stop_dma_recv_id(HAL_SPI_ID_SLCD);
}

int hal_spilcd_set_data_mode(void) {
  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  spi[HAL_SPI_ID_SLCD]->SSPCR1 |= SPI_LCD_DC_DATA;

  clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);
  return 0;
}

int hal_spilcd_set_cmd_mode(void) {
  if (set_bool_flag(&in_use[HAL_SPI_ID_SLCD])) {
    return -31;
  }

  spi[HAL_SPI_ID_SLCD]->SSPCR1 &= ~SPI_LCD_DC_DATA;

  clear_bool_flag(&in_use[HAL_SPI_ID_SLCD]);
  return 0;
}

int hal_spilcd_enable_and_send(const struct HAL_SPI_CTRL_T *ctrl,
                               const void *data, uint32_t len) {
  return hal_spi_enable_and_send_id(HAL_SPI_ID_SLCD, ctrl, data, len);
}

int hal_spilcd_enable_and_recv(const struct HAL_SPI_CTRL_T *ctrl,
                               const void *cmd, void *data, uint32_t len) {
  return hal_spi_enable_and_recv_id(HAL_SPI_ID_SLCD, ctrl, cmd, data, len);
}
#endif // CHIP_HAS_SPILCD

#ifdef CHIP_HAS_SPIPHY
//------------------------------------------------------------
// SPI PHY functions
//------------------------------------------------------------

int hal_spiphy_open(const struct HAL_SPI_CFG_T *cfg) {
  SPI_ASSERT(cfg->tx_bits == cfg->rx_bits && cfg->rx_frame_bits == 0,
             "SPIPHY: Bad bits cfg");

  return hal_spi_open_id(HAL_SPI_ID_PHY, cfg, NULL);
}

int hal_spiphy_close(uint32_t cs) {
  return hal_spi_close_id(HAL_SPI_ID_PHY, cs);
}

void hal_spiphy_activate_cs(uint32_t cs) {
  SPI_ASSERT(cs < HAL_SPI_CS_QTY, "SPIPHY: SPI cs bad: %d", cs);

  hal_spi_set_cs_id(HAL_SPI_ID_PHY, cs);
}

int hal_spiphy_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_PHY); }

int hal_spiphy_send(const void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_PHY])) {
    return -31;
  }

  ret = hal_spi_send_id(HAL_SPI_ID_PHY, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_PHY]);

  return ret;
}

int hal_spiphy_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_PHY])) {
    return -31;
  }

  ret = hal_spi_recv_id(HAL_SPI_ID_PHY, cmd, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_PHY]);

  return ret;
}
#endif // CHIP_HAS_SPIPHY

#ifdef CHIP_HAS_SPIDPD
//------------------------------------------------------------
// SPI DPD functions
//------------------------------------------------------------

int hal_spidpd_open(const struct HAL_SPI_CFG_T *cfg) {
  SPI_ASSERT(cfg->rx_frame_bits == 0, "SPIDPD: Bad bits cfg");

  return hal_spi_open_id(HAL_SPI_ID_DPD, cfg, &spidpd_ctrl);
}

int hal_spidpd_close(uint32_t cs) {
  return hal_spi_close_id(HAL_SPI_ID_DPD, cs);
}

void hal_spidpd_activate_cs(uint32_t cs) {
  SPI_ASSERT(cs < HAL_SPI_CS_QTY, "SPIDPD: SPI cs bad: %d", cs);

  hal_spi_set_cs_id(HAL_SPI_ID_DPD, cs);
}

int hal_spidpd_busy(void) { return hal_spi_busy_id(HAL_SPI_ID_DPD); }

int hal_spidpd_send(const void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_DPD])) {
    return -31;
  }

  ret = hal_spi_send_id(HAL_SPI_ID_DPD, data, len);

  clear_bool_flag(&in_use[HAL_SPI_ID_DPD]);

  return ret;
}

int hal_spidpd_recv(const void *cmd, void *data, uint32_t len) {
  int ret;

  if (set_bool_flag(&in_use[HAL_SPI_ID_DPD])) {
    return -31;
  }

  hal_spi_set_xfer_type_id(HAL_SPI_ID_DPD, &spidpd_ctrl,
                           HAL_SPI_XFER_TYPE_RECV);
  ret = hal_spi_recv_id(HAL_SPI_ID_DPD, cmd, data, len);
  hal_spi_set_xfer_type_id(HAL_SPI_ID_DPD, &spidpd_ctrl,
                           HAL_SPI_XFER_TYPE_SEND);

  clear_bool_flag(&in_use[HAL_SPI_ID_DPD]);

  return ret;
}
#endif // CHIP_HAS_SPIDPD

#endif // !SPI_ROM_ONLY
