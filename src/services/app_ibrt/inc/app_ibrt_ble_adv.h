#ifndef __APP_IBRT__BLE_ADV__H__
#define __APP_IBRT__BLE_ADV__H__

#include "stdint.h"
#include "bluetooth.h"
#ifdef __cplusplus
extern "C" {
#endif


#define APP_IBRT_BLE_ADV_INTERVAL 100

//HCI opcode
#define    HCI_BLE_ADV_CMD_OPCODE      0x200a

typedef enum {
    BLE_STATE_IDLE       = 0,
    BLE_ADVERTISING      = 1,
    BLE_STARTING_ADV     = 2,
    BLE_STOPPING_ADV     = 3,
} SLAVE_BLE_STATE_E;

typedef enum {
    BLE_OP_IDLE       = 0,
    BLE_START_ADV     = 1,
    BLE_STOP_ADV      = 2,
} SLAVE_BLE_OP_E;

typedef struct
{
    SLAVE_BLE_STATE_E state;
    SLAVE_BLE_OP_E op;
} SLAVE_BLE_MODE_T;

typedef struct  {
	// le_adv_param
	uint16_t advInterval_Ms;
	uint8_t adv_type;
    bt_bdaddr_t bd_addr;
    uint8_t own_addr_type;
    uint8_t peer_addr_type;
    uint8_t adv_chanmap;
    uint8_t adv_filter_policy;
	// adv data
	uint8_t adv_data_len;
	uint8_t adv_data[31];
	
	// scan response data	
	uint8_t scan_rsp_data_len;
	uint8_t scan_rsp_data[31];	
}app_ble_adv_para_data_t;
typedef struct
{
    uint8_t   num_hci_cmd_packets;
    uint16_t  cmd_opcode;
    uint8_t   param[1];
} __attribute__ ((packed)) slave_ble_cmd_comp_t;


void app_ibrt_ble_adv_para_data_init(void);
void app_ibrt_ble_adv_start(uint8_t adv_type, uint16_t advInterval);
void app_ibrt_ble_adv_stop(void);
void app_ibrt_ble_adv_data_config(uint8_t *advData, uint8_t advDataLen,
                                            uint8_t *scanRspData, uint8_t scanRspDataLen);
void app_ibrt_ble_switch_activities(void);

#ifdef __cplusplus
}
#endif

#endif


