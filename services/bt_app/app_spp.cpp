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
#include "cmsis_os.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_chipid.h"
#include "analog.h"
#include "app_audio.h"
#include "app_status_ind.h"
#include "app_bt_stream.h"
#include "nvrecord.h"
#include "nvrecord_env.h"
#include "nvrecord_dev.h"
#include "umm_malloc.h"

#include "cqueue.h"
#include "resources.h"
#include "app_spp.h"
#include "spp_api.h"
#include "sdp_api.h"
#include "app_spp.h"
#include "plat_types.h"

#define SPP_MAX_PACKET_NUM  10

struct spp_device *app_create_spp_device(void)
{
    return btif_create_spp_device();
}

bt_status_t app_spp_send_data(struct spp_device *osDev_t, uint8_t* ptrData, uint16_t *length)
{
    bt_status_t status = BT_STS_FAILED;
    uint8_t *ptrBuf = NULL;
    uint16_t len = *length;

    if(!osDev_t->spp_connected_flag) {
        TRACE(1,"%s spp don't connect\n", __func__);
        return status;
    }

    TRACE(2,"%s length %d", __func__, len);
    //DUMP8("0x%02x ",ptrData,length);

    ptrBuf = (uint8_t *)umm_malloc(len);
    if(NULL  == ptrBuf) {
        TRACE(1,"%s failed to malloc for tx buffer", __func__);
        return status;
    }
    memcpy(ptrBuf, ptrData, len);

    status = btif_spp_write(osDev_t, (char *)ptrBuf, length);
    if(status != BT_STS_SUCCESS) {
        TRACE(2,"%s spp send error status %d", __func__, status);
        umm_free(( void * )ptrBuf);
    }

    return status;
}

void app_spp_open(struct spp_device *osDev_t,
                        btif_remote_device_t  *btDevice,
                        btif_sdp_record_param_t *param,
                        osMutexId mid,
                        uint8_t service_id,
                        spp_callback_t callback)
{
    btif_sdp_record_t *spp_sdp_record = NULL;

#if SPP_SERVER == XA_ENABLED
    struct spp_service  *spp_service_t;

    if(osDev_t->portType == BTIF_SPP_SERVER_PORT) {
        spp_service_t = btif_create_spp_service();
        spp_service_t->rf_service.serviceId = service_id;
        spp_service_t->numPorts = 0;

        spp_sdp_record = btif_sdp_create_record();
        btif_sdp_record_setup(spp_sdp_record, param);
        btif_spp_service_setup(osDev_t, spp_service_t, spp_sdp_record);
    }
#endif

    btif_spp_init_device(osDev_t, SPP_MAX_PACKET_NUM, mid);
    btif_spp_open(osDev_t, btDevice, callback);
}


#if defined(__BQB_PROFILE_TEST__)

#if 1
static struct spp_device *spp_test_dev;
osMutexDef(client_spp_test_mutex);
static uint8_t spp_test_rx_buf[SPP_RECV_BUFFER_SIZE];
#define TEST_PORT_SPP 0x1101
static const uint8_t SppTestSearchReq[] = {
    /* First parameter is the search pattern in data element format. It
    * is a list of 3 UUIDs.
    */
    /* Data Element Sequence, 9 bytes */
    SDP_ATTRIB_HEADER_8BIT(9),
    SDP_UUID_16BIT(TEST_PORT_SPP),
    SDP_UUID_16BIT(PROT_L2CAP),  /* L2CAP UUID in Big Endian */
    SDP_UUID_16BIT(PROT_RFCOMM), /* UUID for RFCOMM in Big Endian */
    /* The second parameter is the maximum number of bytes that can be
    * be received for the attribute list.
    */
    0x00,
    0x64, /* Max number of bytes for attribute is 100 */
    SDP_ATTRIB_HEADER_8BIT(9),
    SDP_UINT_16BIT(AID_PROTOCOL_DESC_LIST),
    SDP_UINT_16BIT(AID_BT_PROFILE_DESC_LIST),
    SDP_UINT_16BIT(AID_ADDITIONAL_PROT_DESC_LISTS)
};
int spp_test_handle_data_event_func(void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen)
{
    TRACE(1,"%s", __func__);
    return 0;
}
static void spp_test_client_callback(struct spp_device *locDev, struct spp_callback_parms *Info)
{
    TRACE(2,"%s event %d", __func__, Info->event);
    switch (Info->event)
    {
        case BTIF_SPP_EVENT_REMDEV_CONNECTED:
            TRACE(1,"::SPP_EVENT_REMDEV_CONNECTED %d\n", Info->event);
            break;

        case BTIF_SPP_EVENT_REMDEV_DISCONNECTED:
            TRACE(1,"::SPP_EVENT_REMDEV_DISCONNECTED %d\n", Info->event);
            break;

        case BTIF_SPP_EVENT_DATA_SENT:
            TRACE(1,"::SPP_EVENT_DATA_SENT %d\n", Info->event);
            break;
            
        default:
            TRACE(1,"::unknown event %d\n", Info->event);
            break;
    }
}
#endif

#if 0
void app_spp_test_client_open(btif_remote_device_t *remote_device)
{
#if 1
    TRACE(1,"[%s]", __func__);
    bt_status_t status;
    osMutexId mid;

    if(spp_test_dev == NULL)
        spp_test_dev = app_create_spp_device();
    //app_spp_init_tx_buf(NULL);
    btif_spp_init_rx_buf(spp_test_dev, spp_test_rx_buf, SPP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(client_spp_test_mutex));
    if (!mid)
    {
        ASSERT(0, "cannot create mutex");
    }
    spp_test_dev->portType = BTIF_SPP_CLIENT_PORT;
    spp_test_dev->app_id = 0x8000;
    spp_test_dev->spp_handle_data_event_func = spp_test_handle_data_event_func;

    btif_spp_init_device(spp_test_dev, SPP_MAX_PACKET_NUM, mid);

    spp_test_dev->sppDev.type.client.rfcommServiceSearchRequestPtr = (uint8_t*)SppTestSearchReq;
    spp_test_dev->sppDev.type.client.rfcommServiceSearchRequestLen = sizeof(SppTestSearchReq);
    status  = btif_spp_open(spp_test_dev, remote_device, spp_test_client_callback);
    TRACE(2,"%s status is %d", __func__, status);
#endif
}
#endif

void pts_spp_client_init(void)
{
    TRACE(1,"[%s]", __func__);
    //bt_status_t status;
    osMutexId mid;

    if(spp_test_dev == NULL)
        spp_test_dev = app_create_spp_device();
    //app_spp_init_tx_buf(NULL);
    btif_spp_init_rx_buf(spp_test_dev, spp_test_rx_buf, SPP_RECV_BUFFER_SIZE);

    mid = osMutexCreate(osMutex(client_spp_test_mutex));
    if (!mid)
    {
        ASSERT(0, "cannot create mutex");
    }
    spp_test_dev->portType = BTIF_SPP_CLIENT_PORT;
    spp_test_dev->app_id = 0x8000;
    spp_test_dev->spp_handle_data_event_func = spp_test_handle_data_event_func;

    btif_spp_init_device(spp_test_dev, SPP_MAX_PACKET_NUM, mid);

    spp_test_dev->sppDev.type.client.rfcommServiceSearchRequestPtr = (uint8_t*)SppTestSearchReq;
    spp_test_dev->sppDev.type.client.rfcommServiceSearchRequestLen = sizeof(SppTestSearchReq);
}

#include "conmgr_api.h"
btif_remote_device_t *btif_cmgr_pts_get_remDev(btif_cmgr_handler_t *cmgr_handler);
extern btif_cmgr_handler_t* pts_cmgr_handler;
void pts_cmgr_callback(btif_cmgr_handler_t *cHandler, 
                              cmgr_event_t    Event, 
                              bt_status_t     Status)
{
    btif_remote_device_t *remDev = btif_cmgr_pts_get_remDev(pts_cmgr_handler);

    TRACE(2,"%s Event %d", __func__, Event);
    if (Event == BTIF_BTEVENT_LINK_CONNECT_IND ||
        Event == BTIF_BTEVENT_LINK_CONNECT_CNF)
    {
        if (Status == BT_STS_SUCCESS)
        {
            TRACE(2,"connect ok cHandler %p remDev=%x",cHandler, remDev);
            btif_spp_open(spp_test_dev, remDev, spp_test_client_callback);
        }
        else
        {
            TRACE(0,"connect failed");
        }
    }

    if (Event == BTIF_BTEVENT_LINK_DISCONNECT)
    {
        if (Status == BT_STS_SUCCESS)
        {
            TRACE(0,"disconnect ok");
        }
        else
        {
            TRACE(0,"disconnect failed");
        }
    }
}
#endif

