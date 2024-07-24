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
#ifdef RTOS

#include "bt_drv.h"
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "hal_chipid.h"
#include "hal_i2c.h"
#include "hal_intersys.h"
#include "hal_iomux.h"
#include "hal_uart.h"
#include "plat_types.h"
#include "string.h"

struct dbg_nonsig_tester_result_tag {
  uint16_t pkt_counters;
  uint16_t head_errors;
  uint16_t payload_errors;
  int16_t avg_estsw;
  int16_t avg_esttpl;
  uint32_t payload_bit_errors;
};

struct bt_drv_capval_calc_t {
  int16_t estsw_a;
  int16_t estsw_b;
  uint8_t cdac_a;
  uint8_t cdac_b;
};

static struct dbg_nonsig_tester_result_tag nonsig_tester_result;
#ifdef RTOS
static osThreadId calib_thread_tid = NULL;
#else
static bool calib_thread_tid = false;
#endif
static bool calib_running = false;
static struct bt_drv_capval_calc_t capval_calc = {
    .estsw_a = 0,
    .estsw_b = 0,
    .cdac_a = 0,
    .cdac_b = 0,
};

#define bt_drv_calib_capval_calc_reset()                                       \
  do {                                                                         \
    memset(&capval_calc, 0, sizeof(capval_calc));                              \
  } while (0)
#ifdef RTOS
#define BT_DRV_MUTUX_WAIT(evt)                                                 \
  do {                                                                         \
    if (calib_thread_tid) {                                                    \
      osSignalClear(calib_thread_tid, 0x4);                                    \
      evt = osSignalWait(0x4, 4000);                                           \
    }                                                                          \
  } while (0)

#define BT_DRV_MUTUX_SET()                                                     \
  do {                                                                         \
    if (calib_thread_tid) {                                                    \
      osSignalSet(calib_thread_tid, 0x4);                                      \
    }                                                                          \
  } while (0)
#else

typedef enum {
  osOK = 0,             ///< function completed; no error or event occurred.
  osEventSignal = 0x08, ///< function completed; signal event occurred.
  osErrorOS = 0xFF,     ///< unspecified RTOS error: run-time error but no other
                        ///< error message fits.
  os_status_reserved =
      0x7FFFFFFF ///< prevent from enum down-size compiler optimization.
} osStatus;

typedef struct {
  osStatus status; ///< status code: event or error information
} osEvent;

#define BT_DRV_MUTUX_WAIT(evt)                                                 \
  do {                                                                         \
    uint8_t i = 16;                                                            \
    evt.status = osErrorOS;                                                    \
    do {                                                                       \
      if (calib_thread_tid) {                                                  \
        calib_thread_tid = false;                                              \
        evt.status = osEventSignal;                                            \
        break;                                                                 \
      } else {                                                                 \
        btdrv_delay(500);                                                      \
      }                                                                        \
    } while (i--);                                                             \
  } while (0)

#define BT_DRV_MUTUX_SET()                                                     \
  do {                                                                         \
    calib_thread_tid = true;                                                   \
  } while (0)
#endif

#define BT_DRV_CALIB_BTANA_CAPVAL_CALC_BOTTOM (50)
#define BT_DRV_CALIB_BTANA_CAPVAL_CALC_TOP (200)

#define BT_DRV_CALIB_BTANA_CAPVAL_MIN (20)
#define BT_DRV_CALIB_BTANA_CAPVAL_MAX (255 - BT_DRV_CALIB_BTANA_CAPVAL_MIN)
#define BT_DRV_CALIB_BTANA_CAPVAL_STEP_HZ (200)

#define BT_DRV_CALIB_RETRY_CNT (10)
#define BT_DRV_CALIB_SKIP_RESULT_CNT (2)
#define BT_DRV_CALIB_MDM_FREQ_REFERENCE (-12)
#define BT_DRV_CALIB_MDM_FREQ_STEP_HZ (500)
#define BT_DRV_CALIB_MDM_BIT_TO_FREQ(n, step) ((n) * (step))
#define BT_DRV_CALIB_MDM_FREQ_TO_BIT(n, step) ((uint32_t)n / step)

static int16_t calib_mdm_freq_reference = BT_DRV_CALIB_MDM_FREQ_REFERENCE;

enum HAL_IOMUX_ISPI_ACCESS_T ispi_access;

static void bt_drv_rfcal_timer_handler(void const *param);
osTimerDef(BT_DRV_RFCAL_TIMER, bt_drv_rfcal_timer_handler);
static osTimerId bt_drv_rfcal_timer = NULL;

static void bt_drv_calib_stop_timer(void) {
  if (NULL != bt_drv_rfcal_timer) {
    osTimerStop(bt_drv_rfcal_timer);
  }
}

static void bt_drv_rfcal_timer_handler(void const *start) {
  static int start_nonsig_rx = 0;

  if (NULL == bt_drv_rfcal_timer) {
    bt_drv_rfcal_timer =
        osTimerCreate(osTimer(BT_DRV_RFCAL_TIMER), osTimerOnce, NULL);
  }

  osTimerStop(bt_drv_rfcal_timer);

  if (start_nonsig_rx || start) {
    start_nonsig_rx = 0;

    const uint8_t hci_cmd_hci_reset[] = {0x01, 0x03, 0x0c, 0x00};
    const uint8_t calib_hci_cmd_nonsig_rx_2dh1_pn9_t5[] = {
        0x01, 0x87, 0xfc, 0x1c, 0x01, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x01, 0x01, 0x04, 0x00,
        0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff};

    /* start nonsig rx again */
    btdrv_SendData(hci_cmd_hci_reset, sizeof(hci_cmd_hci_reset));
    btdrv_delay(2);
    btdrv_SendData(calib_hci_cmd_nonsig_rx_2dh1_pn9_t5,
                   sizeof(calib_hci_cmd_nonsig_rx_2dh1_pn9_t5));
    btdrv_delay(100);

    BT_DRV_TRACE(0, "bt_drv_cal start nonsig rx\n");

    BTDIGITAL_REG(0xd0210040) &= 0xff;
    BTDIGITAL_REG(0xd0210040) |= 0x55;

    osTimerStart(bt_drv_rfcal_timer, 1500);
  } else {
    start_nonsig_rx = 1;

    const uint8_t stop_nonsig_rx_cmd[] = {
        0x01, 0x87, 0xfc, 0x1c, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    /* stop nonsig rx and let report cal result */
    btdrv_SendData(stop_nonsig_rx_cmd, sizeof(stop_nonsig_rx_cmd));
    btdrv_delay(2);

    BT_DRV_TRACE(0, "bt_drv_cal stop nonsig rx\n");

    osTimerStart(bt_drv_rfcal_timer, 300);
  }
}

void bt_drv_start_calib(void) {
  if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_1) {
    const uint8_t calib_hci_cmd_nonsig_rx_2dh1_pn9[] = {
        0x01, 0x87, 0xfc, 0x14, 0x01, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x01, 0x01, 0x04, 0x00, 0x36, 0x00};

    if (!calib_running) {
      btdrv_SendData(calib_hci_cmd_nonsig_rx_2dh1_pn9,
                     sizeof(calib_hci_cmd_nonsig_rx_2dh1_pn9));
      btdrv_delay(500);
      BTDIGITAL_REG(0xd0210040) &= 0xff;
      BTDIGITAL_REG(0xd0210040) |= 0x55;
      calib_running = true;
    }
  } else {
    bt_drv_rfcal_timer_handler((void *)1);
    calib_running = true;
  }
}

void bt_drv_stop_calib(void) {
  if (calib_running) {
    btdrv_hci_reset();
    btdrv_delay(200);
    calib_running = false;
  }

  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
    bt_drv_calib_stop_timer();
  }
}

const uint8_t calib_hci_cmd_nonsig_tx_2dh1_pn9_t0[] = {
    0x01, 0x87, 0xfc, 0x14, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x01, 0x01, 0x04, 0x04, 0x36, 0x00};

const uint8_t calib_hci_cmd_nonsig_tx_2dh1_pn9_t1[] = {
    0x01, 0x87, 0xfc, 0x1c, 0x00, 0xe8, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x01, 0x01, 0x04, 0x04,
    0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff};

void bt_drv_check_calib(void) {
  const uint8_t *calib_hci_cmd_nonsig_tx_2dh1_pn9 = NULL;
  uint8_t len = 0;

  if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_1) {
    calib_hci_cmd_nonsig_tx_2dh1_pn9 = calib_hci_cmd_nonsig_tx_2dh1_pn9_t0;
    len = sizeof(calib_hci_cmd_nonsig_tx_2dh1_pn9_t0);
  } else {
    calib_hci_cmd_nonsig_tx_2dh1_pn9 = calib_hci_cmd_nonsig_tx_2dh1_pn9_t1;
    len = sizeof(calib_hci_cmd_nonsig_tx_2dh1_pn9_t1);
  }

  btdrv_SendData(calib_hci_cmd_nonsig_tx_2dh1_pn9, len);
  btdrv_delay(3000);
  BTDIGITAL_REG(0xd0210040) &= 0xff;
  BTDIGITAL_REG(0xd0210040) |= 39;
}

static bool bt_drv_calib_capval_calc_run(uint16_t *capval_step_hz,
                                         int16_t estsw, uint8_t cdac) {
  bool nRet = false;
  int16_t estsw_diff;
  int16_t cdac_diff;

  if (!capval_calc.estsw_a && !capval_calc.estsw_b) {
    capval_calc.estsw_a = estsw;
  } else if (capval_calc.estsw_a && !capval_calc.estsw_b) {
    capval_calc.estsw_b = estsw;
  }

  if (!capval_calc.cdac_a && !capval_calc.cdac_b) {
    capval_calc.cdac_a = cdac;
  } else if (capval_calc.cdac_a && !capval_calc.cdac_b) {
    capval_calc.cdac_b = cdac;
  }

  if (capval_calc.estsw_a && capval_calc.estsw_b && capval_calc.cdac_a &&
      capval_calc.cdac_b) {

    estsw_diff = ABS(capval_calc.estsw_b - capval_calc.estsw_a);
    cdac_diff =
        ABS((int16_t)(capval_calc.cdac_b) - (int16_t)(capval_calc.cdac_a));
    *capval_step_hz = estsw_diff * BT_DRV_CALIB_MDM_FREQ_STEP_HZ / cdac_diff;
    BT_DRV_TRACE(7, "%d/%d %d/%d estsw_diff:%d cdac_diff:%d capval_step_hz:%d",
                 capval_calc.estsw_a, capval_calc.estsw_b, capval_calc.cdac_a,
                 capval_calc.cdac_b, estsw_diff, cdac_diff, *capval_step_hz);
    nRet = true;
  }
  return nRet;
}

void bt_drv_calib_rxonly_porc(void) {
  osEvent evt;

  evt.status = os_status_reserved;
  /*[set tmx] */
  btdrv_write_rf_reg(0xb0, 0x0f00); /* turn off tmx */

  bt_drv_start_calib();

  while (1) {
    BT_DRV_MUTUX_WAIT(evt);
    if (evt.status != osEventSignal) {
      break;
    }
    btdrv_delay(10);
    BT_DRV_TRACE(
        7, "result cnt:%d head:%d payload:%d estsw:%d esttpl:%d bit:%d est:%d",
        nonsig_tester_result.pkt_counters, nonsig_tester_result.head_errors,
        nonsig_tester_result.payload_errors, nonsig_tester_result.avg_estsw,
        nonsig_tester_result.avg_esttpl,
        nonsig_tester_result.payload_bit_errors,
        nonsig_tester_result.avg_estsw + nonsig_tester_result.avg_esttpl);
  };

  bt_drv_stop_calib();
}

int bt_drv_calib_result_porc(uint32_t *capval) {
  osEvent evt;

  uint16_t read_val = 0;
  uint8_t cdac = 0;
  uint8_t cnt = 0;
  uint8_t skip_cnt = 0;
  int diff = 0;
  int est = 0;
  uint8_t next_step = 0;
  uint16_t capval_step_hz = BT_DRV_CALIB_BTANA_CAPVAL_STEP_HZ;
  int nRet = -1;
  int chk_flag = 0;
  bool need_capval_calc = true;
  evt.status = os_status_reserved;

  /*[set to capval_min] */
  btdrv_read_rf_reg(0xE9, &read_val);
  cdac = BT_DRV_CALIB_BTANA_CAPVAL_CALC_BOTTOM;
  read_val = (read_val & 0xff00) | (cdac);
  ;
  btdrv_write_rf_reg(0xE9, read_val);

  /*[set tmx] */
  btdrv_write_rf_reg(0xb0, 0x0f00); /* turn off tmx */

  bt_drv_calib_capval_calc_reset();

  bt_drv_start_calib();

check_again:
  BT_DRV_TRACE(0, "calib run !!!");
  do {
  calib_again:
    BT_DRV_MUTUX_WAIT(evt);
    if (evt.status != osEventSignal) {
      nRet = -1;
      BT_DRV_TRACE(1, "evt:%d", evt.status);
      goto exit;
    }
    if (hal_get_chip_metal_id() < HAL_CHIP_METAL_ID_1) {
      BT_DRV_MUTUX_WAIT(evt);
      if (evt.status != osEventSignal) {
        nRet = -1;
        BT_DRV_TRACE(1, "evt:%d", evt.status);
        goto exit;
      }
      BT_DRV_MUTUX_WAIT(evt);
      if (evt.status != osEventSignal) {
        nRet = -1;
        BT_DRV_TRACE(1, "evt:%d", evt.status);
        goto exit;
      }
    }
    BT_DRV_TRACE(
        7, "result cnt:%d head:%d payload:%d estsw:%d esttpl:%d bit:%d est:%d",
        nonsig_tester_result.pkt_counters, nonsig_tester_result.head_errors,
        nonsig_tester_result.payload_errors, nonsig_tester_result.avg_estsw,
        nonsig_tester_result.avg_esttpl,
        nonsig_tester_result.payload_bit_errors,
        nonsig_tester_result.avg_estsw + nonsig_tester_result.avg_esttpl);

    if (nonsig_tester_result.head_errors > 15) {
      if (++skip_cnt > BT_DRV_CALIB_SKIP_RESULT_CNT)
        break;
      else
        goto calib_again;
    }
    skip_cnt = 0;

    btdrv_read_rf_reg(0xE9, &read_val);
    cdac = read_val & 0x00ff;

    est = nonsig_tester_result.avg_estsw;
    diff = est - calib_mdm_freq_reference;

    if (need_capval_calc) {
      if (bt_drv_calib_capval_calc_run(&capval_step_hz, est, cdac)) {
        need_capval_calc = false;
      }
      if (cdac == BT_DRV_CALIB_BTANA_CAPVAL_CALC_BOTTOM) {
        cdac = BT_DRV_CALIB_BTANA_CAPVAL_CALC_TOP;
        read_val = (read_val & 0xff00) | (cdac);
        btdrv_write_rf_reg(0xE9, read_val);
      }
      goto calib_again;
    }

    if ((BT_DRV_CALIB_MDM_BIT_TO_FREQ(ABS(diff),
                                      BT_DRV_CALIB_MDM_FREQ_STEP_HZ) < 1500) &&
        diff > 0) {
      if (!chk_flag) {
        chk_flag = 1;
        cnt = 0;
        next_step = 2;
        goto check_again;
      } else if (chk_flag && cnt > 4) {
        break;
      }
      nRet = 0;
      break;
    } else if (next_step == 2) {

    } else if (BT_DRV_CALIB_MDM_BIT_TO_FREQ(
                   ABS(diff), BT_DRV_CALIB_MDM_FREQ_STEP_HZ) < 2500) {
      next_step = 2;
    } else {
      next_step = BT_DRV_CALIB_MDM_BIT_TO_FREQ(ABS(diff),
                                               BT_DRV_CALIB_MDM_FREQ_STEP_HZ) /
                  capval_step_hz;
      if (next_step == 0) {
        next_step = 2;
      }
      if (next_step > 200) {
        next_step = 200;
      }
    }
    BT_DRV_TRACE(4, "diff:%d read_val:%x cdac:%d next_step:%d", diff, read_val,
                 cdac, next_step);
    if (est == calib_mdm_freq_reference) {
      if (!chk_flag) {
        chk_flag = 1;
        cnt = 0;
        next_step = 2;
        goto check_again;
      } else if (chk_flag && cnt > 4) {
        break;
      }
      nRet = 0;
      break;
    } else if (est > calib_mdm_freq_reference) {
      if (cdac < (BT_DRV_CALIB_BTANA_CAPVAL_MIN + next_step)) {
        if (cdac == BT_DRV_CALIB_BTANA_CAPVAL_MIN)
          break;
        else
          cdac = BT_DRV_CALIB_BTANA_CAPVAL_MIN;
      } else {
        cdac -= next_step;
      }
      read_val = (read_val & 0xff00) | (cdac);

      BT_DRV_TRACE(2, "-----:%x cdac:%d", read_val, cdac);
    } else if (est < calib_mdm_freq_reference) {
      if (cdac > (BT_DRV_CALIB_BTANA_CAPVAL_MAX - next_step)) {
        if (cdac == BT_DRV_CALIB_BTANA_CAPVAL_MAX)
          break;
        else
          cdac = BT_DRV_CALIB_BTANA_CAPVAL_MAX;
      } else {
        cdac += next_step;
      }
      read_val = (read_val & 0xff00) | (cdac);

      BT_DRV_TRACE(2, "+++++:%x cdac:%d", read_val, cdac);
    }

    btdrv_write_rf_reg(0xE9, read_val);
  } while (cnt++ < BT_DRV_CALIB_RETRY_CNT);
  *capval = cdac;
exit:
  bt_drv_stop_calib();

  BT_DRV_TRACE(3, "capval:0x%x cnt:%d nRet:%d", *capval, cnt, nRet);
  return nRet;
}

static unsigned int bt_drv_calib_rx(const unsigned char *data,
                                    unsigned int len) {
  const unsigned char nonsig_test_report[] = {0x04, 0x0e, 0x12};
  struct dbg_nonsig_tester_result_tag *pResult = NULL;

  hal_intersys_stop_recv(HAL_INTERSYS_ID_0);

  //    BT_DRV_TRACE(2,"%s len:%d", __func__, len);
  BT_DRV_DUMP("%02x ", data, len);
  if (0 == memcmp(data, nonsig_test_report, sizeof(nonsig_test_report))) {
    //        BT_DRV_TRACE(0,"dbg_nonsig_tester_result !!!!!");
    pResult = (struct dbg_nonsig_tester_result_tag *)(data + 7);
    if (pResult->pkt_counters != 0) {
      memcpy(&nonsig_tester_result, pResult, sizeof(nonsig_tester_result));
      BT_DRV_MUTUX_SET();
    }
  }
  hal_intersys_start_recv(HAL_INTERSYS_ID_0);

  return len;
}

void btdrv_tx(const unsigned char *data, unsigned int len);
void bt_drv_calib_open(void) {
  int ret = 0;
  BT_DRV_TRACE(1, "%s", __func__);
#ifdef RTOS
  if (calib_thread_tid == NULL) {
    calib_thread_tid = osThreadGetId();
  }
#endif

  ispi_access = hal_iomux_ispi_access_enable(HAL_IOMUX_ISPI_MCU_RF);

  ret = hal_intersys_open(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI,
                          bt_drv_calib_rx, btdrv_tx, false);

  if (ret) {
    BT_DRV_TRACE(0, "Failed to open intersys");
    return;
  }
  hal_intersys_start_recv(HAL_INTERSYS_ID_0);
}

void bt_drv_calib_close(void) {
  btdrv_hci_reset();
  btdrv_delay(200);
  hal_intersys_close(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI);
}

#endif
