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

#ifndef __BTM_SECURITY_H__
#define __BTM_SECURITY_H__

struct btm_security_auth_pending_item_t {
    struct list_node list;
};

#define BTM_BONDING_NOT_REQUIRED 0x00
#define BTM_DEDICATED_BONDING    0x02
#define BTM_GENERAL_BONDING      0x04
#define BTM_BONDING_NOT_ALLOWED  0x10

#define BTM_AUTH_MITM_PROTECT_NOT_REQUIRED  0x00
#define BTM_AUTH_MITM_PROTECT_REQUIRED      0x01

#define BTM_IO_DISPLAY_ONLY   0
#define BTM_IO_DISPLAY_YESNO  1
#define BTM_IO_KEYBOARD_ONLY  2
#define BTM_IO_NO_IO          3

/*api to handle hci event */
void btm_security_link_key_notify(struct bdaddr_t *remote, uint8 *linkkey, uint8 key_type);
void btm_security_pin_code_req(struct bdaddr_t *remote);
void btm_security_link_key_req(struct bdaddr_t *remote);
void btm_security_authen_complete(uint8 status, uint16 conn_handle);
void btm_security_encryption_change(uint8 status, uint16 conn_handle, uint8 encrypt_en);

#endif /* __BTM_SECURITY_H__ */