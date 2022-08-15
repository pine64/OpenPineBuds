#ifndef APP_AI_TWS_H_
#define APP_AI_TWS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#define   APP_AI_TWS_MASTER       0
#define   APP_AI_TWS_SLAVE        1
#define   APP_AI_TWS_UNKNOW       0xff


typedef struct
{
    bool        is_ai_reboot;
    uint8_t     box_state;
} APP_AI_TWS_REBOOT_T;

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *            app_ai_ibrt_role_switch_direct
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai ibrt role switch direct
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool false -- switch direct, don't need prepare
 *         true -- tws role switch need prepare
 */
bool app_ai_tws_role_switch_direct(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_role_switch_dis_ble
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai disconnect ble before role switch
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool false -- switch direct, don't need prepare
 *         true -- tws role switch need prepare
 */
bool app_ai_tws_role_switch_dis_ble(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_master_role_switch_prepare
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    master role switch API for AI module
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_master_role_switch_prepare(void);

/*---------------------------------------------------------------------------
 *            app_ai_role_switch_prepare
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai role switch prepare, give a time to ibrt for role switch
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint32_t return the AI now use
 */
uint32_t app_ai_ibrt_role_switch_prepare(uint32_t *wait_ms);

/*---------------------------------------------------------------------------
 *            app_ai_tws_role_switch_prepare_done
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    master role switch prepare done, slave receive a masegge from master
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_role_switch_prepare_done(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_role_switch_prepare
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    AI interface controlled tws role switch API
 *
 * Parameters:
 *    wait_ms - time delay before role switch started for AI module
 *
 * Return:
 *    true - need delay
 *    false - not need delay
 */
uint32_t app_ai_tws_role_switch_prepare(uint32_t *wait_ms);

/*---------------------------------------------------------------------------
 *            app_ai_tws_role_switch
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    role switch API for AI module
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_role_switch(void);

/*---------------------------------------------------------------------------
*            app_ai_tws_role_switch_complete
*---------------------------------------------------------------------------
*
*Synopsis:
*    when ibrt role switch done, call this function
*
* Parameters:
*    void
*
* Return:
*    bool
*/
void app_ai_tws_role_switch_complete(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_sync_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai tws sync init
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool
 */
void app_ai_tws_sync_init(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_sync_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai tws synchronize AI info
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool
 */
void app_ai_tws_sync_ai_info(void);

/*---------------------------------------------------------------------------
 *            ai_manager_sync_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai manager sync init
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool
 */
void ai_manager_sync_init(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_sync_ai_manager_info
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    app ai tws synchronize AI manager info
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool
 */
void app_ai_tws_sync_ai_manager_info(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_gsound_sync_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    gsound related environment initialization for AI interface
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_gsound_sync_init(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_send_cmd_to_peer
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for AI send cmd to peer
 *
 * Parameters:
 *    uint8_t *p_buff -- the cmd pointer
 *    uint16_t length -- the cmd length
 *
 * Return:
 *    void
 */
void app_ai_tws_send_cmd_to_peer(uint8_t *p_buff, uint16_t length);

/*---------------------------------------------------------------------------
 *            app_ai_tws_rev_peer_cmd_hanlder
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    for AI handle the cmd receive from peer
 *
 * Parameters:
 *    uint16_t rsp_seq -- seq number
 *    uint8_t *p_buff -- the cmd pointer
 *    uint16_t length -- the cmd length
 *
 * Return:
 *    void
 */
void app_ai_tws_rev_peer_cmd_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ai_tws_send_cmd_with_rsp_to_peer(uint8_t *p_buff, uint16_t length);

void app_ai_tws_send_cmd_rsp_to_peer(uint8_t *p_buff, uint16_t rsp_seq, uint16_t length);

void app_ai_tws_rev_peer_cmd_with_rsp_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ai_tws_rev_cmd_rsp_from_peer_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

void app_ai_tws_rev_cmd_rsp_timeout_hanlder(uint16_t rsp_seq, uint8_t *p_buff, uint16_t length);

/*---------------------------------------------------------------------------
 *            app_ai_tws_init_done
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get tws init status
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true is tws init done
 */
bool app_ai_tws_init_done(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_link_connected
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get tws connect status
 *
 * Parameters:
 *    void
 *
 * Return:
 *    bool -- true is tws connected
 */
bool app_ai_tws_link_connected(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_local_address
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get tws local address
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t -- the address of local
 */
uint8_t *app_ai_tws_local_address(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_peer_address
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    to get tws peer address
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t -- the address of peer
 */
uint8_t *app_ai_tws_peer_address(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_reboot_record_box_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    record the current state of box before reboot
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_reboot_record_box_state(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_reboot_get_box_action
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    return a box action due to the box state before reboot
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t box action
 */
uint8_t app_ai_tws_reboot_get_box_action(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_clear_reboot_box_state
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    clear the current state of box before reboot
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_clear_reboot_box_state(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_disconnect_all_bt_connection
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    ai disconnect all bt connection
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_disconnect_all_bt_connection(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_get_local_role
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    get tws local role
 *
 * Parameters:
 *    void
 *
 * Return:
 *    uint8_t -- the role of local
 */
uint8_t app_ai_tws_get_local_role(void);

/*---------------------------------------------------------------------------
 *            app_ai_tws_init
 *---------------------------------------------------------------------------
 *
 *Synopsis:
 *    init tws for ai
 *
 * Parameters:
 *    void
 *
 * Return:
 *    void
 */
void app_ai_tws_init(void);

#ifdef __cplusplus
    }
#endif


#endif //APP_AI_TWS_H_

