#ifdef __USB_COMM__
#include "sys_api_cdc_comm.h"
#include "app_factory_cdc_comm.h"
#include "hal_bootmode.h"
#include "hwtimer_list.h"
#include "pmu.h"
#ifdef CHIP_HAS_USB
#include "usb_cdc.h"
#endif

#define TIMEOUT_INFINITE                ((uint32_t)-1)

const unsigned int default_recv_timeout_short = MS_TO_TICKS(500);
const unsigned int default_recv_timeout_idle = TIMEOUT_INFINITE; //MS_TO_TICKS(10 * 60 * 1000);
const unsigned int default_recv_timeout_4k_data = MS_TO_TICKS(500);
const unsigned int default_send_timeout = MS_TO_TICKS(500);

static uint32_t send_timeout;
static uint32_t recv_timeout;

static volatile bool cancel_xfer = false;

static uint32_t xfer_err_time = 0;
static uint32_t xfer_err_cnt = 0;

static HWTIMER_ID xfer_timer;

static const struct USB_SERIAL_CFG_T cdc_cfg = {
    .mode = USB_SERIAL_API_NONBLOCKING,
};

void reset_transport(void)
{
    cancel_xfer = false;

    if (xfer_timer) {
        hwtimer_stop(xfer_timer);
    } else {
        xfer_timer = hwtimer_alloc(NULL, NULL);
    }

    usb_serial_flush_recv_buffer();
    usb_serial_init_xfer();
    set_recv_timeout(default_recv_timeout_short);
    set_send_timeout(default_send_timeout);
}

void set_recv_timeout(unsigned int timeout)
{
    recv_timeout = timeout;
}

void set_send_timeout(unsigned int timeout)
{
    send_timeout = timeout;
}

static void usb_send_timeout(void *param)
{
    usb_serial_cancel_send();
}

static void usb_send_timer_start(void)
{
    if (send_timeout == TIMEOUT_INFINITE) {
        return;
    }

    if (xfer_timer) {
        hwtimer_update_then_start(xfer_timer, usb_send_timeout, NULL, send_timeout);
    }
}

static void usb_send_timer_stop(void)
{
    if (xfer_timer) {
        hwtimer_stop(xfer_timer);
    }
}

static int usb_send_data(const unsigned char *buf, size_t len)
{
    int ret;

    usb_send_timer_start();
    ret = usb_serial_send(buf, len);
    usb_send_timer_stop();
    return ret;
}

int send_data(const unsigned char *buf, size_t len)
{
    if (cancel_xfer) {
        return -1;
    }
    return usb_send_data(buf, len);
}

static void usb_recv_timeout(void *param)
{
    usb_serial_cancel_recv();
}

static void usb_recv_timer_start(void)
{
    if (recv_timeout == TIMEOUT_INFINITE) {
        return;
    }

    if (xfer_timer) {
        hwtimer_update_then_start(xfer_timer, usb_recv_timeout, NULL, recv_timeout);
    }
}

static void usb_recv_timer_stop(void)
{
    if (xfer_timer) {
        hwtimer_stop(xfer_timer);
    }
}

static int usb_recv_data(unsigned char *buf, size_t len, size_t *rlen)
{
    int ret;

    usb_recv_timer_start();
    ret = usb_serial_recv(buf, len);
    usb_recv_timer_stop();
    if (ret == 0) {
        *rlen = len;
    }
    return ret;
}

int recv_data_ex(unsigned char *buf, size_t len, size_t expect, size_t *rlen)
{
    if (cancel_xfer) {
        return -1;
    }
    return usb_recv_data(buf, expect, rlen);
}

static int usb_handle_error(void)
{
    int ret;

    TRACE(0,"****** Send break ******");

    // Send break signal, to tell the peer to reset the connection
    ret = usb_serial_send_break();
    if (ret) {
        TRACE(1,"Sending break failed: %d", ret);
    }
    return ret;
}

int handle_error(void)
{
    int ret = 0;
    uint32_t err_time;

    hal_sys_timer_delay(MS_TO_TICKS(50));

    if (!cancel_xfer) {
        ret = usb_handle_error();
    }

    err_time = hal_sys_timer_get();
    if (xfer_err_cnt == 0 || err_time - xfer_err_time > MS_TO_TICKS(5000)) {
        xfer_err_cnt = 0;
        xfer_err_time = err_time;
    }
    xfer_err_cnt++;
    if (xfer_err_cnt < 3) {
        hal_sys_timer_delay(MS_TO_TICKS(100));
    } else if (xfer_err_cnt < 5) {
        hal_sys_timer_delay(MS_TO_TICKS(500));
    } else {
        hal_sys_timer_delay(MS_TO_TICKS(2000));
    }

    return ret;
}

static int usb_cancel_input(void)
{
    return usb_serial_flush_recv_buffer();
}

int cancel_input(void)
{
    return usb_cancel_input();
}

void system_reboot(void)
{
    hal_sys_timer_delay(MS_TO_TICKS(10));
    hal_cmu_sys_reboot();
}

void system_shutdown(void)
{
#if 0
    if (dld_transport == TRANSPORT_USB) {
        // Avoid PC usb serial driver hanging
        usb_serial_close();
    }
#endif
    hal_sys_timer_delay(MS_TO_TICKS(10));
    pmu_shutdown();
}

void system_set_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_set(bootmode);
}

void system_clear_bootmode(unsigned int bootmode)
{
    bootmode &= ~(HAL_SW_BOOTMODE_READ_ENABLED | HAL_SW_BOOTMODE_WRITE_ENABLED);
    hal_sw_bootmode_clear(bootmode);
}

unsigned int system_get_bootmode(void)
{
    return hal_sw_bootmode_get();
}
#endif
