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

#include "bluetooth.h"
#include "cqueue.h"
#ifdef MBED
#include "rtos.h"
#endif
#include "app_spp.h"
#include "app_spp_tota.h"
#include "app_tota.h"
#include "app_tota_cmd_code.h"
#include "app_tota_cmd_handler.h"
#include "app_tota_data_handler.h"
#include "resources.h"

#include "plat_types.h"
#include "sdp_api.h"

osMutexDef(tota_spp_gen_mutex);

static bool isTotaSppGeneralConnected = false;
static struct spp_device *tota_spp_gen_dev = NULL;
static struct spp_service *totaSppGenService = NULL;
static btif_sdp_record_t *tota_sdp_gen_record = NULL;

osThreadId tota_spp_gen_read_thread_id = NULL;

static app_spp_tota_tx_done_t app_spp_tota_gen_tx_done_func = NULL;

#if (TOTA_SHARE_TX_RX_BUF == 1)
extern void app_spp_tota_share_buf_create(uint8_t **tota_tx_buf,
                                          uint8_t **tota_rx_buf);
extern bool app_spp_tota_is_to_share_buf(void);
extern uint8_t *app_spp_tota_share_tx_buf_get(void);
extern uint8_t *app_spp_tota_share_rx_buf_get(void);
extern uint16_t app_spp_tota_tx_buf_size(void);
#else
static uint8_t spp_rx_buf[SPP_RECV_BUFFER_SIZE];
static uint8_t totaSppGenTxBuf[TOTA_SPP_TX_BUF_SIZE];
#endif

static uint32_t occupiedTotaSppGenTxBufSize;
static uint32_t offsetToFillTotaSppGenTxData;
static uint8_t *ptrTotaSppGenTxBuf;

uint16_t app_spp_tota_gen_tx_buf_size(void) { return TOTA_SPP_TX_BUF_SIZE; }

void app_spp_tota_gen_init_tx_buf(uint8_t *ptr) {
  ptrTotaSppGenTxBuf = ptr;
  occupiedTotaSppGenTxBufSize = 0;
  offsetToFillTotaSppGenTxData = 0;
}

static void app_spp_tota_gen_free_tx_buf(uint8_t *ptrData, uint32_t dataLen) {
  if (occupiedTotaSppGenTxBufSize > 0) {
    occupiedTotaSppGenTxBufSize -= dataLen;
  }
  TOTA_LOG_DBG(1, "occupiedTotaSppGenTxBufSize %d",
               occupiedTotaSppGenTxBufSize);
}

uint8_t *app_spp_tota_gen_fill_data_into_tx_buf(uint8_t *ptrData,
                                                uint32_t dataLen) {
  ASSERT((occupiedTotaSppGenTxBufSize + dataLen) < TOTA_SPP_TX_BUF_SIZE,
         "Pending SPP General tx data has exceeded the tx buffer size !");

  if ((offsetToFillTotaSppGenTxData + dataLen) > TOTA_SPP_TX_BUF_SIZE) {
    offsetToFillTotaSppGenTxData = 0;
  }

  uint8_t *filledPtr = ptrTotaSppGenTxBuf + offsetToFillTotaSppGenTxData;
  memcpy(filledPtr, ptrData, dataLen);

  offsetToFillTotaSppGenTxData += dataLen;

  occupiedTotaSppGenTxBufSize += dataLen;

  TOTA_LOG_DBG(3,
               "dataLen %d offsetToFillTotaSppGenTxData %d "
               "occupiedTotaSppGenTxBufSize %d",
               dataLen, offsetToFillTotaSppGenTxData,
               occupiedTotaSppGenTxBufSize);

  return filledPtr;
}

extern "C" APP_TOTA_CMD_RET_STATUS_E
app_tota_data_received(uint8_t *ptrData, uint32_t dataLength);
extern "C" APP_TOTA_CMD_RET_STATUS_E app_tota_cmd_received(uint8_t *ptrData,
                                                           uint32_t dataLength);

/****************************************************************************
 * TOTA SPP SDP Entries
 ****************************************************************************/
#if 0
 static const U8 TotaGenClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(3),        /* Data Element Sequence, 6 bytes */ 
    SDP_UUID_16BIT(SC_SERIAL_PORT),     /* Hands-Free UUID in Big Endian */ 
};
//#else
/* 128 bit UUID in Big Endian a8b1fbc4-7855-4235-8633-ff36c8235e16 */
static const uint8_t TOTA_GENERAL_UUID_128[16] = {
    0x16, 0x5e, 0x23, 0xc8, 0x36, 0xff, 0x33, 0x86, 0x35, 0x42, 0x55, 0x78, 0xc4, 0xfb, 0xb1, 0xa8};

/*---------------------------------------------------------------------------
 *
 * ServiceClassIDList
 */
static const U8 TotaGenClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),            /* Data Element Sequence, 17 bytes */
    SDP_UUID_128BIT(TOTA_GENERAL_UUID_128), /* 128 bit UUID in Big Endian */
};
#else
static const U8 TotaGenClassId[] = {
    SDP_ATTRIB_HEADER_8BIT(17),
    DETD_UUID + DESD_16BYTES,
    0x8a,
    0x48,
    0x2a,
    0x08,
    0x55,
    0x07,
    0x42,
    0xac,
    0xb6,
    0x73,
    0xa8,
    0x8d,
    0xf4,
    0x8b,
    0x3f,
    0xc7,
};

#endif

static const U8 TotaSppGenProtoDescList[] = {
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
    SDP_UINT_8BIT(RFCOMM_CHANNEL_TOTA_GENERAL)};

/*
 * BluetoothProfileDescriptorList
 */
static const U8 TotaSppGenProfileDescList[] = {
    SDP_ATTRIB_HEADER_8BIT(8), /* Data element sequence, 8 bytes */

    /* Data element sequence for ProfileDescriptor, 6 bytes */
    SDP_ATTRIB_HEADER_8BIT(6),

    SDP_UUID_16BIT(SC_SERIAL_PORT), /* Uuid16 SPP */
    SDP_UINT_16BIT(0x0102)          /* As per errata 2239 */
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 TotaSppGenServiceName1[] = {
    SDP_TEXT_8BIT(8), /* Null terminated text string */
    'S',
    'p',
    'p',
    'G',
    'e',
    'n',
    '1',
    '\0'};

static const U8 TotaSppGenServiceName2[] = {
    SDP_TEXT_8BIT(8), /* Null terminated text string */
    'S',
    'p',
    'p',
    'G',
    'e',
    'n',
    '2',
    '\0'};

/* SPP attributes.
 *
 * This is a ROM template for the RAM structure used to register the
 * SPP SDP record.
 */
static sdp_attribute_t TotaSppGenSdpAttributes1[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaGenClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppGenProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppGenProfileDescList),

    // SPP service name
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppGenServiceName1),
};

/*
static sdp_attribute_t TotaSppGenSdpAttributes2[] = {

    SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, TotaGenClassId),

    SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, TotaSppGenProtoDescList),

    SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, TotaSppGenProfileDescList),

    // SPP service name
    SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), TotaSppGenServiceName2),
};
*/

extern "C" void reset_programmer_state(unsigned char **buf, size_t *len);
extern unsigned char *g_buf;
extern size_t g_len;

static int tota_general_spp_handle_data_event_func(void *pDev, uint8_t process,
                                                   uint8_t *pData,
                                                   uint16_t dataLen) {
  TOTA_LOG_DBG(2, "[%s]data receive length = %d", __func__, dataLen);
  TOTA_LOG_DUMP("[0x%x]", pData, dataLen);
  // the first two bytes of the data packet is the fixed value 0xFFFF
  app_tota_handle_received_data(pData, dataLen);

  return 0;
}

#if 0
static void tota_spp_general_read_thread(const void *arg)
{
	uint8_t buffer[TOTA_SPP_MAX_PACKET_SIZE];
	U16 maxBytes;
    
    while (1)
	{
		maxBytes = TOTA_SPP_MAX_PACKET_SIZE;
		
		btif_spp_read(tota_spp_gen_dev, (char *)buffer, &maxBytes);
        TOTA_LOG_DBG(2,"[%s]general data receive length = %d",__func__,maxBytes);
        TOTA_LOG_DUMP("[0x%x]",buffer,maxBytes);

        // the first two bytes of the data packet is the fixed value 0xFFFF
        app_tota_handle_received_data(buffer, maxBytes);
    }
}

static void app_spp_tota_gen_create_read_thread(void)
{
    TOTA_LOG_DBG(2,"%s %d\n", __func__, __LINE__);
    tota_spp_gen_read_thread_id = osThreadCreate(osThread(tota_spp_general_read_thread), NULL);
}

static void app_spp_tota_gen_close_read_thread(void)
{
    TOTA_LOG_DBG(2,"%s %d\n", __func__, __LINE__);
    if(tota_spp_gen_read_thread_id)
    {
        osThreadTerminate(tota_spp_gen_read_thread_id);
        tota_spp_gen_read_thread_id = NULL;
    }
}
#endif
static void spp_tota_gen_callback(struct spp_device *locDev,
                                  struct spp_callback_parms *Info) {
  TOTA_LOG_DBG(1, "%s", __func__);
  if (BTIF_SPP_EVENT_REMDEV_CONNECTED == Info->event) {
    TOTA_LOG_DBG(1, "::SPP_GENERAL_EVENT_REMDEV_CONNECTED %d\n", Info->event);
    isTotaSppGeneralConnected = true;
    //        app_spp_tota_gen_create_read_thread();
    app_tota_general_connected(APP_TOTA_CONNECTED);
    app_tota_update_datapath(APP_TOTA_GEN_VIA_SPP);
    // conn_stop_connecting_mobile_supervising();
  } else if (BTIF_SPP_EVENT_REMDEV_DISCONNECTED == Info->event) {
    TOTA_LOG_DBG(1, "::SPP_GENERAL_EVENT_REMDEV_DISCONNECTED %d\n",
                 Info->event);
    isTotaSppGeneralConnected = false;
    //        app_spp_tota_gen_close_read_thread();
    app_tota_disconnected(APP_TOTA_DISCONNECTED);
    app_tota_update_datapath(APP_TOTA_PATH_IDLE);

    app_spp_tota_gen_tx_done_func = NULL;
  } else if (BTIF_SPP_EVENT_DATA_SENT == Info->event) {
    // app_spp_tota_gen_free_tx_buf(Info->tx_buf, Info->tx_data_len);
    struct spp_tx_done *pTxDone = (struct spp_tx_done *)(Info->p.other);
    app_spp_tota_gen_free_tx_buf(pTxDone->tx_buf, pTxDone->tx_data_length);
    if (app_spp_tota_gen_tx_done_func) {
      app_spp_tota_gen_tx_done_func();
    }
  } else {
    TOTA_LOG_DBG(1, "::spp general unknown event %d\n", Info->event);
  }
}

static void app_spp_tota_gen_send_data(uint8_t *ptrData, uint16_t length) {
  if (!isTotaSppGeneralConnected) {
    return;
  }

  btif_spp_write(tota_spp_gen_dev, (char *)ptrData, &length);
}

void app_tota_gen_send_cmd_via_spp(uint8_t *ptrData, uint32_t length) {
  uint8_t *ptrBuf = app_spp_tota_gen_fill_data_into_tx_buf(ptrData, length);
  app_spp_tota_gen_send_data(ptrBuf, (uint16_t)length);
}

void app_tota_gen_send_data_via_spp(uint8_t *ptrData, uint32_t length) {
  TOTA_LOG_DBG(2, "[%s]tota gen send data length = %d", __func__, length);
  uint8_t *ptrBuf = app_spp_tota_gen_fill_data_into_tx_buf(ptrData, length);
  app_spp_tota_gen_send_data(ptrBuf, (uint16_t)length);
}

void app_spp_tota_gen_init(void) {
  uint8_t *rx_buf;
  uint8_t *tx_buf;
  osMutexId mid;
  btif_sdp_record_param_t param;

  if (tota_spp_gen_dev == NULL) {
    tota_spp_gen_dev = btif_create_spp_device();
  }

  if (app_spp_tota_is_to_share_buf() == true) {
    app_spp_tota_share_buf_create(&tx_buf, &rx_buf);
  }
#if (TOTA_SHARE_TX_RX_BUF == 0)
  else {
    rx_buf = &spp_rx_buf[0];
    tx_buf = &totaSppGenTxBuf[0];
  }
#endif
  tota_spp_gen_dev->rx_buffer = rx_buf;

  app_spp_tota_gen_init_tx_buf(tx_buf);
  btif_spp_init_rx_buf(tota_spp_gen_dev, rx_buf, SPP_RECV_BUFFER_SIZE);

  mid = osMutexCreate(osMutex(tota_spp_gen_mutex));
  if (!mid) {
    ASSERT(0, "cannot create mutex");
  }

  if (tota_sdp_gen_record == NULL)
    tota_sdp_gen_record = btif_sdp_create_record();

  param.attrs = &TotaSppGenSdpAttributes1[0],
  param.attr_count = ARRAY_SIZE(TotaSppGenSdpAttributes1);
  param.COD = BTIF_COD_MAJOR_PERIPHERAL;
  btif_sdp_record_setup(tota_sdp_gen_record, &param);

  if (totaSppGenService == NULL)
    totaSppGenService = btif_create_spp_service();

  totaSppGenService->rf_service.serviceId = RFCOMM_CHANNEL_TOTA_GENERAL;
  totaSppGenService->numPorts = 0;
  btif_spp_service_setup(tota_spp_gen_dev, totaSppGenService,
                         tota_sdp_gen_record);

  tota_spp_gen_dev->portType = BTIF_SPP_SERVER_PORT;
  tota_spp_gen_dev->app_id = BTIF_APP_SPP_SERVER_TOTA_GENERAL_ID;
  tota_spp_gen_dev->spp_handle_data_event_func =
      tota_general_spp_handle_data_event_func;

  btif_spp_init_device(tota_spp_gen_dev, 5, mid);

  btif_spp_open(tota_spp_gen_dev, NULL, spp_tota_gen_callback);
}
