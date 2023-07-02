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
#ifndef __SPPNEW_H__
#define __SPPNEW_H__

#include "btlib_more.h"
#include "l2cap_i.h"
#include "bt_co_list.h"
#include "data_link.h"
#include "sdp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum sppnew_event {
    SPPNEW_EVENT_OPEN,
    SPPNEW_EVENT_NEW_OPEN,
    SPPNEW_EVENT_DATA_IND,
    SPPNEW_EVENT_CLOSE,
    SPPNEW_EVENT_TX_HANDLED,
    SPPNEW_EVENT_QUERY_FAIL,
};

enum sppnew_chnl_state {
    SPPNEW_CHNL_STATE_CLOSE,
    SPPNEW_CHNL_STATE_QUERING,
    SPPNEW_CHNL_STATE_CONNECTING,
    SPPNEW_CHNL_STATE_OUT_CLOSING,
    SPPNEW_CHNL_STATE_OPEN,
};

enum sppnew_tx_state {
    SPPNEW_TX_STATE_IDLE,
    SPPNEW_TX_STATE_BUSY,
};

struct sppnew_callback_param {
    union {
        struct {
            uint8 *buff;
            uint32 buff_len;
        } data_ind;
        struct {
            uint8 *buff;
            uint32 buff_len;
        } tx_handled;
        struct {
            void *remDev;
        } open;
        struct {
            void *remDev;
        } new_open;
        struct {
            uint8 reason;
            void *remDev;
        } close;
    } p;
};

enum sppnew_device_type {
    SPPNEW_DEVICE_TYPE_CLIENT,
    SPPNEW_DEVICE_TYPE_SERVER,
};

struct sppnew_channel;
typedef int8 (*sppnew_callback)(struct sppnew_channel *chnl, enum sppnew_event event, struct sppnew_callback_param *param);

struct sppnew_setup {
    uint32 buad_rate;
};

struct sppnew_packet {
    struct list_node node;
    uint8 *buff;
    uint32 buff_len;
};

struct sppnew_config {
    #if 0
    uint8 *tx_buff;
    uint32 tx_buff_len;
    #endif
    sppnew_callback callback;
    uint8 local_server_channel;
    uint32 tx_packet_list_length;
    enum sppnew_device_type dev_type;
    struct sppnew_packet *tx_free_packet;
    void *priv;
};

struct sppnew_channel {
    struct list_node node;
    uint32 tx_size_max;
    uint32 rfcomm_handle;
    struct bdaddr_t bdaddr;
    struct sppnew_config cfg;
    uint8 remote_server_channel;
    enum sppnew_chnl_state state;
    enum sppnew_tx_state tx_state;
    struct list_node tx_pend_list;
    struct list_node tx_free_list;
    struct sppnew_packet *tx_packet;
    struct sdp_request sdp_request;
};

struct sppnew_ctx_input {
    struct ctx_content ctx;
    struct sppnew_channel *chnl;
    struct sppnew_config *cfg;
    uint32 rfcomm_handle;
    struct bdaddr_t *remote;
};

struct sppnew_ctx_output {
};

int8 sppnew_init(struct sppnew_channel *chnl, struct sppnew_config *config);
int8 sppnew_open_server(struct sppnew_channel *chnl, struct rfcomm_config_t * cfg);
int8 sppnew_open_client(struct sppnew_channel *chnl, struct bdaddr_t *bdaddr, uint8 remote_channel);
int8 sppnew_query_and_open_client(struct sppnew_channel *chnl, struct bdaddr_t *bdaddr, uint8 *query, uint32 query_len);
int8 sppnew_write(struct sppnew_channel *chnl, uint8 *buff, uint32 buff_len);
int8 sppnew_setup(struct sppnew_channel *chnl, struct sppnew_setup *setup);
int8 sppnew_disconnect(struct sppnew_channel *chnl);
int8 sppnew_close(struct sppnew_channel *chnl);
enum sppnew_chnl_state sppnew_get_state(struct sppnew_channel *chnl);
void *sppnew_get_priv(struct sppnew_channel *chnl);
const char *sppnew_event2str(enum sppnew_event event);
void _sppnew_rfc_notify_callback(enum rfcomm_event_enum event, uint32 rfcomm_handle, void *pdata, uint8 reason, void *priv);
void _sppnew_rfc_datarecv_callbac(uint32 rfcomm_handle, struct pp_buff *ppb, void *priv);
void sppnew_delete_chnl_note(struct sppnew_channel *chnl);
uint32 sppnew_save_ctx(struct sppnew_channel *chnl, uint8_t *buf, uint32_t buf_len);
uint32 sppnew_restore_ctx(struct sppnew_ctx_input *input, struct sppnew_ctx_output *output);

#ifdef __cplusplus
}
#endif

#endif /* __SPPNEW_H__ */
