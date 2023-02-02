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
#ifdef __INTERACTION__
#if defined(IBRT)
#include "app_ibrt_ota_update.h"
#include "app_bt_stream.h"
#include "app_ibrt_if.h"
#include "app_interaction.h"
#include "app_interaction_ota.h"
#include "app_media_player.h"
#include "app_tws_ctrl_thread.h"
#include "app_tws_ibrt_cmd_handler.h"
#include "app_tws_ibrt_trace.h"
#include "audioflinger.h"
#include "bt_drv_interface.h"
#include "cmsis.h"
#include "cmsis_os.h"
#include "crc32.h"
#include "hal_norflash.h"
#include "me_api.h"
#include "pmu.h"
#include <string.h>
extern uint32_t __ota_upgrade_log_start[];
extern INTERACTION_OTA_VALIDATION_DONE_CFM_T Ota_validation_done_cfm;
extern uint32_t get_upgradeSize_log(void);
extern uint32_t breakPoint_local;
extern uint32_t breakPoint_remote;
extern app_ibrt_ota_cmd_code_e cmd_rsp_type;

uint8_t *ota_address = NULL;
uint32_t ota_address_offset = 0;
bool ota_check_md5 = 1;
uint32_t error_list[16] = {0};
static void app_ota_reset_timer_handler(void const *param) {
  app_interaction_ota_finished_handler();
}

osTimerDef(APP_OTA_RESET, app_ota_reset_timer_handler);
static osTimerId app_ota_reset_timer = NULL;

/*
 * custom cmd handler add here, this is just a example
 */

static void app_ibrt_ota_update_now(uint8_t *p_buff, uint16_t length);
static void app_ibrt_ota_update_now_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length);
static void app_ibrt_ota_update_now_rsp_timeout_handler(uint16_t rsp_seq,
                                                        uint8_t *p_buff,
                                                        uint16_t length);
static void app_ibrt_ota_update_now_rsp_handler(uint16_t rsp_seq,
                                                uint8_t *p_buff,
                                                uint16_t length);

static void app_ibrt_check_update_info(uint8_t *p_buff, uint16_t length);
static void app_ibrt_check_update_info_handler(uint16_t rsp_seq,
                                               uint8_t *p_buff,
                                               uint16_t length);
static void app_ibrt_check_update_info2_handler(uint16_t rsp_seq,
                                                uint8_t *p_buff,
                                                uint16_t length);

static void app_ibrt_sync_breakpoint(uint8_t *p_buff, uint16_t length);
static void app_ibrt_sync_breakpoint_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                             uint16_t length);
static void app_ibrt_sync_breakpoint_rsp_timeout_handler(uint16_t rsp_seq,
                                                         uint8_t *p_buff,
                                                         uint16_t length);
static void app_ibrt_sync_breakpoint_rsp_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length);

static void app_ibrt_validation(uint8_t *p_buff, uint16_t length);
static void app_ibrt_validation_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                        uint16_t length);
static void app_ibrt_validation_rsp_timeout_handler(uint16_t rsp_seq,
                                                    uint8_t *p_buff,
                                                    uint16_t length);
static void app_ibrt_validation_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length);

static void app_ibrt_update_section(uint8_t *p_buff, uint16_t length);
static void app_ibrt_update_section_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                            uint16_t length);
static void app_ibrt_update_section_rsp_timeout_handler(uint16_t rsp_seq,
                                                        uint8_t *p_buff,
                                                        uint16_t length);
static void app_ibrt_update_section_rsp_handler(uint16_t rsp_seq,
                                                uint8_t *p_buff,
                                                uint16_t length);

void app_ibrt_view_update_list(void) {
  TRACE(0, "view_update_list start: =====================");
  for (uint8_t i = 0; i < 16; i += 8)
    TRACE(8, "0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x",
          error_list[i], error_list[i + 1], error_list[i + 2],
          error_list[i + 3], error_list[i + 4], error_list[i + 5],
          error_list[i + 6], error_list[i + 7]);
  TRACE(0, "view_update_list end: =====================");
}

void app_ibrt_view_update_sector(void) {
  TRACE(0, "view_update_sector start: =====================");
  TRACE(2, "0x%08x 0x%08x\n", __ota_upgrade_log_start[0],
        __ota_upgrade_log_start[1]);
  for (uint16_t i = 2; i < 512; i += 8) {
    if (i != 506) {
      TRACE(9, "%d 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x", i,
            __ota_upgrade_log_start[i], __ota_upgrade_log_start[i + 1],
            __ota_upgrade_log_start[i + 2], __ota_upgrade_log_start[i + 3],
            __ota_upgrade_log_start[i + 4], __ota_upgrade_log_start[i + 5],
            __ota_upgrade_log_start[i + 6], __ota_upgrade_log_start[i + 7]);
    } else {
      TRACE(7, "%d 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x", i,
            __ota_upgrade_log_start[i], __ota_upgrade_log_start[i + 1],
            __ota_upgrade_log_start[i + 2], __ota_upgrade_log_start[i + 3],
            __ota_upgrade_log_start[i + 4], __ota_upgrade_log_start[i + 5]);
    }
  }
  TRACE(0, "view_update_sector end: =====================");
}

void reset_error_list() { memset(error_list, 0, sizeof(error_list)); }

void set_all_error_list() {
  memset(error_list, 0xFFFFFFFF, sizeof(error_list));
}

void set_error_list(uint16_t list) {
  if (list < 512)
    error_list[list / 32] |= (1 << (31 - list % 32));
}

void clr_error_list(uint16_t list) {
  if (list < 512)
    error_list[list / 32] &= ~(1 << (31 - list % 32));
}

uint32_t get_error_list() {
  int8_t i, j;
  for (i = 0; i < 16; i++) {
    for (j = 31; j >= 0; j--) {
      if (error_list[i] & (1 << j))
        return (i * 32 + 32 - j) * 0x1000;
    }
  }
  return 0;
}

static app_tws_cmd_instance_t g_ibrt_ota_cmd_handler_table[] = {
    {APP_TWS_CMD_OTA_UPDATE_NOW, "OTA_UPDATE_NOW", app_ibrt_ota_update_now,
     app_ibrt_ota_update_now_handler, RSP_TIMEOUT_DEFAULT,
     app_ibrt_ota_update_now_rsp_timeout_handler,
     app_ibrt_ota_update_now_rsp_handler},
    {APP_TWS_CMD_CHECK_UPDATE_INFO, "CHECK_UPDATE_INFO",
     app_ibrt_check_update_info, app_ibrt_check_update_info_handler, 0, 0, 0},
    {APP_TWS_CMD_CHECK_UPDATE_INFO2, "CHECK_UPDATE_INFO2", 0,
     app_ibrt_check_update_info2_handler, 0, 0, 0},
    {APP_TWS_CMD_SYNC_BREAKPIONT, "SYNC_BREAKPIONT", app_ibrt_sync_breakpoint,
     app_ibrt_sync_breakpoint_handler, RSP_TIMEOUT_DEFAULT,
     app_ibrt_sync_breakpoint_rsp_timeout_handler,
     app_ibrt_sync_breakpoint_rsp_handler},
    {APP_TWS_CMD_VALIDATION_DONE, "VALIDATION_DONE", app_ibrt_validation,
     app_ibrt_validation_handler, RSP_TIMEOUT_DEFAULT,
     app_ibrt_validation_rsp_timeout_handler, app_ibrt_validation_rsp_handler},
    {APP_TWS_CMD_UPDATE_SECTION, "UPDATE_SECTION", app_ibrt_update_section,
     app_ibrt_update_section_handler, RSP_TIMEOUT_DEFAULT,
     app_ibrt_update_section_rsp_timeout_handler,
     app_ibrt_update_section_rsp_handler},
};

int app_ibrt_ota_cmd_table_get(void **cmd_tbl, uint16_t *cmd_size) {
  *cmd_tbl = (void *)&g_ibrt_ota_cmd_handler_table;
  *cmd_size = ARRAY_SIZE(g_ibrt_ota_cmd_handler_table);
  return 0;
}

/*===============================================================================*/
void app_ibrt_ota_update_immediately(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->nv_role == IBRT_MASTER) {
    if (app_ota_reset_timer == NULL)
      app_ota_reset_timer =
          osTimerCreate(osTimer(APP_OTA_RESET), osTimerOnce, NULL);
    osTimerStop(app_ota_reset_timer);
    osTimerStart(app_ota_reset_timer, 500);
    tws_ctrl_send_cmd(APP_TWS_CMD_OTA_UPDATE_NOW, NULL, 0);
  }
}

void app_ibrt_ota_update_now(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_with_rsp(APP_TWS_CMD_OTA_UPDATE_NOW, NULL, 0);
}

void app_ibrt_ota_update_now_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                     uint16_t length) {
  app_interaction_ota_finished_handler();
}

void app_ibrt_ota_update_now_rsp_timeout_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  TRACE(1, "ibrt_ui_log:%s", __func__);
}

void app_ibrt_ota_update_now_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                         uint16_t length) {
  TRACE(2, "ibrt_ui_log:0x%08x,%d", p_buff, length);
}

/*===============================================================================*/
void app_ibrt_ota_check_update_info(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  reset_error_list();
  if (p_ibrt_ctrl->nv_role == IBRT_MASTER)
    tws_ctrl_send_cmd(APP_TWS_CMD_CHECK_UPDATE_INFO, NULL, 0);
}

void app_ibrt_check_update_info(uint8_t *p_buff, uint16_t length) {
  if (ota_address_offset >= FLASH_SECTOR_SIZE_IN_BYTES) {
    TRACE(0,
          "ibrt_ui_log:app_ibrt_get_update_list before ota completed!!!!!!\n");
    ota_check_md5 = 1;
    ota_address_offset = 0;
    ota_address = (uint8_t *)__ota_upgrade_log_start;
    app_ibrt_view_update_list();
    tws_ctrl_send_cmd(cmd_rsp_type, NULL, 0);
  } else {
    ibrt_ota_check_cmd_t cmd;
    cmd.length = APP_TWS_CTRL_BUFFER_LEN;
    cmd.address = ota_address =
        (uint8_t *)__ota_upgrade_log_start + ota_address_offset;
    breakPoint_local = breakPoint_remote = 0;
    TRACE(2, "== > addr:0x%08x length:%d\n", cmd.address, cmd.length);
    app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_CHECK_UPDATE_INFO,
                                  (uint8_t *)&cmd,
                                  sizeof(ibrt_ota_check_cmd_t));
  }
}

void app_ibrt_check_update_info_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                        uint16_t length) {
  ibrt_ota_check_cmd_t *p_ibrt_ota_check_cmd = (ibrt_ota_check_cmd_t *)p_buff;
  app_ibrt_send_cmd_without_rsp(APP_TWS_CMD_CHECK_UPDATE_INFO2,
                                p_ibrt_ota_check_cmd->address,
                                p_ibrt_ota_check_cmd->length);
}

void app_ibrt_check_update_info2_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                         uint16_t length) {
  for (uint16_t i = 0; i < length / 4; i += 2) {
    if (ota_check_md5) {
      ota_check_md5 = 0;
      if (*((uint32_t *)ota_address) != *((uint32_t *)p_buff)) {
        TRACE(0, "md5 header error!!!!!");
        TRACE(2, "== >>>0 0x%08x 0x%08x", *((uint32_t *)ota_address),
              *((uint32_t *)p_buff));
        set_all_error_list();
        ota_address_offset = FLASH_SECTOR_SIZE_IN_BYTES;
        break;
      }
    } else {
      if ((*((uint32_t *)ota_address + i) != *((uint32_t *)p_buff + i)) ||
          (*((uint32_t *)ota_address + i + 1) !=
           *((uint32_t *)p_buff + i + 1))) {
        TRACE(1, "%d offset or crc error!!!!!", i);
        TRACE(2, "== >>>1 0x%08x 0x%08x", *((uint32_t *)ota_address + i),
              *((uint32_t *)p_buff + i));
        TRACE(2, "== >>>2 0x%08x 0x%08x", *((uint32_t *)ota_address + i + 1),
              *((uint32_t *)p_buff + i + 1));
        set_error_list(ota_address_offset / 8 + (i - 2) / 2);
      }
    }
  }
  ota_address_offset += APP_TWS_CTRL_BUFFER_LEN;
  app_ibrt_check_update_info(NULL, 0);
}

/*===============================================================================*/
void app_ibrt_sync_breakpoint(uint8_t *p_buff, uint16_t length) {
  ibrt_ota_sync_breakpoint_cmd_t cmd;
  cmd.local_point = breakPoint_local = get_upgradeSize_log();
  cmd.remote_point = breakPoint_remote = get_error_list();
  app_ibrt_send_cmd_with_rsp(APP_TWS_CMD_SYNC_BREAKPIONT, (uint8_t *)&cmd,
                             sizeof(ibrt_ota_check_cmd_t));
}

void app_ibrt_sync_breakpoint_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                      uint16_t length) {
  ibrt_ota_sync_breakpoint_cmd_t *cmd =
      (ibrt_ota_sync_breakpoint_cmd_t *)p_buff;
  breakPoint_local = cmd->local_point;
  breakPoint_remote = cmd->remote_point;
  tws_ctrl_send_rsp(APP_TWS_CMD_SYNC_BREAKPIONT, rsp_seq, NULL, 0);
}

void app_ibrt_sync_breakpoint_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                          uint16_t length) {
  app_interaction_ota_upgrade_sys_rsp();
}

void app_ibrt_sync_breakpoint_rsp_timeout_handler(uint16_t rsp_seq,
                                                  uint8_t *p_buff,
                                                  uint16_t length) {}

void app_ibrt_validation(uint8_t *p_buff, uint16_t length) {
  ibrt_ota_sync_validation_cmd_t cmd;
  if (get_error_list() == 0) {
    cmd.flag = INTERACTION_SUCESS;
    Ota_validation_done_cfm.status = INTERACTION_SUCESS;
  } else {
    cmd.flag = INTERACTION_VALIDATION_FAILURE;
    Ota_validation_done_cfm.status = INTERACTION_VALIDATION_FAILURE;
  }
  app_interaction_ota_upgrade_is_validation_done_rsp();
  app_ibrt_send_cmd_with_rsp(APP_TWS_CMD_VALIDATION_DONE, (uint8_t *)&cmd,
                             sizeof(ibrt_ota_sync_validation_cmd_t));
}

void app_ibrt_validation_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                 uint16_t length) {
  ibrt_ota_sync_validation_cmd_t *cmd =
      (ibrt_ota_sync_validation_cmd_t *)p_buff;
  if (cmd->flag == INTERACTION_SUCESS) {
    app_interaction_ota_upgrade_is_complete_hanlder(); // for send 0x0305 cmd to
                                                       // phone;
  }
  tws_ctrl_send_rsp(APP_TWS_CMD_VALIDATION_DONE, rsp_seq, NULL, 0);
  app_interaction_ota_finished_handler();
}

void app_ibrt_validation_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                     uint16_t length) {
  app_interaction_ota_upgrade_is_complete_hanlder(); // for send 0x0305 cmd to
                                                     // phone;
  app_interaction_ota_finished_handler();
}

void app_ibrt_validation_rsp_timeout_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                             uint16_t length) {}

/*===============================================================================*/
void app_ibrt_ota_force_update(void) {
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->nv_role == IBRT_MASTER) {
    static uint8_t number = 0;
    ibrt_ota_update_cmd_t cmd = {0};
    cmd.point = get_error_list();
    TRACE(1, "update_point: 0x%08x\n", cmd.point);
    if (cmd.point != 0) {
      cmd.length = APP_TWS_CTRL_BUFFER_LEN;
      cmd.address = OTA_FLASH_ENTRY2_OFFSET + cmd.point +
                    number * APP_TWS_CTRL_BUFFER_LEN;
      memcpy(cmd.buff, (uint8_t *)cmd.address, APP_TWS_CTRL_BUFFER_LEN);
      cmd.number =
          number % (FLASH_SECTOR_SIZE_IN_BYTES / APP_TWS_CTRL_BUFFER_LEN);
      number++;
      TRACE(
          3,
          "ibrt_ui_log:app_ibrt_ota_force_update addr:0x%08x length:%d no:%d\n",
          cmd.address, cmd.length, cmd.number);
      tws_ctrl_send_cmd(APP_TWS_CMD_UPDATE_SECTION, (uint8_t *)&cmd,
                        sizeof(ibrt_ota_update_cmd_t));
      // TRACE(0,"Calculate the crc32 of the segment.");
      // uint32_t startFlashAddr = cmd.address/FLASH_SECTOR_SIZE_IN_BYTES;
      // ota_crc = cmd.crc = crc32(0, (uint8_t *)(startFlashAddr),
      // FLASH_SECTOR_SIZE_IN_BYTES); TRACE(1,"CRC32 of the segement is 0x%x",
      // cmd.crc); TRACE(2,"startFlashAddr is 0x%x, length is
      // %d",startFlashAddr,FLASH_SECTOR_SIZE_IN_BYTES);
    } else {
      TRACE(0, "ibrt_ui_log:app_ibrt_ota_force_update completed!!!!!!\n");
      app_ibrt_ota_update_immediately();
    }
  }
}

void app_ibrt_update_section(uint8_t *p_buff, uint16_t length) {
  app_ibrt_send_cmd_with_rsp(APP_TWS_CMD_UPDATE_SECTION, p_buff, length);
}

void app_ibrt_update_section_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                     uint16_t length) {
  ibrt_ota_update_cmd_t *p_ibrt_ota_update_cmd =
      (ibrt_ota_update_cmd_t *)p_buff;
  uint32_t lock;
  enum HAL_NORFLASH_RET_T ret = HAL_NORFLASH_OK;
  TRACE(4,
        "ibrt_ui_log:app_ibrt_update_section_handler addr:0x%08x length:%d "
        "no:%d point:%d\n",
        p_ibrt_ota_update_cmd->address, p_ibrt_ota_update_cmd->length,
        p_ibrt_ota_update_cmd->number, p_ibrt_ota_update_cmd->point);

  lock = int_lock();
  pmu_flash_write_config();
  if (p_ibrt_ota_update_cmd->number == 0) {
    ret = hal_norflash_erase(HAL_NORFLASH_ID_0, p_ibrt_ota_update_cmd->address,
                             FLASH_SECTOR_SIZE_IN_BYTES);
    TRACE(1, "hal_norflash_erase Sector. %d", ret);
  }

  ret = hal_norflash_write(HAL_NORFLASH_ID_0, p_ibrt_ota_update_cmd->address,
                           (uint8_t *)p_ibrt_ota_update_cmd->buff,
                           p_ibrt_ota_update_cmd->length);
  TRACE(1, "hal_norflash_write Sector. %d", ret);
  pmu_flash_read_config();
  int_unlock(lock);

  ibrt_ota_update_cmd_t cmd = {0};
  cmd.number = p_ibrt_ota_update_cmd->number;
  cmd.point = p_ibrt_ota_update_cmd->point;
  /*
      if(p_ibrt_ota_update_cmd->number ==
     FLASH_SECTOR_SIZE_IN_BYTES/APP_TWS_CTRL_BUFFER_LEN-1){ TRACE(0,"Calculate
     the crc32 of the segment."); uint32_t startFlashAddr =
     p_ibrt_ota_update_cmd->address - FLASH_SECTOR_SIZE_IN_BYTES +
     APP_TWS_CTRL_BUFFER_LEN; cmd.crc = crc32(0, (uint8_t *)(startFlashAddr),
     FLASH_SECTOR_SIZE_IN_BYTES); TRACE(1,"CRC32 of the segement is 0x%x",
     cmd.crc); TRACE(2,"startFlashAddr is 0x%x, length is
     %d",startFlashAddr,FLASH_SECTOR_SIZE_IN_BYTES);
      }
  */
  tws_ctrl_send_rsp(APP_TWS_CMD_UPDATE_SECTION, rsp_seq, (uint8_t *)&cmd,
                    sizeof(ibrt_ota_update_cmd_t));
}

void app_ibrt_update_section_rsp_timeout_handler(uint16_t rsp_seq,
                                                 uint8_t *p_buff,
                                                 uint16_t length) {
  TRACE(1, "ibrt_ui_log:%s", __func__);
}

void app_ibrt_update_section_rsp_handler(uint16_t rsp_seq, uint8_t *p_buff,
                                         uint16_t length) {
  ibrt_ota_update_cmd_t *p_ibrt_ota_update_rsp =
      (ibrt_ota_update_cmd_t *)p_buff;
  TRACE(3,
        "ibrt_ui_log:app_ibrt_update_section_rsp_handler addr:0x%08x length:%d "
        "no:%d\n",
        p_ibrt_ota_update_rsp->address, p_ibrt_ota_update_rsp->length,
        p_ibrt_ota_update_rsp->number);
  if (p_ibrt_ota_update_rsp->number ==
      FLASH_SECTOR_SIZE_IN_BYTES / APP_TWS_CTRL_BUFFER_LEN - 1) {
    TRACE(1, "CRC32 of the update is 0x%08x", p_ibrt_ota_update_rsp->crc);
    // if(p_ibrt_ota_update_rsp->crc == ota_crc)
    clr_error_list(p_ibrt_ota_update_rsp->point);
    app_ibrt_view_update_list();
  }
  app_ibrt_ota_force_update();
}
#endif
#endif
