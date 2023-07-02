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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "hal_trace.h"
#include "me_api.h"

#include "cmsis.h"
#include "cmsis_os.h"
#include "nvrecord.h"

// static int DDB_Print_Node(BtDeviceRecord* record)
//{
//    nvrec_trace(0,"record_bdAddr = ");
//    OS_DUMP8("%x ",record->bdAddr.addr,sizeof(record->bdAddr.addr));
//    nvrec_trace(0,"record_trusted = ");
//    OS_DUMP8("%d ",&record->trusted,sizeof((uint8_t)record->trusted));
//    nvrec_trace(0,"record_linkKey = ");
//    OS_DUMP8("%x ",record->linkKey,sizeof(record->linkKey));
//    nvrec_trace(0,"record_keyType = ");
//    OS_DUMP8("%x ",&record->keyType,sizeof(record->keyType));
//    nvrec_trace(0,"record_pinLen = ");
//    OS_DUMP8("%x ",&record->pinLen,sizeof(record->pinLen));
//    return 1;
//}

void ddbif_list_saved_flash(void) { nv_record_flash_flush(); }

bt_status_t ddbif_close(void) {
  ddbif_list_saved_flash();
  return BT_STS_SUCCESS;
}

bt_status_t ddbif_add_record(btif_device_record_t *record) {
  return nv_record_add(section_usrdata_ddbrecord, (void *)record);
}

bt_status_t ddbif_open(const bt_bdaddr_t *bdAddr) {
  bt_status_t status = BT_STS_FAILED;
  status = nv_record_open(section_usrdata_ddbrecord);

  return status;
}

bt_status_t ddbif_find_record(const bt_bdaddr_t *bdAddr,
                              btif_device_record_t *record) {
  return nv_record_ddbrec_find(bdAddr, record);
}

bt_status_t ddbif_delete_record(const bt_bdaddr_t *bdAddr) {
  return nv_record_ddbrec_delete(bdAddr);
}

bt_status_t ddbif_enum_device_records(I16 index, btif_device_record_t *record) {
  return nv_record_enum_dev_records((unsigned short)index, record);
}
