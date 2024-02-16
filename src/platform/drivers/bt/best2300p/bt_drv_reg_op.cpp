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
#include "bt_drv_reg_op.h"
#include "CrashCatcher.h"
#include "bt_drv.h"
#include "bt_drv_2300p_internal.h"
#include "bt_drv_interface.h"
#include "bt_drv_internal.h"
#include "cmsis.h"
#include "hal_chipid.h"
#include "hal_iomux.h"
#include "hal_sysfreq.h"
#include "hal_trace.h"
#include <string.h>

static uint32_t bt_ram_start_addr = 0;
static uint32_t hci_fc_env_addr = 0;
static uint32_t ld_acl_env_addr = 0;
static uint32_t bt_util_buf_env_addr = 0;
static uint32_t em_buf_env_addr = 0;
static uint32_t sniffer_env_addr = 0;
static uint32_t dbg_state_addr = 0;
static uint32_t lc_state_addr = 0;
static uint32_t task_message_buffer_addr = 0;
static uint32_t lmp_message_buffer_addr = 0;
static uint32_t ld_sco_env_addr = 0;
static uint32_t rx_monitor_addr = 0;
static uint32_t lc_env_addr = 0;
static uint32_t dbg_bt_setting_addr = 0;
static uint32_t lm_nb_sync_active_addr = 0;
static uint32_t lm_env_addr = 0;
static uint32_t pending_seq_error_link_addr = 0;
static uint32_t hci_env_addr = 0;
static uint32_t lc_sco_env_addr = 0;
static uint32_t llm_le_env_addr = 0;
static uint32_t ld_env_addr = 0;
static uint32_t rwip_env_addr = 0;
static uint32_t MIC_BUFF_addr = 0;
static uint32_t g_mem_dump_ctrl_addr = 0;
static uint32_t ble_rx_monitor_addr = 0;
static uint32_t reconnecting_flag_addr = 0;
static uint32_t link_connect_bak_addr = 0;
static uint32_t tws_link_id_addr = 0;
static uint32_t llc_env_addr = 0;
static uint32_t dbg_bt_setting_ext1_addr = 0;
static uint32_t rwip_rf_addr = 0;
void bt_drv_reg_op_global_symbols_init(void) {
  bt_ram_start_addr = 0xc0000000;
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    hci_fc_env_addr = 0xC00067c4;
    ld_acl_env_addr = 0xc000098c;
    bt_util_buf_env_addr = 0xc00059fc;
    em_buf_env_addr = 0xc000656c;
    sniffer_env_addr = 0xc0006358;
    dbg_state_addr = 0xc00058c4;
    lc_state_addr = 0xc0005c7c;
    task_message_buffer_addr = 0xc00007cc;
    lmp_message_buffer_addr = 0xc0000834;
    ld_sco_env_addr = 0xc0000980;
    rx_monitor_addr = 0xc00058fc;
    lc_env_addr = 0xc0005c70;
    dbg_bt_setting_addr = 0xc0005854;
    lm_nb_sync_active_addr = 0xc00007c0;
    lm_env_addr = 0xc0005afc;
    hci_env_addr = 0xc0006764;
    lc_sco_env_addr = 0xc00007a4;
    llm_le_env_addr = 0xc00063f0;
    ld_env_addr = 0xc0005d34;
    rwip_env_addr = 0xc00003d4;
    MIC_BUFF_addr = 0xc0005920;
    ble_rx_monitor_addr = 0xc00058ec;
    llc_env_addr = 0xc00064f8;
    rwip_rf_addr = 0xc00041a0;

  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    hci_fc_env_addr = 0xC0006838;
    ld_acl_env_addr = 0xc0000998;
    bt_util_buf_env_addr = 0xc0005a18;
    em_buf_env_addr = 0xc00065e0;
    sniffer_env_addr = 0xc00063c8;
    dbg_state_addr = 0xc00058e0;
    lc_state_addr = 0xc0005ce4;
    task_message_buffer_addr = 0xc00007c4;
    lmp_message_buffer_addr = 0xc000082c;
    ld_sco_env_addr = 0xc000098c;
    rx_monitor_addr = 0xc0005918;
    lc_env_addr = 0xc0005cd8;
    dbg_bt_setting_addr = 0xc0005868;
    lm_nb_sync_active_addr = 0xc00007b8;
    lm_env_addr = 0xc0005b64;
    pending_seq_error_link_addr = 0xc0000335;
    hci_env_addr = 0xc00067d8;
    lc_sco_env_addr = 0xc000079c;
    llm_le_env_addr = 0xc0006460;
    ld_env_addr = 0xc0005d9c;
    rwip_env_addr = 0xc00003c8;
    MIC_BUFF_addr = 0xc000593c;
    ble_rx_monitor_addr = 0xc0005908;
    link_connect_bak_addr = 0xc00061c8;
    llc_env_addr = 0xc0006568;
    rwip_rf_addr = 0xc00041b4;

  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    hci_fc_env_addr = 0xC0006940;
    ld_acl_env_addr = 0xc00009d4;
    bt_util_buf_env_addr = 0xc0005b24;
    em_buf_env_addr = 0xc00066e8;
    sniffer_env_addr = 0xc00064d0;
    dbg_state_addr = 0xc00059b0;
    lc_state_addr = 0xc0005df0;
    task_message_buffer_addr = 0xc0000820;
    lmp_message_buffer_addr = 0xc0000888;
    ld_sco_env_addr = 0xc00009e4;
    rx_monitor_addr = 0xc0005a24;
    lc_env_addr = 0xc0005de4;
    dbg_bt_setting_addr = 0xc0005938;
    lm_nb_sync_active_addr = 0xc0000814;
    lm_env_addr = 0xc0005c70;
    pending_seq_error_link_addr = 0xc000034f;
    hci_env_addr = 0xc00068e0;
    lc_sco_env_addr = 0xc00007f8;
    llm_le_env_addr = 0xc0006568;
    ld_env_addr = 0xc0005ea8;
    rwip_env_addr = 0xc00003dc;
    MIC_BUFF_addr = 0xc0005a48;
    g_mem_dump_ctrl_addr = 0xc00059d4;
    ble_rx_monitor_addr = 0xc0005a14;
    reconnecting_flag_addr = 0xc0000a1c;
    link_connect_bak_addr = 0xc00062d0;
    tws_link_id_addr = 0xc000034e;
    llc_env_addr = 0xc0006670;
    dbg_bt_setting_ext1_addr = 0xc000591c;
    rwip_rf_addr = 0xc0004268;
  }
}

void bt_drv_reg_op_rssi_set(uint16_t rssi) {}

void bt_drv_reg_op_scan_intv_set(uint32_t scan_intv) {}

void bt_drv_reg_op_encryptchange_errcode_reset(uint16_t hci_handle) {}

void bt_drv_reg_op_sco_sniffer_checker(void) {}

void bt_drv_reg_op_trigger_time_checker(void) {
  BT_DRV_REG_OP_ENTER();
  BT_DRV_TRACE(1, "BT_REG_OP:0xd02201f0 = %x",
               *(volatile uint32_t *)0xd02201f0);
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_tws_output_power_fix_separate(uint16_t hci_handle,
                                                 uint16_t pwr) {}

#define SNIFF_IN_SCO 2
/// BD Address structure
struct bd_addr {
  /// 6-byte array address value
  uint8_t addr[6];
};
/// device info structure
struct ld_device_info {
  struct bd_addr bd_addr;
  uint8_t link_id;
  uint8_t state;
};

bool bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_get(void) {
  return false;
}

void bt_drv_reg_op_ld_sniffer_env_monitored_dev_state_set(bool state) {}

void bt_drv_reg_op_ibrt_env_reset(void) {
  struct ld_device_info *mobile_device_info;
  BT_DRV_REG_OP_ENTER();
  bool status = false;
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    mobile_device_info = (struct ld_device_info *)(sniffer_env_addr + 4);
    status = true;
  }

  if (status) {
    mobile_device_info->link_id = 3;
    mobile_device_info->state = 0;
  }
  BT_DRV_REG_OP_EXIT();
}

int bt_drv_reg_op_currentfreeaclbuf_get(void) {
  BT_DRV_REG_OP_ENTER();
  // ACL packert number of host set - ACL number of controller has been send to
  // host hci_fc_env.host_set.acl_pkt_nb - hci_fc_env.cntr.acl_pkt_sent
  if (hci_fc_env_addr != 0) {
    return (*(volatile uint16_t *)(hci_fc_env_addr + 0x4) -
            *(volatile uint16_t *)(hci_fc_env_addr + 0xc));
  } else {
    return 0;
  }
  BT_DRV_REG_OP_EXIT();
}

static uint16_t mobile_sync_info;
void bt_drv_reg_op_save_mobile_airpath_info(uint16_t hciHandle) {
  BT_DRV_REG_OP_ENTER();

  uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

  if (btdrv_is_link_index_valid(link_id)) {
    mobile_sync_info =
        BT_DRIVER_GET_U16_REG_VAL(EM_BT_BTADDR1_ADDR + BT_EM_SIZE * link_id);
    BT_DRV_TRACE(1, "BT_REG_OP: save xfer mobile sync info 0x%x ",
                 mobile_sync_info);
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_block_xfer_with_mobile(uint16_t hciHandle) {
  BT_DRV_REG_OP_ENTER();

  uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

  if (btdrv_is_link_index_valid(link_id)) {
    BT_DRIVER_PUT_U16_REG_VAL((EM_BT_BTADDR1_ADDR + BT_EM_SIZE * link_id),
                              mobile_sync_info ^ 0x8000);
    BT_DRV_TRACE(2,
                 "BT_REG_OP: block xfer with mobile, save hci handle 0x%x sync "
                 "info as 0x%x",
                 hciHandle, mobile_sync_info ^ 0x8000);
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_resume_xfer_with_mobile(uint16_t hciHandle) {
  BT_DRV_REG_OP_ENTER();
  uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

  if (btdrv_is_link_index_valid(link_id)) {
    BT_DRIVER_PUT_U16_REG_VAL((EM_BT_BTADDR1_ADDR + BT_EM_SIZE * link_id),
                              mobile_sync_info);
    BT_DRV_TRACE(2,
                 "BT_REG_OP:resume xfer with mobile, hci handle 0x%x as 0x%x",
                 hciHandle, mobile_sync_info);
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_block_fast_ack_with_mobile(void) {
  /*
   * ibrt master set its fast ack sync bit wrong,let itself NACK mobile link
   * always
   */

  BT_DRV_REG_OP_ENTER();

  bt_drv_set_fa_invert_enable(BT_FA_INVERT_DISABLE);

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_resume_fast_ack_with_mobile(void) {

  /*
   *  old master recovery its fast ack sync bit right
   */

  BT_DRV_REG_OP_ENTER();

  bt_drv_set_fa_invert_enable(BT_FA_INVERT_EN);

  BT_DRV_REG_OP_EXIT();
}

int bt_drv_reg_op_packet_type_checker(uint16_t hciHandle) { return 0; }

void bt_drv_reg_op_max_slot_setting_checker(uint16_t hciHandle) {}

void bt_drv_reg_op_force_task_dbg_idle(void) {}

void bt_drv_reg_op_afh_follow_mobile_mobileidx_set(uint16_t hciHandle) {}

void bt_drv_reg_op_afh_follow_mobile_twsidx_set(uint16_t hciHandle) {}

void bt_drv_reg_op_afh_bak_reset(void) {}

void bt_drv_reg_op_afh_bak_save(uint8_t role, uint16_t mobileHciHandle) {}

void bt_drv_reg_op_connection_checker(void) {}

void bt_drv_reg_op_sco_status_store(void) {}

void bt_drv_reg_op_sco_status_restore(void) {}

void bt_drv_reg_op_afh_set_default(void) {}

void bt_drv_reg_op_ld_sniffer_master_addr_set(uint8_t *addr) {}

void bt_drv_reg_op_lsto_hack(uint16_t hciHandle, uint16_t lsto) {
  uint32_t acl_par_ptr = 0;
  // ld_acl_env

  if (ld_acl_env_addr) {
    acl_par_ptr = *(uint32_t *)(ld_acl_env_addr + (hciHandle - 0x80) * 4);
  }

  if (acl_par_ptr) {
    BT_DRV_TRACE(3,
                 "BT_REG_OP:Set the lsto for hciHandle=0x%x, from:0x%x to 0x%x",
                 hciHandle, *(uint16_t *)(acl_par_ptr + 154), lsto);

    *(uint16_t *)(acl_par_ptr + 154) = lsto;
  } else {
    BT_DRV_TRACE(0, "BT_REG_OP:ERROR,acl par address error");
  }
}

uint16_t bt_drv_reg_op_get_lsto(uint16_t hciHandle) {
  uint32_t acl_par_ptr = 0;
  uint16_t lsto = 0;
  // ld_acl_env

  if (ld_acl_env_addr) {
    acl_par_ptr = *(uint32_t *)(ld_acl_env_addr + (hciHandle - 0x80) * 4);
  }

  if (acl_par_ptr) {
    lsto = *(uint16_t *)(acl_par_ptr + 154);
    BT_DRV_TRACE(2, "BT_REG_OP:lsto=0x%x for hciHandle=0x%x", lsto, hciHandle);
  } else {
    lsto = 0xffff;
    BT_DRV_TRACE(0, "BT_REG_OP:ERROR,acl par null ptr");
  }

  return lsto;
}

uint8_t bt_drv_reg_opmsbc_find_tx_sync(uint8_t *buff) {
  BT_DRV_REG_OP_ENTER();

  uint8_t i;
  for (i = 0; i < 60; i++) {
    if (buff[i] == 0x1 && buff[(i + 2) % 60] == 0xad) {
      //    BT_DRV_TRACE(1,"MSBC tx sync find =%d",i);
      return i;
    }
  }
  BT_DRV_TRACE(0, "BT_REG_OP:TX No pACKET");

  BT_DRV_REG_OP_EXIT();
  return 0;
}

bool bt_drv_reg_op_sco_tx_buf_restore(uint8_t *trigger_test) {
  uint8_t offset;
  bool nRet = false;

  BT_DRV_REG_OP_ENTER();

  offset = bt_drv_reg_opmsbc_find_tx_sync((uint8_t *)EM_BT_AUDIOBUF_OFF);
  if (offset != 0) {
#ifndef APB_PCM
    *trigger_test = (((BTDIGITAL_REG(0xd022045c) & 0x3f)) + (60 - offset)) % 64;
#endif
    BT_DRV_TRACE(2, "BT_REG_OP:TX BUF ERROR trigger_test=%p,offset=%x",
                 trigger_test, offset);
    DUMP8("%02x ", (uint8_t *)EM_BT_AUDIOBUF_OFF, 10);
    //   BT_DRV_TRACE(2,"pcm reg=%x %x",*(uint32_t *)0xd0220468,*(uint32_t
    //   *)0x400000f0);
    nRet = true;
  }

  BT_DRV_REG_OP_EXIT();

  return nRet;
}

int bt_drv_reg_op_acl_chnmap(uint16_t hciHandle, uint8_t *chnmap,
                             uint8_t chnmap_len) {
  uint32_t acl_evt_ptr = 0;
  uint8_t *chnmap_ptr = 0;
  uint8_t link_id = btdrv_conhdl_to_linkid(hciHandle);

  if (!btdrv_is_link_index_valid(link_id)) {
    memset(chnmap, 0, chnmap_len);
    return -1;
  }
  if (chnmap_len < 10) {
    memset(chnmap, 0, chnmap_len);
    return -1;
  }

  if (ld_acl_env_addr) {
    acl_evt_ptr = *(volatile uint32_t *)(ld_acl_env_addr + link_id * 4);
  }

  if (acl_evt_ptr != 0) {
    chnmap_ptr = (uint8_t *)(acl_evt_ptr + 0x34);
  }

  if (!chnmap_ptr) {
    memset(chnmap, 0, chnmap_len);
    return -1;
  } else {
    memcpy(chnmap, chnmap_ptr, chnmap_len);
  }

  return 0;
}

extern "C" uint32_t hci_current_left_tx_packets_left(void);
extern "C" uint32_t hci_current_left_rx_packets_left(void);
extern "C" uint32_t hci_current_rx_packet_complete(void);
extern "C" uint8_t hci_current_rx_aclfreelist_cnt(void);
void bt_drv_reg_op_bt_info_checker(void) {
#ifdef __CLK_GATE_DISABLE__
  uint32_t *rx_buf_ptr = NULL;
  uint32_t *tx_buf_ptr = NULL;
  uint8_t rx_free_buf_count = 0, i = 0;
  uint8_t tx_free_buf_count = 0;
  uint16_t em_buf;

  BT_DRV_REG_OP_ENTER();

  if (bt_util_buf_env_addr) {
    rx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr +
                              0x14); // bt_util_buf_env.acl_rx_free
    tx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr +
                              0x28); // bt_util_buf_env.acl_tx_free
  }

  while (rx_buf_ptr && *rx_buf_ptr) {
    rx_free_buf_count++;
    rx_buf_ptr = (uint32_t *)(*rx_buf_ptr);
  }

  uint8_t em_buf_count = 0;
  for (i = 0; i < 4; i++) {
    em_buf = BT_DRIVER_GET_U16_REG_VAL(EM_BT_RXACLBUFPTR_ADDR +
                                       i * REG_EM_BT_RXDESC_SIZE);
    if (em_buf != 0)
      em_buf_count++;
  }
  BT_DRV_TRACE(
      4,
      "BT_REG_OP: rxbuff = %x,embuff =%x , host rx buff in controller = %x,%x",
      rx_free_buf_count, em_buf_count, hci_current_left_rx_packets_left(),
      bt_drv_reg_op_currentfreeaclbuf_get());

  // check tx buff
  while (tx_buf_ptr && *tx_buf_ptr) {
    tx_free_buf_count++;
    tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
  }

  BT_DRV_TRACE(2, "BT_REG_OP:txbuff = %x,acl tx host  in controller = %x ",
               tx_free_buf_count, hci_current_left_tx_packets_left());

  bt_drv_reg_op_controller_state_checker();
  BT_DRV_REG_OP_EXIT();
#endif
}

#if 0
uint32_t *rxbuff;
uint16_t *send_count;
uint16_t *free_count;
uint16_t *bt_send_count;
#endif
void bt_drv_reg_op_ble_buffer_cleanup(void) {}

struct ke_timer {
  /// next ke timer
  struct ke_timer *next;
  /// message identifier
  uint16_t id;
  /// task identifier
  uint16_t task;
  /// time value
  uint32_t time;
};

struct co_list_hdr {
  /// Pointer to next co_list_hdr
  struct co_list_hdr *next;
};

/// structure of a list
struct co_list_con {
  /// pointer to first element of the list
  struct co_list_hdr *first;
  /// pointer to the last element
  struct co_list_hdr *last;

  /// number of element in the list
  uint32_t cnt;
  /// max number of element in the list
  uint32_t maxcnt;
  /// min number of element in the list
  uint32_t mincnt;
};

struct mblock_free {
  /// Next free block pointer
  struct mblock_free *next;
  /// Previous free block pointer
  struct mblock_free *previous;
  /// Size of the current free block (including delimiter)
  uint16_t free_size;
  /// Used to check if memory block has been corrupted or not
  uint16_t corrupt_check;
};

bool bt_drv_reg_op_get_dbg_state(void) {
  // dbg_state

  if (dbg_state_addr) {
    return (*(uint8_t *)dbg_state_addr);
  } else {
    return false;
  }
}

uint8_t bt_drv_reg_op_get_controller_tx_free_buffer(void) {
  uint32_t *tx_buf_ptr = NULL;
  uint8_t tx_free_buf_count = 0;

  if (bt_util_buf_env_addr) {
    tx_buf_ptr = (uint32_t *)(bt_util_buf_env_addr +
                              0x28); // bt_util_buf_env.acl_tx_free
  } else {
    BT_DRV_TRACE(1, "REG_OP: please fix %s", __func__);
    return 0;
  }

  // check tx buff
  while (tx_buf_ptr && *tx_buf_ptr) {
    tx_free_buf_count++;
    tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
  }
  return tx_free_buf_count;
}

uint8_t bt_drv_reg_op_get_controller_ble_tx_free_buffer(void) {
  uint32_t *tx_buf_ptr = NULL;
  uint8_t tx_free_buf_count = 0;

  if (em_buf_env_addr) {
    tx_buf_ptr =
        (uint32_t *)(em_buf_env_addr + 0x14); // em_buf_env.tx_buff_free
  } else {
    BT_DRV_TRACE(1, "REG_OP: please fix %s", __func__);
    return 0;
  }

  // check tx buff
  while (tx_buf_ptr && *tx_buf_ptr) {
    tx_free_buf_count++;
    tx_buf_ptr = (uint32_t *)(*tx_buf_ptr);
  }

  return tx_free_buf_count;
}

void bt_drv_reg_op_controller_state_checker(void) {
  BT_DRV_REG_OP_ENTER();

  if (lc_state_addr != 0) {
    BT_DRV_TRACE(1, "BT_REG_OP: LC_STATE=0x%x", *(uint32_t *)lc_state_addr);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_tx_flow_checker(void) {
  BT_DRV_REG_OP_ENTER();
  uint8_t link_id = 0;
  uint32_t acl_par_ptr = 0;
  for (link_id = 0; link_id < 3; link_id++) {
    if (ld_acl_env_addr) {
      acl_par_ptr = *(volatile uint32_t *)(ld_acl_env_addr + link_id * 4);
    }

    if (acl_par_ptr) {
      BT_DRV_TRACE(2, "linkid =%d, tx flow=%d", link_id,
                   *(uint8_t *)(acl_par_ptr + 0xc0));
    }
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_controller_ble_flow_checker(void) {
  uint8_t link_id = 0;
  uint32_t llc_env_tag_ptr = 0;
  uint32_t ea_elt_tag_ptr = 0;
  uint32_t *tx_acl_rdy_ptr = NULL;
  uint32_t *tx_acl_tofree_ptr = NULL;
  uint32_t tx_acl_rdy_count = 0;
  uint32_t tx_acl_tofree_count = 0;

  if (!llc_env_addr) {
    BT_DRV_TRACE(1, "REG_OP: please fix %s", __func__);
    return;
  }

  for (link_id = 0; link_id < 3; link_id += 1) {
    llc_env_tag_ptr = *(volatile uint32_t *)(llc_env_addr + link_id * 4);
    if (!llc_env_tag_ptr) {
      continue;
    }

    ea_elt_tag_ptr = *(volatile uint32_t *)(((uint32_t)llc_env_tag_ptr) + 16);
    if (!ea_elt_tag_ptr) {
      continue;
    }

    tx_acl_rdy_ptr = (uint32_t *)(((uint32_t)ea_elt_tag_ptr) + 48);
    tx_acl_rdy_count = 0;
    while (tx_acl_rdy_ptr && *tx_acl_rdy_ptr) {
      tx_acl_rdy_count++;
      tx_acl_rdy_ptr = (uint32_t *)(*tx_acl_rdy_ptr);
    }

    tx_acl_tofree_ptr = (uint32_t *)(((uint32_t)ea_elt_tag_ptr) + 68);
    tx_acl_tofree_count = 0;
    while (tx_acl_tofree_ptr && *tx_acl_tofree_ptr) {
      tx_acl_tofree_count++;
      tx_acl_tofree_ptr = (uint32_t *)(*tx_acl_tofree_ptr);
    }

    BT_DRV_TRACE(3, "linkid =%d, ble tx alloc=%d tofree=%d", link_id,
                 tx_acl_rdy_count, tx_acl_tofree_count);
  }
}

uint8_t bt_drv_reg_op_force_get_lc_state(uint16_t conhdl) {
  if (lc_state_addr != 0) {
    // BT_DRV_TRACE(1,"BT_REG_OP: read LC_STATE=0x%x",*(uint32_t
    // *)lc_state_addr);
    uint8_t idx = btdrv_conhdl_to_linkid(conhdl);

    if (btdrv_is_link_index_valid(idx)) {
      uint8_t *lc_state = (uint8_t *)lc_state_addr;
      return lc_state[idx];
    }
  }
  return 0;
}
void bt_drv_reg_op_force_set_lc_state(uint16_t conhdl, uint8_t state) {
  // lc_state

  if (lc_state_addr != 0) {
    BT_DRV_TRACE(1, "BT_REG_OP: read LC_STATE=0x%x",
                 *(uint32_t *)lc_state_addr);
    uint8_t idx = btdrv_conhdl_to_linkid(conhdl);

    if (btdrv_is_link_index_valid(idx)) {
      uint8_t *lc_state = (uint8_t *)lc_state_addr;
      lc_state[idx] = state;
    }
  }
}

void bt_drv_reg_op_crash_dump(void) {
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0) {
    uint8_t *bt_dump_mem_start = (uint8_t *)bt_ram_start_addr;
    uint32_t bt_dump_len_max = 0x8000;

    // uint8_t *mcu_dump_mem_start = (uint8_t*)0x20000000;
    // uint32_t mcu_dump_len_max   = RAM_SIZE;

    // uint8_t *em_dump_area_1_start = (uint8_t*)0xd0220000;
    // uint32_t em_area_1_len_max    = 0x04A8;

    uint8_t *em_dump_area_2_start = (uint8_t *)0xd0210000;
    uint32_t em_area_2_len_max = 0x8000;

    BT_DRV_TRACE(1, "BT_REG_OP:BT 2300p: metal id=%d", hal_get_chip_metal_id());
    // first move R3 to R9, lost R9
    BT_DRV_TRACE(1, "BT controller BusFault_Handler:\nREG:[LR] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE));
    BT_DRV_TRACE(1, "REG:[R0] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 120));
    BT_DRV_TRACE(1, "REG:[R1] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 124));
    BT_DRV_TRACE(1, "REG:[R2] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 128));
    BT_DRV_TRACE(1, "REG:[R3] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 132));
    BT_DRV_TRACE(1, "REG:[R4] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 4));
    BT_DRV_TRACE(1, "REG:[R5] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 8));
    BT_DRV_TRACE(1, "REG:[R6] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 12));
    BT_DRV_TRACE(1, "REG:[R7] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 16));
    BT_DRV_TRACE(1, "REG:[R8] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 20));
    hal_sys_timer_delay(MS_TO_TICKS(100));

    // BT_DRV_TRACE(1,"REG:[R9] = 0x%08x",
    // BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE +24));
    BT_DRV_TRACE(1, "REG:[sl] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 28));
    BT_DRV_TRACE(1, "REG:[fp] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 32));
    BT_DRV_TRACE(1, "REG:[ip] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 36));
    BT_DRV_TRACE(1, "REG:[SP,#0] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 40));
    BT_DRV_TRACE(1, "REG:[SP,#4] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 44));
    BT_DRV_TRACE(1, "REG:[SP,#8] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 48));
    BT_DRV_TRACE(1, "REG:[SP,#12] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 52));
    BT_DRV_TRACE(1, "REG:[SP,#16] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 56));
    BT_DRV_TRACE(1, "REG:[SP,#20] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 60));
    BT_DRV_TRACE(1, "REG:[SP,#24] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 64));
    BT_DRV_TRACE(1, "REG:[SP,#28] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 68));
    hal_sys_timer_delay(MS_TO_TICKS(100));

    BT_DRV_TRACE(1, "REG:[SP,#32] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 72));
    BT_DRV_TRACE(1, "REG:[SP,#36] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 76));
    BT_DRV_TRACE(1, "REG:[SP,#40] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 80));
    BT_DRV_TRACE(1, "REG:[SP,#44] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 84));
    BT_DRV_TRACE(1, "REG:[SP,#48] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 88));
    BT_DRV_TRACE(1, "REG:[SP,#52] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 92));
    BT_DRV_TRACE(1, "REG:[SP,#56] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 96));
    BT_DRV_TRACE(1, "REG:[SP,#60] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 100));
    BT_DRV_TRACE(1, "REG:[SP,#64] = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 104));
    BT_DRV_TRACE(1, "REG:SP = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 108));
    BT_DRV_TRACE(1, "REG:MSP = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 112));
    BT_DRV_TRACE(1, "REG:PSP = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 116));
    BT_DRV_TRACE(1, "REG:CFSR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 120));
    BT_DRV_TRACE(1, "REG:BFAR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 124));
    BT_DRV_TRACE(1, "REG:HFSR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 128));
    BT_DRV_TRACE(1, "REG:ICSR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 132));
    BT_DRV_TRACE(1, "REG:AIRCR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 136));
    BT_DRV_TRACE(1, "REG:SCR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 140));
    BT_DRV_TRACE(1, "REG:CCR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 144));
    BT_DRV_TRACE(1, "REG:SHCSR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 148));
    BT_DRV_TRACE(1, "REG:AFSR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 152));
    BT_DRV_TRACE(1, "REG:MMFAR = 0x%08x",
                 BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 156));
    hal_sys_timer_delay(MS_TO_TICKS(100));
    // task_message_buffer
    uint32_t buff_addr = 0;

    if (task_message_buffer_addr) {
      buff_addr = task_message_buffer_addr;
    }

    BT_DRV_TRACE(2, "0xd0330050: 0x%x, 54:0x%x", *(uint32_t *)0xd0330050,
                 *(uint32_t *)0xd0330054);
    BT_DRV_TRACE(2, "0x400000a0: 0x%x, a4:0x%x", *(uint32_t *)0x400000a0,
                 *(uint32_t *)0x400000a4);

    if (g_mem_dump_ctrl_addr) {
      BT_DRV_TRACE(1, "LMP addr=0x%x", *(uint32_t *)(g_mem_dump_ctrl_addr + 4));
      BT_DRV_TRACE(1, "STA addr=0x%x",
                   *(uint32_t *)(g_mem_dump_ctrl_addr + 0x10));
      BT_DRV_TRACE(1, "MSG addr=0x%x",
                   *(uint32_t *)(g_mem_dump_ctrl_addr + 0x1c));
      BT_DRV_TRACE(1, "SCH addr=0x%x",
                   *(uint32_t *)(g_mem_dump_ctrl_addr + 0x28));
      BT_DRV_TRACE(1, "ISR addr=0x%x",
                   *(uint32_t *)(g_mem_dump_ctrl_addr + 0x34));
    }

    BT_DRV_TRACE(0, "task msg buff:");
    if (buff_addr != 0) {
      for (uint8_t j = 0; j < 5; j++) {
        DUMP8("%02x ", (uint8_t *)(buff_addr + j * 20), 20);
      }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    BT_DRV_TRACE(0, " ");
    BT_DRV_TRACE(0, "lmp buff:");
    // lmp_message_buffer

    if (lmp_message_buffer_addr) {
      buff_addr = lmp_message_buffer_addr;
    }

    if (buff_addr != 0) {
      for (uint8_t j = 0; j < 10; j++) {
        DUMP8("%02x ", (uint8_t *)(buff_addr + j * 20), 20);
      }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    uint8_t link_id = 0;
    uint32_t evt_ptr = 0;
    uint32_t acl_par_ptr = 0;
    for (link_id = 0; link_id < 3; link_id++) {
      BT_DRV_TRACE(1, "acl_par: link id %d", link_id);

      if (ld_acl_env_addr) {
        evt_ptr = *(uint32_t *)(ld_acl_env_addr + link_id * 4);
      }

      if (evt_ptr) {
        acl_par_ptr = evt_ptr;
        BT_DRV_TRACE(5,
                     "acl_par: acl_par_ptr 0x%x, clk off 0x%x, bit off 0x%x, "
                     "last sync clk off 0x%x, last sync bit off 0x%x",
                     acl_par_ptr, *(uint32_t *)(acl_par_ptr + 140),
                     *(uint16_t *)(acl_par_ptr + 150),
                     *(uint32_t *)(acl_par_ptr + 136),
                     ((*(uint32_t *)(acl_par_ptr + 150)) & 0xFFFF0000) >> 16);
      }
    }
    hal_sys_timer_delay(MS_TO_TICKS(100));

    // ld_sco_env
    evt_ptr = 0;

    if (ld_sco_env_addr) {
      evt_ptr = *(uint32_t *)ld_sco_env_addr;
    }

    if (evt_ptr != 0) {
      BT_DRV_TRACE(1, "esco linkid :%d", *(uint8_t *)(evt_ptr + 70));
      for (link_id = 0; link_id < 3; link_id++) {
        BT_DRV_TRACE(2, "bt_linkcntl_linklbl 0x%x: link id %d",
                     *(uint16_t *)(EM_BT_LINKCNTL_ADDR + link_id * BT_EM_SIZE),
                     link_id);
        BT_DRV_TRACE(
            1, "rxcount :%x",
            *(uint16_t *)(EM_BT_RXDESCCNT_ADDR + link_id * BT_EM_SIZE));
      }
    }
    btdrv_dump_mem(bt_dump_mem_start, bt_dump_len_max, BT_SUB_SYS_TYPE);
    // btdrv_dump_mem(em_dump_area_1_start, em_area_1_len_max,
    // BT_EM_AREA_1_TYPE);
    btdrv_dump_mem(em_dump_area_2_start, em_area_2_len_max, BT_EM_AREA_2_TYPE);
    // btdrv_dump_mem(mcu_dump_mem_start, mcu_dump_len_max, MCU_SYS_TYPE);
  }
}

uint8_t bt_drv_reg_op_get_tx_pwr(uint16_t connHandle) {
  uint8_t idx;
  uint16_t localVal;
  uint8_t tx_pwr = 0;

  idx = btdrv_conhdl_to_linkid(connHandle);

  if (btdrv_is_link_index_valid(idx)) {
    localVal = BT_DRIVER_GET_U16_REG_VAL(EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE);
    tx_pwr = ((localVal & ((uint16_t)0x000000FF)) >> 0);
  }

  return tx_pwr;
}

void bt_drv_reg_op_set_tx_pwr(uint16_t connHandle, uint8_t txpwr) {
  BT_DRV_REG_OP_ENTER();

  uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

  if (btdrv_is_link_index_valid(idx)) {
    BT_DRIVER_PUT_U16_REG_VAL(
        EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE,
        (BT_DRIVER_GET_U16_REG_VAL(EM_BT_PWRCNTL_ADDR + idx * BT_EM_SIZE) &
         ~((uint16_t)0x000000FF)) |
            ((uint16_t)txpwr << 0));
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_fix_tx_pwr(uint16_t connHandle) {
  BT_DRV_REG_OP_ENTER();
  bt_drv_reg_op_set_tx_pwr(connHandle, LBRT_TX_PWR_FIX);
  BT_DRV_REG_OP_EXIT();
}

//#define __ACCESS_MODE_ADJUST_GAIN__
//#define __SWAGC_MODE_ADJUST_GAIN__
#define __REBOOT_PAIRING_MODE_ADJUST_GAIN__

void bt_drv_reg_op_set_accessible_mode(uint8_t mode) {}

void bt_drv_reg_op_set_swagc_mode(uint8_t mode) {}

void bt_drv_reg_op_set_reboot_pairing_mode(uint8_t mode) {}

void bt_drv_reg_op_force_sco_retrans(bool enable) {
#ifdef __FORCE_SCO_MAX_RETX__
  BT_DRV_REG_OP_ENTER();
  if (enable) {
    BTDIGITAL_REG_SET_FIELD(0xd0220468, 3, 24, 3);
  } else {
    BTDIGITAL_REG_SET_FIELD(0xd0220468, 3, 24, 0);
  }
  BT_DRV_REG_OP_EXIT();
#endif
}

void bt_drv_reg_op_enable_pcm_tx_hw_cal(void) {
  BT_DRV_REG_OP_ENTER();
  *(volatile uint32_t *)0xd0220468 |= 1 << 22;
  *(volatile uint32_t *)0x400000f0 |= 1;
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_monitor_clk(void) {
  BT_DRV_REG_OP_ENTER();
  uint32_t env0 = 0;
  uint32_t env1 = 0;
  uint32_t env2 = 0;

  if (ld_acl_env_addr) {
    env0 = *(uint32_t *)ld_acl_env_addr;
    env1 = *(uint32_t *)(ld_acl_env_addr + 4);
    env1 = *(uint32_t *)(ld_acl_env_addr + 8);
  }

  if (env0 & bt_ram_start_addr) {
    env0 += 0x8c;
    BT_DRV_TRACE(2, "BT_REG_OP:env0 clk off=%x %x", *(uint32_t *)env0,
                 *(uint16_t *)EM_BT_CLKOFF0_ADDR |
                     (*(uint16_t *)EM_BT_CLKOFF1_ADDR << 16));
  }
  if (env1 & bt_ram_start_addr) {
    env1 += 0x8c;
    BT_DRV_TRACE(2, "BT_REG_OP:env1 clk off=%x %x", *(uint32_t *)env1,
                 *(uint16_t *)(EM_BT_CLKOFF0_ADDR + BT_EM_SIZE) |
                     (*(uint16_t *)(EM_BT_CLKOFF1_ADDR + BT_EM_SIZE) << 16));
  }
  if (env2 & bt_ram_start_addr) {
    env2 += 0x8c;
    BT_DRV_TRACE(
        2, "BT_REG_OP:env2 clk off=%x %x", *(uint32_t *)env2,
        *(uint16_t *)(EM_BT_CLKOFF0_ADDR + BT_EM_SIZE * 2) |
            (*(uint16_t *)(EM_BT_CLKOFF1_ADDR + BT_EM_SIZE * 2) << 16));
  }

  BT_DRV_REG_OP_EXIT();
}

bool bt_drv_reg_op_read_rssi_in_dbm(uint16_t connHandle, rx_agc_t *rx_val) {
#ifdef BT_RSSI_MONITOR
  BT_DRV_REG_OP_ENTER();

  if (connHandle == 0xFFFF)
    return false;

  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0) {
    uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
    /// Accumulated RSSI (to compute an average value)
    int16_t rssi_acc = 0;
    /// Counter of received packets used in RSSI average
    uint8_t rssi_avg_cnt = 1;
    rx_agc_t *rx_monitor_env = NULL;

    if (!btdrv_is_link_index_valid(idx)) {
      return false;
    }

    // rx_monitor
    if (rx_monitor_addr) {
      rx_monitor_env = (rx_agc_t *)rx_monitor_addr;
    }

    if (rx_monitor_env != NULL) {
      for (int i = 0; i < rssi_avg_cnt; i++) {
        rssi_acc += rx_monitor_env[idx].rssi;
      }
      rx_val->rssi = rssi_acc / rssi_avg_cnt;
      rx_val->rxgain = rx_monitor_env[idx].rxgain;
    }
  }

  BT_DRV_REG_OP_EXIT();
  return true;
#else
  return false;
#endif
}

bool bt_drv_reg_op_read_ble_rssi_in_dbm(uint16_t connHandle, rx_agc_t *rx_val) {
#ifdef BT_RSSI_MONITOR
  BT_DRV_REG_OP_ENTER();

  if (connHandle == 0xFFFF)
    return false;

  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0) {
    uint8_t idx = connHandle;
    /// Accumulated RSSI (to compute an average value)
    int16_t rssi_acc = 0;
    /// Counter of received packets used in RSSI average
    uint8_t rssi_avg_cnt = 1;
    rx_agc_t *rx_monitor_env = NULL;
    if (idx > 2)
      return false; // invalid idx

    // rx_monitor

    if (ble_rx_monitor_addr) {
      rx_monitor_env = (rx_agc_t *)ble_rx_monitor_addr;
    }

    if (rx_monitor_env != NULL) {
      for (int i = 0; i < rssi_avg_cnt; i++) {
        rssi_acc += rx_monitor_env[idx].rssi;
      }
      rx_val->rssi = rssi_acc / rssi_avg_cnt;
      rx_val->rxgain = rx_monitor_env[idx].rxgain;
    }
  }

  BT_DRV_REG_OP_EXIT();
  return true;
#else
  return false;
#endif
}

void bt_drv_reg_op_acl_silence(uint16_t connHandle, uint8_t silence) {
  BT_DRV_REG_OP_ENTER();

  uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

  if (btdrv_is_link_index_valid(idx)) {
    BT_DRIVER_PUT_U16_REG_VAL(
        EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE,
        (BT_DRIVER_GET_U16_REG_VAL(EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE) &
         ~((uint16_t)0x00008000)) |
            ((uint16_t)silence << 15));
  }

  BT_DRV_REG_OP_EXIT();
}

/// sniffer connect information environment structure
struct ld_sniffer_connect_env {
  ///
  uint16_t bitoff;
  ///
  uint32_t clk_offset;
  ///
  uint8_t map[10];
  ///
  uint32_t afh_instant;
  ///
  uint8_t afh_mode;
  ///
  uint8_t enc_mode;
  ///
  uint8_t ltk[16];
  ///
  uint8_t role;
};

void bt_drv_reg_op_call_monitor(uint16_t connHandle, uint8_t tws_role) {}

void bt_drv_reg_op_lock_sniffer_sco_resync(void) {}

void bt_drv_reg_op_unlock_sniffer_sco_resync(void) {}

void bt_drv_reg_op_ibrt_retx_att_nb_set(uint8_t retx_nb) {
  BT_DRV_REG_OP_ENTER();

  int ret = -1;
  uint32_t sco_evt_ptr = 0x0;
  // TODO: [ld_sco_env address] based on CHIP id

  if (ld_sco_env_addr) {
    sco_evt_ptr = *(volatile uint32_t *)ld_sco_env_addr;
    ret = 0;
  }

  if (ret == 0) {
    uint32_t retx_ptr = 0x0;
    if (sco_evt_ptr != 0) {
      // offsetof(struct ea_elt_tag, env) + sizeof(struct ld_sco_evt_params)
      retx_ptr = sco_evt_ptr + 0x44;
    } else {
      BT_DRV_TRACE(0, "BT_REG_OP:Error, ld_sco_env[0].evt ==NULL");
      ret = -2;
    }

    if (ret == 0) {
      *(volatile uint8_t *)retx_ptr = retx_nb;
    }
  }

  BT_DRV_TRACE(3, "BT_REG_OP:%s,ret=%d,retx nb=%d", __func__, ret, retx_nb);
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_acl_tx_type_set(uint16_t hciHandle, uint8_t slot_sel) {
  uint32_t lc_ptr = 0;
  uint32_t acl_par_ptr = 0;
  uint32_t packet_type_addr = 0;
  // lc_env and ld_acl_env

  if (lc_env_addr) {
    lc_ptr = *(uint32_t *)(lc_env_addr + (hciHandle - 0x80) * 4);
  }

  if (ld_acl_env_addr) {
    acl_par_ptr = *(uint32_t *)(ld_acl_env_addr + (hciHandle - 0x80) * 4);
  }

  // sanity check
  if (lc_ptr == 0) {
    return;
  } else {
    packet_type_addr = (uint32_t)(acl_par_ptr + 176);
    BT_DRV_TRACE(3, "BT_REG_OP:%s   hdl=0x%x packet_types=0x%x", __func__,
                 hciHandle, *(uint16_t *)(packet_type_addr));
    BT_DRV_TRACE(2, "BT_REG_OP:lc_ptr 0x%x, acl_par_ptr 0x%x", lc_ptr,
                 acl_par_ptr);
    uint16_t edr_type = 0;
    if (slot_sel == USE_1_SLOT_PACKET) {
#if defined(__3M_PACK__)
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX) | (1 << DH1_3_IDX);
#else
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX);
#endif
    } else if (slot_sel == USE_3_SLOT_PACKET) {
#if defined(__3M_PACK__)
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX) | (1 << DH1_3_IDX) |
                 (1 << DH3_2_IDX) | (1 << DH3_3_IDX);
#else
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX) | (1 << DH3_2_IDX);
#endif
    } else if (slot_sel == USE_5_SLOT_PACKET) {
#if defined(__3M_PACK__)
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX) | (1 << DH1_3_IDX) |
                 (1 << DH3_2_IDX) | (1 << DH3_3_IDX) | (1 << DH5_2_IDX) |
                 (1 << DH5_3_IDX);
#else
      edr_type = (1 << DM1_IDX) | (1 << DH1_2_IDX) | (1 << DH3_2_IDX) |
                 (1 << DH5_2_IDX);
#endif
    }
    *(uint16_t *)(packet_type_addr) =
        ((edr_type << 8) | (*(uint16_t *)(packet_type_addr)&0xff));

    BT_DRV_TRACE(1, "BT_REG_OP:After op,packet_types 0x%x",
                 *(uint16_t *)(packet_type_addr));
  }
}

void bt_drv_reg_op_acl_tx_type_trace(uint16_t hciHandle) {
  uint32_t acl_par_ptr = 0;
  uint32_t packet_type_addr = 0;
  // ld_acl_env

  if (ld_acl_env_addr) {
    acl_par_ptr = *(uint32_t *)(ld_acl_env_addr + (hciHandle - 0x80) * 4);
  }

  // sanity check
  if (acl_par_ptr == 0) {
    return;
  } else {
    packet_type_addr = (uint32_t)(acl_par_ptr + 176);
    BT_DRV_TRACE(1, "BT_REG_OP:tx packet_types=0x%x",
                 *(uint16_t *)(packet_type_addr));
  }
}

uint8_t bt_drv_reg_op_acl_tx_type_get(uint16_t hciHandle, uint8_t *br_type,
                                      uint8_t *edr_type) {
  uint8_t status = 0xff;
  uint32_t lc_ptr = 0;
  uint32_t acl_par_ptr = 0;
  uint32_t packet_type_addr = 0;

  if (lc_env_addr) {
    lc_ptr = *(uint32_t *)(lc_env_addr + (hciHandle - 0x80) * 4);
  }

  if (ld_acl_env_addr) {
    acl_par_ptr = *(uint32_t *)(ld_acl_env_addr + (hciHandle - 0x80) * 4);
  }

  // sanity check
  if (lc_ptr == 0) {
    BT_DRV_TRACE(2, "BT_REG_OP:%s hdl=0x%x,lc_env_ptr = NULL", __func__,
                 hciHandle);
  } else {
    packet_type_addr = (uint32_t)(acl_par_ptr + 176);
    uint16_t packet_type = *(uint16_t *)(packet_type_addr);
    if (br_type) {
      *br_type = packet_type & 0xff;
    }
    if (edr_type) {
      *br_type = (packet_type >> 8) & 0xff;
    }
    status = 0;
    BT_DRV_TRACE(3, "BT_REG_OP:%s hdl=0x%x packet_types=0x%x", __func__,
                 hciHandle, packet_type);
  }

  return status;
}

uint16_t em_bt_bitoff_getf(int elt_idx) {
  uint16_t localVal = BTDIGITAL_BT_EM(EM_BT_BITOFF_ADDR + elt_idx * BT_EM_SIZE);
  ASSERT_ERR((localVal & ~((uint16_t)0x000003FF)) == 0);
  return (localVal >> 0);
}

void em_bt_bitoff_setf(int elt_idx, uint16_t bitoff) {
  ASSERT_ERR((((uint16_t)bitoff << 0) & ~((uint16_t)0x000003FF)) == 0);
  BTDIGITAL_EM_BT_WR(EM_BT_BITOFF_ADDR + elt_idx * BT_EM_SIZE,
                     (uint16_t)bitoff << 0);
}

void em_bt_clkoff0_setf(int elt_idx, uint16_t clkoff0) {
  ASSERT_ERR((((uint16_t)clkoff0 << 0) & ~((uint16_t)0x0000FFFF)) == 0);
  BTDIGITAL_EM_BT_WR(EM_BT_CLKOFF0_ADDR + elt_idx * BT_EM_SIZE,
                     (uint16_t)clkoff0 << 0);
}

uint16_t em_bt_clkoff0_getf(int elt_idx) {
  uint16_t localVal =
      BTDIGITAL_BT_EM(EM_BT_CLKOFF0_ADDR + elt_idx * BT_EM_SIZE);
  ASSERT_ERR((localVal & ~((uint16_t)0x0000FFFF)) == 0);
  return (localVal >> 0);
}
void em_bt_clkoff1_setf(int elt_idx, uint16_t clkoff1) {
  ASSERT_ERR((((uint16_t)clkoff1 << 0) & ~((uint16_t)0x000007FF)) == 0);
  BTDIGITAL_EM_BT_WR(EM_BT_CLKOFF1_ADDR + elt_idx * BT_EM_SIZE,
                     (uint16_t)clkoff1 << 0);
}

uint16_t em_bt_clkoff1_getf(int elt_idx) {
  uint16_t localVal =
      BTDIGITAL_BT_EM(EM_BT_CLKOFF1_ADDR + elt_idx * BT_EM_SIZE);
  ASSERT_ERR((localVal & ~((uint16_t)0x000007FF)) == 0);
  return (localVal >> 0);
}

void em_bt_wincntl_pack(int elt_idx, uint8_t rxwide, uint16_t rxwinsz) {
  ASSERT_ERR((((uint16_t)rxwide << 15) & ~((uint16_t)0x00008000)) == 0);
  ASSERT_ERR((((uint16_t)rxwinsz << 0) & ~((uint16_t)0x00000FFF)) == 0);
  BTDIGITAL_EM_BT_WR(EM_BT_WINCNTL_ADDR + elt_idx * BT_EM_SIZE,
                     ((uint16_t)rxwide << 15) | ((uint16_t)rxwinsz << 0));
}

void bt_drv_reg_op_update_sniffer_bitoffset(uint16_t mobile_conhdl,
                                            uint16_t master_conhdl) {}

void bt_drv_reg_op_modify_bitoff_timer(uint16_t time_out) {}

void bt_drv_reg_op_cs_monitor(void) {
  uint32_t addr;
  addr = BT_EM_ADDR_BASE + 0x1e;
  BT_DRV_TRACE(0, "BT_REG_OP:AFH 0:");
  DUMP8("%02x ", (uint8_t *)addr, 10);
  addr = BT_EM_ADDR_BASE + 0x1e + 110;
  BT_DRV_TRACE(0, "AFH 1:");
  DUMP8("%02x ", (uint8_t *)addr, 10);
  addr = BT_EM_ADDR_BASE + 0x1e + 220;
  BT_DRV_TRACE(0, "AFH 2:");
  DUMP8("%02x ", (uint8_t *)addr, 10);
  uint32_t tmp1, tmp2, tmp3;
  tmp1 = BT_EM_ADDR_BASE + 0x8;
  tmp2 = BT_EM_ADDR_BASE + 0x8 + 110;
  tmp3 = BT_EM_ADDR_BASE + 0x8 + 220;
  BT_DRV_TRACE(3, "AFH EN:%x %x %x ", *(uint16_t *)tmp1, *(uint16_t *)tmp2,
               *(uint16_t *)tmp3);
  tmp1 = BT_EM_ADDR_BASE + 0x28;
  tmp2 = BT_EM_ADDR_BASE + 0x28 + 110;
  tmp3 = BT_EM_ADDR_BASE + 0x28 + 220;
  BT_DRV_TRACE(3, "AFH ch num:%x %x %x ", *(uint16_t *)tmp1, *(uint16_t *)tmp2,
               *(uint16_t *)tmp3);

  tmp1 = BT_EM_ADDR_BASE + 0x4;
  tmp2 = BT_EM_ADDR_BASE + 0x4 + 110;
  tmp3 = BT_EM_ADDR_BASE + 0x4 + 220;
  BT_DRV_TRACE(3, "clk off:%x %x %x ", *(uint32_t *)tmp1, *(uint32_t *)tmp2,
               *(uint32_t *)tmp3);

  tmp1 = BT_EM_ADDR_BASE + 0x2;
  tmp2 = BT_EM_ADDR_BASE + 0x2 + 110;
  tmp3 = BT_EM_ADDR_BASE + 0x2 + 220;
  BT_DRV_TRACE(3, "bitoff:%x %x %x ", *(uint16_t *)tmp1, *(uint16_t *)tmp2,
               *(uint16_t *)tmp3);
}

void bt_drv_reg_op_ble_llm_substate_hacker(void) {}

bool bt_drv_reg_op_check_esco_acl_sniff_conflict(uint16_t hciHandle) {
  return false;
}

void bt_drv_reg_op_esco_acl_sniff_delay_cal(uint16_t hciHandle, bool enable) {}

uint8_t bt_drv_reg_op_get_role(uint8_t linkid) {
  uint32_t lc_evt_ptr = 0;
  uint32_t role_ptr = 0;

  if (lc_env_addr) {
    lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + linkid * 4); // lc_env
  }

  if (lc_evt_ptr != 0) {
    role_ptr = lc_evt_ptr + 0x40;
  } else {
    BT_DRV_TRACE(1, "BT_REG_OP:ERROR LINKID =%x", linkid);
    return 0xFF;
  }
  return *(uint8_t *)role_ptr;
}

void bt_drv_reg_op_set_tpoll(uint8_t linkid, uint16_t poll_interval) {
  uint32_t acl_evt_ptr = 0x0;
  uint32_t poll_addr;

  if (ld_acl_env_addr) {
    acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr + linkid * 4);
  }

  if (acl_evt_ptr != 0) {
    poll_addr = acl_evt_ptr + 0xb8;
    *(uint16_t *)poll_addr = poll_interval;
  } else {
    BT_DRV_TRACE(1, "BT_REG_OP:ERROR LINK ID FOR TPOLL %x", linkid);
  }
}

int8_t bt_drv_reg_op_rssi_correction(int8_t rssi) {
#ifdef __HW_AGC__
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0) {
    // the rssi is already corrected
    return rssi;
  }
#endif

  return rssi;
}

void bt_drv_reg_op_set_music_link(uint8_t link_id) {
  // dbg_bt_setting.music_playing_link
  if (dbg_bt_setting_addr) {
    *(uint8_t *)(dbg_bt_setting_addr + 0x10) = link_id;
  }
}

void bt_drv_reg_op_set_music_link_duration_extra(uint8_t slot) {
  if (dbg_bt_setting_addr) {
    *(uint32_t *)(dbg_bt_setting_addr + 0x14) = slot * 625;
  }
}

void bt_drv_reg_op_set_hwagc_read_en(uint8_t en) {}
uint32_t bt_drv_reg_op_get_reconnecting_flag() {
  uint32_t ret = 0;

  if (reconnecting_flag_addr) {
    ret = *(uint32_t *)reconnecting_flag_addr;
  }
  return ret;
}

void bt_drv_reg_op_set_reconnecting_flag() {

  if (reconnecting_flag_addr) {
    *(uint32_t *)reconnecting_flag_addr = 1;
  }
}

void bt_drv_reg_op_clear_reconnecting_flag() {

  if (reconnecting_flag_addr) {
    *(uint32_t *)reconnecting_flag_addr = 0;
  }
}

void bt_drv_reg_op_music_link_config(uint16_t active_link, uint8_t active_role,
                                     uint16_t inactive_link,
                                     uint8_t inactive_role) {
  BT_DRV_TRACE(4, "BT_REG_OP:bt_drv_reg_op_music_link_config %x %d %x %d",
               active_link, active_role, inactive_link, inactive_role);
  if (active_role == 0) // MASTER
  {
    bt_drv_reg_op_set_tpoll(active_link - 0x80, 0x10);
    if (inactive_role == 0) {
      bt_drv_reg_op_set_tpoll(inactive_link - 0x80, 0x40);
    }
  } else {
    bt_drv_reg_op_set_music_link(active_link - 0x80);
    bt_drv_reg_op_set_music_link_duration_extra(11);
    if (inactive_role == 0) {
      bt_drv_reg_op_set_tpoll(inactive_link - 0x80, 0x40);
    }
  }
}

bool bt_drv_reg_op_check_bt_controller_state(void) {
  bool ret = true;
  BT_DRV_REG_OP_ENTER();
  if (hal_sysfreq_get() <= HAL_CMU_FREQ_32K)
    return ret;

  if ((BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 0) == 0x00) &&
      (BTDIGITAL_REG(BT_CONTROLLER_CRASH_DUMP_ADDR_BASE + 8) == MIC_BUFF_addr))
    ret = true;
  else
    ret = false;

  if (false == ret) {
    BT_DRV_TRACE(0, "controller dead!!!");
    btdrv_trigger_coredump();
  }

  BT_DRV_REG_OP_EXIT();
  return ret;
}

void bt_drv_reg_op_piconet_clk_offset_get(uint16_t connHandle,
                                          int32_t *clock_offset,
                                          uint16_t *bit_offset) {
  uint8_t index = 0;
  uint32_t clock_offset_raw = 0;

  BT_DRV_REG_OP_ENTER();
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_0) {
    if (connHandle) {
      index = btdrv_conhdl_to_linkid(connHandle);

      if (btdrv_is_link_index_valid(index)) {
        *bit_offset = em_bt_bitoff_getf(index);
        clock_offset_raw =
            (em_bt_clkoff1_getf(index) << 16) | em_bt_clkoff0_getf(index);
        *clock_offset = clock_offset_raw;
        *clock_offset = (*clock_offset << 5) >> 5;
      } else {
        *bit_offset = 0;
        *clock_offset = 0;
      }
    } else {
      *bit_offset = 0;
      *clock_offset = 0;
    }
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_afh_env_reset(void) {
  BT_DRV_REG_OP_ENTER();
  int ret = -1;
  uint32_t ld_afh_env_addr = 0x0;

  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    ld_afh_env_addr = ld_env_addr + 0x198;
    ret = 0;

    if (ret == 0) {
      struct ld_afh_env *ld_afh_env = (struct ld_afh_env *)ld_afh_env_addr;
      if (ld_afh_env) {
        BT_DRV_TRACE(0, "BT_REG_OP:reset ld_afh_env");
        ld_afh_env->rf_rssi_interf_thr = BT_AFH_RSSI_INTERF_THR;
        ld_afh_env->afh_update_period = BT_AFH_UPDATE_PERIOD;
        ld_afh_env->afh_access_valid_to = BT_AFH_ASSESS_VALID_TO;
        ld_afh_env->afh_reaccess_to = BT_AFH_REASSESS_TO;
        ld_afh_env->afh_access_count_max = BT_AFH_ASSESS_COUNT_MAX;
        ld_afh_env->afh_access_count_thr_good = BT_AFH_ASSESS_COUNT_THR_GOOD;
        ld_afh_env->afh_access_count_thr_bad = BT_AFH_ASSESS_COUNT_THR_BAD;
        ld_afh_env->afh_access_count_min = BT_AFH_ASSESS_COUNT_MIN;
      }
    }
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_dma_tc_clkcnt_get(uint32_t *btclk, uint16_t *btcnt) {
  BT_DRV_REG_OP_ENTER();
  *btclk = *((volatile uint32_t *)0xd02201fc);
  *btcnt = *((volatile uint32_t *)0xd02201f8);
  BT_DRV_REG_OP_EXIT();
}

void em_bt_bt_ext1_tx_silence_setf(int elt_idx, uint8_t txsilence) {
  ASSERT_ERR((((uint16_t)txsilence << 15) & ~((uint16_t)0x00008000)) == 0);
  BTDIGITAL_EM_BT_WR(
      EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE,
      (BTDIGITAL_BT_EM(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE) &
       ~((uint16_t)0x00008000)) |
          ((uint16_t)txsilence << 15));
}

void em_bt_bt_ext1_bw_2m_setf(int elt_idx, uint8_t bw2m) {
  ASSERT_ERR((((uint16_t)bw2m << 11) & ~((uint16_t)0x00000800)) == 0);
  BTDIGITAL_EM_BT_WR(
      EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE,
      (BTDIGITAL_BT_EM(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE) &
       ~((uint16_t)0x00000800)) |
          ((uint16_t)bw2m << 11));
}

void em_bt_bt_ext1_sco_tx_silence_setf(int elt_idx, uint8_t scotxsilence) {
  ASSERT_ERR((((uint16_t)scotxsilence << 13) & ~((uint16_t)0x00002000)) == 0);
  BTDIGITAL_EM_BT_WR(
      EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE,
      (BTDIGITAL_BT_EM(EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE) &
       ~((uint16_t)0x00002000)) |
          ((uint16_t)scotxsilence << 13));
}

void bt_drv_reg_op_acl_tx_silence(uint16_t connHandle, uint8_t on) {
  return;
  BT_DRV_REG_OP_ENTER();
  uint8_t idx = btdrv_conhdl_to_linkid(connHandle);
  BT_DRV_TRACE(2, "BT_REG_OP:BT ACL tx silence idx=%d,on=%d\n", idx, on);

  if (btdrv_is_link_index_valid(idx)) {
    if (TX_SILENCE_ON == on) {
      // config ACL silence cs
      em_bt_bt_ext1_tx_silence_setf(idx, TX_SILENCE_ON);
      if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
        // config SCO silence cs
        em_bt_bt_ext1_sco_tx_silence_setf(idx, TX_SILENCE_ON);
        // select ACL silence
        BTDIGITAL_REG_SET_FIELD(0xd02204a8, 3, 2, 2);
      }
    } else if (TX_SILENCE_OFF == on) {
      // config ACL silence cs
      em_bt_bt_ext1_tx_silence_setf(idx, TX_SILENCE_OFF);
      if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
        // config SCO silence cs
        em_bt_bt_ext1_sco_tx_silence_setf(idx, TX_SILENCE_OFF);
        // select ACL silence
        BTDIGITAL_REG_SET_FIELD(0xd02204a8, 3, 2, 3);
      }
    }
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_acl_tx_silence_clear(uint16_t connHandle) {
  uint8_t idx = btdrv_conhdl_to_linkid(connHandle);

  BT_DRV_TRACE(1, "BT_REG_OP:ACL tx silence clear idx=%d\n", idx);

  if (btdrv_is_link_index_valid(idx)) {
    em_bt_bt_ext1_tx_silence_setf(idx, TX_SILENCE_OFF);
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
      // config SCO silence cs
      em_bt_bt_ext1_sco_tx_silence_setf(idx, TX_SILENCE_OFF);
      // select ACL silence
      BTDIGITAL_REG_SET_FIELD(0xd02204a8, 3, 2, 3);
    }
  }
  bt_drv_reg_op_resume_fast_ack_with_mobile();
}

void bt_drv_reg_op_sw_seq_filter(uint16_t connHandle) {}

void bt_drv_reg_op_pcm_set(uint8_t en) {
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
    if (en)
      *(volatile unsigned int *)(0xd02204a8) &= 0xffffffdf;
    else
      *(volatile unsigned int *)(0xd02204a8) |= 1 << 5;
  }
}
uint8_t bt_drv_reg_op_pcm_get() {
  uint8_t ret = 1;
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
    ret = (*(volatile unsigned int *)(0xd02204a8) & 0x00000020) >> 5;
  }
  return ~ret;
}
void bt_drv_reg_op_clear_skip_flag() {}

void bt_drv_reg_op_clean_flags_of_ble_and_sco(void) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_0) {
    // 0xc0006a18,////sco_find_sync_flag
    // 0xc000699c,///ble_sco_need_move_flag
    // 0xc000693c,///prev_conflict_type_record
    // 0xc000099f,///anchor_resume_flag
    ASSERT(*(uint32_t *)0xc0006a14 == 0xc0006a18,
           "ERROR 2300pt1 sco_find_sync_flag!");
    ASSERT(*(uint32_t *)0xc0006998 == 0xc000699c,
           "ERROR 2300pt1 ble_sco_need_move_flag!");
    ASSERT(*(uint32_t *)0xc0006938 == 0xc000693c,
           "ERROR 2300pt1 prev_conflict_type_record!");
    // BT_DRV_TRACE(4,"0xc0006a18=%x,0xc000699c=%x,0xc000693c=%x,0xc000099f=%x\n",*(volatile
    // uint32_t *)0xc0006a18,*(volatile uint32_t *)0xc000699c,*(volatile
    // uint32_t
    // *)0xc000693c,*(volatile uint32_t *)0xc000099f);
    *(volatile uint32_t *)0xc0006a18 = 0;
    *(volatile uint32_t *)0xc0006a88 = 0;
    *(volatile uint32_t *)0xc000693c = 0;
    *(volatile uint32_t *)0xc000099f = 0;
  }
}

const uint8_t msbc_mute_patten[] = {
    0x01, 0x38, 0xad, 0x0,  0x0,  0xc5, 0x0,  0x0,  0x0,  0x0,  0x77, 0x6d,
    0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb, 0x77,
    0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6, 0xdb,
    0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6d, 0xdd, 0xb6,
    0xdb, 0x77, 0x6d, 0xb6, 0xdd, 0xdb, 0x6d, 0xb7, 0x76, 0xdb, 0x6c, 0x00};
#define SCO_TX_FIFO_BASE (0xd0210000)

#if defined(CVSD_BYPASS)
#define SCO_TX_MUTE_PATTERN (0x5555)
#else
#define SCO_TX_MUTE_PATTERN (0x0000)
#endif

void bt_drv_reg_op_sco_txfifo_reset(uint16_t codec_id) {
  uint32_t reg_val = BTDIGITAL_REG(0xd0220148);
  uint32_t reg_offset0, reg_offset1;
  uint16_t *patten_p = (uint16_t *)msbc_mute_patten;

  reg_offset0 = (reg_val & 0xffff);
  reg_offset1 = (reg_val >> 16) & 0xffff;

  if (codec_id == 2) {
    for (uint8_t i = 0; i < 60; i += 2) {
      BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE + reg_offset0 + i) = *patten_p;
      BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE + reg_offset1 + i) = *patten_p;
      patten_p++;
    }
  } else {
    for (uint8_t i = 0; i < 120; i += 2) {
      BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE + reg_offset0 + i) = SCO_TX_MUTE_PATTERN;
      BTDIGITAL_BT_EM(SCO_TX_FIFO_BASE + reg_offset1 + i) = SCO_TX_MUTE_PATTERN;
    }
  }
}

/*****************************************************************************
 Prototype    : btdrv_set_tws_acl_poll_interval
 Description  : in ibrt mode, set tws acl poll interval
 Input        : uint16_t poll_interval
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/19
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void btdrv_reg_op_set_private_tws_poll_interval(uint16_t poll_interval,
                                                uint16_t poll_interval_in_sco) {
  BT_DRV_TRACE(
      2,
      "BT_REG_OP:Set private tws interval,acl interv=%d,acl interv insco =%d",
      poll_interval, poll_interval_in_sco);

  if (dbg_bt_setting_addr) {
    *(volatile uint16_t *)(dbg_bt_setting_addr + 0x40) = poll_interval;
    *(volatile uint16_t *)(dbg_bt_setting_addr + 0x42) = poll_interval_in_sco;
  }
}

void btdrv_reg_op_set_tws_link_duration(uint8_t slot_num) {
  BT_DRV_REG_OP_ENTER();
  // dbg_setting address
  uint32_t op_addr = 0;

  if (dbg_bt_setting_addr) {
    op_addr = dbg_bt_setting_addr;
  }

  if (op_addr != 0) {
    // uint16_t acl_slot = dbg_setting->acl_slot_in_snoop_mode;
    uint16_t val = *(volatile uint16_t *)(op_addr + 0x44);
    val &= 0xff00;
    val |= slot_num;
    *(volatile uint16_t *)(op_addr + 0x44) = val;
    BT_DRV_TRACE(1, "BT_REG_OP:Set private tws link duration,val=%d",
                 *((volatile uint16_t *)(op_addr + 0x44)) & 0xff);
  }
  BT_DRV_REG_OP_EXIT();
}

void btdrv_reg_op_enable_private_tws_schedule(bool enable) {
  BT_DRV_REG_OP_ENTER();
  // sniffer_env
  uint32_t op_addr = 0;

  if (sniffer_env_addr) {
    op_addr = sniffer_env_addr;
  }

  if (op_addr != 0) {
    // sniffer_env.acl_switch_flag.in_process
    uint16_t val = *((volatile uint16_t *)(op_addr + 0x7e));
    val &= 0xff;
    if (enable) {
      val |= 1 << 8;
    }
    *((volatile uint16_t *)(op_addr + 0x7e)) = val;
    BT_DRV_TRACE(1, "BT_REG_OP:Enable private tws function,flag=%d",
                 (*((volatile uint16_t *)(op_addr + 0x7e))) >> 8);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_decrease_tx_pwr_when_reconnect(bool enable) {

  if (enable) {
    BT_DRV_TRACE(0, "BT_REG_OP:Decrese tx pwr");
    // drease defualt TX pwr
    BTDIGITAL_REG(0xd0350300) = 0x33;
  } else {
    BT_DRV_TRACE(0, "BT_REG_OP:Increase tx pwr");
    // resume defualt TX pwr
    BTDIGITAL_REG(0xd0350300) = 0x11;
  }
}
/*
    rwip_heap_env
    from addr=0xc00027b8 left_length=6K
    rwip_heap_non_ret
    from addr=0xc00052b8 left_length=1.5K
*/
void bt_drv_reg_op_controller_mem_log_config(void) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    uint32_t each_log_length = 0x200;          // 512Byte
    uint32_t lmp_mem_addr = 0xc00027b8 + 0x64; // lmp:each_log_length
    uint32_t msg_mem_addr =
        lmp_mem_addr + each_log_length + 0x64; // msg:each_log_length
    uint32_t sta_mem_addr =
        msg_mem_addr + each_log_length + 0x64; // sta:each_log_length
    uint32_t sch_mem_addr =
        sta_mem_addr + each_log_length + 0x64; // sch:each_log_length
    uint32_t isr_mem_addr =
        sch_mem_addr + each_log_length + 0x64; // isr:each_log_length

    // LMP_DUMP_TRACE
    *(uint32_t *)g_mem_dump_ctrl_addr =
        each_log_length; // g_mem_dump_ctrl[0].len_max  default:1024
    *(uint32_t *)(g_mem_dump_ctrl_addr + 4) =
        lmp_mem_addr; // g_mem_dump_ctrl[0].start_addr
    *(uint8_t *)(g_mem_dump_ctrl_addr + 8) =
        0x01; // g_mem_dump_ctrl[0].trace_enable

    // MSG_DUMP_TRACE
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0xc) =
        each_log_length; // g_mem_dump_ctrl[1].len_max  default:1024
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x10) =
        msg_mem_addr; // g_mem_dump_ctrl[1].start_addr
    *(uint8_t *)(g_mem_dump_ctrl_addr + 0x14) =
        0x01; // g_mem_dump_ctrl[1].trace_enable

    // STA_DUMP_TRACE
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x18) =
        each_log_length; // g_mem_dump_ctrl[2].len_max  default:1024
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x1c) =
        sta_mem_addr; // g_mem_dump_ctrl[2].start_addr
    *(uint8_t *)(g_mem_dump_ctrl_addr + 0x20) =
        0x01; // g_mem_dump_ctrl[2].trace_enable

    // SCH_DUMP_TRACE
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x24) =
        each_log_length; // g_mem_dump_ctrl[3].len_max  default:1024
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x28) =
        sch_mem_addr; // g_mem_dump_ctrl[3].start_addr
    *(uint8_t *)(g_mem_dump_ctrl_addr + 0x2c) =
        0x01; // g_mem_dump_ctrl[3].trace_enable

    // ISR_DUMP_TRACE
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x30) =
        each_log_length; // g_mem_dump_ctrl[4].len_max  default:1024
    *(uint32_t *)(g_mem_dump_ctrl_addr + 0x34) =
        isr_mem_addr; // g_mem_dump_ctrl[4].start_addr
    *(uint8_t *)(g_mem_dump_ctrl_addr + 0x38) =
        0x01; // g_mem_dump_ctrl[4].trace_enable
  }
}

void bt_drv_reg_op_lm_nb_sync_hacker(uint8_t sco_status) {
  BT_DRV_REG_OP_ENTER();
  // lm_nb_sync_active
  uint32_t op_addr = 0;

  if (lm_nb_sync_active_addr) {
    op_addr = lm_nb_sync_active_addr;
  }

  if (op_addr != 0) {
    uint8_t org_val = *(volatile uint8_t *)(op_addr);
    // sanity check
    if (org_val > 0xf0)
      org_val = 0;

    if ((sco_status == SNIFFER_SCO_STOP || sco_status == SNIFFER_SCO_START ||
         sco_status == SNIFFER_ACL_DISCONNECT_STOP_SCO) &&
        (org_val >= 0 && org_val <= 2)) {
      if (sco_status == SNIFFER_SCO_STOP ||
          sco_status == SNIFFER_ACL_DISCONNECT_STOP_SCO) {
        if (org_val > 0) {
          *(volatile uint8_t *)(op_addr) = org_val - 1;
        }
      } else if (sco_status == SNIFFER_SCO_START) {
        *(volatile uint8_t *)(op_addr) = org_val + 1;
      }
      BT_DRV_TRACE(
          3, "BT_REG_OP:modify lm_nb_sync org val=%d, modify val=%d, status=%d",
          org_val, *(volatile uint8_t *)(op_addr), sco_status);
    } else {
      BT_DRV_TRACE(2,
                   "BT_REG_OP:do not need modify lm_nb_sync val=%d, status=%d",
                   org_val, sco_status);
    }
  }
  BT_DRV_REG_OP_EXIT();
}

#define BT_FASTACK_ADDR 0xD0220468
#define BT_SCO_TRIGGER_BYPASS_ADDR 0xD022046C
void bt_drv_reg_op_fastack_status_checker(uint16_t conhdl) {
  BT_DRV_REG_OP_ENTER();

  uint8_t elt_idx = btdrv_conhdl_to_linkid(conhdl);

  if (btdrv_is_link_index_valid(elt_idx)) {
    uint32_t fast_cs_addr = EM_BT_BT_EXT1_ADDR + elt_idx * BT_EM_SIZE;

    BT_DRV_TRACE(
        3,
        "BT_DRV_REG: fastack cs=0x%x,fast ack reg=0x%x,Seq filter by pass=0x%x",
        *(volatile uint16_t *)(fast_cs_addr),    // fa rx bit10,tx bit9
        *(volatile uint32_t *)(BT_FASTACK_ADDR), // fast ack reg bit22
        *(volatile uint32_t
              *)(BT_SCO_TRIGGER_BYPASS_ADDR)); // seq bypass reg bit 18
  }

  bt_drv_reg_op_bt_info_checker();

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_clear_ibrt_snoop_status(void) {
  BT_DRV_REG_OP_ENTER();

  // sniffer_env.snoop_status
  uint32_t op_addr = 0;

  if (sniffer_env_addr) {
    op_addr = sniffer_env_addr + 0x84;
  }

  if (op_addr != 0) {
    BT_DRV_TRACE(0, "BT_REG_OP:clear snoop_status");
    *(volatile uint8_t *)(op_addr) = 0;
  }
  // close fastack register
  bt_drv_reg_op_fa_set(0);
  // clear fastack cs
  for (uint8_t idx = 0; idx < 3; idx++) {
    // fastack cs: rx bit10,tx bit9
    uint32_t fast_cs_addr = EM_BT_BT_EXT1_ADDR + idx * BT_EM_SIZE;
    *(volatile uint16_t *)(fast_cs_addr) &= ~0x600;
  }

  BT_DRV_REG_OP_EXIT();
}

uint8_t bt_drv_reg_op_linkcntl_aclltaddr_getf(uint16_t conhdl) {
  uint8_t elt_idx = btdrv_conhdl_to_linkid(conhdl);

  if (btdrv_is_link_index_valid(elt_idx)) {
    uint16_t localVal =
        BTDIGITAL_BT_EM(EM_BT_LINKCNTL_ADDR + elt_idx * BT_EM_SIZE);
    return ((localVal & ((uint16_t)0x00000700)) >> 8);
  } else {
    return 0;
  }
}

uint8_t bt_drv_reg_op_sync_bt_param(uint8_t *param, uint16_t len) {
  uint8_t status = 0xff;
  BT_DRV_REG_OP_ENTER();
  // link_connect_bak
  uint32_t op_addr = 0;

  if (link_connect_bak_addr) {
    op_addr = link_connect_bak_addr;
  }

  if (op_addr != 0) {
    BT_DRV_TRACE(1, "BT_REG_OP:sync bt param len=%d", len);

    memcpy((uint8_t *)(op_addr), param, len);
    status = 0;
  }

  BT_DRV_REG_OP_EXIT();
  return status;
}

uint32_t bt_drv_reg_op_get_lc_env_ptr(uint16_t conhdl, uint8_t type) {
  uint32_t lc_ptr = 0;
  uint32_t op_ptr = 0;
  BT_DRV_REG_OP_ENTER();

  if (lc_env_addr) {
    lc_ptr = *(uint32_t *)(lc_env_addr + (conhdl - 0x80) * 4);
  }

  BT_DRV_TRACE(3, "BT_REG_OP:lc_env_ptr =0x%x, conhdl=0x%x,type=%d", lc_ptr,
               conhdl, type);

  if (lc_ptr == 0)
    return 0;

  switch (type) {
  case LC_LINK_TAG:
    // link tag len=108
    op_ptr = lc_ptr;
    break;
  case LC_INFO_TAG:
    op_ptr = lc_ptr + 0x6c;
    break;
  case LC_ENC_TAG:
    // enc tag len=166
    op_ptr = lc_ptr + 0x196;
    break;
  case LC_AFH_TAG:
    op_ptr = lc_ptr + 0x26c;
    break;
  case LC_SP_TAG:
    // link tag len=108
    op_ptr = lc_ptr + 0x28c;
    break;
  case LC_EPR_TAG:
    op_ptr = lc_ptr + 0x2f8;
    break;
  case LC_EPC_TAG:
    op_ptr = lc_ptr + 0x2fc;
    break;
  default:
    break;
  }

  BT_DRV_REG_OP_EXIT();
  return op_ptr;
}

#ifdef __BT_ONE_BRING_TWO__
void bt_drv_reg_op_set_music_ongong_flag() {}

void bt_drv_reg_op_clear_music_ongong_flag() {}
#endif
uint16_t bt_drv_reg_op_get_ble_conn_latency(uint16_t ble_conhdl) {
  uint16_t curr_conn_latency = 0xffff;
  return curr_conn_latency;
}
bool bt_drv_reg_op_set_ble_conn_latency(uint16_t ble_conhdl,
                                        uint16_t conn_latency) {
  return false;
}

void bt_drv_reg_op_write_private_public_key(uint8_t *private_key,
                                            uint8_t *public_key) {
  uint8_t *lm_env_ptr = 0;
  uint8_t *lm_private_key_ptr = 0;
  uint8_t *lm_public_key_ptr = 0;
  // lm_env
  if (lm_env_addr) {
    lm_env_ptr = (uint8_t *)lm_env_addr;
  } else {
    return;
  }

  lm_private_key_ptr = lm_env_ptr + 0x6c;
  lm_public_key_ptr = lm_private_key_ptr + 0x18;
  memcpy(lm_private_key_ptr, private_key, 24);
  memcpy(lm_public_key_ptr, public_key, 48);
  BT_DRV_TRACE(0, "private key");
  DUMP8("%02x", lm_private_key_ptr, 24);
  BT_DRV_TRACE(0, "public key");
  DUMP8("%02x", lm_public_key_ptr, 48);
}

void bt_drv_reg_op_for_test_mode_disable(void) {

  if (dbg_bt_setting_addr) {
    *(volatile uint32_t *)(dbg_bt_setting_addr + 0x64) &=
        ~(1 << 8); ////sw_seq_filter_en set 0
    *(volatile uint32_t *)(dbg_bt_setting_addr + 0x64) &=
        ~(1 << 0); ////ecc_enable set 0
    *(volatile uint32_t *)(dbg_bt_setting_addr + 0x1c) &=
        0x00ffffff; ////dbg_trace_level set 0
  }
}

uint16_t bt_drv_reg_op_get_ibrt_sco_hdl(uint16_t acl_hdl) {
  // FIXME
  return acl_hdl | 0x100;
}

void bt_drv_reg_op_get_ibrt_address(uint8_t *addr) {
  struct ld_device_info *mobile_device_info = 0;
  BT_DRV_REG_OP_ENTER();

  if (sniffer_env_addr) {
    mobile_device_info = (struct ld_device_info *)(sniffer_env_addr + 4);
  }

  if (mobile_device_info) {
    if (addr != NULL) {
      memcpy(addr, (uint8_t *)mobile_device_info->bd_addr.addr, 6);
      BT_DRV_TRACE(0, "BT_REG_OP:get mobile address");
      DUMP8("%02x ", addr, 6);
    }
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_set_tws_link_id(uint8_t link_id) {
  // ASSERT();
  BT_DRV_TRACE(1, "set tws link id =%x", link_id);
  ASSERT(link_id < 3 || link_id == 0xff, "error tws link id set");
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    ASSERT(*(uint32_t *)0xc0006e28 == 0xc0006e2c, "PATCH ERROR!!");
    *(uint32_t *)0xc0006e2c = link_id;
  } else if (tws_link_id_addr) {
    if (link_id == 0xff) {
      link_id = 3;
    }
    *(uint8_t *)tws_link_id_addr = link_id;
  }
}

void bt_drv_reg_op_hack_max_slot(uint8_t link_id, uint8_t slot) {
  uint32_t acl_evt_ptr = 0x0;
  uint32_t slot_addr;
  uint32_t stop_lantency_addr = 0;
  BT_DRV_TRACE(2, "hack slot id=%d,slot=%d", link_id, slot);
  if (link_id >= 3)
    return;

  if (ld_acl_env_addr) {
    acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr + link_id * 4);
  }

  if (acl_evt_ptr) {
    slot_addr = acl_evt_ptr + 0xc4;
    stop_lantency_addr = acl_evt_ptr + 0x18;
    *(uint8_t *)slot_addr = slot;
    *(uint8_t *)stop_lantency_addr =
        slot + 3 + (*(uint16_t *)(acl_evt_ptr + 150) > 128 ? 1 : 0);
  }
}

void bt_drv_reg_op_rx_slot_stop_lantency_checker(void) {
  uint32_t acl_evt_ptr = 0x0;
  uint32_t slot_addr;
  uint32_t stop_addr;

  for (uint8_t i = 0; i < 3; i++) {

    if (ld_acl_env_addr) {
      acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr + i * 4);
    }

    if (acl_evt_ptr) {
      slot_addr = acl_evt_ptr + 0xc4;
      stop_addr = acl_evt_ptr + 0x18;
      BT_DRV_TRACE(
          5, "id=%d,rx slot=%d stop=%d,acl_par: clk off 0x%x, bit off 0x%x", i,
          *(uint8_t *)slot_addr, *(uint8_t *)stop_addr,
          *(uint32_t *)(acl_evt_ptr + 140), *(uint16_t *)(acl_evt_ptr + 150));
    }
  }
}

void bt_drv_reg_op_fa_set(uint8_t en) {
  if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
    if (en) {
      *(uint32_t *)0xd0220468 |= 0x00400000;
      BT_DRV_TRACE(1, "%s", __func__);
    } else {
      *(uint32_t *)0xd0220468 &= ~0x00400000;
    }
  }
}

//////return true means controller had received a data packet
bool bt_drv_reg_op_check_seq_pending_status(void) {

  if (pending_seq_error_link_addr) {
    return (*(uint8_t *)pending_seq_error_link_addr == 3);
  } else {
    return TRUE;
  }
}

/// if the flag is not clear when disconnect happened  call this function
void bt_drv_reg_op_clear_seq_pending_status(void) {
  if (pending_seq_error_link_addr) {
    *(uint8_t *)pending_seq_error_link_addr = 3;
  }
}

void bt_drv_reg_op_rssi_checker(void) {
  //  BT_DRV_TRACE(1,"rssi thd=%x",*(uint32_t *)0xc00041e4);

  uint32_t acl_evt_ptr = 0x0;
  uint32_t rssi_acc_addr;
  uint32_t rssi_count_addr;

  for (uint8_t i = 0; i < 3; i++) {

    if (ld_acl_env_addr) {
      acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr + i * 4);
    }

    if (acl_evt_ptr) {
      rssi_acc_addr = acl_evt_ptr + 0xac;
      rssi_count_addr = acl_evt_ptr + 0xae;
      BT_DRV_TRACE(3, "id=%d,rssi acc=%d count=%d", i,
                   *(int16_t *)rssi_acc_addr, *(uint8_t *)rssi_count_addr);
    }
  }
  uint32_t lc_evt_ptr = 0x0;
  uint32_t recv_addr;

  for (uint8_t i = 0; i < 3; i++) {
    if (lc_env_addr) {
      lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + i * 4); // lc_env
    }
    if (lc_evt_ptr) {
      recv_addr = lc_evt_ptr + 0x60;
      BT_DRV_TRACE(2, "id=%d,recv=%x", i, *(uint32_t *)recv_addr);
    }
  }
}

void bt_drv_reg_op_set_link_policy(uint8_t linkid, uint8_t policy) {
  BT_DRV_REG_OP_ENTER();
  uint32_t lc_evt_ptr = 0x0;
  uint32_t policy_addr;
  if (linkid >= 0x3)
    return;

  BT_DRV_TRACE(2, "REG_OP: set link=%d, policy=%d", linkid, policy);

  if (lc_env_addr) {
    lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + linkid * 4); // lc_env
  }

  if (lc_evt_ptr) {
    policy_addr = lc_evt_ptr + 0x2c;
    *(uint8_t *)policy_addr = policy;
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_voice_settings_set(uint16_t voice_settings) {
  uint32_t voice_settings_addr = 0;

  if (hci_env_addr) {
    voice_settings_addr = hci_env_addr + 90;
  }

  if (voice_settings_addr != 0) {
    BT_DRV_TRACE(2, "Fast_bt_init1:%s %d", __func__, voice_settings);
    *(uint16_t *)voice_settings_addr = voice_settings;
  }
}

uint8_t bt_drv_reg_op_lmp_sco_hdl_get(uint16_t sco_handle) {
  if (sco_handle == 0xffff) {
    return 0;
  }

  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    //  uint32_t lc_sco_env_addr = 0xc000079c;
    uint16_t sco_link_id = ((sco_handle >> 8) - 1);
    uint32_t link_params_addr =
        *((uint32_t *)(lc_sco_env_addr + sco_link_id * 8));
    uint8_t sync_hdl = 0;

    if (sco_link_id > 2 || link_params_addr == 0) {
      return 0;
    }

    sync_hdl = *((uint8_t *)(link_params_addr + 4));

    return sync_hdl;
  } else {
    BT_DRV_TRACE(0, "BT_DRV:no need to get sco hdl\n");
  }

  return 0;
}

bool bt_drv_reg_op_lmp_sco_hdl_set(uint16_t sco_handle, uint8_t lmp_hdl) {
  if (sco_handle == 0xffff || lmp_hdl == 0x00) {
    return false;
  }

  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    //   uint32_t lc_sco_env_addr = 0xc000079c;
    uint16_t sco_link_id = ((sco_handle >> 8) - 1);
    uint32_t link_params_addr =
        *((uint32_t *)(lc_sco_env_addr + sco_link_id * 8));
    uint8_t *p_sync_hdl = 0;

    if (sco_link_id > 2 || link_params_addr == 0) {
      return false;
    }

    p_sync_hdl = (uint8_t *)(link_params_addr + 4);

    *p_sync_hdl = lmp_hdl;
    return true;
  } else {
    BT_DRV_TRACE(0, "BT_DRV:no need to set sco hdl\n");
  }

  return false;
}

void bt_drv_reg_op_host_buff_and_flow_ctl_set(uint16_t acl_pkt_len,
                                              uint16_t acl_pkt_num,
                                              uint16_t sync_pkt_len,
                                              uint16_t sync_pkt_num,
                                              bool flow_ctl_en) {
  uint32_t acl_pkt_len_addr = 0;
  uint32_t acl_pkt_num_addr = 0;
  uint32_t sync_pkt_len_addr = 0;
  uint32_t sync_pkt_num_addr = 0;
  uint32_t flow_ctl_addr = 0;

  if (hci_fc_env_addr) {
    flow_ctl_addr = hci_fc_env_addr;
  }

  if (flow_ctl_addr != 0) {
    // hci_fc_env.host_set.acl_pkt_len
    acl_pkt_len_addr = flow_ctl_addr + 2;
    // hci_fc_env.host_set.acl_pkt_num
    acl_pkt_num_addr = flow_ctl_addr + 4;
    // hci_fc_env.host_set.sync_pkt_len;
    sync_pkt_len_addr = flow_ctl_addr + 9;
    // hci_fc_env.host_set.sync_pkt_nb;
    sync_pkt_num_addr = flow_ctl_addr + 10;

    BT_DRV_TRACE(6,
                 "Fast_bt_init2:%s,acl len=%x,acl num=%x,sync len=%x sync "
                 "num=%x,fl_ctl=%d",
                 __func__, acl_pkt_len, acl_pkt_num, sync_pkt_len, sync_pkt_num,
                 flow_ctl_en);

    *(uint8_t *)flow_ctl_addr = flow_ctl_en;
    *(uint16_t *)acl_pkt_len_addr = acl_pkt_len;
    *(uint16_t *)acl_pkt_num_addr = acl_pkt_num;
    *(uint8_t *)sync_pkt_len_addr = sync_pkt_len;
    *(uint16_t *)sync_pkt_num_addr = sync_pkt_num;
  }
}

void bt_drv_reg_op_page_to_set(uint16_t page_to) {
  BT_DRV_REG_OP_ENTER();
  uint32_t page_to_addr = 0;

  if (lm_env_addr) {
    page_to_addr = lm_env_addr + 20;
  }

  if (page_to_addr != 0) {
    BT_DRV_TRACE(2, "Fast_bt_init3:%s,to=%x", __func__, page_to);
    *(uint16_t *)page_to_addr = page_to;
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_btdm_address_set(uint8_t *bt_addr, uint8_t *ble_addr) {
  BT_DRV_REG_OP_ENTER();
  uint32_t btaddr_addr = 0;
  uint32_t bleaddr_addr = 0;
  uint32_t bch_addr = 0;

  if (llm_le_env_addr) {
    bleaddr_addr = llm_le_env_addr + 0x8f;
  }

  if (ld_env_addr) {
    btaddr_addr = ld_env_addr;
    bch_addr = btaddr_addr + 451;
  }

  if (bleaddr_addr != 0) {
    BT_DRV_TRACE(1, "Fast_bt_init4:%s", __func__);
    DUMP8("%02x ", bt_addr, 6);
    DUMP8("%02x ", ble_addr, 6);

    for (uint8_t i = 0; i < 6; i++) {
      *(uint8_t *)(bleaddr_addr + i) = ble_addr[i];
    }

    for (uint8_t i = 0; i < 6; i++) {
      *(uint8_t *)(btaddr_addr + i) = bt_addr[i];
    }
    ld_util_bch_create(bt_addr, (uint8_t *)bch_addr);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_evtmask_set(uint8_t ble_en) {
  BT_DRV_REG_OP_ENTER();
  uint32_t bt_evtmsk_addr = 0;
  uint32_t ble_evtmask_addr = 0;

  if (llm_le_env_addr) {
    ble_evtmask_addr = llm_le_env_addr + 0x64;
  }

  if (hci_env_addr) {
    bt_evtmsk_addr = hci_env_addr;
  }

  if (bt_evtmsk_addr != 0) {
    BT_DRV_TRACE(2, "Fast_bt_init5:%s,ble enable=%x", __func__, ble_en);
    uint8_t bt_mask[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f};
    if (ble_en) {
      bt_mask[7] = 0x3f;
    }
    memcpy((uint8_t *)bt_evtmsk_addr, bt_mask, 8);

    uint8_t ble_mask[8] = {0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy((uint8_t *)ble_evtmask_addr, ble_mask, 8);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_simplepair_mode_set(uint8_t en) {
  BT_DRV_REG_OP_ENTER();

  if (lm_env_addr) {
    *(uint8_t *)(lm_env_addr + 44) = en;
  }

  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_class_of_dev_set(uint8_t *class_of_dev, uint8_t len) {
  BT_DRV_REG_OP_ENTER();
  uint32_t class_of_dev_addr = 0;

  if (ld_env_addr) {
    // ld_env.class_of_dev.A
    class_of_dev_addr = ld_env_addr + 6;
  }

  if (class_of_dev_addr != 0) {
    BT_DRV_TRACE(1, "Fast_bt_init7:%s", __func__);
    DUMP8("%02x ", class_of_dev, 3);
    memcpy((uint8_t *)class_of_dev_addr, class_of_dev, 3);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_sleep_set(bool en) {
  BT_DRV_REG_OP_ENTER();
  uint32_t sleep_addr = 0;

  if (rwip_env_addr) {
    sleep_addr = rwip_env_addr + 12;
  }

  if (sleep_addr != 0) {
    BT_DRV_TRACE(2, "Fast_bt_init:%s en=%d", __func__, en);
    *(uint8_t *)(sleep_addr) = en;
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_sco_fifo_reset(void) {
  uint16_t sco_ptr0 = 0, sco_ptr1 = 0;
  uint8_t *em_area_base = (uint8_t *)0xd0210000;

  BT_DRV_REG_OP_ENTER();
  BTDIGITAL_REG_GET_FIELD(0xd02201cc, 0xffff, 0, sco_ptr0);
  BTDIGITAL_REG_GET_FIELD(0xd02201cc, 0xffff, 16, sco_ptr1);
  BT_DRV_TRACE(3, "bt_drv_reg_op_sco_fifo_reset %d/%d %08x", sco_ptr0, sco_ptr1,
               BTDIGITAL_REG(0xd02201cc));
  memset(em_area_base + sco_ptr0, 0x55, 60);
  memset(em_area_base + sco_ptr1, 0x55, 60);
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_sco_fifo_dump(void) {
  uint16_t sco_ptr0 = 0, sco_ptr1 = 0;
  uint8_t *em_area_base = (uint8_t *)0xd0210000;
  BT_DRV_REG_OP_ENTER();
  BTDIGITAL_REG_GET_FIELD(0xd02201cc, 0xffff, 0, sco_ptr0);
  BTDIGITAL_REG_GET_FIELD(0xd02201cc, 0xffff, 16, sco_ptr1);
  BT_DRV_TRACE(3, "bt_drv_reg_op_sco_fifo_reset %d/%d %08x", sco_ptr0, sco_ptr1,
               BTDIGITAL_REG(0xd02201cc));
  DUMP8("%02x ", em_area_base + sco_ptr0, 10);
  DUMP8("%02x ", em_area_base + sco_ptr1, 10);
  BT_DRV_REG_OP_EXIT();
}

uint8_t bt_drv_reg_op_get_sync_id_op(uint8_t op, uint8_t sync_id) {
  uint8_t sync_link_id = 0xff;
  BT_DRV_REG_OP_ENTER();

  uint32_t sync_id_addr = 0;
  // only 2300P chip id =1 need this hack
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    sync_id_addr = (sniffer_env_addr + 0x3c);
  }

  if (sync_id_addr != 0) {
    if (READ_OP == op) {
      sync_link_id = *(uint8_t *)(sync_id_addr);
    } else if (WRITE_OP == op) {
      *(uint8_t *)(sync_id_addr) = sync_id;
    }
  }
  BT_DRV_REG_OP_EXIT();

  return sync_link_id;
}

void bt_drv_reg_op_disable_reporting_en(uint16_t conhdl) {
  BT_DRV_REG_OP_ENTER();
  uint32_t lc_evt_ptr = 0x0;
  uint32_t report_en_addr;
  uint8_t linkid = btdrv_conhdl_to_linkid(conhdl);

  if (btdrv_is_link_index_valid(linkid)) {
    if (lc_env_addr) {
      lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + linkid * 4); // lc_env
      report_en_addr = lc_evt_ptr + 0x28a;
      *(uint8_t *)report_en_addr = 0;
      BT_DRV_TRACE(0, "REG_OP:disable afh report");
    }
  }
  BT_DRV_REG_OP_EXIT();
}

uint8_t bt_drv_reg_opcheck_controller_ld_lc_layer_mismatch(void) {
  uint8_t cnt = 0;
  BT_DRV_REG_OP_ENTER();
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    for (uint8_t link_id = 0; link_id < 3; link_id++) {
      uint32_t ld_acl_evt_ptr = 0;
      uint32_t lc_evt_ptr = 0;
      ld_acl_evt_ptr = *(uint32_t *)(ld_acl_env_addr + link_id * 4);
      lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + link_id * 4);
      if ((ld_acl_evt_ptr && !lc_evt_ptr) || (!ld_acl_evt_ptr && lc_evt_ptr)) {
        cnt++;
      }
    }
  }
  BT_DRV_REG_OP_EXIT();
  return cnt;
}
void bt_drv_reg_op_ble_sup_timeout_set(uint16_t ble_conhdl, uint16_t sup_to) {
  uint32_t llc_env_ptr = 0;

  if (llc_env_addr) {
    uint32_t llc_env_address = llc_env_addr + ble_conhdl * 4;
    llc_env_ptr = *(volatile uint32_t *)llc_env_address;
  }

  if (llc_env_ptr != 0) {
    uint32_t llc_env_sup_to_address = llc_env_ptr + 0x54;
    *(volatile uint16_t *)llc_env_sup_to_address = sup_to;
    BT_DRV_TRACE(1, "BT_DRV:set ble sup_timeout to %d", sup_to);
  }
}

void bt_drv_reg_op_disable_swagc_nosync_count(void) {
  if (dbg_bt_setting_ext1_addr) {
    *(uint16_t *)(dbg_bt_setting_ext1_addr + 0x14) = 0xa604;
  }
}

void bt_drv_reg_op_hw_sw_agc_select(uint8_t agc_mode) {
  uint32_t hw_agc_select_addr = 0;
  BT_DRV_REG_OP_ENTER();

  if (rwip_rf_addr) {
    hw_agc_select_addr = rwip_rf_addr + 0x37;
  }
  if (hw_agc_select_addr != 0) {
    *(volatile uint8_t *)hw_agc_select_addr = agc_mode;
  }

  BT_DRV_REG_OP_EXIT();
}

void ble_maxevtime_set(int elt_idx, uint16_t value) {
  BTDIGITAL_EM_BT_WR(BLE_MAXEVTIME_ADDR + elt_idx * BLE_EM_CS_SIZE, value);
}

uint16_t ble_maxevtime_get(int elt_idx) {
  return BTDIGITAL_BT_EM(BLE_MAXEVTIME_ADDR + elt_idx * BLE_EM_CS_SIZE);
}

void bt_drv_reg_op_afh_follow_en(bool enable, uint8_t be_followed_link_id,
                                 uint8_t follow_link_id) {
#if BT_AFH_FOLLOW
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    *(uint8_t *)(lm_env_addr + 0x168) =
        be_followed_link_id; // be follow follow link id
    *(uint8_t *)(lm_env_addr + 0x169) = follow_link_id; // follow link id
    *(uint8_t *)(lm_env_addr + 0x167) = enable;         // afh follow enable
  }
#endif
}

void bt_drv_reg_op_set_agc_thd(bool isMaster, bool isSco) {
  BT_DRV_REG_OP_ENTER();
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    ASSERT(*(uint32_t *)0xc00072ec == 0xc0007310,
           "patch ERROR 2300p t2 bt_drv_reg_op_set_agc_thd!");
    BT_DRV_TRACE(1, "isMaster: %d, isSco: %d ", isMaster, isSco);
    if (isMaster) {
      if (isSco) {
        BTDIGITAL_REG(0xc0007310) =
            0x00000020; // link_agc_thd_mobile_time£¬ reg address£º0xc0007310
        BTDIGITAL_REG(0xc0007300) =
            0x00000020; // link_agc_thd_tws_time£¬ reg address£º0xc0007300
      } else {
        BTDIGITAL_REG(0xc0007310) =
            0x00000038; // link_agc_thd_mobile_time£¬ reg address£º0xc0007310
        BTDIGITAL_REG(0xc0007300) =
            0x00000038; // link_agc_thd_tws_time£¬ reg address£º0xc0007300
      }
    } else {
      if (isSco) {
        BTDIGITAL_REG(0xc0007310) =
            0x00000020; // link_agc_thd_mobile_time£¬ reg address£º0xc0007310
        BTDIGITAL_REG(0xc0007300) =
            0x00000020; // link_agc_thd_tws_time£¬ reg address£º0xc0007300
      } else {
        BTDIGITAL_REG(0xc0007310) =
            0x00000038; // link_agc_thd_mobile_time£¬ reg address£º0xc0007310
        BTDIGITAL_REG(0xc0007300) =
            0x00000038; // link_agc_thd_tws_time£¬ reg address£º0xc0007300
      }
    }
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_force_set_sniff_att(uint16_t conhdle) {
  BT_DRV_REG_OP_ENTER();
  uint8_t linkid = btdrv_conhdl_to_linkid(conhdle);
  uint32_t acl_evt_ptr = 0x0;

  if (btdrv_is_link_index_valid(linkid)) {
    if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
      acl_evt_ptr =
          *(volatile uint32_t *)(0xc0000998 + linkid * 4); // ld_acl_env
    } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
      acl_evt_ptr =
          *(volatile uint32_t *)(0xc00009d4 + linkid * 4); // ld_acl_env
    }

    if (acl_evt_ptr != 0) {
      uint32_t sniff_att_addr = acl_evt_ptr + 0xe2;
      if (BT_DRIVER_GET_U16_REG_VAL(sniff_att_addr) < 3) {
        BT_DRIVER_PUT_U16_REG_VAL(sniff_att_addr, 3);
      }
      // set sniff timeout to 0
      BT_DRIVER_PUT_U16_REG_VAL(sniff_att_addr + 4, 0);

      BT_DRV_TRACE(0, "BT_REG_OP:force set sniff att");
    }
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_fa_gain_direct_set(uint8_t gain_idx) {
  BTDIGITAL_REG(0xd0220484) =
      (BTDIGITAL_REG(0xd0220484) & (~0xffff)) | 0x9401 |
      (gain_idx << 1); // lxd 20191105 fixed ecc receive gain
}

uint8_t bt_drv_reg_op_fa_gain_direct_get(void) {
  uint32_t raw_reg = BTDIGITAL_REG(0xd0220484);
  return (raw_reg >> 1) & 0x07;
}

struct rx_gain_fixed_t {
  uint8_t enable;    // enable or disable
  uint8_t bt_or_ble; // 0:bt 1:ble
  uint8_t cs_id;     // link id
  uint8_t gain_idx;  // gain index
};

void bt_drv_reg_op_dgb_link_gain_ctrl_set(uint16_t connHandle,
                                          uint8_t bt_ble_mode, uint8_t gain_idx,
                                          uint8_t enable) {
  if (hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_1) {
    TRACE(1, "%s to all link", __func__);
    uint16_t raw_reg = *((volatile uint16_t *)0xc00041ec);
    if (bt_ble_mode) {
      raw_reg = raw_reg & (~0xff << 8);
      raw_reg = raw_reg | (gain_idx << 8);
    } else {
      raw_reg = raw_reg & ~0xff;
      raw_reg = raw_reg | gain_idx;
    }
    *((volatile uint16_t *)0xc00041ec) = raw_reg;
  }

  struct rx_gain_fixed_t *rx_gain_fixed_p =
      (struct rx_gain_fixed_t *)0xc00004dc;
  uint8_t cs_id = btdrv_conhdl_to_linkid(connHandle);

  if (btdrv_is_link_index_valid(cs_id)) {
    for (uint8_t i = 0; i < 3; i++) {
      if ((rx_gain_fixed_p->cs_id == cs_id) &&
          (rx_gain_fixed_p->bt_or_ble == bt_ble_mode)) {
        rx_gain_fixed_p->enable = enable;
        rx_gain_fixed_p->bt_or_ble = bt_ble_mode;
        rx_gain_fixed_p->gain_idx = gain_idx;
        BT_DRV_TRACE(5, "%s hdl:%x/%x mode:%d idx:%d", __func__, connHandle,
                     cs_id, rx_gain_fixed_p->bt_or_ble, gain_idx);
      }
      rx_gain_fixed_p++;
    }
  }
}

void bt_drv_reg_op_dgb_link_gain_ctrl_clear(uint16_t connHandle,
                                            uint8_t bt_ble_mode) {
  if (hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_1) {
    BT_DRV_TRACE(1, "%s REG_OP: not support", __func__);
  }

  struct rx_gain_fixed_t *rx_gain_fixed_p =
      (struct rx_gain_fixed_t *)0xc00004dc;
  uint8_t cs_id = btdrv_conhdl_to_linkid(connHandle);

  if (btdrv_is_link_index_valid(cs_id)) {
    for (uint8_t i = 0; i < 3; i++) {
      if ((rx_gain_fixed_p->cs_id == cs_id) &&
          (rx_gain_fixed_p->bt_or_ble == bt_ble_mode)) {
        rx_gain_fixed_p->enable = 0;
        rx_gain_fixed_p->bt_or_ble = bt_ble_mode;
        rx_gain_fixed_p->gain_idx = 0;
      }
      rx_gain_fixed_p++;
    }
  }
}

void bt_drv_reg_op_dgb_link_gain_ctrl_init(void) {
  if (hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_1) {
    BT_DRV_TRACE(1, "%s REG_OP: not support", __func__);
  }

  struct rx_gain_fixed_t *rx_gain_fixed_p =
      (struct rx_gain_fixed_t *)0xc00004dc;

  for (uint8_t i = 0; i < 3; i++) {
    rx_gain_fixed_p->cs_id = i;
    rx_gain_fixed_p++;
  }
}

void bt_drv_reg_op_rx_gain_fix(uint16_t connHandle, uint8_t bt_ble_mode,
                               uint8_t gain_idx, uint8_t enable,
                               uint8_t table_idx) {
  if (hal_get_chip_metal_id() <= HAL_CHIP_METAL_ID_1) {
    BT_DRV_TRACE(1, "%s REG_OP: not support", __func__);
  }

  struct rx_gain_fixed_t *rx_gain_fixed_p =
      (struct rx_gain_fixed_t *)0xc00004dc;
  uint8_t cs_id;

  if (bt_ble_mode == 0) // bt
  {
    cs_id = btdrv_conhdl_to_linkid(connHandle);
  } else if (bt_ble_mode == 1) // ble
  {
    cs_id = connHandle;
  } else {
    BT_DRV_TRACE(1, "%s:fix gain fail", __func__);
    return;
  }

  if (table_idx < 3) {
    rx_gain_fixed_p[table_idx].enable = enable;
    rx_gain_fixed_p[table_idx].bt_or_ble = bt_ble_mode;
    rx_gain_fixed_p[table_idx].cs_id = cs_id;
    rx_gain_fixed_p[table_idx].gain_idx = gain_idx;
  }
}

void bt_drv_reg_op_set_ibrt_auto_accept_sco(bool en) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    ASSERT(*(uint32_t *)0xc0007344 == 0xc0007348,
           "ibrt_auto_accept_sco PATCH ERROR!!");
    if (en) {
      BTDIGITAL_REG(0xc0007348) = 1;
      BT_DRV_TRACE(0, "BT_REG_OP:enable ibrt_auto_accept_sco");
    } else {
      BTDIGITAL_REG(0xc0007348) = 0;
    }
  }
}

void bt_drv_reg_op_update_dbg_state(void) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    ASSERT(*(uint32_t *)0xc00076a0 == 0xc00076a4,
           "update_dbg_state PATCH ERROR!!");
    BTDIGITAL_REG(0xc00076a4) = 1;
    BT_DRV_TRACE(0, "BT_REG_OP:update_dbg_state");
  }
}

void bt_drv_reg_op_set_ibrt_reject_sniff_req(bool en) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    ASSERT(*(uint32_t *)0xc0006e6c == 0xbb5ef615,
           "ibrt_reject_sniff_req PATCH ERROR!!");
    BTDIGITAL_REG(0xc0006e70) = en ? 1 : 0;
    // BT_DRV_TRACE(1,"BT_REG_OP:ibrt reject sniff %d", en);
  }
}

uint8_t bt_drv_reg_op_get_esco_nego_airmode(uint8_t sco_link_id) {
  BT_DRV_REG_OP_ENTER();

  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    //  uint32_t lc_sco_env_addr = 0xc000079c;
    uint32_t p_nego_params_addr =
        *((uint32_t *)(lc_sco_env_addr + 4 + sco_link_id * 8));
    uint8_t airmode = 0;

    if (sco_link_id > 2 || p_nego_params_addr == 0) {
      return 0xff;
    }

    airmode = *((uint8_t *)(p_nego_params_addr + 0x5e));

    BT_DRV_TRACE(1, "BT_REG_OP:Get nego esco airmode=%d", airmode);
    return airmode;
  }

  BT_DRV_REG_OP_EXIT();

  return 0;
}

void bt_drv_reg_op_set_max_pwr_rcv(uint16_t connHandle) {
  BT_DRV_REG_OP_ENTER();
  uint32_t lc_evt_ptr = 0x0;
  uint32_t max_power_rcv_addr;
  uint8_t linkid = btdrv_conhdl_to_linkid(connHandle);

  if (lc_env_addr) {
    lc_evt_ptr = *(volatile uint32_t *)(lc_env_addr + linkid * 4); // lc_env
  }

  if (lc_evt_ptr) {
    // lc_env_ptr->link.MaxPowerRcv
    max_power_rcv_addr = lc_evt_ptr + 0x61;
    *(bool *)max_power_rcv_addr = true;
    BT_DRV_TRACE(1, "BT_REG_OP:conhdl=%x, max pwr recieve", connHandle);
  }
  BT_DRV_REG_OP_EXIT();
}

void bt_drv_reg_op_set_rand_seed(uint32_t seed) {
  if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_1) {
    *(volatile uint32_t *)(0xc0006890) = seed;
  } else if (hal_get_chip_metal_id() == HAL_CHIP_METAL_ID_2) {
    *(volatile uint32_t *)(0xc0006998) = seed;
  }
}

uint32_t bt_drv_reg_op_get_host_ref_clk(void) { return 0; }

void bt_drv_reg_op_hci_vender_ibrt_ll_monitor(uint8_t *ptr, uint16_t *p_sum_err,
                                              uint16_t *p_rx_total) {
  const char *monitor_str[29] = {
      "TX DM1",     "TX DH1",    "TX DM3",    "TX DH3",    "TX DM5",
      "TX DH5",     "TX 2DH1",   "TX 3DH1",   "TX 2DH3",   "TX 3DH3",
      "TX 2DH5",    "TX 3DH5",

      "RX DM1",     "RX DH1",    "RX DM3",    "RX DH3",    "RX DM5",
      "RX DH5",     "RX 2DH1",   "RX 3DH1",   "RX 2DH3",   "RX 3DH3",
      "RX 2DH5",    "RX 3DH5",   "hec error", "crc error", "fec error",
      "gard error", "ecc count",
  };

  uint32_t *p = (uint32_t *)ptr;
  uint32_t sum_err = 0;
  uint32_t rx_data_sum = 0;
  uint32_t val;

  TRACE(0, "ibrt_ui_log:ll_monitor");
  for (uint8_t i = 0; i < 29; i++) {
    val = co_read32p(p);
    if (val) {
      if (i >= 12 && i <= 23) {
        rx_data_sum += val;
      }
      if (i > 23) {
        sum_err += val;
      }
      TRACE(2, "%s %d", monitor_str[i], val);
    }
    p += 4;
  }
  *p_sum_err = sum_err;
  *p_rx_total = rx_data_sum;
}

uint8_t bt_drv_reg_op_bt_sync_swagc_en_get(void) { return 0; }
void bt_drv_reg_op_ecc_softbit_process(uint16_t *p_conn_handle1,
                                       uint16_t *p_conn_handle2,
                                       uint16_t length, uint8_t *data) {}

#if defined(PCM_FAST_MODE) && defined(PCM_PRIVATE_DATA_FLAG)
void bt_drv_reg_op_set_pcm_flag() {}
#endif
void bt_drv_reg_op_key_gen_after_reset(bool enable) {}
