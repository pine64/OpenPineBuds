#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "string.h"
#include "hal_dma.h"
#include "communication_cmd_handle.h"

static bool communication_cmd_inited = false;
static int (*send_reply_cb)(const unsigned char *, unsigned int);

extern "C" int extend_cmd_pmu_open(void);

int communication_cmd_init(int (* cb)(const unsigned char *, unsigned int))
{
    if (!communication_cmd_inited){
        send_reply_cb = cb;
        communication_cmd_inited = true;
    }
    return 0;
}


int communication_cmd_send_reply(const unsigned char *payload, unsigned int len)
{
    return send_reply_cb(payload, len);
}

enum ERR_CODE communication_cmd_check_msg_hdr(struct message_t *msg)
{
    return ERR_NONE;
}

void FloatToByte(float floatNum,unsigned char* byteArry)
{
    char* pchar=(char*)&floatNum;
    for(u32 i=0;i<sizeof(float);i++)
    {
     *byteArry=*pchar;
     pchar++;
     byteArry++;
    }
}


enum ERR_CODE communication_cmd_handle_cmd(enum COMMUNICATION_CMD_TYPE cmd, unsigned char *param, unsigned int len)
{
    enum ERR_CODE nRet = ERR_NONE;
    //uint8_t cret[5]={0x01,0x02,0x03,0x04,0x05};

    //cret[0] = ERR_NONE;
	

	TRACE(1,"###communication_cmd_handle_cmd,len=%d",len);
    switch (cmd) {
        case COMMUNICATION_CMD_EQ_OP: {
#ifdef __PC_CMD_UART__	
		//DUMP8("%02x ",param,len);
			communication_cmd_send_reply(param,len);
#endif
        }
        break;
        case COMMUNICATION_CMD_DRC_OP: {

        }        
        break;
        case COMMUNICATION_CMD_HF_OP: {
        }        
        break;
        default: {
            TRACE(1,"Invalid command: 0x%x", cmd);
            nRet = ERR_INTERNAL;
        }
    }
    //communication_cmd_send_reply(cret,10);
    return nRet;
}
