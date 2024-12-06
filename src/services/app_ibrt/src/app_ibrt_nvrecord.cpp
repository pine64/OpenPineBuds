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
#include "app_ibrt_nvrecord.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui.h"
#include "app_tws_ibrt_trace.h"
#include "ddbif.h"
#include "factory_section.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include <string.h>

/*****************************************************************************

Prototype    : app_ibrt_ui_config_ibrt_info
Description  : app ui init
Input        : void
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_config_load(void *config) {
  struct nvrecord_env_t *nvrecord_env;
  ibrt_config_t *ibrt_config = (ibrt_config_t *)config;
  // factory_section_original_btaddr_get(ibrt_config->local_addr.address);
  nv_record_env_get(&nvrecord_env);
  if (nvrecord_env->ibrt_mode.mode != IBRT_UNKNOW) {
    ibrt_config->nv_role = nvrecord_env->ibrt_mode.mode;
    ibrt_config->peer_addr = nvrecord_env->ibrt_mode.record.bdAddr;
    ibrt_config->local_addr = nvrecord_env->ibrt_mode.record.bdAddr;
    if (nvrecord_env->ibrt_mode.tws_connect_success == 0) {
      app_ibrt_ui_clear_tws_connect_success_last();
    } else {
      app_ibrt_ui_set_tws_connect_success_last();
    }
  } else {
    ibrt_config->nv_role = IBRT_UNKNOW;
  }
}
/*****************************************************************************

Prototype    : app_ibrt_ui_config_ibrt_info
Description  : app ui init
Input        : void
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
int app_ibrt_nvrecord_find(const bt_bdaddr_t *bd_ddr,
                           nvrec_btdevicerecord **record) {
  return nv_record_btdevicerecord_find(bd_ddr, record);
}
/*****************************************************************************

Prototype    : app_ibrt_nvrecord_update_ibrt_mode_tws
Description  : app_ibrt_nvrecord_update_ibrt_mode_tws
Input        : status
Output       : None
Return Value :
Calls        :
Called By    :

History        :
Date         : 2019/3/29
Author       : bestechnic
Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_update_ibrt_mode_tws(bool status) {
  struct nvrecord_env_t *nvrecord_env;

  TRACE(2, "##%s,%d", __func__, status);
  nv_record_env_get(&nvrecord_env);
  nvrecord_env->ibrt_mode.tws_connect_success = status;
  nv_record_env_set(nvrecord_env);
}
/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_get_latest_mobiles_addr
 Description  : get latest mobile addr from nv
 Input        : bt_bdaddr_t *mobile_addr
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
int app_ibrt_nvrecord_get_latest_mobiles_addr(bt_bdaddr_t *mobile_addr1,
                                              bt_bdaddr_t *mobile_addr2) {
  btif_device_record_t record;
  int found_mobile_addr_count = 0;
  int paired_dev_count = nv_record_get_paired_dev_count();

  /* the tail is latest, the head is oldest */
  for (int i = paired_dev_count - 1; i >= 0; --i) {
    if (BT_STS_SUCCESS != nv_record_enum_dev_records(i, &record)) {
      break;
    }

    if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr)) {
      found_mobile_addr_count += 1;

      if (found_mobile_addr_count == 1) {
        *mobile_addr1 = record.bdAddr;
      } else if (found_mobile_addr_count == 2) {
        *mobile_addr2 = record.bdAddr;
        break;
      }
    }
  }

  return found_mobile_addr_count;
}

int app_ibrt_nvrecord_choice_mobile_addr(bt_bdaddr_t *mobile_addr,
                                         uint8_t index) {
  btif_device_record_t record;
  int paired_dev_count = nv_record_get_paired_dev_count();
  int mobile_found_count = 0;
  if (paired_dev_count < index + 1) {
    return 0;
  }

  /* the tail is latest, the head is oldest */
  for (int i = paired_dev_count - 1; i >= 0; --i) {
    if (BT_STS_SUCCESS != nv_record_enum_dev_records(i, &record)) {
      break;
    }

    if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr)) {
      if (mobile_found_count == index) {
        *mobile_addr = record.bdAddr;
        return 1;
      }

      mobile_found_count += 1;
    }
  }

  return 0;
}
/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_get_mobile_addr
 Description  : get mobile addr from nv
 Input        : bt_bdaddr_t *mobile_addr
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
bt_status_t app_ibrt_nvrecord_get_mobile_addr(bt_bdaddr_t *mobile_addr) {
  btif_device_record_t record1;
  btif_device_record_t record2;
  btif_device_record_t record;
  ibrt_link_type_e link_type;

  uint8_t null_addr[6] = {0};
  ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  int record_num = nv_record_enum_latest_two_paired_dev(&record1, &record2);

  if (memcmp(&p_ibrt_ctrl->mobile_addr.address[0], &null_addr[0],
             BTIF_BD_ADDR_SIZE) &&
      (ddbif_find_record((const bt_bdaddr_t *)&p_ibrt_ctrl->mobile_addr,
                         &record) == BT_STS_SUCCESS)) {
    // first check if mobile addr exist in RAM
    *mobile_addr = p_ibrt_ctrl->mobile_addr;
    return BT_STS_SUCCESS;
  }

  if ((record_num == 1) || (record_num == 2)) {
    // check nv mobile addr
    link_type = app_tws_ibrt_get_link_type_by_addr(&record1.bdAddr);
    if (link_type == MOBILE_LINK) {
      *mobile_addr = record1.bdAddr;
      return BT_STS_SUCCESS;
    } else if (record_num == 2) {
      link_type = app_tws_ibrt_get_link_type_by_addr(&record2.bdAddr);
      if (link_type == MOBILE_LINK) {
        *mobile_addr = record2.bdAddr;
        return BT_STS_SUCCESS;
      }
    }
  }
  return BT_STS_FAILED;
}
/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_delete_all_mobile_record
 Description  : app_ibrt_nvrecord_delete_all_mobile_record
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_delete_all_mobile_record(void) {
  btif_device_record_t record = {0};
  int paired_dev_count = nv_record_get_paired_dev_count();

  for (int i = paired_dev_count - 1; i >= 0; --i) {
    nv_record_enum_dev_records(i, &record);

    if (MOBILE_LINK == app_tws_ibrt_get_link_type_by_addr(&record.bdAddr)) {
      nv_record_ddbrec_delete(&record.bdAddr);
    }
  }
  app_ibrt_if_config_keeper_clear();
}

/*****************************************************************************
 Prototype    : app_ibrt_nvrecord_rebuild
 Description  : app_ibrt_nvrecord_rebuild
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History        :
 Date         : 2019/4/25
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_nvrecord_rebuild(void) {
  struct nvrecord_env_t *nvrecord_env;
  nv_record_env_get(&nvrecord_env);

  nv_record_sector_clear();
  nv_record_env_init();
  nv_record_update_factory_tester_status(
      NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT);
  nv_record_env_set(nvrecord_env);
  nv_record_flash_flush();
  app_ibrt_if_config_keeper_clear();
  app_ibrt_ui_reboot_sdk();
}
