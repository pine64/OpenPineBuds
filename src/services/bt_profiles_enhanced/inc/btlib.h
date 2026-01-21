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
#ifndef __BTLIB_H__
#define __BTLIB_H__

#include "bt_common.h"
#include "hci.h"
#include "co_timer.h"
#include "btm.h"

#if defined(__cplusplus)
extern "C" {
#endif

int8 btlib_send_hci_cmd(uint16 opcode, uint8 *param_data_ptr, uint8 param_len);
int8 btlib_send_acl_data( uint16 conn_handle, uint8 *data_ptr, uint16 data_len, uint8 *priv);
int8 btlib_hcicmd_acl_connect(struct bdaddr_t *bdaddr, 
                              uint16 pkt_type, 
                              uint8 page_scan_repetition_mode, 
                              uint16 clk_off, 
                              uint8 allow_role_switch);
int8 btlib_hcicmd_addsyc_conn(struct btm_conn_item_t *conn);
int8 btlib_hcicmd_addsco_conn(struct btm_conn_item_t *conn, uint16 pkt_type);
int8 btlib_hcicmd_write_scan_enable(uint8 scan_enable);
int8 btlib_hcicmd_write_current_iac_lap(uint8 num);
int8 btlib_hcicmd_reject_conn_req(struct bdaddr_t *bdaddr, uint8 reason);
int8 btlib_hcicmd_accep_conn_req(struct bdaddr_t *bdaddr, uint8 role);
int8 btlib_hcicmd_switch_role(struct bdaddr_t *bdaddr, uint8 role);
int8 btlib_hcicmd_write_page_timeout(uint16_t timeout);
int8 btlib_hcicmd_write_superv_timeout(uint16 connhandle, uint16 superv_timeout);
int8 btlib_hcicmd_create_connection_cancel(struct bdaddr_t *bdaddr);
int8 btlib_hcicmd_set_extended_inquiry_response(uint8 fec, uint8 *buff, uint32 len);
int8 btlib_hcicmd_start_tws_exchange(uint16_t tws_slave_conn_handle, uint16_t mobile_conn_handle);
int8 btlib_hcicmd_enable_lmp_filter(uint16_t conhdl, uint8_t enable);
int8 btlib_hcicmd_enable_fastack(uint16_t conhdl, uint8_t direction, uint8_t enable);
int8 btlib_hcicmd_suspend_ibrt(void);
int8 btlib_hcicmd_stop_ibrt(uint8_t enable,uint8_t reason);
int8 btlib_hcicmd_ibrt_mode_init(uint8_t enable);
int8 btlib_hcicmd_ibrt_role_switch(uint8_t switch_op);
int8 btlib_hcicmd_start_ibrt(uint16 slaveConnHandle, uint16 mobileConnHandle);
int8 btlib_hcicmd_resume_ibrt(uint8_t enable);
int8 btlib_hcicmd_set_tws_pool_interval(uint16_t conn_handle, uint16_t poll_interval);
int8 btlib_hcicmd_set_normal_sync_pos(uint8_t flag, uint8_t linkid, uint8_t normalSyncPos);
int8 btlib_hcicmd_set_connection_qos_info(void *remDev, void *qosInfo);
int8 btlib_hcicmd_set_sniffer_env(uint8 sniffer_active, uint8 sniffer_role, struct bdaddr_t *monitor_bdaddr, struct bdaddr_t *sniffer_bdaddr);
int8 btlib_hcicmd_get_slave_mobile_rssi(uint16_t conn_handle);
int8 btlib_hcicmd_set_link_lbrt_enable(uint16 conn_handle, uint8 enable);
int8 btlib_hcicmd_qos_setup(uint16 conn_handle);
int8 btlib_hcicmd_ble_write_random_addr(struct bdaddr_t *bdaddr);
int8 btlib_hcicmd_ble_write_adv_param(struct hci_write_adv_param *para);
int8 btlib_hcicmd_ble_write_adv_data(U8 len, U8 *data);
int8 btlib_hcicmd_ble_write_scan_rsp_data(U8 len, U8 *data);
int8 btlib_hcicmd_ble_write_adv_en(uint8 en);
int8 btlib_hcicmd_ble_write_scan_param(struct hci_write_ble_scan_param *para);
int8 btlib_hcicmd_ble_write_scan_en(uint8 scan_en, uint8 filter_duplicate);
int8 btlib_hcicmd_ble_clear_wl(void);
int8 btlib_hcicmd_ble_add_dev_to_wl(uint8 addr_type, struct bdaddr_t *bdaddr);
int8 btlib_hcicmd_tws_bdaddr_exchange(uint16 conn_handle);
int8 btlib_hcicmd_accept_sync_conn_req(struct bdaddr_t *bdaddr, uint32 tx_bandwidth, uint32 rx_bandwidth, uint16 max_latency, uint16 voice_setting, uint8 retx_effort, uint16 pkt_type);

int8 btlib_hcicmd_pincode_reply(struct bdaddr_t *bdaddr, uint8 *pin, int8 pinlen);

int8 btlib_hcicmd_pincode_neg_reply(struct bdaddr_t *bdaddr);
int8 btlib_hcicmd_linkkey_reply(struct bdaddr_t *bdaddr, uint8 *linkkey);
int8 btlib_hcicmd_linkkey_neg_reply(struct bdaddr_t *bdaddr);

int8 btlib_hcicmd_authentication_req (uint16 conn_handle);
int8 btlib_hcicmd_write_auth_enable(uint8 flag);
int8 btlib_hcicmd_set_conn_encryption (uint16 conn_handle, uint8 encry_enable);
int8 btlib_hcicmd_disconnect (uint16 conn_handle, uint8 reason);

int8 btlib_hcicmd_write_classofdevice (uint8 *class_de);
int8 btlib_hcicmd_write_localname (uint8 *local_name);
int8 btlib_hcicmd_set_bdaddr (uint8 *address);
int8 btlib_hcicmd_set_ble_bdaddr (const uint8 *address);
int8 btlib_hcicmd_write_memory(uint32 addr, uint32 value, uint8 bytelen);

int8 btlib_hcicmd_sniff_mode(uint16 conn_handle, 
                            uint16 sniff_max_interval, 
                            uint16 sniff_min_interval, 
                            uint16 sniff_attempt, 
                            uint16 sniff_timeout);

int8 btlib_hcicmd_exit_sniff_mode(uint16 conn_handle);
int8 btlib_hcicmd_bt_role_discovery(uint16 conn_handle);
int8 btlib_hcicmd_read_remote_version_info(uint16 conn_handle);
int8 btlib_hcicmd_read_remote_supported_feat(uint16 conn_handle);
int8 btlib_hcicmd_read_remote_extended_feat(uint16 conn_handle, uint8 page_n);
int8 btlib_hcicmd_write_link_policy(uint16 conn_handle, uint16 link_policy_settings);
int8 btlib_hcicmd_lowlayer_monitor(uint16 conn_handle, uint8 control_flag, uint8 report_format, uint32 data_format, uint8 report_unit);
int8 btlib_hcicmd_read_stored_linkkey(struct bdaddr_t *bdaddr, uint8 read_all_flag);
int8 btlib_hcicmd_write_stored_linkkey(struct bdaddr_t *bdaddr, uint8 *linkkey);
int8 btlib_hcicmd_delete_stored_linkkey(struct bdaddr_t *bdaddr, uint8 delete_all_flag);

int8 btlib_hcicmd_inquiry(uint32 lap, uint8 inq_period, uint8 num_rs);
int8 btlib_hcicmd_inquiry_cancel(void);

int8 btlib_hcicmd_remote_name_request(struct hci_cp_remote_name_request *req);
int8 btlib_hcicmd_remote_name_cancel(struct bdaddr_t *bdaddr);
int8 btlib_hcicmd_write_pagescan_type (const uint8 pagescan_type);
int8 btlib_hcicmd_write_inqscan_type (const uint8 inqscan_type);
int8 btlib_hcicmd_write_sleep_enable(uint8 sleep_en);
int8 btlib_hcicmd_write_inquiry_mode(uint8 mode);
int8 btlib_hcicmd_set_sco_switch (const uint16 sco_handle);
int8 btlib_hcicmd_dbg_sniffer_interface (const uint8 subcode, const uint16 connhandle);
int8 btlib_hcicmd_sco_tx_silence (const uint16 connhandle, const uint8 slience_on);

extern void delay_ms(int num);
extern char *co_strncat( char *dst, const char *src, uint32 n );
extern void *co_memcpy_reverse(void *dst, const void *src, uint32 n);
extern void *co_memcpy(void *dst, const void *src, uint32 n);
extern int co_memcmp( const void *s1, const void *s2, uint32 n );
extern int co_strncmp( const char *s1, const char *s2, uint32 n );
extern char *co_strncpy( char *dst, const char *src, uint32 n );
extern char *co_strcpy( char *dst, const char *src );
extern void *co_memset( void *s, int c, uint32 n);

#define bdaddr_equal(addr1, addr2) \
            (co_memcmp((const void *)(addr1),(const void *)(addr2),6) == \
                        0 ? TRUE : FALSE)

#define bdaddr_set(dest, src) \
    do { \
        memcpy((void *)(dest),(void *)(src),6); \
    } while (0);

static inline void bdaddr_cpy(struct bdaddr_t *dst, const struct bdaddr_t *src) {
    co_memcpy(dst, src, sizeof(struct bdaddr_t));
}

void print_bdaddr(const struct bdaddr_t *bdaddr);
int ba2str(const struct bdaddr_t *bdaddr, char *str);
int sprintf(char *buf, const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* __BTLIB_H__ */
