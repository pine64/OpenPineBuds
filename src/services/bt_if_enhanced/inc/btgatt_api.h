#ifdef __GATT_OVER_BR_EDR__
#ifndef __BTGATT_API_H__
#define __BTGATT_API_H__

//#include "cqueue.h"
#include "bluetooth.h"
#include "me_api.h"
#include "btgatt.h"

#define BTIF_BTGATT_EVENT_CONTROL_CONNECTED            0x21
#define BTIF_BTGATT_EVENT_CONTROL_DISCONNECTED         0x22
#define BTIF_BTGATT_EVENT_CONTROL_DATA_IND             0x23
#define BTIF_BTGATT_EVENT_CONTROL_DATA_SENT            0x24
#define BTIF_BTGATT_EVENT_CONTROL_SET_IDLE             0x30

#define L2CAP_HEADER_LEN    4
#define BTGATT_DATA_BUF_SIZE (L2CAP_CFG_MTU)
#define BD_ADDR_LEN 6

typedef uint16_t btif_btgatt_event;

typedef void (*btif_btgatt_status_change_callback)(btif_btgatt_event event);

typedef void (*btif_btgatt_data_received_callback)(uint8_t*, uint16_t);

typedef struct
{
    uint8_t data[BTGATT_DATA_BUF_SIZE];
}__attribute__((packed)) BTGATT_DATA_PACKET_T;

#ifdef __cplusplus
extern "C" {
#endif
uint8_t btif_btgatt_is_connected(void);;
void btif_btgatt_client_create(btif_remote_device_t *dev);
void btif_btgatt_server_create(btif_btgatt_status_change_callback callback);
void btif_btgatt_data_received_register(btif_btgatt_data_received_callback callback);
void btif_btgatt_send_cmd(uint8_t* cmd, uint16_t len);
bt_status_t btif_btgatt_send_cmd_no_wait(uint8_t* cmd, uint16_t len);
void btif_btgatt_init(void);
uint8_t btif_btgatt_get_connection_index(void);
int32_t btif_btgatt_get_l2cap_buffer_size(void);
void btif_btgatt_get_device_address(uint8_t *addr);

void btif_btgatt_callback(BtgattChannel *Chan, BtgattCallbackParms *Info);
uint32_t btif_btgatt_get_l2cap_handle(void);
uint16_t btif_btgatt_get_conn_handle(void);
void btif_btgatt_addsdp(uint16_t pServiceUUID, uint16_t startHandle, uint16_t endHandle);


#ifdef __cplusplus
}
#endif

#endif
#endif
