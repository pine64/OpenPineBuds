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

#ifndef __RFCOMM_I_H__
#define __RFCOMM_I_H__

#include "co_ppbuff.h"
#include "l2cap_i.h"
#include "cmsis_os.h"

enum rfcomm_event_enum {
    RFCOMM_OPENED,
    RFCOMM_NEW_OPENED,
    RFCOMM_CLOSED,
    RFCOMM_QUERY_FAIL,
    RFCOMM_TX_FINISHED
};

struct rfcomm_config_t {
    uint8 initial_credit;
    uint8 credit_give_limit;
    osMutexId creditMutex;
};

typedef void (*rfcomm_notify_callback_t)(enum rfcomm_event_enum event,
                                    uint32 rfcomm_handle,
                                    void *pdata, uint8 reason, void *priv);

typedef void (*rfcomm_datarecv_callback_t)(uint32 rfcomm_handle,
                                            struct pp_buff *ppb, void *priv);

struct rfcomm_ctx_input {
    struct ctx_content ctx;
    struct bdaddr_t *remote;
    uint32 l2cap_handle;
    uint8  server_channel;
    rfcomm_notify_callback_t rfcomm_notify_callback;
    rfcomm_datarecv_callback_t rfcomm_datarecv_callback;
};

struct rfcomm_ctx_output {
    uint32 rfcomm_handle;
};

#if defined(__cplusplus)
extern "C" {
#endif

int8 rfcomm_init(void);

int8 rfcomm_register_server(int8 server_channel,
                            rfcomm_notify_callback_t notify_callback,
                            rfcomm_datarecv_callback_t datarecv_callback,
                            void *priv);

int8 rfcomm_register_server_ex(int8 server_channel,
                                        rfcomm_notify_callback_t notify_callback,
                                        rfcomm_datarecv_callback_t datarecv_callback,
                                        void *priv,
                                        struct rfcomm_config_t *cfg);

uint32 rfcomm_open(struct bdaddr_t *remote, int8 server_channel,
                   rfcomm_notify_callback_t notify_callback,
                   rfcomm_datarecv_callback_t datarecv_callback);

uint32 rfcomm_open_ex(struct bdaddr_t *remote, int8 server_channel,
                               rfcomm_notify_callback_t rfcomm_notify_callback,
                               rfcomm_datarecv_callback_t rfcomm_datarecv_callback,
                               uint8 initial_credits,
                               uint8 credit_give_limit);

int8 rfcomm_send (uint32 rfcomm_handle, uint8 *data, uint32 datalen, void *priv);

int8 rfcomm_get_server_channel(uint32 rfcomm_handle);

//int8 rfcomm_send_ppb (uint32 rfcomm_handle, struct pp_buff *ppb);

//struct pp_buff *rfcomm_data_ppb_alloc (struct rfcomm_session *s, uint32 datalen);

int8 rfcomm_close (uint32 rfcomm_handle);

int8 rfcomm_unregister_server (int8 server_channel);

int rfcomm_l2cap_notify_callback(enum l2cap_event_enum event,
                            uint32 l2cap_handle, void *pdata, uint8 reason);

void rfcomm_l2cap_datarecv_callback(uint32 l2cap_handle, struct pp_buff *ppb);

uint32 rfcomm_get_l2cap_handle(uint32 rfcomm_handle);

uint32 rfcomm_save_ctx(uint32 rfcomm_handle, uint8_t *buf, uint32_t buf_len);

uint32 rfcomm_restore_ctx(struct rfcomm_ctx_input *input, struct rfcomm_ctx_output *output);
void rfcomm_pts_register_dlci2_channel1(void);
void pts_rfcomm_close(void);
void pts_rfcomm_close_dlci0(void);
void pts_rfcomm_send_data(void);
struct rfcomm_session *rfcomm_session_search_l2caphandle(uint32 l2cap_handle);
struct rfcomm_session *rfcomm_session_search_bdaddr (struct bdaddr_t *bdaddr);
struct rfcomm_dlc *rfcomm_dlc_search(struct rfcomm_session *s, uint8 dlci);
const char *rfcomm_event2str(enum rfcomm_event_enum event);
uint32 rfcomm_get_rfcomm_handle(struct bdaddr_t *remote,int8 server_channel);

struct rfcomm_registered_server_item_t *rfcomm_registered_server_search(
    int8 server_channel );

int8 rfcomm_send_credits(struct rfcomm_session *s, uint8 addr, uint8 credits);
bool rfcomm_l2cap_channle_is_creating(struct bdaddr_t *remote);

#if defined(__cplusplus)
}
#endif

#endif /* __RFCOMM_I_H__ */
