/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#ifdef CHIP_HAS_USB

#include "plat_addr_map.h"
#include "reg_usb.h"
#include "hal_usbhost.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "string.h"
#include "cmsis_nvic.h"
#include "hwtimer_list.h"
#include "pmu.h"

#define MAX_CHAN_NUM                            10
#define MAX_EP_NUM                              16

#define MAX_XFER_SIZE                           (USBC_HCTSIZN_XFERSIZE_MASK >> USBC_HCTSIZN_XFERSIZE_SHIFT)
#define MAX_XFER_PKT                            (USBC_HCTSIZN_PKTCNT_MASK >> USBC_HCTSIZN_PKTCNT_SHIFT)

#define HAL_USBC_PHY_FREQ_MHZ                   48
#define HAL_USBC_PHY_FREQ_MHZ_HS                60
#define HAL_USBC_HPRT_WC_MASK                   \
    (USBC_HPRT_PRTSUSP | USBC_HPRT_PRTOVRCURRCHNG | USBC_HPRT_PRTENCHNG | USBC_HPRT_PRTENA | USBC_HPRT_PRTCONNDET)

#define HAL_USBHOST_TIMEOUT_TICKS               MS_TO_TICKS(200)

#define USBHOST_TRACE(n, mask, str, ...)        { if (usbhost_trmask & (1 << mask)) { TRACE(n, str, ##__VA_ARGS__); } }
#define USBHOST_FUNC_ENTRY_TRACE(mask)          { if (usbhost_trmask & (1 << mask)) { FUNC_ENTRY_TRACE(); } }

enum HAL_USBHOST_CHAN_STATE_T {
    HAL_USBHOST_CHAN_IDLE,
    HAL_USBHOST_CHAN_ALLOC,
    HAL_USBHOST_CHAN_INIT,
    HAL_USBHOST_CHAN_XFER,

    HAL_USBHOST_CHAN_QTY
};

struct HAL_USBHOST_CHAN_DESC_T {
    enum HAL_USBHOST_CHAN_STATE_T state;
    uint32_t start_xfer_size;
    uint16_t start_pkt_cnt;
    uint8_t err_cnt;
    struct HAL_USBHOST_CHAN_TYPE_T type;
    struct HAL_USBHOST_XFER_T xfer;
};

static const uint32_t usbhost_trmask = (1 << 0); //~0UL; //(1 << 3) | (1 << 4);

static struct USBC_T * const usbc = (struct USBC_T *)USB_BASE;

static HAL_USBHOST_PORT_HANDLER port_handler;
static HAL_USBHOST_DELAY_FUNC delay_func;

static struct HAL_USBHOST_CHAN_DESC_T chan_desc[MAX_CHAN_NUM];

static struct HAL_USBHOST_SETUP_XFER_T setup_xfer;

static volatile bool in_setup = false;

static bool usbhost_opened = false;

static enum HAL_USBHOST_SETUP_STAGE_T cur_setup_stage;

static HWTIMER_ID usbhost_timer[MAX_CHAN_NUM];

#ifdef PMU_USB_PIN_CHECK
static HAL_USBHOST_PLUG_HANDLER plug_handler;
#endif

static uint32_t hal_usbhost_get_xfer_size(uint8_t chan, int complete);
static void hal_usbhost_irq_handler(void);
static void hal_usbhost_timeout(void *param);

void hal_usbhost_halt_chan(uint8_t chan)
{
    uint32_t mask;

    USBHOST_TRACE(2,17, "%s: %d", __FUNCTION__, chan);

    if (chan >= MAX_CHAN_NUM) {
        return;
    }

    hwtimer_stop(usbhost_timer[chan]);

    mask = usbc->HCSR[chan].HCINTMSKn;
    usbc->HCSR[chan].HCINTMSKn = 0;
    if ((usbc->HCSR[chan].HCCHARn & USBC_HCCHARN_CHENA) == 0) {
        goto _exit;
    }
    usbc->HCSR[chan].HCINTn = USBC_HCINTN_CHHLTD;
    usbc->HCSR[chan].HCCHARn |= USBC_HCCHARN_CHENA | USBC_HCCHARN_CHDIS;
    while ((usbc->HCSR[chan].HCINTn & USBC_HCINTN_CHHLTD) == 0);

_exit:
    usbc->HCSR[chan].HCINTn = ~0UL;
    usbc->HCSR[chan].HCINTMSKn = mask;
}

static void hal_usbhost_soft_reset(void)
{
    usbc->GRSTCTL |= USBC_CSFTRST;
    while ((usbc->GRSTCTL & USBC_CSFTRST) != 0);
    while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);
}

static void hal_usbhost_init_phy(void)
{
    usbc->GUSBCFG |= USBC_FORCEHSTMODE | USBC_ULPIAUTORES | USBC_ULPIFSLS |
        USBC_PHYSEL | USBC_ULPI_UTMI_SEL;
    usbc->GUSBCFG &= ~(USBC_FSINTF | USBC_PHYIF | USBC_USBTRDTIM_MASK);
    // USBC_USBTRDTIM(9) if AHB bus is 26M
    usbc->GUSBCFG |= USBC_USBTRDTIM(5);
}

int hal_usbhost_open(HAL_USBHOST_PORT_HANDLER port_cb, HAL_USBHOST_DELAY_FUNC delay_fn)
{
    int i;

    USBHOST_FUNC_ENTRY_TRACE(16);

    for (i = 0; i < MAX_CHAN_NUM; i++) {
        chan_desc[i].state = HAL_USBHOST_CHAN_IDLE;
    }

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_52M);

    hal_cmu_usb_set_host_mode();
    hal_cmu_usb_clock_enable();

    hal_usbhost_soft_reset();
    hal_usbhost_init_phy();
    // Reset after selecting PHY
    hal_usbhost_soft_reset();
    // Some core cfg (except for PHY selection) will also be reset during soft reset
    hal_usbhost_init_phy();

#ifdef USB_HIGH_SPEED
    usbc->HCFG = USBC_HCFG_FSLSPCLKSEL(0);
#else
    usbc->HCFG = USBC_HCFG_FSLSSUPP | USBC_HCFG_FSLSPCLKSEL(1);
#endif

    usbc->HPRT = USBC_HPRT_PRTPWR;

    // Clear previous interrupts
    usbc->GINTMSK = 0;
    usbc->GINTSTS = ~0UL;
    usbc->HAINTMSK = 0;
    for (i = 0; i < MAX_CHAN_NUM; i++) {
        usbc->HCSR[i].HCINTMSKn = 0;
        usbc->HCSR[i].HCINTn = ~0UL;
    }
    usbc->GINTMSK = USBC_PRTINT | USBC_HCHINT | USBC_DISCONNINT;

    // Enable DMA mode
    // Burst size 16 words
    usbc->GAHBCFG  = USBC_DMAEN | USBC_HBSTLEN(7);
    usbc->GAHBCFG |= USBC_GLBLINTRMSK;

    port_handler = port_cb;
    delay_func = delay_fn;

    usbhost_opened = true;

    NVIC_SetVector(USB_IRQn, (uint32_t)hal_usbhost_irq_handler);
    NVIC_SetPriority(USB_IRQn, IRQ_PRIORITY_NORMAL);
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);

    //usbc->TPORTDBG1 = 0x11;

    return 0;
}

void hal_usbhost_close(void)
{
    uint8_t chan;

    USBHOST_FUNC_ENTRY_TRACE(15);

#ifdef PMU_USB_PIN_CHECK
    pmu_usb_disable_pin_status_check();
#endif

    NVIC_DisableIRQ(USB_IRQn);

    usbhost_opened = false;

    hal_cmu_usb_clock_disable();

    for (chan = 0; chan < MAX_CHAN_NUM; chan++) {
        hal_usbhost_free_chan(chan);
    }

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_32K);
}

static void hal_usbhost_delay(uint32_t ms)
{
    if (delay_func) {
        delay_func(ms);
    } else {
        hal_sys_timer_delay(MS_TO_TICKS(ms));
    }
}

void hal_usbhost_port_reset(uint32_t ms)
{
    int lock;

    USBHOST_TRACE(2,14, "%s: %d", __FUNCTION__, ms);

    lock = int_lock();
    usbc->HPRT = (usbc->HPRT & ~HAL_USBC_HPRT_WC_MASK) | USBC_HPRT_PRTRST;
    int_unlock(lock);

    hal_usbhost_delay(ms);

    lock = int_lock();
    usbc->HPRT = (usbc->HPRT & ~HAL_USBC_HPRT_WC_MASK) & ~USBC_HPRT_PRTRST;
    int_unlock(lock);
}

static void hal_usbhost_port_suspend(void)
{
    int lock;

    USBHOST_FUNC_ENTRY_TRACE(22);

    lock = int_lock();
    usbc->HPRT = (usbc->HPRT & ~HAL_USBC_HPRT_WC_MASK) | USBC_HPRT_PRTSUSP;
    int_unlock(lock);

    hal_usbhost_delay(3);
}

static void hal_usbhost_port_resume(void)
{
    int lock;

    USBHOST_FUNC_ENTRY_TRACE(22);

    lock = int_lock();
    usbc->HPRT = (usbc->HPRT & ~HAL_USBC_HPRT_WC_MASK) | USBC_HPRT_PRTRES;
    int_unlock(lock);

    hal_usbhost_delay(20);

    lock = int_lock();
    usbc->HPRT = (usbc->HPRT & ~HAL_USBC_HPRT_WC_MASK) & ~USBC_HPRT_PRTRES;
    int_unlock(lock);
}

int hal_usbhost_get_chan(uint8_t *chan)
{
    int i;
    uint32_t lock;

    lock = int_lock();
    for (i = 0; i < MAX_CHAN_NUM; i++) {
        if (chan_desc[i].state == HAL_USBHOST_CHAN_IDLE) {
            chan_desc[i].state = HAL_USBHOST_CHAN_ALLOC;
            break;
        }
    }
    int_unlock(lock);

    USBHOST_TRACE(2,13, "%s: %d", __FUNCTION__, i);

    if (i == MAX_CHAN_NUM) {
        return 1;
    }

    *chan = i;

    ASSERT(usbhost_timer[i] == NULL, "%s: Prev hwtimer not released: 0x%08x", __FUNCTION__, (uint32_t)usbhost_timer[i]);
    usbhost_timer[i] = hwtimer_alloc(hal_usbhost_timeout, (void *)(uint32_t)i);
    if (usbhost_timer[i] == NULL) {
        USBHOST_TRACE(2,0, "%s: WARNING: Failed to alloc hwtimer for chan=%d", __FUNCTION__, i);
        // Continue even if usbhost_timer is null
    }

    return 0;
}

int hal_usbhost_free_chan(uint8_t chan)
{
    USBHOST_TRACE(2,12, "%s: %d", __FUNCTION__, chan);

    if (chan >= MAX_CHAN_NUM) {
        return 1;
    }

    hwtimer_stop(usbhost_timer[chan]);
    hwtimer_free(usbhost_timer[chan]);
    usbhost_timer[chan] = NULL;

    chan_desc[chan].state = HAL_USBHOST_CHAN_IDLE;

    return 0;
}

int hal_usbhost_init_chan(uint8_t chan, const struct HAL_USBHOST_CHAN_TYPE_T *type)
{
    USBHOST_TRACE(7,11, "%s: chan=%d mps=%d ep=%d in=%d type=%d addr=%d",
        __FUNCTION__, chan, type->mps, type->ep_num, type->ep_in, type->ep_type, type->dev_addr);

    if (chan >= MAX_CHAN_NUM) {
        return 1;
    }
    if (chan_desc[chan].state != HAL_USBHOST_CHAN_ALLOC) {
        return 2;
    }
    if (usbc->HCSR[chan].HCCHARn & USBC_HCCHARN_CHENA) {
        return 3;
    }
    if ((type->dev_addr & (USBC_HCCHARN_DEVADDR_MASK >> USBC_HCCHARN_DEVADDR_SHIFT)) != type->dev_addr) {
        return 4;
    }
    if (type->ep_num >= MAX_EP_NUM) {
        return 5;
    }
    if (type->ep_type >= HAL_USBHOST_EP_QTY) {
        return 6;
    }
    if ((type->mps & (USBC_HCCHARN_MPS_MASK >> USBC_HCCHARN_MPS_SHIFT)) != type->mps) {
        return 7;
    }
    if (type->mps == 0) {
        return 8;
    }

    memcpy(&chan_desc[chan].type, type, sizeof(chan_desc[chan].type));
    chan_desc[chan].err_cnt = 0;

    usbc->HCSR[chan].HCINTMSKn = 0;
    usbc->HCSR[chan].HCINTn = ~0UL;

    usbc->HCSR[chan].HCCHARn = USBC_HCCHARN_MPS(type->mps) | USBC_HCCHARN_EPNUM(type->ep_num) |
        (type->ep_in ? USBC_HCCHARN_EPDIR : 0) | USBC_HCCHARN_EPTYPE(type->ep_type) |
        USBC_HCCHARN_DEVADDR(type->dev_addr);

    usbc->HCSR[chan].HCINTMSKn = USBC_HCINTN_AHBERR | USBC_HCINTN_CHHLTD;
    usbc->HAINTMSK |= (1 << chan);
    usbc->GINTMSK |= USBC_HCHINT;

    chan_desc[chan].state = HAL_USBHOST_CHAN_INIT;

    return 0;
}

int hal_usbhost_update_chan_dev_addr(uint8_t chan, uint8_t dev_addr)
{
    USBHOST_TRACE(3,10, "%s: chan=%d dev_addr=%d", __FUNCTION__, chan, dev_addr);

    if (chan >= MAX_CHAN_NUM) {
        return 1;
    }
    if (chan_desc[chan].state != HAL_USBHOST_CHAN_INIT) {
        return 2;
    }

    chan_desc[chan].type.dev_addr = dev_addr;

    usbc->HCSR[chan].HCCHARn = SET_BITFIELD(usbc->HCSR[chan].HCCHARn, USBC_HCCHARN_DEVADDR, dev_addr);

    chan_desc[chan].state = HAL_USBHOST_CHAN_INIT;

    return 0;
}

int hal_usbhost_update_chan_mps(uint8_t chan, uint16_t mps)
{
    USBHOST_TRACE(3,9, "%s: chan=%d mps=%d", __FUNCTION__, chan, mps);

    if (chan >= MAX_CHAN_NUM) {
        return 1;
    }
    if (chan_desc[chan].state != HAL_USBHOST_CHAN_INIT) {
        return 2;
    }

    chan_desc[chan].type.mps = mps;

    usbc->HCSR[chan].HCCHARn = SET_BITFIELD(usbc->HCSR[chan].HCCHARn, USBC_HCCHARN_MPS, mps);

    chan_desc[chan].state = HAL_USBHOST_CHAN_INIT;

    return 0;
}

int hal_usbhost_start_xfer(uint8_t chan, const struct HAL_USBHOST_XFER_T *xfer)
{
    uint32_t max_periodic_len;
    uint32_t pkt_cnt;
    uint32_t size;
    enum HAL_USBHOST_PID_TYPE_T pid;
    uint8_t multi_cnt;

    USBHOST_TRACE(5,7, "%s: chan=%d size=%u mc=%d pid=%d",
        __FUNCTION__, chan, xfer->size, xfer->multi_cnt, xfer->pid);

    if (chan >= MAX_CHAN_NUM) {
        return 1;
    }
    if (chan_desc[chan].state != HAL_USBHOST_CHAN_INIT) {
        return 2;
    }
    if (usbc->HCSR[chan].HCCHARn & USBC_HCCHARN_CHENA) {
        return 3;
    }
    if (((uint32_t)xfer->buf & 0x3) != 0) {
        return 4;
    }
    if (chan_desc[chan].type.ep_type == HAL_USBHOST_EP_ISO ||
            chan_desc[chan].type.ep_type == HAL_USBHOST_EP_INT) {
        max_periodic_len = xfer->multi_cnt * chan_desc[chan].type.mps;
        if (max_periodic_len < xfer->size) {
            return 5;
        }
    } else {
        if (xfer->size > MAX_XFER_SIZE) {
            return 6;
        }
    }

    pkt_cnt = (xfer->size + (chan_desc[chan].type.mps - 1)) / chan_desc[chan].type.mps;
    if (pkt_cnt > MAX_XFER_PKT) {
        return 7;
    }
    if (pkt_cnt == 0) {
        pkt_cnt = 1;
    }

    if (chan_desc[chan].type.ep_in) {
        size = pkt_cnt * chan_desc[chan].type.mps;
    } else {
        size = xfer->size;
    }

    chan_desc[chan].start_xfer_size = size;
    chan_desc[chan].start_pkt_cnt = pkt_cnt;
    memcpy(&chan_desc[chan].xfer, xfer, sizeof(chan_desc[chan].xfer));

    pid = xfer->pid;
    if (pid == HAL_USBHOST_PID_AUTO) {
        pid = GET_BITFIELD(usbc->HCSR[chan].HCTSIZn, USBC_HCTSIZN_PID);
    }
    multi_cnt = xfer->multi_cnt;

    if (chan_desc[chan].type.ep_type == HAL_USBHOST_EP_ISO ||
            chan_desc[chan].type.ep_type == HAL_USBHOST_EP_INT) {
        multi_cnt = pkt_cnt;

        if (chan_desc[chan].type.ep_type == HAL_USBHOST_EP_ISO) {
            pid = HAL_USBHOST_PID_DATA0; // Full speed
        }

        if (usbc->HFNUM & 0x1) {
            usbc->HCSR[chan].HCCHARn &= ~USBC_HCCHARN_ODDFRM;
        } else {
            usbc->HCSR[chan].HCCHARn |= USBC_HCCHARN_ODDFRM;
        }
    }

    chan_desc[chan].state = HAL_USBHOST_CHAN_XFER;

    usbc->HCSR[chan].HCTSIZn = USBC_HCTSIZN_PID(pid) | USBC_HCTSIZN_PKTCNT(pkt_cnt) | USBC_HCTSIZN_XFERSIZE(size);
    usbc->HCSR[chan].HCDMAn = (uint32_t)xfer->buf;

    usbc->HCSR[chan].HCINTn = ~0UL;

    usbc->HCSR[chan].HCCHARn = SET_BITFIELD(usbc->HCSR[chan].HCCHARn, USBC_HCCHARN_EC, multi_cnt);
    usbc->HCSR[chan].HCCHARn &= ~USBC_HCCHARN_CHDIS;
    usbc->HCSR[chan].HCCHARn |= USBC_HCCHARN_CHENA;

    if (xfer->handler) {
        hwtimer_start(usbhost_timer[chan], HAL_USBHOST_TIMEOUT_TICKS);
    }

    return 0;
}

static void hal_usbhost_setup_xfer_handler(uint8_t chan, uint8_t *buf, uint32_t len, enum HAL_USBHOST_XFER_ERR_T error)
{
    int ret;
    struct HAL_USBHOST_XFER_T xfer;
    enum HAL_USBHOST_SETUP_STAGE_T handler_stage;
    uint8_t handler_chan;

    ret = 1;
    if (error != HAL_USBHOST_XFER_ERR_NONE) {
        goto _exit;
    }

    USBHOST_TRACE(5,6, "%s: chan=%d cur=%d next=%d error=%d",
        __FUNCTION__, chan, cur_setup_stage, setup_xfer.next_stage, error);

    handler_stage = setup_xfer.next_stage;
    handler_chan = HAL_USBHOST_CHAN_NONE;

    switch (handler_stage) {
        case HAL_USBHOST_SETUP_DATA_IN:
        case HAL_USBHOST_SETUP_DATA_OUT:
            xfer.buf = setup_xfer.data_buf;
            xfer.size = setup_xfer.setup_pkt.wLength;
            xfer.pid = HAL_USBHOST_PID_DATA1;
            xfer.multi_cnt = 0;
            xfer.handler = hal_usbhost_setup_xfer_handler;
            if (handler_stage == HAL_USBHOST_SETUP_DATA_IN) {
                handler_chan = setup_xfer.chan_in;
                setup_xfer.next_stage = HAL_USBHOST_SETUP_STATUS_OUT;
            } else {
                handler_chan = setup_xfer.chan_out;
                setup_xfer.next_stage = HAL_USBHOST_SETUP_STATUS_IN;
            }
            ret = hal_usbhost_start_xfer(handler_chan, &xfer);
            if (ret) {
                goto _exit;
            }
            break;
        case HAL_USBHOST_SETUP_STATUS_IN:
        case HAL_USBHOST_SETUP_STATUS_OUT:
            if (cur_setup_stage == HAL_USBHOST_SETUP_DATA_IN) {
                if (setup_xfer.setup_pkt.wLength != len) {
                    USBHOST_TRACE(2,0, "Invalid setup data length: %d expect=%d", len, setup_xfer.setup_pkt.wLength);
                    // Update received len
                    setup_xfer.setup_pkt.wLength = len;
                }
            }
            xfer.buf = NULL;
            xfer.size = 0;
            xfer.pid = HAL_USBHOST_PID_DATA1;
            xfer.multi_cnt = 0;
            xfer.handler = hal_usbhost_setup_xfer_handler;
            if (handler_stage == HAL_USBHOST_SETUP_STATUS_IN) {
                handler_chan = setup_xfer.chan_in;
            } else {
                handler_chan = setup_xfer.chan_out;
            }
            setup_xfer.next_stage = HAL_USBHOST_SETUP_DONE;
            ret = hal_usbhost_start_xfer(handler_chan, &xfer);
            if (ret) {
                goto _exit;
            }
            break;
        case HAL_USBHOST_SETUP_DONE:
            ret = 0;
            goto _exit;
            break;
        default:
            ASSERT(false, "Invalid setup next stage %d for chan %d", setup_xfer.next_stage, chan);
            ret = 1;
            goto _exit;
    }

    cur_setup_stage = handler_stage;

    return;

_exit:
    if (error != HAL_USBHOST_XFER_ERR_NONE) {
        setup_xfer.next_stage = HAL_USBHOST_SETUP_ERROR;
    }

    {
        struct HAL_USBHOST_SETUP_XFER_T setup;

        memcpy(&setup, &setup_xfer, sizeof(setup));
        in_setup = false;
        if (setup.handler) {
            setup.handler(&setup, ret);
        }
    }

    return;
}

int hal_usbhost_start_setup_xfer(const struct HAL_USBHOST_SETUP_XFER_T *setup, uint32_t *recv_len)
{
    int ret;
    struct HAL_USBHOST_XFER_T xfer;

    USBHOST_TRACE(7,6, "%s: out=%d in=%d type=0x%02x req=0x%02x wlen=%d next_stage=%d",
        __FUNCTION__, setup->chan_out, setup->chan_in, setup->setup_pkt.bmRequestType,
        setup->setup_pkt.bRequest, setup->setup_pkt.wLength, setup->next_stage);

    if (setup->next_stage >= HAL_USBHOST_SETUP_STATUS_OUT) {
        return -1;
    }

    if (in_setup) {
        return -2;
    }
    in_setup = true;

    memcpy(&setup_xfer, setup, sizeof(setup_xfer));
    cur_setup_stage = HAL_USBHOST_SETUP_STAGE_QTY;

    xfer.buf = (uint8_t *)&setup_xfer.setup_pkt;
    xfer.size = sizeof(setup_xfer.setup_pkt);
    xfer.pid = HAL_USBHOST_PID_SETUP;
    xfer.multi_cnt = 0;
    xfer.handler = hal_usbhost_setup_xfer_handler;

    ret = hal_usbhost_start_xfer(setup_xfer.chan_out, &xfer);
    if (ret) {
        in_setup = false;
        return ret;
    }

    if (setup->handler == NULL) {
        while (in_setup);
        if (setup_xfer.next_stage != HAL_USBHOST_SETUP_DONE) {
            return -3;
        }
        if (recv_len) {
            if (setup->next_stage == HAL_USBHOST_SETUP_DATA_IN) {
                *recv_len = setup_xfer.setup_pkt.wLength;
            } else {
                *recv_len = 0;
            }
        }
    }

    return 0;
}

static void hal_usbhost_stop_all_chans(void)
{
    int i;
    uint32_t xfer;
    HAL_USBHOST_XFER_COMPL_HANDLER handler;
    uint8_t *buf;

    usbc->HAINTMSK = 0;

    for (i = 0; i < MAX_CHAN_NUM; i++) {
        hal_usbhost_halt_chan(i);

        if (chan_desc[i].state != HAL_USBHOST_CHAN_XFER) {
            continue;
        }

        if (chan_desc[i].xfer.handler) {
            // TODO: Check whether HCTSIZn is reset after channel is halted
            xfer = hal_usbhost_get_xfer_size(i, 0);

            // Reset the chan_desc to INIT state so that it can be reused in callback
            handler = chan_desc[i].xfer.handler;
            buf = chan_desc[i].xfer.buf;
            chan_desc[i].state = HAL_USBHOST_CHAN_INIT;

            handler(i, buf, xfer, HAL_USBHOST_XFER_ERR_DISCONN);
        }
    }

    usbc->HAINT = ~0UL;
}

static void hal_usbhost_disconn_handler(void)
{
    USBHOST_FUNC_ENTRY_TRACE(5);

    usbc->GINTMSK &= ~USBC_HCHINT;

    hal_usbhost_stop_all_chans();

    if (port_handler) {
        port_handler(HAL_USBHOST_PORT_DISCONN);
    }
}

static void hal_usbhost_alloc_fifo(void)
{
    // FIFO configuration should be started after port enabled, or 60 ms after soft reset

    hal_usbhost_stop_all_chans();

    // RX FIFO Calculation
    // -------------------
    // DATA Packets + Status Info   : (MPS / 4 + 1) * m
    // OutEp XFER COMPL             : 1 * m
    // NAK/NYET Handling            : 1 * outEpNum

#define RXFIFOSIZE  (2 * (MAX_USBHOST_PACKET_SIZE / 4 + 1 + 1) + USBHOST_EPNUM)
#define TXFIFOSIZE  (2 * (MAX_USBHOST_PACKET_SIZE / 4))

    // Rx Fifo Size (and init fifo_addr)
    usbc->GRXFSIZ = USBC_RXFDEP(RXFIFOSIZE);

    // EP0 / Non-periodic Tx Fifo Size
    usbc->GNPTXFSIZ = USBC_NPTXFSTADDR(RXFIFOSIZE) | USBC_NPTXFDEPS(TXFIFOSIZE);

    // Flush all FIFOs
    usbc->GRSTCTL = USBC_TXFNUM(0x10) | USBC_TXFFLSH | USBC_RXFFLSH;
    while ((usbc->GRSTCTL & (USBC_TXFFLSH | USBC_RXFFLSH)) != 0);
}

static void hal_usbhost_port_handler(void)
{
    uint32_t prt;
    uint32_t speed;
    enum HAL_USBHOST_PORT_EVENT_T event;

    prt = usbc->HPRT;
    // USBC_HPRT_PRTENA also controls the port status
    usbc->HPRT = prt & ~(USBC_HPRT_PRTENA | USBC_HPRT_PRTSUSP);

    USBHOST_TRACE(2,4, "%s: 0x%08x", __FUNCTION__, prt);

    if (prt & USBC_HPRT_PRTCONNDET) {
        if (port_handler) {
            port_handler(HAL_USBHOST_PORT_CONN);
        }
    }

    if (prt & USBC_HPRT_PRTENCHNG) {
        if (prt & USBC_HPRT_PRTENA) {
            speed = GET_BITFIELD(usbc->HPRT,USBC_HPRT_PRTSPD);
            if (speed == 1 ||
#ifdef USB_HIGH_SPEED
                    speed == 0 ||
#endif
                    0) {
#ifdef USB_HIGH_SPEED
                if (speed == 0) {
                    // High speed
                    usbc->HFIR = SET_BITFIELD(usbc->HFIR, USBC_HFIR_FRINT, 125 * HAL_USBC_PHY_FREQ_MHZ_HS);
                    event = HAL_USBHOST_PORT_EN_HS;
                } else
#else
                {
                    // Full speed
                    usbc->HFIR = SET_BITFIELD(usbc->HFIR, USBC_HFIR_FRINT, 1000 * HAL_USBC_PHY_FREQ_MHZ);
                    event = HAL_USBHOST_PORT_EN_FS;
                }
#endif
                // Config FIFOs
                hal_usbhost_alloc_fifo();
                // Notify upper layer
                if (port_handler) {
                    port_handler(event);
                }
            } else {
                // High (0) or low (2) speed not supported
                if (port_handler) {
                    port_handler(HAL_USBHOST_PORT_EN_BAD);
                }
            }
        }
    }
}

static void hal_usbhost_retry_chan(uint8_t chan, uint32_t size)
{
    uint32_t pkt_cnt;

    USBHOST_TRACE(3,20, "%s: chan=%d size=%u", __FUNCTION__, chan, size);

#if 0
    if (chan_desc[chan].state != HAL_USBHOST_CHAN_XFER) {
        return;
    }
#endif

    hal_usbhost_halt_chan(chan);

    pkt_cnt = (size + chan_desc[chan].type.mps - 1) / chan_desc[chan].type.mps;
    if (pkt_cnt == 0) {
        pkt_cnt = 1;
    }

    usbc->HCSR[chan].HCTSIZn = (usbc->HCSR[chan].HCTSIZn & ~(USBC_HCTSIZN_PKTCNT_MASK | USBC_HCTSIZN_XFERSIZE_MASK)) |
        USBC_HCTSIZN_PKTCNT(pkt_cnt) | USBC_HCTSIZN_XFERSIZE(size);
    usbc->HCSR[chan].HCDMAn = (uint32_t)chan_desc[chan].xfer.buf + (chan_desc[chan].start_xfer_size - size);

    usbc->HCSR[chan].HCINTn = ~0UL;

    //usbc->HCSR[chan].HCCHARn = SET_BITFIELD(usbc->HCSR[chan].HCCHARn, USBC_HCCHARN_EC, multi_cnt);
    usbc->HCSR[chan].HCCHARn &= ~USBC_HCCHARN_CHDIS;
    usbc->HCSR[chan].HCCHARn |= USBC_HCCHARN_CHENA;
}

static uint32_t hal_usbhost_get_xfer_size(uint8_t chan, int complete)
{
    uint32_t xfer;

    if (complete) {
        if (chan_desc[chan].type.ep_in) {
            xfer = GET_BITFIELD(usbc->HCSR[chan].HCTSIZn, USBC_HCTSIZN_XFERSIZE);
            if (chan_desc[chan].start_xfer_size > xfer) {
                xfer = chan_desc[chan].start_xfer_size - xfer;
            } else {
                xfer = 0;
            }
        } else {
            xfer = chan_desc[chan].start_xfer_size;
        }
    } else {
        xfer = GET_BITFIELD(usbc->HCSR[chan].HCTSIZn, USBC_HCTSIZN_PKTCNT);
        if (chan_desc[chan].start_pkt_cnt > xfer) {
            xfer = (chan_desc[chan].start_pkt_cnt - xfer) * chan_desc[chan].type.mps;
        } else {
            xfer = 0;
        }
    }

    return xfer;
}

static enum HAL_USBHOST_XFER_ERR_T hal_usbhost_get_xfer_error(uint32_t irq)
{
    if (irq & USBC_HCINTN_XFERCOMPL)    return HAL_USBHOST_XFER_ERR_NONE;
    if (irq & USBC_HCINTN_AHBERR)       return HAL_USBHOST_XFER_ERR_AHB;
    if (irq & USBC_HCINTN_STALL)        return HAL_USBHOST_XFER_ERR_STALL;
    if (irq & USBC_HCINTN_XACTERR)      return HAL_USBHOST_XFER_ERR_TRANSACTION;
    if (irq & USBC_HCINTN_BBLERR)       return HAL_USBHOST_XFER_ERR_BABBLE;
    if (irq & USBC_HCINTN_FRMOVRUN)     return HAL_USBHOST_XFER_ERR_FRAME_OVERRUN;
    if (irq & USBC_HCINTN_DATATGLERR)   return HAL_USBHOST_XFER_ERR_DATA_TOGGLE;

    return HAL_USBHOST_XFER_ERR_QTY;
}

static void hal_usbhost_chan_n_handler(uint8_t chan)
{
    uint32_t raw_irq;
    uint32_t irq;
    uint32_t xfer;
    enum HAL_USBHOST_XFER_ERR_T error;
    HAL_USBHOST_XFER_COMPL_HANDLER handler;
    uint8_t *buf;

    USBHOST_TRACE(2,3, "%s: %d", __FUNCTION__, chan);

    if (chan_desc[chan].state != HAL_USBHOST_CHAN_XFER) {
        return;
    }

    raw_irq = usbc->HCSR[chan].HCINTn;
    usbc->HCSR[chan].HCINTn = raw_irq;
    irq = raw_irq & usbc->HCSR[chan].HCINTMSKn;

    xfer = hal_usbhost_get_xfer_size(chan, (raw_irq & USBC_HCINTN_XFERCOMPL));

    USBHOST_TRACE(4,18, "%s: chan=%d HCINTn=0x%08x xfer=%u", __FUNCTION__, chan, raw_irq, xfer);

    error = HAL_USBHOST_XFER_ERR_QTY;

    if (chan_desc[chan].type.ep_type == HAL_USBHOST_EP_BULK ||
            chan_desc[chan].type.ep_type == HAL_USBHOST_EP_CTRL) {
        if (chan_desc[chan].type.ep_in) {
            if (raw_irq & USBC_HCINTN_CHHLTD) {
                if (raw_irq & (USBC_HCINTN_XFERCOMPL | USBC_HCINTN_STALL | USBC_HCINTN_BBLERR)) {
                    chan_desc[chan].err_cnt = 0;
                    usbc->HCSR[chan].HCINTMSKn &= ~(USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_DATATGLERR);
                    error = hal_usbhost_get_xfer_error(raw_irq);
                } else if (raw_irq & USBC_HCINTN_XACTERR) {
                    if (chan_desc[chan].err_cnt >= 2) {
                        error = HAL_USBHOST_XFER_ERR_TRANSACTION;
                    } else {
                        chan_desc[chan].err_cnt++;
                        usbc->HCSR[chan].HCINTMSKn |= USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_DATATGLERR;
                        hal_usbhost_retry_chan(chan, chan_desc[chan].start_xfer_size - xfer);
                        return;
                    }
                }
            } else if (raw_irq & (USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_DATATGLERR)) {
                chan_desc[chan].err_cnt = 0;
                usbc->HCSR[chan].HCINTMSKn &= ~(USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_DATATGLERR);
                return;
            }
        } else {
            if (raw_irq & USBC_HCINTN_CHHLTD) {
                if (raw_irq & (USBC_HCINTN_XFERCOMPL | USBC_HCINTN_STALL)) {
                    chan_desc[chan].err_cnt = 0;
                    usbc->HCSR[chan].HCINTMSKn &= ~(USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_NYET);
                    error = hal_usbhost_get_xfer_error(raw_irq);
                } else if (raw_irq & USBC_HCINTN_XACTERR) {
                    if (chan_desc[chan].err_cnt >= 2) {
                        usbc->HCSR[chan].HCINTMSKn &= ~(USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_NYET);
                        error = HAL_USBHOST_XFER_ERR_TRANSACTION;
                    } else {
                        chan_desc[chan].err_cnt++;
                        usbc->HCSR[chan].HCINTMSKn |= (USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_NYET);
                        hal_usbhost_retry_chan(chan, chan_desc[chan].start_xfer_size - xfer);
                        return;
                    }
                }
            } else if (raw_irq & (USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_NYET)) {
                chan_desc[chan].err_cnt = 0;
                usbc->HCSR[chan].HCINTMSKn &= ~(USBC_HCINTN_ACK | USBC_HCINTN_NAK | USBC_HCINTN_NYET);
                return;
            }
        }
    }

    if (error == HAL_USBHOST_XFER_ERR_QTY) {
        error = hal_usbhost_get_xfer_error(raw_irq);
    }

    if (error == HAL_USBHOST_XFER_ERR_QTY) {
        // Unknown IRQ
        usbc->HCSR[chan].HCINTMSKn &= ~irq;
        USBHOST_TRACE(3,19, "%s: Got unknown IRQ chan=%d irq=0x%08x", __FUNCTION__, chan, irq);
    } else {
        // Stop xfer timer
        hwtimer_stop(usbhost_timer[chan]);

        // Reset the chan_desc to INIT state so that it can be reused in callback
        handler = chan_desc[chan].xfer.handler;
        buf = chan_desc[chan].xfer.buf;
        chan_desc[chan].state = HAL_USBHOST_CHAN_INIT;

        if (error != HAL_USBHOST_XFER_ERR_NONE) {
            usbc->HAINTMSK &= ~(1 << chan);
            if ((raw_irq & USBC_HCINTN_CHHLTD) == 0) {
                hal_usbhost_halt_chan(chan);
            }
        }
        if (handler) {
            handler(chan, buf, xfer, error);
        }
    }
}

static void hal_usbhost_chan_handler(void)
{
    uint8_t i;

    USBHOST_FUNC_ENTRY_TRACE(2);

    for (i = 0; i < MAX_CHAN_NUM; i++) {
        if (usbc->HAINT & (1 << i)) {
            hal_usbhost_chan_n_handler(i);
        }
    }
}

static void hal_usbhost_irq_handler(void)
{
    uint32_t status;

    // Store interrupt flag and reset it
    status = usbc->GINTSTS;
    usbc->GINTSTS = status;

    status &= (usbc->GINTMSK & (USBC_PRTINT | USBC_HCHINT | USBC_DISCONNINT));

    USBHOST_TRACE(2,1, "%s: 0x%08x", __FUNCTION__, status);

    if (status & USBC_DISCONNINT) {
        hal_usbhost_disconn_handler();
        return;
    }
    if (status & USBC_PRTINT) {
        hal_usbhost_port_handler();
    }
    if (status & USBC_HCHINT) {
        hal_usbhost_chan_handler();
    }
}

static void hal_usbhost_timeout(void *param)
{
    uint8_t chan = (uint8_t)(uint32_t)param;
    uint32_t xfer;
    HAL_USBHOST_XFER_COMPL_HANDLER handler;
    uint8_t *buf;

    USBHOST_TRACE(2,21, "%s: %d", __FUNCTION__, chan);

    if (chan_desc[chan].state != HAL_USBHOST_CHAN_XFER) {
        return;
    }

    hal_usbhost_halt_chan(chan);

    if (chan_desc[chan].xfer.handler) {
        // TODO: Check whether HCTSIZn is reset after channel is halted
        xfer = hal_usbhost_get_xfer_size(chan, 0);

        // Reset the chan_desc to INIT state so that it can be reused in callback
        handler = chan_desc[chan].xfer.handler;
        buf = chan_desc[chan].xfer.buf;
        chan_desc[chan].state = HAL_USBHOST_CHAN_INIT;

        handler(chan, buf, xfer, HAL_USBHOST_XFER_ERR_TIMEOUT);
    }
}

#ifdef PMU_USB_PIN_CHECK
static void hal_usbhost_pin_status_change(enum PMU_USB_PIN_CHK_STATUS_T status)
{
    USBHOST_TRACE(2,24, "%s: %d", __FUNCTION__, status);

    if (plug_handler) {
        if (status == PMU_USB_PIN_CHK_DEV_CONN) {
            plug_handler(HAL_USBHOST_PLUG_IN);
        } else if (status == PMU_USB_PIN_CHK_DEV_DISCONN) {
            plug_handler(HAL_USBHOST_PLUG_OUT);
        }
    }
}
#endif

void hal_usbhost_detect(enum HAL_USBHOST_PLUG_STATUS_T status, HAL_USBHOST_PLUG_HANDLER handler)
{
#ifdef PMU_USB_PIN_CHECK
    enum PMU_USB_PIN_CHK_STATUS_T pmu_status;

    USBHOST_FUNC_ENTRY_TRACE(23);

    if (status == HAL_USBHOST_PLUG_IN) {
        pmu_status = PMU_USB_PIN_CHK_DEV_CONN;
    } else if (status == HAL_USBHOST_PLUG_OUT) {
        pmu_status = PMU_USB_PIN_CHK_DEV_DISCONN;
    } else {
        pmu_status = PMU_USB_PIN_CHK_NONE;
    }

    plug_handler = handler;
    if (handler && pmu_status != PMU_USB_PIN_CHK_NONE) {
        pmu_usb_config_pin_status_check(pmu_status, hal_usbhost_pin_status_change, true);
    } else {
        pmu_usb_disable_pin_status_check();
    }
#else
    ASSERT(false, "No aux usb pin status check support");
#endif
}

void hal_usbhost_sleep(void)
{
    USBHOST_FUNC_ENTRY_TRACE(22);

    if (usbhost_opened) {
        hal_usbhost_port_suspend();
#ifdef PMU_USB_PIN_CHECK
        hal_cmu_clock_disable(HAL_CMU_MOD_H_USBC);
        hal_cmu_clock_disable(HAL_CMU_MOD_O_USB);
        pmu_usb_config_pin_status_check(PMU_USB_PIN_CHK_DEV_DISCONN, hal_usbhost_pin_status_change, true);
#endif
    }
}

void hal_usbhost_wakeup(void)
{
    USBHOST_FUNC_ENTRY_TRACE(22);

    if (usbhost_opened) {
#ifdef PMU_USB_PIN_CHECK
        pmu_usb_disable_pin_status_check();
        hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
        hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);
#endif
        hal_usbhost_port_resume();
    }
}

#endif // CHIP_HAS_USB
