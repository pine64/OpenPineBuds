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
#ifndef __APP_IBRT_AUTO_TEST_CMD_HANDLE_H__
#define __APP_IBRT_AUTO_TEST_CMD_HANDLE_H__
#include <stdint.h>

#ifdef BES_AUTOMATE_TEST

//automate test group code
#define AUTO_TEST_A2DP          0
#define AUTO_TEST_HFP           1
#define AUTO_TEST_BLE           2

#define AUTO_TEST_UI            20
#define AUTO_TEST_VOICE_PROMPT  21

#define AUTO_TEST_AI            30
#define AUTO_TEST_FLASH         31
#define AUTO_TEST_OTA           32

#define AUTO_TEST_UNUSE         127

//voice promt automate test operation code

// A2DP automate test operation code, configure the respective
// .ini file used for auto test script
#define A2DP_AUTO_TEST_AUDIO_PLAY       0
#define A2DP_AUTO_TEST_AUDIO_PAUSE      1
#define A2DP_AUTO_TEST_AUDIO_FORWARD    2
#define A2DP_AUTO_TEST_AUDIO_BACKWARD   3
#define A2DP_AUTO_TEST_AVRCP_VOL_UP     4
#define A2DP_AUTO_TEST_AVRCP_VOL_DOWN   5

// HFP automate test operation code, configure the respective
// .ini file used for auto test script
#define HFP_AUTO_TEST_SCO_CREATE        0
#define HFP_AUTO_TEST_SCO_DISC          1
#define HFP_AUTO_TEST_CALL_REDIAL       2
#define HFP_AUTO_TEST_CALL_ANSWER       3
#define HFP_AUTO_TEST_CALL_HANGUP       4
#define HFP_AUTO_TEST_VOLUME_UP         5
#define HFP_AUTO_TEST_VOLUME_DOWN       6

// UI automate test operation code , configure the respective
// .ini file used for auto test script
#define UI_AUTO_TEST_OPEN_BOX           0
#define UI_AUTO_TEST_CLOSE_BOX          1
#define UI_AUTO_TEST_FETCH_OUT_BOX      2
#define UI_AUTO_TEST_PUT_IN_BOX         3
#define UI_AUTO_TEST_WEAR_UP            4
#define UI_AUTO_TEST_WEAR_DOWN          5
#define UI_AUTO_TEST_ROLE_SWITCH        6
#define UI_AUTO_TEST_PHONE_CONN_EVENT   7
#define UI_AUTO_TEST_RECONN_EVENT       8
#define UI_AUTO_TEST_CONN_SECOND_MOBILE 9
#define UI_AUTO_TEST_MOBILE_TWS_DISC    10
#define UI_AUTO_TEST_PAIRING_MODE       11
#define UI_AUTO_TEST_FREEMAN_MODE       12
#define UI_AUTO_TEST_SUSPEND_IBRT       13
#define UI_AUTO_TEST_RESUME_IBRT        14
#define UI_AUTO_TEST_SHUT_DOWN          15
#define UI_AUTO_TEST_REBOOT             16
#define UI_AUTO_TEST_FACTORY_RESET      17
#define UI_AUTO_TEST_ASSERT             18
#define UI_AUTO_TEST_CONNECT_MOBILE     19
#define UI_AUTO_TEST_DISCONNECT_MOBILE  20
#define UI_AUTO_TEST_CONNECT_TWS        21
#define UI_AUTO_TEST_DISCONNECT_TWS     22
#define UI_AUTO_TEST_ENABLE_TPORTS      23

// AI automate test operation code, configure the respective
// .ini file used for auto test script
#define AI_AUTO_TEST_PTT_BUTTON_ACTION  0
#define AI_AUTO_TEST_DISCONNECT_AI      1

// BLE automate test operation code, configure the respective
// .ini file used for auto test script
#define BLE_AUTO_TEST_START_ADV         0
#define BLE_AUTO_TEST_STOP_ADV          1
#define BLE_AUTO_TEST_DISCONNECT        2
#define BLE_AUTO_TEST_START_CONNECT     3
#define BLE_AUTO_TEST_STOP_CONNECT      4
#define BLE_AUTO_TEST_START_SCAN        5
#define BLE_AUTO_TEST_STOP_SCAN         6
#define BLE_AUTO_TEST_UPDATE_CONN_PARAM 7

// flash automate test operation code, configure the respective
// .ini file used for auto test script
#define FLASH_AUTO_TEST_PROGRAM         0
#define FLASH_AUTO_TEST_ERASE           1
#define FLASH_AUTO_TEST_FLUSH_NV        2

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif
#endif

