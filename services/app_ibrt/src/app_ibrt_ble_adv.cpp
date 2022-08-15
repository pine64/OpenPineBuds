#include "app_ibrt_ble_adv.h"
#include "hal_trace.h"
#include "me_api.h"
#include "cmsis_os.h"
#include "factory_section.h"
#include "co_bt_defines.h"
#include "app_tws_ibrt.h"

SLAVE_BLE_MODE_T slaveBleMode;
static app_ble_adv_para_data_t app_ble_adv_para_data_cfg;
#define APP_IBRT_BLE_ADV_DATA_MAX_LEN (31)
#define APP_IBRT_BLE_SCAN_RSP_DATA_MAX_LEN (31)

const char *g_slave_ble_state_str[] =
{
    "BLE_STATE_IDLE",
    "BLE_ADVERTISING",
    "BLE_STARTING_ADV",
    "BLE_STOPPING_ADV",
};
    
const char *g_slave_ble_op_str[] =
{
    "BLE_OP_IDLE",
    "BLE_START_ADV",
    "BLE_STOP_ADV",
    "BLE_STOPPING_ADV",
};

#define SET_SLAVE_BLE_STATE(newState)                                                                                     \
    do                                                                                                              \
    {                                                                                                               \
        TRACE(3,"[BLE][STATE]%s->%s at line %d", g_slave_ble_state_str[slaveBleMode.state], g_slave_ble_state_str[newState], __LINE__); \
        slaveBleMode.state = (newState);                                                                              \
    } while (0);

#define SET_SLAVE_BLE_OP(newOp)                                                                            \
    do                                                                                               \
    {                                                                                                \
        TRACE(3,"[BLE][OP]%s->%s at line %d", g_slave_ble_op_str[slaveBleMode.op], g_slave_ble_op_str[newOp], __LINE__); \
        slaveBleMode.op = (newOp);                                                                     \
    } while (0);
/*****************************************************************************
 Prototype    : app_slave_ble_cmd_complete_callback
 Description  : stop ble adv
 Input        : const void *para
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_slave_ble_cmd_complete_callback(const void *para)
{
    slave_ble_cmd_comp_t *p_cmd_complete = (slave_ble_cmd_comp_t *)para;

    switch (p_cmd_complete->cmd_opcode)
    {
        case HCI_BLE_ADV_CMD_OPCODE:
            if(!p_cmd_complete->param[0])
                app_ibrt_ble_switch_activities();
            else
            {
                TRACE(2,"%s error p_cmd_complete->param[0] %d", __func__, p_cmd_complete->param[0]);
                SET_SLAVE_BLE_STATE(BLE_STATE_IDLE);
                SET_SLAVE_BLE_OP(BLE_OP_IDLE);
            } 
        default:
        break;
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ble_adv_para_data_init
 Description  : initialize ble adv parameter and adv data
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_adv_para_data_init(void)
{
    app_ble_adv_para_data_t *adv_para_cfg = &app_ble_adv_para_data_cfg;
    
    adv_para_cfg->adv_type = ADV_CONN_UNDIR;
    adv_para_cfg->advInterval_Ms = APP_IBRT_BLE_ADV_INTERVAL;
    adv_para_cfg->own_addr_type = ADDR_PUBLIC;
    adv_para_cfg->peer_addr_type = ADDR_PUBLIC;
    adv_para_cfg->adv_chanmap = ADV_ALL_CHNLS_EN;
    adv_para_cfg->adv_filter_policy = ADV_ALLOW_SCAN_ANY_CON_ANY;
    memset(adv_para_cfg->bd_addr.address, 0, BTIF_BD_ADDR_SIZE);

    const char* ble_name_in_nv = (const char*)factory_section_get_ble_name();
    uint32_t nameLen = strlen(ble_name_in_nv);
    
    ASSERT(APP_IBRT_BLE_ADV_DATA_MAX_LEN >= nameLen, "ble adv data exceed");
    
    adv_para_cfg->adv_data_len = 0;
    adv_para_cfg->adv_data[adv_para_cfg->adv_data_len++] = nameLen+1;
    adv_para_cfg->adv_data[adv_para_cfg->adv_data_len++] = 0x08;
    memcpy(&adv_para_cfg->adv_data[adv_para_cfg->adv_data_len], ble_name_in_nv, nameLen);
    adv_para_cfg->adv_data_len += nameLen;
    
    adv_para_cfg->scan_rsp_data_len = 0;
    memset(adv_para_cfg->scan_rsp_data,0,sizeof(adv_para_cfg->scan_rsp_data));
    
    memset(&slaveBleMode, 0, sizeof(slaveBleMode));
    btif_me_register_cmd_complete_callback(HCI_CMD_COMPLETE_USER_BLE, app_ibrt_slave_ble_cmd_complete_callback);
}

/*****************************************************************************
 Prototype    : app_ibrt_ble_set_adv_para_handler
 Description  : config ble adv parameter
 Input        : app_ble_adv_para_data_t *adv_para_cfg
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_set_adv_para_handler(app_ble_adv_para_data_t *adv_para_cfg)
{
    btif_adv_para_struct_t adv_para;
    adv_para.adv_type = adv_para_cfg->adv_type;
    adv_para.interval_max = adv_para_cfg->advInterval_Ms * 8 / 5;
    adv_para.interval_min = adv_para_cfg->advInterval_Ms * 8 / 5;
    adv_para.own_addr_type = adv_para_cfg->own_addr_type;
    adv_para.peer_addr_type = adv_para_cfg->peer_addr_type;
    adv_para.adv_chanmap = adv_para_cfg->adv_chanmap;
    adv_para.adv_filter_policy = adv_para_cfg->adv_filter_policy;
    memcpy(adv_para.bd_addr.address, adv_para_cfg->bd_addr.address, BTIF_BD_ADDR_SIZE);
    
    btif_me_ble_set_adv_parameters(&adv_para);
}
/*****************************************************************************
 Prototype    : app_ibrt_ble_set_adv_data_handler
 Description  : config ble adv data and scan response data
 Input        : app_ble_adv_para_data_t *adv_data_cfg
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_set_adv_data_handler(app_ble_adv_para_data_t *adv_data_cfg)
{
    btif_me_ble_set_adv_data(adv_data_cfg->adv_data_len, adv_data_cfg->adv_data);
    btif_me_ble_set_scan_rsp_data(adv_data_cfg->scan_rsp_data_len, adv_data_cfg->scan_rsp_data);
}
/*****************************************************************************
 Prototype    : app_ibrt_ble_adv_start
 Description  : enable ble adv
 Input        : uint8_t adv_type, uint16_t advInterval
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_adv_start(uint8_t adv_type, uint16_t advInterval)
{
    TRACE(2,"ble adv start with adv_type %d advIntervalms %dms", adv_type, advInterval);
    app_ble_adv_para_data_cfg.adv_type = adv_type;
    app_ble_adv_para_data_cfg.advInterval_Ms = advInterval;

    switch(slaveBleMode.state)
    {
        case BLE_ADVERTISING:
            SET_SLAVE_BLE_STATE(BLE_STOPPING_ADV);
            SET_SLAVE_BLE_OP(BLE_START_ADV);
            btif_me_ble_set_adv_en(false);
            break;
        case BLE_STARTING_ADV:
        case BLE_STOPPING_ADV:
            SET_SLAVE_BLE_OP(BLE_START_ADV);
            break;
        case BLE_STATE_IDLE:
            SET_SLAVE_BLE_STATE(BLE_STARTING_ADV);
            app_ibrt_ble_set_adv_para_handler(&app_ble_adv_para_data_cfg);
            app_ibrt_ble_set_adv_data_handler(&app_ble_adv_para_data_cfg);
            btif_me_ble_set_adv_en(true); 
            break;
        default:
        break;
    }

}
/*****************************************************************************
 Prototype    : app_ibrt_ble_adv_stop
 Description  : disable ble adv
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_adv_stop(void)
{
    TRACE(1,"%s",__func__);
    SET_SLAVE_BLE_STATE(BLE_STOPPING_ADV);
    SET_SLAVE_BLE_OP(BLE_STOP_ADV);
    btif_me_ble_set_adv_en(false);
}
/*****************************************************************************
 Prototype    : app_ibrt_ble_switch_activities
 Description  : ble adv state machine switch
 Input        : None
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_switch_activities(void)
{
    switch(slaveBleMode.state)
    {
        case BLE_STARTING_ADV:
            SET_SLAVE_BLE_STATE(BLE_ADVERTISING);
            if(slaveBleMode.op == BLE_START_ADV)
            {
                SET_SLAVE_BLE_OP(BLE_OP_IDLE);
            }
            break;
       case BLE_STOPPING_ADV:
            SET_SLAVE_BLE_STATE(BLE_STATE_IDLE);
            if(slaveBleMode.op == BLE_STOP_ADV)
            {
                SET_SLAVE_BLE_OP(BLE_OP_IDLE);
            }
            break;            
       default:
        break; 
    }
    switch(slaveBleMode.op)
    {
        case BLE_START_ADV:
             app_ibrt_ble_adv_start(ADV_CONN_UNDIR, app_ble_adv_para_data_cfg.advInterval_Ms);
             break;
        default:
             break;
    }
}

/*****************************************************************************
 Prototype    : app_ibrt_ble_adv_data_config
 Description  : config ble adv data and scan response data by customer
 Input        : uint8_t *advData, uint8_t advDataLen
              : uint8_t *scanRspData, uint8_t scanRspDataLen
 Output       : None
 Return Value :
 Calls        :
 Called By    :

 History      :
 Date         : 2019/12/28
 Author       : bestechnic
 Modification : Created function

*****************************************************************************/
void app_ibrt_ble_adv_data_config(uint8_t *advData, uint8_t advDataLen,
                                            uint8_t *scanRspData, uint8_t scanRspDataLen)
{
    ASSERT(APP_IBRT_BLE_ADV_DATA_MAX_LEN >= advDataLen, "ble adv data len exceed");
    ASSERT(APP_IBRT_BLE_SCAN_RSP_DATA_MAX_LEN >= scanRspDataLen, "scan response data len exceed")
    memcpy(app_ble_adv_para_data_cfg.adv_data, advData, advDataLen);
    memcpy(app_ble_adv_para_data_cfg.scan_rsp_data, scanRspData, scanRspDataLen);
    app_ble_adv_para_data_cfg.adv_data_len = advDataLen;
    app_ble_adv_para_data_cfg.scan_rsp_data_len = scanRspDataLen;
}



