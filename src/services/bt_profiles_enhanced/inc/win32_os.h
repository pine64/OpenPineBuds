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
#include <stdbool.h>
#include <assert.h>

#ifndef __WIN32_OS__
#define __WIN32_OS__

#if defined(__cplusplus)
extern "C" {
#endif

//---- MACROS ----//
#define Plt_Assert(expr,...) assert(expr)
#define Plt_TICKS_TO_MS(ticks) (ticks)
#define POSSIBLY_UNUSED
#define ITER_ASSERT(...)
#define Plt_DUMP8(...)

#define OS_CRITICAL_METHOD      0
#define OS_ENTER_CRITICAL()
#define OS_EXIT_CRITICAL()
#define OSTimeDly(a)

//---- TYPES ----//
typedef short I16;
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned char BOOL;
#define ALIGNED(a)  __attribute__((aligned(a)))
typedef U32 bt_status_t;

enum _bt_status {
    BT_STS_SUCCESS = 0,
    BT_STS_FAILED = 1,
    BT_STS_PENDING = 2,
    BT_STS_BUSY = 11,
    BT_STS_NO_RESOURCES = 12,
    BT_STS_NOT_FOUND = 13,
    BT_STS_DEVICE_NOT_FOUND = 14,
    BT_STS_CONNECTION_FAILED = 15,
    BT_STS_TIMEOUT = 16,
    BT_STS_NO_CONNECTION = 17,
    BT_STS_INVALID_PARM = 18,
    BT_STS_IN_PROGRESS = 19,
    BT_STS_RESTRICTED = 20,
    BT_STS_INVALID_TYPE = 21,
    BT_STS_HCI_INIT_ERR = 22,
    BT_STS_NOT_SUPPORTED = 23,
    BT_STS_IN_USE = 5,
    BT_STS_SDP_CONT_STATE = 24,
    BT_STS_CONTINUE =24,
    BT_STS_CANCELLED = 25,

    /* The last defined status code */
    BT_STS_LAST_CODE = BT_STS_CANCELLED,
};

typedef struct {
    unsigned char address[6];
} __attribute__ ((packed)) bt_bdaddr_t;

typedef unsigned char btif_link_key_type_t;

typedef struct {
    bt_bdaddr_t bdAddr;
    bool trusted;
    unsigned char linkKey[16];
    btif_link_key_type_t keyType;

    unsigned char pinLen;
} btif_device_record_t;

typedef void (*ibrt_cmd_complete_callback)(const unsigned char *para);
typedef bool (*ibrt_io_capbility_callback)(void *bdaddr);

#define BLE_ADV_REPORT_MAX_LEN 31

typedef struct {
    unsigned char type;
    unsigned char addr_type;
    bt_bdaddr_t addr;
    unsigned char data_len;
    unsigned char data[BLE_ADV_REPORT_MAX_LEN];
    unsigned char rssi;
} btif_ble_adv_report;

#define BTIF_BEC_USER_TERMINATED    0x13

//------ VARS ------//
#define BT_LOCAL_NAME "bes_enhanced_stack_core"
extern unsigned char bt_addr[6];
extern unsigned char ble_addr[6];

extern struct besbt_cfg_t besbt_cfg;

//------ DDB ------//
bt_status_t ddbif_delete_record(const bt_bdaddr_t *bdAddr);
bt_status_t ddbif_open(const bt_bdaddr_t *bdAddr);
bt_status_t ddbif_add_record(btif_device_record_t *record);
bt_status_t ddbif_find_record(const bt_bdaddr_t *bdAddr, btif_device_record_t *record);
bt_status_t ddbif_enum_device_records(I16 index, btif_device_record_t *record);

//------ BTIF ------//
void btif_adv_event_report(const btif_ble_adv_report* event);

#if defined(__cplusplus)
}
#endif

#endif // __WIN32_OS__
