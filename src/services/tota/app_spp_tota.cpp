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
#include "analog.h"
#include "app_audio.h"
#include "app_bt_stream.h"
#include "app_status_ind.h"
#include "audioflinger.h"
#include "cmsis_os.h"
#include "hal_chipid.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "lockcqueue.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include <stdio.h>
//#include "nvrecord_dev.h"

#include "app_spp.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_data_handler.h"
#include "bluetooth.h"
#include "cqueue.h"
#include "plat_types.h"
#include "resources.h"
#include "sdp_api.h"
#include "spp_api.h"
//#include "app_bt_conn_mgr.h"

#include "tota_stream_data_transfer.h"

static bool isTotaSppConnected = false;
static struct spp_device *tota_spp_dev = NULL;
static struct spp_service *totaSppService = NULL;

osMutexDef(tota_spp_mutex);
osMutexDef(tota_credit_mutex);

static btif_sdp_record_t *tota_sdp_record = NULL;

static uint8_t totaSppTxBuf[TOTA_SPP_TX_BUF_SIZE];
static uint8_t spp_rx_buf[SPP_RECV_BUFFER_SIZE];

static uint32_t occupiedTotaSppTxBufSize;
static uint32_t offsetToFillTotaSppTxData;
static uint8_t *ptrTotaSppTxBuf;

#if (TOTA_SHARE_TX_RX_BUF == 1)
static const bool spp_share_application_rx_tx_buf = true;
#else
static const bool spp_share_application_rx_tx_buf = false;
#endif

/**/
static map<APP_TOTA_MODULE_E, tota_callback_func_t> s_module_map;
static tota_callback_func_t s_module_func;
static APP_TOTA_MODULE_E s_module;

/* register callback module */
void tota_callback_module_register(APP_TOTA_MODULE_E module,
                                   tota_callback_func_t tota_callback_func) {
  map<APP_TOTA_MODULE_E, tota_callback_func_t>::iterator it =
      s_module_map.find(module);
  if (it == s_module_map.end()) {
    TOTA_LOG_DBG(0, "add to map");
    s_module_map.insert(make_pair(module, tota_callback_func));
  } else {
    TOTA_LOG_DBG(0, "already exist, not add");
  }
}
/* set current module */
void tota_callback_module_set(APP_TOTA_MODULE_E module) {
  map<APP_TOTA_MODULE_E, tota_callback_func_t>::iterator it =
      s_module_map.find(module);
  if (it != s_module_map.end()) {
    s_module = module;
    s_module_func = it->second;

    TOTA_LOG_DBG(1, "set %d module success", module);
  } else {
    TOTA_LOG_DBG(0, "not find callback func by module");
  }
}

/* get current module */
APP_TOTA_MODULE_E tota_callback_module_get() { return s_module; }

/* is tota busy, use to handle sniff */
bool spp_tota_in_progress(void) {
  TOTA_LOG_DBG(2, "[%s] isTotaSppConnected:%d", __func__, isTotaSppConnected);
  if (isTotaSppConnected == true)
    return true;
  else
    return false;
}

void app_spp_tota_share_buf_create(uint8_t **tota_tx_buf,
                                   uint8_t **tota_rx_buf) {
  *tota_tx_buf = NULL;
  *tota_rx_buf = NULL;

  if (spp_share_application_rx_tx_buf == true) {
    *tota_tx_buf = totaSppTxBuf;
    *tota_rx_buf = spp_rx_buf;
  }
}

bool app_spp_tota_is_to_share_buf(void) {
  return spp_share_application_rx_tx_buf;
}

uint8_t *app_spp_tota_share_tx_buf_get(void) {
  return (spp_share_application_rx_tx_buf == true) ? (totaSppTxBuf) : (NULL);
}
uint8_t *app_spp_tota_share_rx_buf_get(void) {
  return (spp_share_application_rx_tx_buf == true) ? (spp_rx_buf) : (NULL);
}

uint16_t app_spp_tota_tx_buf_size(void) { return TOTA_SPP_TX_BUF_SIZE; }

void app_spp_tota_init_tx_buf(uint8_t *ptr) {
  ptrTotaSppTxBuf = ptr;
  occupiedTotaSppTxBufSize = 0;
  offsetToFillTotaSppTxData = 0;
}

extern "C" APP_TOTA_CMD_RET_STATUS_E
app_tota_data_received(uint8_t *ptrData, uint32_t dataLength);
extern "C" APP_TOTA_CMD_RET_STATUS_E app_tota_cmd_received(uint8_t *ptrData,
                                                           uint32_t dataLength);

/****************************************************************************
 * TOTA SPP SDP Entries
 ****************************************************************************/

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
static const U8 TotaSppClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),      /* Data Element Sequence, 6 bytes */
    SDP_UUID_16BIT(SC_SERIAL_PORT), /* Hands-Free UUID in Big Endian */
};

static const U8 TotaSppProtoDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(12), /* Data element sequence, 12 bytes */

    /* Each element of the list is a Protocol descriptor which is a
     * data element sequence. The first element is L2CAP which only
     * has a UUID element.
     */
    SDP_ATTRIB_HEADER_8BIT(3), /* Data element sequence for L2CAP, 3
                                * bytes
                                */

    SDP_UUID_16BIT(PROT_L2CAP), /* Uuid16 L2CAP */

    /* Next protocol descriptor in the list is RFCOMM. It contains two
     * elements which are the UUID and the channel. Ultimately this
     * channel will need to filled in with value returned by RFCOMM.
     */

    /* Data element sequence for RFCOMM, 5 bytes */
    SDP_ATTRIB_HEADER_8BIT(5),

    SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

    /* Uint8 RFCOMM channel number - value can vary */
    SDP_UINT_8BIT(RFCOMM_CHANNEL_TOTA)};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 TotaSppProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8), /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT), /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)          /* As per errata 2239 */
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 TotaSppServiceName1[] = {
    SDP_TEXT_8BIT(5), /* Null terminated text string */
    'S',
    'p',
    'p',
    '1',
    '\0'};

static const U8 TotaSppServiceName2[] = {
    SDP_TEXT_8BIT(5), /* Null terminated text string */
    'S',
    'p',
    'p',
    '2',
    '\0'};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
// static const SdpAttribute TotaSppSdpAttributes1[] = {
static sdp_attribute_t TotaSppSdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaSppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppProfileDescList),

    /* SPP service name*/
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppServiceName1),
};

/*
static sdp_attribute_t TotaSppSdpAttributes2[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaSppClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppProfileDescList),


    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppServiceName2),
};
*/

int tota_spp_handle_data_event_func(void *pDev, uint8_t process, uint8_t *pData,
                                    uint16_t dataLen) {
  TOTA_LOG_DBG(2, "[%s]data receive length = %d", __func__, dataLen);
  TOTA_LOG_DUMP("[0x%x]", pData, dataLen);

  if (s_module_func.tota_spp_data_receive_hanle != NULL)
    s_module_func.tota_spp_data_receive_hanle(pData, (uint32_t)dataLen);

  // the first two bytes of the data packet is the fixed value 0xFFFF
  app_tota_handle_received_data(pData, dataLen);

  return 0;
}

static void spp_tota_callback(struct spp_device *locDev,
                              struct spp_callback_parms *Info) {
  if (BTIF_SPP_EVENT_REMDEV_CONNECTED == Info->event) {
    TOTA_LOG_DBG(1, "::BTIF_SPP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
    isTotaSppConnected = true;
    app_tota_connected(APP_TOTA_CONNECTED);
    app_tota_update_datapath(APP_TOTA_VIA_SPP);
    // conn_stop_connecting_mobile_supervising();
    if (s_module_func.tota_spp_connected != NULL)
      s_module_func.tota_spp_connected();
  } else if (BTIF_SPP_EVENT_REMDEV_DISCONNECTED == Info->event) {
    TOTA_LOG_DBG(1, "::BTIF_SPP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
    isTotaSppConnected = false;
    app_tota_disconnected(APP_TOTA_DISCONNECTED);
    app_tota_update_datapath(APP_TOTA_PATH_IDLE);
    if (s_module_func.tota_spp_disconnected != NULL)
      s_module_func.tota_spp_disconnected();
  } else if (BTIF_SPP_EVENT_DATA_SENT == Info->event) {
    app_tota_tx_done_callback();
    if (s_module_func.tota_spp_tx_done != NULL)
      s_module_func.tota_spp_tx_done();
  } else {
    TOTA_LOG_DBG(1, "::unknown event %d\n", Info->event);
  }
}

bool app_spp_tota_send_data(uint8_t *ptrData, uint16_t length) {
  if (!isTotaSppConnected) {
    return false;
  }
  btif_spp_write(tota_spp_dev, (char *)ptrData, &length);
  return true;
}

void app_spp_tota_init(void) {
  uint8_t *rx_buf;
  uint8_t *tx_buf;
  osMutexId mid;
  btif_sdp_record_param_t param;

  if (tota_spp_dev == NULL)
    tota_spp_dev = btif_create_spp_device();

  if (app_spp_tota_is_to_share_buf() == true) {
    app_spp_tota_share_buf_create(&tx_buf, &rx_buf);
  } else {
    rx_buf = &spp_rx_buf[0];
    tx_buf = &totaSppTxBuf[0];
  }

  tota_spp_dev->rx_buffer = rx_buf;

  app_spp_tota_init_tx_buf(tx_buf);
  btif_spp_init_rx_buf(tota_spp_dev, rx_buf, SPP_RECV_BUFFER_SIZE);

  mid = osMutexCreate(osMutex(tota_spp_mutex));
  if (!mid) {
    ASSERT(0, "cannot create mutex");
  }

  osMutexId creditMutex = osMutexCreate(osMutex(tota_credit_mutex));

  tota_spp_dev->creditMutex = creditMutex;

  if (tota_sdp_record == NULL)
    tota_sdp_record = btif_sdp_create_record();

  param.attrs = &TotaSppSdpAttributes1[0],
  param.attr_count = ARRAY_SIZE(TotaSppSdpAttributes1);
  param.COD = BTIF_COD_MAJOR_PERIPHERAL;
  btif_sdp_record_setup(tota_sdp_record, &param);

  if (totaSppService == NULL)
    totaSppService = btif_create_spp_service();

  totaSppService->rf_service.serviceId = RFCOMM_CHANNEL_TOTA;
  totaSppService->numPorts = 0;
  btif_spp_service_setup(tota_spp_dev, totaSppService, tota_sdp_record);

  tota_spp_dev->portType = BTIF_SPP_SERVER_PORT;
  tota_spp_dev->app_id = BTIF_APP_SPP_SERVER_TOTA_ID;
  tota_spp_dev->spp_handle_data_event_func = tota_spp_handle_data_event_func;
  btif_spp_init_device(tota_spp_dev, 5, mid);

  btif_spp_open(tota_spp_dev, NULL, spp_tota_callback);
}
