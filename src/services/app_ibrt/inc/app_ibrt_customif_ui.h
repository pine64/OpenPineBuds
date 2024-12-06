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
#ifndef __APP_IBRT_CUSTOMIF_UI_UI__
#define __APP_IBRT_CUSTOMIF_UI_UI__

#define  IBRT_UI_ROLE_SWITCH_TIME_THRESHOLD                         (2)  //
#define  IBRT_UI_ROLE_SWITCH_THRESHOLD_WITH_RSSI                    (10) //dbm
#define  IBRT_UI_CLOSE_BOX_EVENT_WAIT_RESPONSE_TIMEOUT              (400)//ms
#define  IBRT_UI_MOBILE_RECONNECT_WAIT_READY_TIMEOUT                (1000)//ms
#define  IBRT_UI_TWS_RECONNECT_WAIT_READY_TIMEOUT                   (500)//ms
#define  IBRT_UI_RECONNECT_MOBILE_WAIT_RESPONSE_TIMEOUT             (5000)//5s
#define  IBRT_UI_RECONNECT_IBRT_WAIT_RESPONSE_TIMEOUT               (300)//ms
#define  IBRT_UI_NV_SLAVE_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT       (1000)//ms
#define  IBRT_UI_NV_MASTER_RECONNECT_TWS_WAIT_RESPONSE_TIMEOUT      (300)//ms
#define  IBRT_UI_DISABLE_BT_SCAN_TIMEOUT                            (3000000)//5min
#define  IBRT_UI_STOP_IBRT_TIMEOUT                                  (3000000)//40min


#define  IBRT_UI_OPEN_RECONNECT_MOBILE_MAX_TIMES            (0)
#define  IBRT_UI_OPEN_RECONNECT_TWS_MAX_TIMES               (0)
#define  IBRT_UI_RECONNECT_MOBILE_MAX_TIMES                 (30)
#define  IBRT_UI_RECONNECT_TWS_MAX_TIMES                    (30)
#define  IBRT_UI_RECONNECT_IBRT_MAX_TIMES                   (1)

#define  IBRT_UI_LONG_POLL_INTERVAL                         (0x68)
#define  IBRT_UI_DEFAULT_POLL_INTERVAL                      (0x34)
#define  IBRT_UI_SHORT_POLL_INTERVAL                        (0x34)

#define  IBRT_UI_DEFAULT_POLL_INTERVAL_IN_SCO               (0x9c)
#define  IBRT_UI_SHORT_POLL_INTERVAL_IN_SCO                 (0x3C)

#define  IBRT_TWS_PAGE_TIMEOUT_ON_CONNECT_SUCCESS_LAST      (0x2000)
#define  IBRT_TWS_PAGE_TIMEOUT_ON_CONNECT_FAILED_LAST       (0x2000)
#define  IBRT_MOBILE_PAGE_TIMEOUT                           (0x2000)
#define  IBRT_TWS_PAGE_TIMEOUT_ON_RECONNECT_MOBILE_FAILED   (0x2000)
#define  IBRT_TWS_PAGE_TIMEOUT_ON_RECONNECT_MOBILE_SUCCESS  (0x2000)

#define  IBRT_UI_TWS_CONNECTION_TIMEOUT                   (3200)//1600 slot=1s/3200 slot = 2s/8000slot=5s


#define  IBRT_TWS_RECONNECT_ONE_CYCLE                     (3)
#define  IBRT_MOBILE_RECONNECT_ONE_CYCLE                  (5)

#define  IBRT_CONTROLLER_DBG_STATE_TIMEOUT                (200)

#define  IBRT_UI_RX_SEQ_ERROR_TIMEOUT                     (10000)
#define  IBRT_UI_RX_SEQ_ERROR_THRESHOLD                   (50)
#define  IBRT_UI_RX_SEQ_ERROR_RECOVER_TIMEOUT             (5000)


#define  IBRT_STOP_IBRT_WAIT_TIME                         (500)
#define  TWS_CONN_FAILED_WAIT_TIME                        (5)
#define  IBRT_EVENT_HUNG_TIMEOUT                          (200)//ms

#define  IBRT_TWS_SWITCH_RSSI_THRESHOLD                   (30)
#define  IBRT_UI_RADICAL_SAN_INTERVAL_NV_MASTER           (BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL/4)
#define  IBRT_UI_RADICAL_SAN_INTERVAL_NV_SLAVE            ((BTIF_BT_DEFAULT_PAGE_SCAN_INTERVAL/4)-0x48)

/*
* ibrt ui event error handling timeout
*/
#define  SM_RUNNING_TIMEOUT                               (20000)
#define  PEER_SM_RUNNING_TIMEOUT                          (2 * (IBRT_UI_OPEN_RECONNECT_MOBILE_MAX_TIMES+2) * (IBRT_UI_RECONNECT_MOBILE_WAIT_RESPONSE_TIMEOUT+5000))
#define  RECONNECT_PEER_SM_RUNNING_TIMEOUT                (2 * (IBRT_UI_RECONNECT_MOBILE_MAX_TIMES+2) * (IBRT_UI_RECONNECT_MOBILE_WAIT_RESPONSE_TIMEOUT+5000))
#define  CONNECT_NO_03_TIMEOUT                            (15000)
#define  DISCONNECT_NO_05_TIMEOUT                         (8000)

#define  IBRT_UI_TWS_CMD_SEND_TIMEOUT                     (10000)
#define  IBRT_UI_TWS_COUNTER_THRESHOLD                    (500000)
#define  IBRT_UI_TWS_SWITCH_STABLE_TIMEOUT                (300)
#define  IBRT_UI_DISC_TWS_TIMEOUT                         (10000)
#ifdef TEST_OVER_THE_AIR_ENANBLED
#define  IBRT_UI_RSSI_MONITOR_TIMEOUT                     (1000)//ms
#else
#define  IBRT_UI_RSSI_MONITOR_TIMEOUT                     (5000)//ms
#endif
int app_ibrt_customif_ui_start(void);
bool app_ibrt_customif_ui_tws_switch(void);
bool app_ibrt_customif_ui_is_tws_switching(void);

#endif
