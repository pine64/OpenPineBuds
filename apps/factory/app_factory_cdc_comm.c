#ifdef __USB_COMM__
#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "string.h"
#include "stdio.h"
#include "tool_msg.h"
#include "sys_api_cdc_comm.h"
#include "app_factory_cdc_comm.h"

static enum PARSE_STATE parse_state;
static struct message_t recv_msg;
static struct message_t send_msg = { { PREFIX_CHAR, }, };

static unsigned char check_sum(unsigned char *buf, unsigned char len)
{
    int i;
    unsigned char sum = 0;

    for (i = 0; i < len; i++) {
        sum += buf[i];
    }

    return sum;
}

int send_reply(const unsigned char *payload, unsigned int len)
{
    int ret = 0;

    if (len + 1 > sizeof(send_msg.data)) {
        TRACE(1,"Packet length too long: %u", len);
        return -1;
    }

    send_msg.hdr.type = recv_msg.hdr.type;
    send_msg.hdr.seq = recv_msg.hdr.seq;
    send_msg.hdr.len = len;
    memcpy(&send_msg.data[0], payload, len);
    send_msg.data[len] = ~check_sum((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg) - 1);

    ret = send_data((unsigned char *)&send_msg, MSG_TOTAL_LEN(&send_msg));

    return ret;
}

static void reset_parse_state(unsigned char **buf, size_t *len)
{
    parse_state = PARSE_HEADER;
    memset(&recv_msg.hdr, 0, sizeof(recv_msg.hdr));

    *buf = (unsigned char *)&recv_msg.hdr;
    *len = sizeof(recv_msg.hdr);
}

static enum ERR_CODE check_msg_hdr(void)
{
    enum ERR_CODE errcode = ERR_NONE;

    switch (recv_msg.hdr.type) {
        case TYPE_SYS:
            if (recv_msg.hdr.len != 1 && recv_msg.hdr.len != 5) {
                //TRACE(1,"SYS msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_READ:
            if (recv_msg.hdr.len != 4) {
                //TRACE(1,"READ msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        case TYPE_WRITE:
            if (recv_msg.hdr.len <= 4 || recv_msg.hdr.len > 20) {
                //TRACE(1,"WRITE msg length error: %u", recv_msg.hdr.len);
                errcode = ERR_LEN;
            }
            break;
        default:
            break;
    }

    if (errcode == ERR_NONE && recv_msg.hdr.len + 1 > sizeof(recv_msg.data)) {
        errcode = ERR_LEN;
    }

    return errcode;
}

static enum ERR_CODE handle_sys_cmd(enum SYS_CMD_TYPE cmd, unsigned char *param, unsigned int len)
{
    unsigned char cret[5];
    unsigned int bootmode;

    cret[0] = ERR_NONE;

    if (cmd == SYS_CMD_SET_BOOTMODE || cmd == SYS_CMD_CLR_BOOTMODE) {
        if (len != 4) {
            TRACE(2,"Invalid SYS CMD len %u for cmd: 0x%x", len, cmd);
            return ERR_DATA_LEN;
        }
    } else {
        if (len != 0) {
            TRACE(2,"Invalid SYS CMD len %u for cmd: 0x%x", len, cmd);
            return ERR_DATA_LEN;
        }
    }

    switch (cmd) {
        case SYS_CMD_REBOOT: {
            TRACE(0,"--- Reboot---");
            send_reply(cret, 1);
            system_reboot();
            break;
        }
        case SYS_CMD_SHUTDOWN: {
            TRACE(0,"--- Shutdown ---");
            send_reply(cret, 1);
            system_shutdown();
            break;
        }
        case SYS_CMD_SET_BOOTMODE: {
            TRACE(0,"--- Set bootmode ---");
            memcpy(&bootmode, param, 4);
            system_set_bootmode(bootmode);
            send_reply(cret, 1);
            break;
        }
        case SYS_CMD_CLR_BOOTMODE: {
            TRACE(0,"--- Clear bootmode ---");
            memcpy(&bootmode, param, 4);
            system_clear_bootmode(bootmode);
            send_reply(cret, 1);
            break;
        }
        case SYS_CMD_GET_BOOTMODE: {
            TRACE(0,"--- Get bootmode ---");
            bootmode = system_get_bootmode();
            memcpy(&cret[1], &bootmode, 4);
            send_reply(cret, 5);
            break;
        }
        default: {
            TRACE(1,"Invalid command: 0x%x", recv_msg.data[0]);
            return ERR_SYS_CMD;
        }
    }

    return ERR_NONE;
}

static enum ERR_CODE handle_data(unsigned char **buf, size_t *len, int *extra)
{
    enum ERR_CODE errcode = ERR_NONE;
#if 0
	uint32_t rlen = 0;
#endif
    *extra = 0;

    // Checksum
    if (check_sum((unsigned char *)&recv_msg, MSG_TOTAL_LEN(&recv_msg)) != 0xFF) {
        TRACE(0,"Checksum error");
        return ERR_CHECKSUM;
    }

    switch (recv_msg.hdr.type) {
        case TYPE_SYS: {
            TRACE_TIME(0,"------ SYS CMD ------");
            errcode = handle_sys_cmd((enum SYS_CMD_TYPE)recv_msg.data[0], &recv_msg.data[1], recv_msg.hdr.len - 1);
            if (errcode != ERR_NONE) {
                return errcode;
            }
            break;
        }
        case TYPE_READ: {
            TRACE_TIME(0,"------ READ CMD ------");
#if 0
            uint32_t addr = (recv_msg.data[0] << 16) | (recv_msg.data[1] << 8) | recv_msg.data[2];
            uint8_t data[4] = {0};
            rlen = read_reg(addr, data);
            if(rlen == 0)
                return ERR_LEN;
            else {
                send_reply(data, rlen);
            }
#endif
            break;
        }
        case TYPE_WRITE: {
            TRACE_TIME(0,"------ WRITE CMD ------");
#if 0
            uint32_t addr = (recv_msg.data[0] << 16) | (recv_msg.data[1] << 8) | recv_msg.data[2];
            uint32_t wdata = (recv_msg.data[3] << 24) | (recv_msg.data[4] << 16) | (recv_msg.data[5] << 8) | recv_msg.data[6];
            uint8_t data[1] = {0};
            errcode = write_reg(addr, wdata);
            if (errcode != ERR_NONE)
                return errcode;
            else
                send_reply(data, 1);
#endif
            break;
        }

        default:
            break;
    }

    return ERR_NONE;
}

static int parse_packet(unsigned char **buf, size_t *len)
{
    enum ERR_CODE errcode;
    int rlen = *len;
    unsigned char *data;
    int i;
    int extra;
    unsigned char cret;

    switch (parse_state) {
        case PARSE_HEADER:
            ASSERT(rlen > 0 && rlen <= sizeof(recv_msg.hdr), "Invalid rlen!");

            if (recv_msg.hdr.prefix == PREFIX_CHAR) {
                errcode = check_msg_hdr();
                if (errcode != ERR_NONE) {
                    goto _err;
                }
                parse_state = PARSE_DATA;
                *buf = &recv_msg.data[0];
                *len = recv_msg.hdr.len + 1;
            } else {
                data = (unsigned char *)&recv_msg.hdr.prefix;
                for (i = 1; i < rlen; i++) {
                    if (data[i] == PREFIX_CHAR) {
                        memmove(&recv_msg.hdr.prefix, &data[i], rlen - i);
                        break;
                    }
                }
                *buf = &data[rlen - i];
                *len = sizeof(recv_msg.hdr) + i - rlen;
            }
            break;
        case PARSE_DATA:
            errcode = handle_data(buf, len, &extra);
            if (errcode != ERR_NONE) {
                goto _err;
            }
            // Receive next message
            reset_parse_state(buf, len);
            break;
        default:
            TRACE(1,"Invalid parse_state: %d", parse_state);
            break;
    }

    return 0;

_err:
    cancel_input();
    cret = (unsigned char)errcode;
    send_reply(&cret, 1);

    return 1;
}

void comm_loop(void)
{
    int ret;
    unsigned char *buf = NULL;
    size_t len = 0;
    size_t buf_len, rlen;

_sync:
    reset_transport();
    reset_parse_state(&buf, &len);

    while (1) {
        rlen = 0;
        if (parse_state == PARSE_HEADER) {
            set_recv_timeout(default_recv_timeout_idle);
        } else {
            set_recv_timeout(default_recv_timeout_short);
        }
        buf_len = 0;

        ret = recv_data_ex(buf, buf_len, len, &rlen);
        if (ret) {
            TRACE(1,"Receiving data failed: %d", ret);
            goto _err;
        }

        if (len != rlen) {
            TRACE(2,"Receiving part of the data: expect=%u real=%u", len, rlen);
            goto _err;
        }

        ret = parse_packet(&buf, &len);
        if (ret) {
            TRACE(0,"Parsing packet failed");
            goto _err;
        }
    }

_err:
    ret = handle_error();
    if (ret == 0) {
        TRACE(0,"retry ...");
        goto _sync;
    }

    return;
}
#endif
