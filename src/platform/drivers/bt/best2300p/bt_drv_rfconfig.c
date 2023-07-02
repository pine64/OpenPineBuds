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
#include "bt_drv.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "plat_types.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv_internal.h"
#include "bt_drv_reg_op.h"
#include "cmsis.h"
#include "hal_btdump.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "iqcorrect.h"
#include "pmu.h"
#include "tgt_hardware.h"

#define AUTO_CAL 0

#ifndef BT_RF_MAX_XTAL_TUNE_PPB
// Default 10 ppm/bit or 10000 ppb/bit
#define BT_RF_MAX_XTAL_TUNE_PPB 10000
#endif

#ifndef BT_RF_XTAL_TUNE_FACTOR
// Default 0.2 ppm/bit or 200 ppb/bit
#define BT_RF_XTAL_TUNE_FACTOR 200
#endif

#define DEFAULT_XTAL_FCAP 0x8080

#define XTAL_FCAP_NORMAL_SHIFT 0
#define XTAL_FCAP_NORMAL_MASK (0xFF << XTAL_FCAP_NORMAL_SHIFT)
#define XTAL_FCAP_NORMAL(n) BITFIELD_VAL(XTAL_FCAP_NORMAL, n)

#define BT_XTAL_CMOM_DR (1 << 13)

#define RF_REG_XTAL_FCAP 0xE9
#define RF_REG_XTAL_CMOM_DR 0xE8

/* ljh add for sync modify rf RF_REG_XTAL_FCAP value*/
#define SPI_TRIG_RX_NEG_TIMEOUT MS_TO_TICKS(3)

#define SPI_TRIG_NEG_TIMEOUT SPI_TRIG_RX_NEG_TIMEOUT

enum BIT_OFFSET_CMD_TYPE_T {
  BIT_OFFSET_CMD_STOP = 0,
  BIT_OFFSET_CMD_START,
  BIT_OFFSET_CMD_ACK,
};

static uint16_t xtal_fcap = DEFAULT_XTAL_FCAP;
static uint16_t init_xtal_fcap = DEFAULT_XTAL_FCAP;

// rampup start     1--->2->3 rampup ok
// rampdown start 3-->2-->1-->0  rampdown ok

// 3a06=3b3f
// for pa@1.8v
struct bt_drv_tx_table_t {
  uint16_t tbl[16][3];
};

struct RF_SYS_INIT_ITEM {
  uint16_t reg;
  uint16_t set;
  uint16_t mask;
  uint16_t delay;
};

static const struct RF_SYS_INIT_ITEM rf_sys_init_tbl[] = {
    {0xb5, 0x8000, 0x8000, 0},
    {0xc2, 0x7188, 0x7fff, 0},
#ifdef LBRT
    {0xc4, 0x0043, 0x0043, 0}, // enable lbrt adc clk[6]
#else
    {0xc4, 0x0003, 0x0003, 0},
#endif
};

// zhangzhd agc config use default 1216
#define REG_EB_VAL 0x000c
#define REG_181_VAL (0x00bf)
#define REG_EC_VAL 0x000d
#define REG_182_VAL 0x00bf
#define REG_ED_VAL 0x000e
#define REG_183_VAL 0x00bf
#define REG_EE_VAL 0x000b
#define REG_184_VAL 0x00bf
#define REG_EF_VAL 0x0007
#define REG_185_VAL 0x00bf
#define REG_F0_VAL 0x0007
#define REG_186_VAL 0x00af
#define REG_F1_VAL 0x0007
#define REG_187_VAL 0x009f
#define REG_F2_VAL 0x0007
#define REG_188_VAL 0x008f

// zhangzhd agc config end
const uint16_t rf_init_tbl_1[][3] = {
    {0x88, 0x8640, 0},
    {0x8b, 0x8a4a, 0}, // set rx flt cap,filter start up
#if defined(__FANG_HW_AGC_CFG_ADC__)
    {0xd1, 0x8401, 0}, // set ra adc gain -3db
    {0x8e, 0x0D28, 0}, // adc noise reduction
#else
    {0xd1, 0x8403, 0}, // set gain
    {0x8e, 0x0128, 0}, // adc noise reduction
#endif
    {0x90, 0x8e1f, 0}, // enlarge txflt BW
    {0x91, 0x05c0, 0}, // by walker 20180427
    {0x92, 0x668e, 0}, // update by  luobin 2019/3/22
    {0x97, 0x2523, 0}, // update by  luobin 2019.0523
    {0x98, 0x1324, 0}, // update by  walker 2019.01.14, modify for yield
    {0x9a, 0x4470, 0}, // div2 rc
    {0x9b, 0xfd52, 0}, // update by  walker 2018.10.24
    {0x9c, 0x180f, 0}, /////////luobin
    {0xa3, 0x0789, 0},
    {0xb0, 0x0000, 0},
    {0xb1, 0x0000, 0},
    {0xb3, 0x31f3, 0}, //
    {0xb4, 0x883c, 0},
    {0xb6, 0x3156, 0},
    {0xb7, 0x183f, 0}, // update by  walker 2018.07.30
    {0xb9, 0x8000, 0}, // cap3g6 off
    {0xba, 0x104e, 0},
#if defined(__FANG_LNA_CFG__)
    {0xac, 0x080e, 0}, // pre-charge set to 5us
    {0x85, 0x7f00, 0}, // NFMI RXADC LDO rise up
    // {0xc3,0x0068,0},//add by xrz increase pll cal time 2018/10/20
    {0xc3, 0x0044, 0},
#else
    {0xc3, 0x0050, 0},
#endif
    {0xc5, 0x4b50, 0}, // vco ictrl dr
    {0xc9, 0x3a08, 0}, // vco ictrl
    {0xd3, 0xc1c1, 0},
    {0xd4, 0x000f, 0},
    {0xd5, 0x4000, 0},
    {0xd6, 0x7980, 0},
    {0xe8, 0xe000, 0},
    {0xf3, 0x0c41, 0}, // by fang
    {0x1a6, 0x0600, 0},
    {0x1ae, 0x6a00, 0}, // fastlock en
    {0x1d4, 0x0000, 0},
    {0x1d7, 0xc4ff, 0}, // update by  walker 2018.10.10
    {0x1de, 0x2000, 0},
    {0x1df, 0x2087, 0},
    {0x1f4, 0x2241, 0}, // by walker 20180427
    {0x1fa, 0x03df, 0}, // rst needed
    {0x1fa, 0x83df, 0},
    {0xeb, REG_EB_VAL, 0}, // gain_idx:0
    {0x181, REG_181_VAL, 0},
    {0xec, REG_EC_VAL, 0}, // gain_idx:1
    {0x182, REG_182_VAL, 0},
    {0xed, REG_ED_VAL, 0}, // gain_idx:2
    {0x183, REG_183_VAL, 0},
    {0xee, REG_EE_VAL, 0}, // gain_idx:3
    {0x184, REG_184_VAL, 0},
    {0xef, REG_EF_VAL, 0}, // gain_idx:4
    {0x185, REG_185_VAL, 0},
    {0xf0, REG_F0_VAL, 0}, // gain_idx:5
    {0x186, REG_186_VAL, 0},
    {0xf1, REG_F1_VAL, 0}, // gain_idx:6
    {0x187, REG_187_VAL, 0},
    {0xf2, REG_F2_VAL, 0}, // gain_idx:7
    {0x188, REG_188_VAL, 0},
    {0x1d6, 0x7e58, 0}, // set lbrt rxflt dccal_ready=1
    {0x1c8, 0xdfc0, 0}, // en rfpll bias in lbrt mode
    {0x1c9, 0xa0cf, 0}, // en log bias
    {0x82, 0x0caa, 0},  // select rfpll and cplx_dir=0 and en rxflt test
    {0xf4, 0x2181, 0},  // set flt_cap to match flt_gain
    {0x86, 0xdc99, 0},  // set nfmi tmix
    {0xdf, 0x2006, 0},  // set nfmi tmix vcm
#ifdef __HW_AGC__
    {0x1c7, 0x007d, 0}, // open lbrt adc and vcm bias current
#endif
    {0x18c, 0x0000, 0}, // max tx gain lbrt
    // air @ LBRT 31.5m
    {0x1d8, 0x9162, 0}, // lbrt tx freqword
    {0x1d9, 0x9162, 0}, // lbrt tx freqword
    {0x1da, 0x9162, 0}, // lbrt tx freqword
    {0x1db, 0x9162, 0}, // lbrt tx freqword
    {0x1dc, 0x0000, 0}, // txlccap
    {0x1dd, 0x0000, 0}, // txlccap
    {0x1ed, 0x0000, 0}, // rxlccap
    {0x1ee, 0x0000, 0}, // rxlccap
    {0x1e9, 0x0a0a, 0}, // set rfpll divn in lbrt tx mode
    {0x1ea, 0x0a0a, 0},
    {0x1eb, 0x0a0a, 0}, // set rfpll divn in lbrt rx mode
    {0x1ec, 0x0a0a, 0},
    {0x1ef, 0x8a76, 0}, // lbrt rx freqword
    {0x1f0, 0x8a76, 0}, // lbrt rx freqword
    {0x1f1, 0x8a76, 0}, // lbrt rx freqword
    {0x1f2, 0x8a76, 0}, // lbrt rx freqword
    {0x87, 0x9f00, 0},  //{0x87,0x9f40,0}
    {0x81, 0x9207, 0},
    {0xce, 0xfc08, 0},
    {0x8A, 0x4EA4, 0}, //  use rfpll for nfmi adclk
    //[below for ibrt]
    {0x8c, 0x9100, 0},
    {0xab, 0x1c0c, 0},  // 2019.05.08 by luofei
    {0xa5, 0x0404, 0},  // rfpll pu time
    {0xa6, 0x2D20, 0},  // 2019.05.23 by luobin
    {0xa8, 0x2D22, 0},  // 2019.05.23 by luobin
    {0x1b7, 0x2c00, 0}, // 2019.05.08 by luofei
    {0x1ad, 0x9000, 0}, // 2M tx filter baseband
    {0xdc, 0x8820, 0},  // fa pa pwrup time
    {0x8f, 0x71b8, 0},  // tx dac by luobin
};

#ifdef __HW_AGC__
const uint16_t rf_init_tbl_1_hw_agc[][3] = // hw agc table
    {
        {0xad, 0xa04a, 1}, // hwagc en=1
        {0xCD, 0x0040, 0}, // default 0x0000
        {0xcf, 0x7f32, 0}, // lna gain dr=0 //default 0x0000
        {0xd0, 0xe91f, 0}, // i2v flt gain dr=0 //default 0x0000

        {0xeb, 0x0007, 0}, // gain_idx:0
        {0xec, 0x0007, 0}, // gain_idx:1
        {0xed, 0x0007, 0}, // gain_idx:2
        {0xee, 0x000d, 0}, // gain_idx:3
        {0xef, 0x000d, 0}, // gain_idx:4
        {0xf0, 0x000d, 0}, // gain_idx:5
        {0xf1, 0x000d, 0}, // gain_idx:6
        {0xf2, 0x000c, 0}, // gain_idx:7
};
#endif //__HW_AGC__

const uint16_t rf_init_tbl_1_sw_agc[][3] = // sw agc table
    {
        {0xad, 0xa00a, 1},     // hwagc en=0
        {0xCD, 0x0000, 0},     // default 0x0000
        {0xcf, 0x0000, 0},     // lna gain dr=0 //default 0x0000
        {0xd0, 0x0000, 0},     // i2v flt gain dr=0 //default 0x0000
        {0xeb, REG_EB_VAL, 0}, // gain_idx:0
        {0xec, REG_EC_VAL, 0}, // gain_idx:1
        {0xed, REG_ED_VAL, 0}, // gain_idx:2
        {0xee, REG_EE_VAL, 0}, // gain_idx:3
        {0xef, REG_EF_VAL, 0}, // gain_idx:4
        {0xf0, REG_F0_VAL, 0}, // gain_idx:5
        {0xf1, REG_F1_VAL, 0}, // gain_idx:6
        {0xf2, REG_F2_VAL, 0}, // gain_idx:7
};

uint32_t btdrv_rf_get_max_xtal_tune_ppb(void) {
  return BT_RF_MAX_XTAL_TUNE_PPB;
}

uint32_t btdrv_rf_get_xtal_tune_factor(void) { return BT_RF_XTAL_TUNE_FACTOR; }

void btdrv_rf_init_xtal_fcap(uint32_t fcap) {
  xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
  btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
  init_xtal_fcap = xtal_fcap;
}

uint32_t btdrv_rf_get_init_xtal_fcap(void) {
  return GET_BITFIELD(init_xtal_fcap, XTAL_FCAP_NORMAL);
}

uint32_t btdrv_rf_get_xtal_fcap(void) {
  return GET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL);
}

void btdrv_rf_set_xtal_fcap(uint32_t fcap, uint8_t is_direct) {
  xtal_fcap = SET_BITFIELD(xtal_fcap, XTAL_FCAP_NORMAL, fcap);
  btdrv_write_rf_reg(RF_REG_XTAL_FCAP, xtal_fcap);
}

int btdrv_rf_xtal_fcap_busy(uint8_t is_direct) { return 0; }

void btdrv_rf_bit_offset_track_enable(bool enable) { return; }

uint32_t btdrv_rf_bit_offset_get(void) { return 0; }

uint16_t btdrv_rf_bitoffset_get(uint8_t conidx) {
  uint16_t bitoffset;
  bitoffset = BTDIGITAL_REG(EM_BT_BITOFF_ADDR + conidx * BT_EM_SIZE) & 0x3ff;
  return bitoffset;
}

void btdrv_rf_log_delay_cal(void) {
  unsigned short read_value;
  unsigned short write_value;
  BT_DRV_TRACE(0, "btdrv_rf_log_delay_cal\n");

  btdrv_write_rf_reg(0xa7, 0x0028);

  btdrv_write_rf_reg(0xd4, 0x0000);
  btdrv_write_rf_reg(0xd5, 0x4002);
  //   *(volatile uint32_t*)(0xd02201e4) = 0x000a00b0;
  *(volatile uint32_t *)(0xd0340020) = 0x030e01c1;
  BT_DRV_TRACE(1, "0xd0340020 =%x\n", *(volatile uint32_t *)(0xd0340020));

  btdrv_delay(1);

  btdrv_write_rf_reg(0xd2, 0x5003);

  btdrv_delay(1);

  btdrv_read_rf_reg(0x1e2, &read_value);
  BT_DRV_TRACE(1, "0x1e2 read_value:%x\n", read_value);
  if (read_value == 0xff80) {
    btdrv_write_rf_reg(0xd3, 0xffff);
  } else {
    write_value = ((read_value >> 7) & 0x0001) | ((read_value & 0x007f) << 1) |
                  ((read_value & 0x8000) >> 7) | ((read_value & 0x7f00) << 1);
    BT_DRV_TRACE(1, "d3 write_value:%x\n", write_value);
    btdrv_write_rf_reg(0xd3, write_value);
  }
  btdrv_delay(1);

  //    *(volatile uint32_t*)(0xd02201e4) = 0x00000000;
  *(volatile uint32_t *)(0xd0340020) = 0x010e01c0;
  BT_DRV_TRACE(1, "0xd0340020 =%x\n", *(volatile uint32_t *)(0xd0340020));

  btdrv_write_rf_reg(0xd4, 0x000f);
  btdrv_write_rf_reg(0xd2, 0x1003);
  btdrv_write_rf_reg(0xd5, 0x4000);
}

void btdrv_rf_rx_gain_adjust_req(uint32_t user, bool lowgain) { return; }

// rf Image calib
void btdtv_rf_image_calib(void) {

  uint16_t read_val;
  // read calibrated val from efuse 0x05 register
  pmu_get_efuse(PMU_EFUSE_PAGE_SW_CFG, &read_val);
  // check if bit 11 has been set
  uint8_t calb_done_flag = ((read_val & 0x800) >> 11);
  if (calb_done_flag) {
    BT_DRV_TRACE(1, "EFUSE REG[5]=%x", read_val);
  } else {
    BT_DRV_TRACE(0, "EFUSE REG[5] rf image has not been calibrated!");
    return;
  }
  //[bit 12] calib flag
  uint8_t calib_val = ((read_val & 0x1000) >> 12);
  btdrv_read_rf_reg(0x9b, &read_val);
  read_val &= 0xfcff;

  if (calib_val == 0) {
    read_val |= 1 << 8;
  } else if (calib_val == 1) {
    read_val |= 1 << 9;
  }

  BT_DRV_TRACE(1, "write rf image calib val=%x in REG[0x9b]", read_val);
  btdrv_write_rf_reg(0x9b, read_val);
}

#ifdef TX_IQ_CAL
const uint16_t tx_cal_rfreg_set[][3] = {
    // iq cal path
    {0x1c4, 0xffcf, 0},
    {0x1c5, 0xf8ff, 0},
    {0x1c6, 0x007f, 0},
    {0x1c7, 0x03ff, 0},
    {0x1d6, 0x7a58, 0},
    {0x1d7, 0xc0f7, 0},
    {0x1f4, 0x2481, 0},
    {0x94, 0x11c3, 0},
    {0x95, 0xe5c0, 0},
    {0xd1, 0x846b, 0},
    {0xae, 0x0403, 0},
    {0x86, 0xccd9, 0},
    {0xf3, 0x0c75, 0},
    {0xf4, 0x0309, 0},
    {0xcf, 0x8090, 0},
    {0x82, 0x2cab, 0},

    // nfmi adc
    {0xc4, 0x00c3, 0},
    {0xae, 0x07ff, 0},
    {0x85, 0x470f, 0},
    {0x82, 0x4aab, 0},
    {0x82, 0xecab, 0},
    {0x87, 0xa211, 0},
    {0x87, 0xe211, 0},
    {0x84, 0xfff8, 0},
};

const uint16_t tx_cal_rfreg_store[][1] = {
    {0x1c4}, {0x1c5}, {0x1c6}, {0x1c7}, {0x1d6}, {0x1d7}, {0x1f4},
    {0x94},  {0x95},  {0xd1},  {0xae},  {0x86},  {0xf3},  {0xf4},
    {0xcf},  {0x82},  {0xc4},  {0x85},  {0x87},  {0x84},
};

int bt_iqimb_ini();
int bt_iqimb_test_ex(int mismatch_type);
void btdrv_tx_iq_cal(void) {
  uint8_t i;
  const uint16_t(*tx_cal_rfreg_set_p)[3];
  const uint16_t(*tx_cal_rfreg_store_p)[1];
  uint32_t reg_set_tbl_size;
  uint32_t reg_store_tbl_size;
  uint16_t value;
  uint32_t tx_cal_digreg_store[5];

  tx_cal_rfreg_store_p = &tx_cal_rfreg_store[0];
  reg_store_tbl_size =
      sizeof(tx_cal_rfreg_store) / sizeof(tx_cal_rfreg_store[0]);
  uint16_t tx_cal_rfreg_store[reg_store_tbl_size];
  BT_DRV_TRACE(0, "reg_store:\n");
  for (i = 0; i < reg_store_tbl_size; i++) {
    btdrv_read_rf_reg(tx_cal_rfreg_store_p[i][0], &value);
    tx_cal_rfreg_store[i] = value;
    BT_DRV_TRACE(2, "reg=%x,v=%x", tx_cal_rfreg_store_p[i][0], value);
  }
  tx_cal_digreg_store[0] = *(volatile uint32_t *)(0xd0330038);
  tx_cal_digreg_store[1] = *(volatile uint32_t *)(0xd0350364);
  tx_cal_digreg_store[2] = *(volatile uint32_t *)(0xd0350360);
  tx_cal_digreg_store[3] = *(volatile uint32_t *)(0xd035037c);
  tx_cal_digreg_store[4] = *(volatile uint32_t *)(0xd02201e4);
  BT_DRV_TRACE(1, "0xd0330038:%x\n", tx_cal_digreg_store[0]);
  BT_DRV_TRACE(1, "0xd0350364:%x\n", tx_cal_digreg_store[1]);
  BT_DRV_TRACE(1, "0xd0350360:%x\n", tx_cal_digreg_store[2]);
  BT_DRV_TRACE(1, "0xd035037c:%x\n", tx_cal_digreg_store[3]);
  BT_DRV_TRACE(1, "0xd02201e4:%x\n", tx_cal_digreg_store[4]);

  tx_cal_rfreg_set_p = &tx_cal_rfreg_set[0];
  reg_set_tbl_size = sizeof(tx_cal_rfreg_set) / sizeof(tx_cal_rfreg_set[0]);
  BT_DRV_TRACE(0, "reg_set:\n");
  for (i = 0; i < reg_set_tbl_size; i++) {
    btdrv_write_rf_reg(tx_cal_rfreg_set_p[i][0], tx_cal_rfreg_set_p[i][1]);
    if (tx_cal_rfreg_set_p[i][2] != 0)
      btdrv_delay(tx_cal_rfreg_set_p[i][2]); // delay
    btdrv_read_rf_reg(tx_cal_rfreg_set_p[i][0], &value);
    BT_DRV_TRACE(2, "reg=%x,v=%x", tx_cal_rfreg_set_p[i][0], value);
  }
  // iq cal cordic
  *(volatile uint32_t *)(0xd0330038) = 0x0020010d;
  btdrv_delay(1);
  *(volatile uint32_t *)(0xd0350364) = 0x002eb948;
  *(volatile uint32_t *)(0xd0350360) = 0x007fc240;
  *(volatile uint32_t *)(0xd035037c) = 0x00020405;
  *(volatile uint32_t *)(0xd02201e4) = 0x000a0000;
  BT_DRV_TRACE(1, "0xd0330038:%x\n", *(volatile uint32_t *)(0xd0330038));
  BT_DRV_TRACE(1, "0xd0350364:%x\n", *(volatile uint32_t *)(0xd0350364));
  BT_DRV_TRACE(1, "0xd0350360:%x\n", *(volatile uint32_t *)(0xd0350360));
  BT_DRV_TRACE(1, "0xd035037c:%x\n", *(volatile uint32_t *)(0xd035037c));
  BT_DRV_TRACE(1, "0xd02201e4:%x\n", *(volatile uint32_t *)(0xd02201e4));

  bt_iqimb_ini();
  bt_iqimb_test_ex(1);

  BT_DRV_TRACE(0, "reg_reset:\n");
  for (i = 0; i < reg_store_tbl_size; i++) {
    btdrv_write_rf_reg(tx_cal_rfreg_store_p[i][0], tx_cal_rfreg_store[i]);

    btdrv_read_rf_reg(tx_cal_rfreg_store_p[i][0], &value);
    BT_DRV_TRACE(2, "reg=%x,v=%x", tx_cal_rfreg_store_p[i][0], value);
  }
  *(volatile uint32_t *)(0xd0330038) = tx_cal_digreg_store[0];
  *(volatile uint32_t *)(0xd0350364) = tx_cal_digreg_store[1];
  *(volatile uint32_t *)(0xd0350360) = tx_cal_digreg_store[2];
  *(volatile uint32_t *)(0xd035037c) = tx_cal_digreg_store[3];
  *(volatile uint32_t *)(0xd02201e4) = tx_cal_digreg_store[4];
  BT_DRV_TRACE(1, "0xd0330038:%x\n", *(volatile uint32_t *)(0xd0330038));
  BT_DRV_TRACE(1, "0xd0350364:%x\n", *(volatile uint32_t *)(0xd0350364));
  BT_DRV_TRACE(1, "0xd0350360:%x\n", *(volatile uint32_t *)(0xd0350360));
  BT_DRV_TRACE(1, "0xd035037c:%x\n", *(volatile uint32_t *)(0xd035037c));
  BT_DRV_TRACE(1, "0xd02201e4:%x\n", *(volatile uint32_t *)(0xd02201e4));
}
#endif

uint8_t btdrv_rf_init(void) {
  uint16_t value;
  const uint16_t(*rf_init_tbl_p)[3];
  uint32_t tbl_size;
  // uint8_t ret;
  uint8_t i;

  for (i = 0; i < ARRAY_SIZE(rf_sys_init_tbl); i++) {
    btdrv_read_rf_reg(rf_sys_init_tbl[i].reg, &value);
    value = (value & ~rf_sys_init_tbl[i].mask) |
            (rf_sys_init_tbl[i].set & rf_sys_init_tbl[i].mask);
    if (rf_sys_init_tbl[i].delay) {
      btdrv_delay(rf_sys_init_tbl[i].delay);
    }
    btdrv_write_rf_reg(rf_sys_init_tbl[i].reg, value);
  }

  rf_init_tbl_p = &rf_init_tbl_1[0];
  tbl_size = sizeof(rf_init_tbl_1) / sizeof(rf_init_tbl_1[0]);

  for (i = 0; i < tbl_size; i++) {
    btdrv_write_rf_reg(rf_init_tbl_p[i][0], rf_init_tbl_p[i][1]);
    if (rf_init_tbl_p[i][2] != 0)
      btdrv_delay(rf_init_tbl_p[i][2]); // delay
  }

  bt_drv_tx_pwr_init();

#ifdef __HW_AGC__
  for (i = 0;
       i < sizeof(rf_init_tbl_1_hw_agc) / sizeof(rf_init_tbl_1_hw_agc[0]);
       i++) {
    btdrv_write_rf_reg(rf_init_tbl_1_hw_agc[i][0], rf_init_tbl_1_hw_agc[i][1]);
    if (rf_init_tbl_1_hw_agc[i][2] != 0)
      btdrv_delay(rf_init_tbl_1_hw_agc[i][2]); // delay
  }
#endif

  // need before rf log delay cal
  btdtv_rf_image_calib();

  btdrv_rf_log_delay_cal();

  btdrv_spi_trig_init();

#ifdef TX_IQ_CAL
  hal_btdump_clk_enable();
  bt_iq_calibration_setup();
  hal_btdump_clk_disable();
#endif

  return 1;
}

void bt_drv_rf_reset(void) {
  btdrv_write_rf_reg(0x80, 0xcafe);
  btdrv_write_rf_reg(0x80, 0x5fee);
}

static void bt_drv_switch_hw_sw_agc(enum BT_AGC_MODE_T agc_mode) {
  static enum BT_AGC_MODE_T agc_mode_bak = BT_AGC_MODE_NONE;
  uint16_t i = 0;

  if (agc_mode_bak != agc_mode) {
    agc_mode_bak = agc_mode;
    BT_DRV_TRACE(1, "BT_DRV:use AGC mode=%d[1:HW,0:SW]", agc_mode);
    uint32_t lock = int_lock_global();
    if (agc_mode == BT_AGC_MODE_HW) {
#ifdef __HW_AGC__
      bt_drv_reg_op_hw_sw_agc_select(BT_AGC_MODE_HW);
      for (i = 0;
           i < sizeof(rf_init_tbl_1_hw_agc) / sizeof(rf_init_tbl_1_hw_agc[0]);
           i++) {
        btdrv_write_rf_reg(rf_init_tbl_1_hw_agc[i][0],
                           rf_init_tbl_1_hw_agc[i][1]);
        if (rf_init_tbl_1_hw_agc[i][2] != 0)
          btdrv_delay(rf_init_tbl_1_hw_agc[i][2]); // delay
      }
#endif
    } else {
      bt_drv_reg_op_hw_sw_agc_select(BT_AGC_MODE_SW);
      for (i = 0;
           i < sizeof(rf_init_tbl_1_sw_agc) / sizeof(rf_init_tbl_1_sw_agc[0]);
           i++) {
        btdrv_write_rf_reg(rf_init_tbl_1_sw_agc[i][0],
                           rf_init_tbl_1_sw_agc[i][1]);
        if (rf_init_tbl_1_sw_agc[i][2] != 0)
          btdrv_delay(rf_init_tbl_1_sw_agc[i][2]); // delay
      }
    }
    int_unlock_global(lock);
  }
}

void bt_drv_select_agc_mode(enum BT_WORK_MODE_T mode) {
#ifdef __HYBIRD_AGC__
  enum BT_AGC_MODE_T agc_mode = BT_AGC_MODE_SW;
  switch (mode) {
  case BT_IDLE_MODE:
    agc_mode = BT_AGC_MODE_HW;
    break;
  case BT_A2DP_WORK_MODE:
  case BT_HFP_WORK_MODE:
    agc_mode = BT_AGC_MODE_SW;
    break;
  default:
    BT_DRV_TRACE(1, "BT_DRV:set error mork mode=%d", mode);
    break;
  }

  bt_drv_switch_hw_sw_agc(agc_mode);
#endif
}

static uint16_t efuse;
static int16_t rf18b_6_4, rf18b_10_8, rf18b_14_12;
#define TX_POWET_CALIB_FACTOR 0x2

static int check_btpower_efuse_invalid(void) {
  pmu_get_efuse(PMU_EFUSE_PAGE_BT_POWER, &efuse);
  BT_DRV_TRACE(1, "efuse_8=0x%x", efuse);

  rf18b_6_4 = (efuse & 0x70) >> 4;      // address_8 [6:4]
  rf18b_10_8 = (efuse & 0x700) >> 8;    // address_8 [10:8]
  rf18b_14_12 = (efuse & 0x7000) >> 12; // address_8 [14:12]

  if ((0 == efuse) || (rf18b_6_4 > 5) || (rf18b_10_8 > 5) ||
      (rf18b_14_12 > 5)) {
    BT_DRV_TRACE(0, "invalid efuse value.");
    return 0;
  }

  return 1;
}

void bt_drv_tx_pwr_init(void) {
  // ble txpower need modify ble tx idx @ bt_drv_config.c
  // modify bit4~7 to change ble tx gain
  btdrv_write_rf_reg(0x189, 0x0071); // min tx gain  2019.02.26
  btdrv_write_rf_reg(0x18a, 0x0071); // mid tx gain  2019.02.26
  if (0 == check_btpower_efuse_invalid())
    btdrv_write_rf_reg(0x18b, 0x0071); // max tx gain 2019.02.26
}

void bt_drv_tx_pwr_init_for_testmode(void) {
  // ble txpower need modify ble tx idx @ bt_drv_config.c
  // modify bit4~7 to change ble tx gain
  btdrv_write_rf_reg(0x189, 0x007a); // min tx gain  2019.02.26
  btdrv_write_rf_reg(0x18a, 0x0076); // mid tx gain  2019.02.26
  if (0 == check_btpower_efuse_invalid())
    btdrv_write_rf_reg(0x18b, 0x0071); // max tx gain 2019.02.26
}

void btdrv_txpower_calib(void) {
  uint16_t rf92_11_8;
  uint16_t tmp_val;
  int16_t average_value; // may be negative, so use signed numbers
  uint16_t read_value;

  uint16_t bit7_symbol, bit11_symbol, bit15_symbol;

  if (0 == check_btpower_efuse_invalid())
    return;

  bit7_symbol = (efuse & 0x80) >> 4;
  bit11_symbol = (efuse & 0x800) >> 8;
  bit15_symbol = (efuse & 0x8000) >> 12;
  rf92_11_8 = efuse & 0xf; // address_8 [3:0]

  BT_DRV_TRACE(3, "bit7_symbol=%d, bit11_symbol=%d, bit15_symbol=%d",
               bit7_symbol, bit11_symbol, bit15_symbol);
  BT_DRV_TRACE(4, "rf92_11_8=%x, rf18b_6_4=%x, rf18b_10_8=%x, rf18b_14_12=%x",
               rf92_11_8, rf18b_6_4, rf18b_10_8, rf18b_14_12);

  rf18b_6_4 = (bit7_symbol == 0) ? rf18b_6_4 : -rf18b_6_4;
  rf18b_10_8 = (bit11_symbol == 0) ? rf18b_10_8 : -rf18b_10_8;
  rf18b_14_12 = (bit15_symbol == 0) ? rf18b_14_12 : -rf18b_14_12;

  // set 0x92[11:8] begin
  btdrv_read_rf_reg(0x92, &tmp_val);
  BT_DRV_TRACE(1, "read reg 0x92 val=%x", tmp_val);
  tmp_val &= 0xf0ff;                      // clear [11:8]
  tmp_val |= ((rf92_11_8 & 0xffff) << 8); // set efuse[3:0] to rf_92 [11:8]
  BT_DRV_TRACE(2, "%d write reg 0x92 val=%x", __LINE__, tmp_val);
  btdrv_write_rf_reg(0x92, tmp_val);
  // set 0x92[11:8] end

  // set 0x18b[3:0] begin
  btdrv_read_rf_reg(0x18b, &read_value); // 0x18b

  average_value =
      (int16_t)(((float)(rf18b_6_4 + rf18b_10_8 + rf18b_14_12) / 6.f) + 0.5f);
  BT_DRV_TRACE(2, "calc average_value=0x%x, dec:%d", average_value,
               average_value);

  if (0 < average_value) {
    btdrv_read_rf_reg(0x92, &tmp_val);
    tmp_val &= 0x0fff; // clear [15:12]

    if (1 == average_value) {
      tmp_val |= ((0x8 & 0xffff) << 12);
    } else // average_value [2,~]
    {
      tmp_val |= ((0xA & 0xffff) << 12);
    }
    BT_DRV_TRACE(2, "%d write reg 0x92 val=%x", __LINE__, tmp_val);
    btdrv_write_rf_reg(0x92, tmp_val);
  }

  tmp_val = TX_POWET_CALIB_FACTOR - average_value;
  if (average_value > TX_POWET_CALIB_FACTOR)
    tmp_val = 0;

  BT_DRV_TRACE(1, "finally tmp_val=%x", tmp_val);

  read_value &= 0xff00;                 // clear [3:0] & [7:4]
  read_value |= (tmp_val | (0x7 << 4)); // get 0x18b [3:0] & [7:4]
  btdrv_write_rf_reg(0x18b, read_value);
  BT_DRV_TRACE(1, "write reg 0x18b val=0x%x", read_value);
  // set 0x18b[3:0] end
}
