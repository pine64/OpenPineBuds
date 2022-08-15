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
/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_APP_SEC)

#include <string.h>
#include "co_math.h"
#include "gapc_task.h"      // GAP Controller Task API Definition
#include "gap.h"            // GAP Definition
#include "gapc.h"           // GAPC Definition
#include "prf_types.h"

#include "app.h"            // Application API Definition
#include "app_ble_include.h"
#include "app_sec.h"        // Application Security API Definition
#include "app_task.h"       // Application Manager API Definition

#if (DISPLAY_SUPPORT)
#include "app_display.h"    // Display Application Definitions
#endif //(DISPLAY_SUPPORT)

#if (NVDS_SUPPORT)
#include "nvds.h"           // NVDS API Definitions
#endif //(NVDS_SUPPORT)

#ifdef BLE_APP_AM0
#include "am0_app.h"
#endif // BLE_APP_AM0

#if (BLE_APP_ANCC)
#include "app_ancc.h"       // ANC Module Definition
#endif // (BLE_APP_ANCC)

#if (BLE_APP_AMS)
#include "app_amsc.h"       // AMS Module Definition
#endif // (BLE_APP_AMS)

#include "app_gfps.h"
#include "nvrecord_ble.h"
#include "tgt_hardware.h"
#include "nvrecord_extension.h"

#ifdef _BLE_NVDS_
#define BLE_KEY_PRINT
BleDeviceinfo ble_save_info;
#endif

#ifdef TWS_SYSTEM_ENABLED
#include "app_tws_if.h"
#endif



/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Security Environment Structure
struct app_sec_env_tag app_sec_env;

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_sec_init(void)
{
#ifdef _BLE_NVDS_
    if (nv_record_ble_record_Once_a_device_has_been_bonded())
    {
        app_sec_env.bonded = true;
    }
    else
    {
        app_sec_env.bonded = false;
    }

    TRACE(2,"[%s] once bonded:%d", __func__, app_sec_env.bonded);
#endif
}

bool app_sec_get_bond_status(void)
{
    return app_sec_env.bonded;
}

void app_sec_send_security_req(uint8_t conidx, enum gap_auth authority)
{
    // Send security request
    struct gapc_security_cmd *cmd = KE_MSG_ALLOC(GAPC_SECURITY_CMD,
                                                 KE_BUILD_ID(TASK_GAPC, conidx), TASK_APP,
                                                 gapc_security_cmd);

    cmd->operation = GAPC_SECURITY_REQ;

    cmd->auth = authority;

    // Send the message
    ke_msg_send(cmd);
}

/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */

static int gapc_bond_req_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_bond_req_ind const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    TRACE(1,"Get bond req %d", param->request);

    // Prepare the GAPC_BOND_CFM message
    struct gapc_bond_cfm *cfm = KE_MSG_ALLOC(GAPC_BOND_CFM,
                                             src_id, TASK_APP,
                                             gapc_bond_cfm);

    switch (param->request)
    {
        case (GAPC_PAIRING_REQ):
        {
            cfm->request = GAPC_PAIRING_RSP;

            cfm->accept  = true;

            cfm->data.pairing_feat.auth      = BLE_AUTHENTICATION_LEVEL;

        #if (BLE_APP_GFPS)
            cfm->data.pairing_feat.iocap     = GAP_IO_CAP_DISPLAY_YES_NO;
        #else
            cfm->data.pairing_feat.iocap     = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
        #endif

            cfm->data.pairing_feat.key_size  = 16;
            cfm->data.pairing_feat.oob       = GAP_OOB_AUTH_DATA_NOT_PRESENT;
        #if (BLE_APP_GFPS)
            cfm->data.pairing_feat.sec_req   = GAP_SEC1_SEC_CON_PAIR_ENC;
        #else
            cfm->data.pairing_feat.sec_req   = GAP_SEC1_NOAUTH_PAIR_ENC;
        #endif

            cfm->data.pairing_feat.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY;
            cfm->data.pairing_feat.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY;
        } break;

        case (GAPC_LTK_EXCH):
        {
            // Counter
            uint8_t counter;

            cfm->accept  = true;
            cfm->request = GAPC_LTK_EXCH;

            // Generate all the values
            cfm->data.ltk.ediv = (uint16_t)co_rand_word();

            for (counter = 0; counter < RAND_NB_LEN; counter++)
            {
                cfm->data.ltk.randnb.nb[counter] = (uint8_t)co_rand_word();
            }

            for (counter = 0; counter < GAP_KEY_LEN; counter++)
            {
                cfm->data.ltk.ltk.key[counter]    = (uint8_t)co_rand_word();
            }

#ifdef _BLE_NVDS_
#ifdef BLE_KEY_PRINT
            TRACE(0,"<==============>LTK IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk,16);
            TRACE(1,"<==============>EDIV IS: %04x:",cfm->data.ltk.ediv);
            TRACE(0,"<==============>RANDOM IS:");
            DUMP8("%02x ",(uint8_t *)&cfm->data.ltk.randnb.nb,8);
#endif
            ble_save_info.EDIV = cfm->data.ltk.ediv;
            memcpy(&ble_save_info.RANDOM, (uint8_t *)&cfm->data.ltk.randnb.nb, 8);
            memcpy(&ble_save_info.LTK, (uint8_t *)&cfm->data.ltk, 16);

            ble_save_info.bonded = false;
#endif
        } break;

        case (GAPC_IRK_EXCH):
        {
            cfm->accept  = true;
            cfm->request = GAPC_IRK_EXCH;

            // Load IRK
            memcpy(cfm->data.irk.irk.key, app_env.loc_irk, KEY_LEN);
            // load identity ble address
            memcpy(cfm->data.irk.addr.addr.addr, ble_addr, BLE_ADDR_SIZE);
            cfm->data.irk.addr.addr_type = ADDR_PUBLIC;
        } break;
        case (GAPC_TK_EXCH):
        {
            // Generate a PIN Code- (Between 100000 and 999999)
            uint32_t pin_code = (100000 + (co_rand_word()%900000));

            cfm->accept  = true;
            cfm->request = GAPC_TK_EXCH;

            // Set the TK value
            memset(cfm->data.tk.key, 0, KEY_LEN);

            cfm->data.tk.key[0] = (uint8_t)((pin_code & 0x000000FF) >>  0);
            cfm->data.tk.key[1] = (uint8_t)((pin_code & 0x0000FF00) >>  8);
            cfm->data.tk.key[2] = (uint8_t)((pin_code & 0x00FF0000) >> 16);
            cfm->data.tk.key[3] = (uint8_t)((pin_code & 0xFF000000) >> 24);
        } break;
        case GAPC_NC_EXCH:
            cfm->accept  = true;
            cfm->request = GAPC_NC_EXCH;
            break;
        default:
        {
            ASSERT_ERR(0);
        } break;
    }

    // Send the message
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

void app_sec_reset_env_on_connection(void)
{
#ifdef _BLE_NVDS_
    memset(&ble_save_info, 0, sizeof(BleDeviceinfo));
#endif
}

static int gapc_bond_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_bond_ind const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint8_t conidx = KE_IDX_GET(src_id);
    switch (param->info)
    {
        case (GAPC_PAIRING_SUCCEED):
        {
            TRACE(0,"GAPC_PAIRING_SUCCEED");
            // Update the bonding status in the environment
            app_sec_env.bonded = true;
#ifdef _BLE_NVDS_
            ble_save_info.bonded = true;

            if (!nv_record_blerec_add(&ble_save_info))
            {
#ifdef TWS_SYSTEM_ENABLED
                app_ble_sync_ble_info();
                TRACE(0,"BLE NVDS SETUP SUCCESS!!!,the key is the newest");
#endif
            }
#endif
            break;
        }
        case (GAPC_REPEATED_ATTEMPT):
        {
            appm_disconnect(conidx);
            break;
        }
        case (GAPC_IRK_EXCH):
        {
            TRACE(0,"Peer device IRK is:");
#ifdef _BLE_NVDS_
            DUMP8("%02x ", param->data.irk.irk.key, BLE_IRK_SIZE);
            DUMP8("%02x ", (uint8_t *)&(param->data.irk.addr), BLE_ADDR_SIZE+1);
            app_env.context[conidx].isBdAddrResolvingInProgress = false;
            app_env.context[conidx].isGotSolvedBdAddr = true;
            memcpy(app_env.context[conidx].solvedBdAddr, param->data.irk.addr.addr.addr, BLE_ADDR_SIZE);
            memcpy(ble_save_info.IRK, param->data.irk.irk.key, BLE_IRK_SIZE);
            memcpy(ble_save_info.peer_bleAddr, param->data.irk.addr.addr.addr, BLE_ADDR_SIZE);
#endif
            app_exchange_remote_feature(conidx);
            break;
        }
        case (GAPC_PAIRING_FAILED):
        {
            TRACE(1,"GAPC_PAIRING_FAILED!!! Error code 0x%x", param->data.reason);
#ifdef _BLE_NVDS_
            nv_record_ble_delete_entry(app_env.context[conidx].solvedBdAddr);
            app_sec_env.bonded = false;
#endif
            break;
        }
        case GAPC_LTK_EXCH:
        {
#ifdef _BLE_NVDS_
            TRACE(1,"isLocal %d", param->data.ltk.isLocal);
            TRACE(0,"Peer device LTK is:");
            
            DUMP8("%02x ", param->data.ltk.ltk.key, BLE_LTK_SIZE);
            TRACE(1,"EDIV %04x ", param->data.ltk.ediv);
            TRACE(0,"Peer device random number is:");
            DUMP8("%02x ", param->data.ltk.randnb.nb, BLE_ENC_RANDOM_SIZE);


            if (param->data.ltk.isLocal)
            {
                ble_save_info.EDIV = param->data.ltk.ediv;
                memcpy(&ble_save_info.RANDOM, (uint8_t *)&param->data.ltk.randnb.nb, BLE_ENC_RANDOM_SIZE);
                memcpy(&ble_save_info.LTK, (uint8_t *)&param->data.ltk, BLE_LTK_SIZE);
            }
#endif
            break;
        }
        default:
        {
            ASSERT_ERR(0);
            break;
        }
    }

    return (KE_MSG_CONSUMED);
}

static int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_encrypt_req_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    TRACE(1,"%s, master ask for LTK TO encrypt!!!!", __FUNCTION__);

    uint8_t conidx = KE_IDX_GET(src_id);

    appm_check_and_resolve_ble_address(conidx);

    if (!app_env.context[conidx].isGotSolvedBdAddr)
    {
        return (KE_MSG_SAVED);
    }

    TRACE(0,"Random ble address solved. Can rsp enc req.");

    // Prepare the GAPC_ENCRYPT_CFM message
    struct gapc_encrypt_cfm *cfm = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM,
                                                src_id, TASK_APP,
                                                gapc_encrypt_cfm);

    cfm->found    = false;

#ifdef _BLE_NVDS_

    struct gapc_ltk ltk;

    bool ret;
    ret = nv_record_ble_record_find_ltk_through_static_bd_addr(
        app_env.context[conidx].solvedBdAddr, ltk.ltk.key);
    if(ret){
        TRACE(0,"FIND LTK SUCCESSED!!!");
#ifdef BLE_KEY_PRINT
        DUMP8("%02x ", (uint8_t *)ltk.ltk.key, 16);
#endif
        cfm->found    = true;
        cfm->key_size = 16;
        memcpy(&cfm->ltk, ltk.ltk.key, sizeof(struct gap_sec_key));

    }else{
        TRACE(0,"FIND LTK failed!!!");
    }
#endif

    TRACE(2,"%s app_sec_env.bonded %d", __FUNCTION__, app_sec_env.bonded);

    // Send the message
    ke_msg_send(cfm);

    // encryption has been completed, we can start exchanging remote features now
    app_exchange_remote_feature(conidx);

    return (KE_MSG_CONSUMED);
}


static int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_encrypt_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    TRACE(2,"%s param->auth %d", __FUNCTION__, param->auth);

    return (KE_MSG_CONSUMED);
}

static int app_sec_msg_dflt_handler(ke_msg_id_t const msgid,
                                    void *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Drop the message

    return (KE_MSG_CONSUMED);
}

 /*
  * LOCAL VARIABLE DEFINITIONS
  ****************************************************************************************
  */

/// Default State handlers definition
const struct ke_msg_handler app_sec_msg_handler_list[] =
{
    // Note: first message is latest message checked by kernel so default is put on top.
    {KE_MSG_DEFAULT_HANDLER,  (ke_msg_func_t)app_sec_msg_dflt_handler},

    {GAPC_BOND_REQ_IND,       (ke_msg_func_t)gapc_bond_req_ind_handler},
    {GAPC_BOND_IND,           (ke_msg_func_t)gapc_bond_ind_handler},

    {GAPC_ENCRYPT_REQ_IND,    (ke_msg_func_t)gapc_encrypt_req_ind_handler},
    {GAPC_ENCRYPT_IND,        (ke_msg_func_t)gapc_encrypt_ind_handler},
};

const struct ke_state_handler app_sec_table_handler =
    {&app_sec_msg_handler_list[0], (sizeof(app_sec_msg_handler_list)/sizeof(struct ke_msg_handler))};

#endif //(BLE_APP_SEC)

/// @} APP
