#ifndef __APP_TWS_BESAUD_H__
#define __APP_TWS_BESAUD_H__

#include "bluetooth.h"
#include "besaud_api.h"

#define TWS_BESAUD_EVENT_CONTROL_CONNECTED            (BTIF_BESAUD_EVENT_CONTROL_CONNECTED)

#define TWS_BESAUD_EVENT_CONTROL_DISCONNECTED         (BTIF_BESAUD_EVENT_CONTROL_DISCONNECTED)

#define TWS_BESAUD_EVENT_CONTROL_DATA_IND             (BTIF_BESAUD_EVENT_CONTROL_DATA_IND)

#define TWS_BESAUD_EVENT_CONTROL_DATA_SENT            (BTIF_BESAUD_EVENT_CONTROL_DATA_SENT)

#define TWS_BESAUD_EVENT_CONTROL_SET_IDLE             (BTIF_BESAUD_EVENT_CONTROL_SET_IDLE)

typedef btif_besaud_event tws_besaud_event;

typedef btif_besaud_status_change_callback tws_besaud_status_change_callback;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t tws_besaud_is_connected(void);
uint8_t tws_besaud_is_cmd_sending(void);
void tws_besaud_client_create(btif_remote_device_t *dev);
void tws_besaud_server_create(tws_besaud_status_change_callback callback);
void tws_besaud_init(void);
void tws_besaud_send_cmd(uint8_t* cmd, uint16_t len);
bt_status_t tws_besaud_send_cmd_no_wait(uint8_t* cmd, uint16_t len);
void tws_besaud_status_changed(tws_besaud_event event);

void tws_besaud_create_extra_channel(bt_bdaddr_t *remote);
void tws_besaud_extra_channel_send_data(const void* data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif

