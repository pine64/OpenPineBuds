#ifndef __HFP_H__
#define __HFP_H__
#include "btlib_type.h"

struct hfp_response {
	const char *data;
	unsigned int offset;
};

enum hfp_result {
	HFP_RESULT_OK		= 0,
	HFP_RESULT_CONNECT	= 1,
	HFP_RESULT_RING		= 2,
	HFP_RESULT_NO_CARRIER	= 3,
	HFP_RESULT_ERROR	= 4,
	HFP_RESULT_NO_DIALTONE	= 6,
	HFP_RESULT_BUSY		= 7,
	HFP_RESULT_NO_ANSWER	= 8,
	HFP_RESULT_DELAYED	= 9,
	HFP_RESULT_BLACKLISTED	= 10,
	HFP_RESULT_CME_ERROR	= 11,
};

enum hfp_error {
	HFP_ERROR_AG_FAILURE			= 0,
	HFP_ERROR_NO_CONNECTION_TO_PHONE	= 1,
	HFP_ERROR_OPERATION_NOT_ALLOWED		= 3,
	HFP_ERROR_OPERATION_NOT_SUPPORTED	= 4,
	HFP_ERROR_PH_SIM_PIN_REQUIRED		= 5,
	HFP_ERROR_SIM_NOT_INSERTED		= 10,
	HFP_ERROR_SIM_PIN_REQUIRED		= 11,
	HFP_ERROR_SIM_PUK_REQUIRED		= 12,
	HFP_ERROR_SIM_FAILURE			= 13,
	HFP_ERROR_SIM_BUSY			= 14,
	HFP_ERROR_INCORRECT_PASSWORD		= 16,
	HFP_ERROR_SIM_PIN2_REQUIRED		= 17,
	HFP_ERROR_SIM_PUK2_REQUIRED		= 18,
	HFP_ERROR_MEMORY_FULL			= 20,
	HFP_ERROR_INVALID_INDEX			= 21,
	HFP_ERROR_MEMORY_FAILURE		= 23,
	HFP_ERROR_TEXT_STRING_TOO_LONG		= 24,
	HFP_ERROR_INVALID_CHARS_IN_TEXT_STRING	= 25,
	HFP_ERROR_DIAL_STRING_TO_LONG		= 26,
	HFP_ERROR_INVALID_CHARS_IN_DIAL_STRING	= 27,
	HFP_ERROR_NO_NETWORK_SERVICE		= 30,
	HFP_ERROR_NETWORK_TIMEOUT		= 31,
	HFP_ERROR_NETWORK_NOT_ALLOWED		= 32,
};

typedef void (*hfp_destroy_func_t)(void *user_data);
typedef void (*hfp_debug_func_t)(const char *str, void *user_data);

typedef void (*hfp_command_func_t)(const char *command, void *user_data);
extern struct hshf_control *hshf_ctl;

typedef void (*hfp_hf_result_func_t)(struct hfp_response *context,
							void *user_data);

typedef void (*hfp_response_func_t)(enum hfp_result result,
							enum hfp_error cme_err,
							void *user_data);

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(HFP_MOBILE_AG_ROLE)
void hf_process_input(struct hshf_control *hfp, const char *data, size_t len);
#else
void ag_process_input(struct hshf_control *hfp, const char *data, size_t len);
#endif

bool hfp_hf_send_command(struct hshf_control *hfp, hfp_response_func_t resp_cb,
				void *user_data, const char *data, unsigned int len);
bool hfp_hf_send_command_do(struct hshf_control *hfp, hfp_response_func_t resp_cb,
				void *user_data, const char *data, unsigned int len, bool is_cust_cmd ,uint8 param);

#define hfp_hf_send_command(h,rc,ud,d,dl) hfp_hf_send_command_do(h,rc,ud,d,dl,false,0xFF)

#define hfp_hf_send_command_private(h,rc,ud,d,dl,param) hfp_hf_send_command_do(h,rc,ud,d,dl,false,param)



bool hfp_context_get_string(struct hfp_response *context, char *buf,
								uint8_t len);

void hfp_context_skip_field(struct hfp_response *context);

void skip_whitespace(struct hfp_response *context);

bool hfp_hf_register(struct hshf_control *hfp, hfp_hf_result_func_t callback,
						const char *prefix,
						void *user_data,
						hfp_destroy_func_t destroy);

bool hfp_hf_unregister(struct hshf_control *hfp, const char *prefix);

bool hfp_context_open_container(struct hfp_response *context);

bool hfp_context_close_container(struct hfp_response *context);

bool hfp_context_get_unquoted_string(struct hfp_response *context,
							char *buf, uint8_t len);

bool hfp_context_has_next(struct hfp_response *context);

bool hfp_context_get_range(struct hfp_response *context, unsigned int *min,
								unsigned int *max);

bool hfp_context_get_number(struct hfp_response *context,
							unsigned int *val);

void hfp_hf_destory_resource(struct hshf_control *hfp);

void hf_rfcomm_data_recv_cb(uint32 rfcomm_handle,
                                    struct pp_buff *ppb, void *priv);

void hf_rfcomm_notify_cb(enum rfcomm_event_enum event,
                                uint32 rfcomm_handle,
                                void *data, uint8 reason, void *priv);

struct hshf_control *hfp_search_address(struct bdaddr_t *bdaddr);

bool hfp_msbc_is_enable(struct bdaddr_t *bdaddr);
struct hshf_control *hf_find_unused_channel(void);
#if defined(HFP_MOBILE_AG_ROLE)
struct hfp_mobile_module_handler;
void hfp_ag_send_call_active_status(struct hshf_control *hfp, bool active);
void hfp_ag_send_callsetup_status(struct hshf_control *hfp, uint8 status);
void hfp_ag_send_callheld_status(struct hshf_control *hfp, uint8 status);
void hfp_ag_send_calling_ring(struct hshf_control *hfp, const char* number);
bool hfp_ag_set_speaker_gain(struct hshf_control *hfp, uint8 volume);
bool hfp_ag_set_microphone_gain(struct hshf_control *hfp, uint8 volume);
bool hfp_ag_send_result_code(struct hshf_control *hfp, const char *data, int len);
void hfp_ag_register_mobile_module(struct hshf_control* hfp, struct hfp_mobile_module_handler* handler);
void hfp_ag_send_result_ok(struct hshf_control *hfp);
void hfp_ag_send_result_error(struct hshf_control *hfp);
void hfp_ag_send_service_status(struct hshf_control *hfp, bool enabled);
void hfp_ag_send_mobile_signal_level(struct hshf_control *hfp, uint8 level);
void hfp_ag_send_mobile_roam_status(struct hshf_control *hfp, bool enabled);
bool hfp_ag_send_mobile_battery_level(struct hshf_control *hfp, uint8 level);
#endif

#if defined(__cplusplus)
}
#endif

#endif /*__HFP_H__*/
