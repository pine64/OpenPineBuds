#ifdef _VENDOR_MSG_SUPPT_

#include <stdio.h>
#include "stdlib.h"
#include <string.h>
#include "hal_trace.h"
#include "crc32.h"
#include "hal_norflash.h"
#include "pmu.h"
#include "cmsis.h"
#include "hal_cmu.h"
#include "tgt_hardware.h"
#include "hal_timer.h"
#include "hal_bootmode.h"
#ifdef USB_EQ_TUNING
#include "hal_cmd.h"
#endif

#ifdef USB_ANC_MC_EQ_TUNING
#include "anc_process.h"
#endif

#include "usb_vendor_msg.h"

#define SYS_CHECK_VAL   "1.1"

#define FACT_SEC_VER		2
#define FACT_SEC_MAGIC		0xba80

typedef enum {
	FACT_SEC_VER_MAGIC,
	FACT_SEC_CRC,
	
	FACT_SEC_DATA_START,
	FACT_SEC_SEQ = FACT_SEC_DATA_START,
	FACT_SEC_PROD_SN,
	FACT_SEC_END = FACT_SEC_PROD_SN + 8,
	
	FACT_SEC_NUM = 256
} FACT_SEC_E;

typedef enum {
    PC_TOOL_CMD_IDLE,
    PC_TOOL_CMD_GET_FW_VER,
    PC_TOOL_CMD_GET_PROD_SN,
//    QUERY_VOLTAGE,
//    BURN_SN,
    PC_TOOL_CMD_SYS_REBOOT,
    PC_TOOL_CMD_SYS_SHUTDOWN,
    PC_TOOL_CMD_PING,
    PC_TOOL_CMD_CHECK,
    PC_TOOL_CMD_FW_UPDATE,
#ifdef USB_EQ_TUNING    
    PC_TOOL_CMD_EQ_TUNING,
#endif
#ifdef USB_ANC_MC_EQ_TUNING    
    PC_TOOL_CMD_ANC_MC_EQ_TUNING,
#endif	
    PC_TOOL_CMD_NUM,
}PC_TOOL_CMD_E;

static char* s_pc_cmd_lst[PC_TOOL_CMD_NUM] = {
    " ",
    "QUERY_SW_VER",
    "QUERY_SN",
//    "QUERY_VOL",
//    "BURN_SN",
    "SYS_REBOOT",
    "SYS_SHUTDOWN",
    "PING_THROUGH_VENDOR",
    "CHECK",
    "FW_UPDATE",
#ifdef USB_EQ_TUNING
    "EQ<",
#endif
#ifdef USB_ANC_MC_EQ_TUNING
    "ANC_MC_EQ",
#endif	
};

static PC_TOOL_CMD_E s_cur_cmd = PC_TOOL_CMD_IDLE;

static uint8_t* s_sn_ptr;
static uint8_t* s_fw_ver_ptr;

static uint8_t s_sn_sz;
static uint8_t s_fw_ver_sz;

uint8_t vendor_msg_rx_buf[VENDOR_RX_BUF_SZ];

extern void analog_aud_codec_mute(void);

int WEAK vendor_get_sn_info(uint8_t **p_ptr, uint8_t *p_size)
{
    static const char sn[] = "SN-001";

    *p_ptr = (uint8_t *)sn;
    *p_size = sizeof(sn) - 1;
    return 0;
}

int WEAK vendor_get_fw_ver_info(uint8_t **p_ptr, uint8_t *p_size)
{
    static const char ver[] = "VER-001";

    *p_ptr = (uint8_t *)ver;
    *p_size = sizeof(ver) - 1;
    return 0;
}

void vendor_info_init (void)
{
    vendor_get_sn_info(&s_sn_ptr, &s_sn_sz);
    vendor_get_fw_ver_info(&s_fw_ver_ptr, &s_fw_ver_sz);
}

static void pc_usb_cmd_set (struct USB_AUDIO_VENDOR_MSG_T* msg)
{
    size_t ret;
    uint8_t cmd_id = 0;
    
    if (0 == msg->length)
        return;

    for (cmd_id = 1; cmd_id < PC_TOOL_CMD_NUM; cmd_id++) {
        ret = memcmp((void *)msg->data, (void *)s_pc_cmd_lst[cmd_id], strlen((char *)s_pc_cmd_lst[cmd_id]));
        if (!ret) break;
    }

    if (cmd_id > (PC_TOOL_CMD_NUM-1)) {
        return;
	}

    s_cur_cmd = cmd_id;
    //TRACE(3,"%s: cmd[%s], id[%d]", __func__, s_pc_cmd_lst[cmd_id], s_cur_cmd);
    msg->data += strlen((char*)s_pc_cmd_lst[cmd_id]);
#ifdef USB_ANC_MC_EQ_TUNING    
    msg->length -=strlen((char*)s_pc_cmd_lst[cmd_id]);
#endif

    switch (s_cur_cmd) {
#ifdef USB_EQ_TUNING
	case PC_TOOL_CMD_EQ_TUNING:
		hal_cmd_list_process(msg->data);
		break;
#endif

#ifdef USB_ANC_MC_EQ_TUNING
	case PC_TOOL_CMD_ANC_MC_EQ_TUNING:
        //TRACE(1,"recev len:%d",msg->length);
        /*
      	       TRACE(0,"***********recev test*************");
               for(int i=0;i<msg->length;i++)
               {
                     TRACE(2,"msg->data[%d]:0x%x",i,msg->data[i]);
               }
               TRACE(0,"***********recev test*************");
               */
               anc_cmd_receve_process(msg->data,msg->length);
		break;
#endif
      default:
             break;
    }
}

static void pc_usb_cmd_exec (struct USB_AUDIO_VENDOR_MSG_T* msg)
{
    //TRACE(2,"%s: cmd[%d]", __func__, s_cur_cmd);
    static const char* s_str_ret1 = "1";
#ifdef USB_EQ_TUNING
    static const char* s_str_ret0 = "0";
#endif
    static const char* s_str_failure = "failure";

    switch (s_cur_cmd) {
        case PC_TOOL_CMD_GET_FW_VER:
            msg->data = s_fw_ver_ptr;
            msg->length = strlen((char*)s_fw_ver_ptr);
            break;

		case PC_TOOL_CMD_GET_PROD_SN:
			msg->data = s_sn_ptr;
			msg->length = strlen((char*)s_sn_ptr);
			break;

        case PC_TOOL_CMD_SYS_REBOOT:
            //TRACE(0,"-> cmd_exec: reboot.....");
            hal_sys_timer_delay(MS_TO_TICKS(500));
            hal_cmu_sys_reboot();
            break;

        case PC_TOOL_CMD_SYS_SHUTDOWN:
            pmu_shutdown();
            break;

        case PC_TOOL_CMD_PING:
            msg->data = (uint8_t *)s_pc_cmd_lst[PC_TOOL_CMD_PING];
            msg->length = (uint32_t)strlen((const char *)msg->data);
            break;

        case PC_TOOL_CMD_CHECK:
            msg->data = (uint8_t*)SYS_CHECK_VAL;
            msg->length = strlen((char*)SYS_CHECK_VAL);
            break;

        case PC_TOOL_CMD_FW_UPDATE:
            hal_sw_bootmode_clear(1<<8);	// to clear HAL_SW_BOOTMODE_FORCE_UART_DLD
            hal_sw_bootmode_set(1<<7);	// to set HAL_SW_BOOTMODE_FORCE_USB_DLD
#ifdef TGT_PLL_FREQ
			extern void pll_config_update (uint32_t);
			pll_config_update(960000000);
#endif
            analog_aud_codec_mute();
            msg->data = (uint8_t*)s_str_ret1;
            msg->length = (uint32_t)strlen(s_str_ret1);
            break;

#ifdef USB_EQ_TUNING
	case PC_TOOL_CMD_EQ_TUNING:
		hal_cmd_tx_process(&msg->data, &msg->length);
		if (!msg->length) {
			msg->data = (uint8_t*)s_str_ret0;
            msg->length = (uint32_t)strlen(s_str_ret0);
		}
        break;
#endif

#ifdef USB_ANC_MC_EQ_TUNING    
	case PC_TOOL_CMD_ANC_MC_EQ_TUNING:
               anc_cmd_send_process(&msg->data, &msg->length);
               TRACE(1,"sned len:%d",msg->length);

/*      	       TRACE(0,"***********send test*************");
               for(int i=0;i<msg->length;i++)
               {
                    TRACE(2,"msg->data[%d]:0x%x",i,msg->data[i]);
               }
               TRACE(0,"***********send test*************");
               */
               break;
#endif

        default:
            msg->data = (uint8_t*)s_str_failure;
            msg->length = (uint32_t)strlen(s_str_failure);
            break;
    }

    s_cur_cmd = PC_TOOL_CMD_IDLE;
}


int usb_vendor_callback (struct USB_AUDIO_VENDOR_MSG_T* msg)
{

    if (0 == msg->length) {
        pc_usb_cmd_exec(msg);
    } else {
        pc_usb_cmd_set(msg);
    }

    return 0;
}

#endif
