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
#include "hal_usb.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_sysfreq.h"
#include "hal_chipid.h"
#include "usb_descriptor.h"
#include "string.h"
#include "cmsis_nvic.h"
#include "hwtimer_list.h"
#include "pmu.h"
#ifdef CHIP_HAS_USBPHY
#include "usbphy.h"
#endif

// TODO List:
// 1) Thread-safe issue (race condition)

// Hardware configuration:
// GHWCFG1 = 0x00000000
// GHWCFG2 = 0x228a5512
// GHWCFG3 = 0x03f404e8
// GHWCFG4 = 0x16108020

#define USB_TRACE(n, mask, str, ...)        { if (usb_trmask & (1 << mask)) { TRACE(n, str, ##__VA_ARGS__); } }
#define USB_FUNC_ENTRY_TRACE(mask)          { if (usb_trmask & (1 << mask)) { FUNC_ENTRY_TRACE(); } }

#ifndef CHIP_BEST1000
#define CHIP_HAS_USBIF
#endif

#ifdef USB_HIGH_SPEED
#ifndef USB_ISO_INTERVAL
#define USB_ISO_INTERVAL                    8 // ANY value larger than 1
#endif
#if (USB_ISO_INTERVAL <= 1)
#error "Invalid USB_ISO_INTERVAL"
#endif
#else
#undef USB_ISO_INTERVAL
#define USB_ISO_INTERVAL                    1
#endif

#ifdef SIMU_UAUD_MAX_PKT
// SEND/RECV with mps = USB_MAX_PACKET_SIZE_ISO
#define USB_FIFO_MPS_ISO_RECV               USB_FIFO_MPS_ISO_SEND
#else
#ifdef USB_HIGH_SPEED
// 384K sample rate, 32 bits, 2 channels = 3072 bytes/ms
#define USB_FIFO_MPS_ISO_RECV               1200
#else
// 192K sample rate, 16 bits, 2 channels = 768 bytes/ms
#define USB_FIFO_MPS_ISO_RECV               USB_FIFO_MPS_ISO_SEND
#endif
#endif

#define PIN_CHECK_ENABLE_INTERVAL           MS_TO_TICKS(1)
#define PIN_CHECK_WAIT_RESUME_INTERVAL      MS_TO_TICKS(30)
#define LPM_CHECK_INTERVAL                  MS_TO_TICKS(100) //(US_TO_TICKS(8 + 50) + 2)

#define USB_SYS_FREQ                        HAL_CMU_FREQ_52M

enum DEVICE_STATE {
    ATTACHED,
    POWERED,
    DEFAULT,
    ADDRESS,
    CONFIGURED,
};

struct EPN_OUT_TRANSFER {
    uint8_t *data;
    uint32_t length;
    bool enabled;
};

struct EPN_IN_TRANSFER {
    const uint8_t *data;
    uint32_t length;
    uint16_t pkt_cnt;
    bool zero_len_pkt;
    bool enabled;
};

enum DATA_PID_T {
    DATA_PID_DATA0 = 0,
    DATA_PID_DATA1 = 2,
    DATA_PID_DATA2 = 1,
    DATA_PID_MDATA = 3,
};

enum USB_SLEEP_T {
    USB_SLEEP_NONE,
    USB_SLEEP_SUSPEND,
    USB_SLEEP_L1,
};

static const uint32_t usb_trmask = (1 << 0); //(1 << 2) | (1 << 6) | (1 << 8);

static struct USBC_T * const usbc = (struct USBC_T *)USB_BASE;
#ifdef CHIP_HAS_USBIF
static struct USBIF_T * const usbif = (struct USBIF_T *)(USB_BASE + 0x00040000);
#endif

static uint16_t fifo_addr;

static uint32_t ep0_out_buffer[USB_MAX_PACKET_SIZE_CTRL / 4];
static uint32_t ep0_in_buffer[USB_MAX_PACKET_SIZE_CTRL / 4];

static struct EPN_OUT_TRANSFER epn_out_transfer[EPNUM - 1];
static struct EPN_IN_TRANSFER epn_in_transfer[EPNUM - 1];
#ifdef USB_HIGH_SPEED
static uint8_t epn_in_mc[EPNUM - 1];
#endif

static struct EP0_TRANSFER ep0_transfer;

static struct HAL_USB_CALLBACKS callbacks;

static volatile enum DEVICE_STATE device_state = ATTACHED;
static uint8_t device_cfg = 0;
static uint8_t currentAlternate;
static uint16_t currentInterface;

static enum USB_SLEEP_T device_sleep_status;
static uint8_t device_pwr_wkup_status;
static uint8_t device_test_mode;

#if defined(USB_SUSPEND) && (defined(PMU_USB_PIN_CHECK) || defined(USB_LPM))
#ifdef USB_LPM
static bool lpm_entry;
#endif
static bool usbdev_timer_active;
static HWTIMER_ID usbdev_timer;

static void hal_usb_stop_usbdev_timer(void);
#endif

static void hal_usb_irq_handler(void);

static void hal_usb_init_ep0_transfer(void)
{
    ep0_transfer.stage = NONE_STAGE;
    ep0_transfer.data = NULL;
    ep0_transfer.length = 0;
    ep0_transfer.trx_len = 0;
}

static void hal_usb_init_epn_transfer(void)
{
    memset(&epn_out_transfer[0], 0, sizeof(epn_out_transfer));
    memset(&epn_in_transfer[0], 0, sizeof(epn_in_transfer));
}

static void get_setup_packet(uint8_t *data, struct SETUP_PACKET *packet)
{
    packet->bmRequestType.direction = (data[0] & 0x80) >> 7;
    packet->bmRequestType.type = (data[0] & 0x60) >> 5;
    packet->bmRequestType.recipient = data[0] & 0x1f;
    packet->bRequest = data[1];
    packet->wValue = (data[2] | data[3] << 8);
    packet->wIndex = (data[4] | data[5] << 8);
    packet->wLength = (data[6] | data[7] << 8);
}

static void reset_epn_out_transfer(uint8_t ep)
{
    bool enabled;

    enabled = epn_out_transfer[ep - 1].enabled;

    // Clear epn_out_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_out_transfer[ep - 1], 0, sizeof(epn_out_transfer[0]));

    if (enabled && callbacks.epn_recv_compl[ep - 1]) {
        callbacks.epn_recv_compl[ep - 1](NULL, 0, XFER_COMPL_ERROR);
    }
}

static void reset_epn_in_transfer(uint8_t ep)
{
    bool enabled;

    enabled = epn_in_transfer[ep - 1].enabled;

    // Clear epn_in_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_in_transfer[ep - 1], 0, sizeof(epn_in_transfer[0]));

    if (enabled && callbacks.epn_send_compl[ep - 1]) {
        callbacks.epn_send_compl[ep - 1](NULL, 0, XFER_COMPL_ERROR);
    }
}

static uint32_t get_ep_type(enum EP_DIR dir, uint8_t ep)
{
    uint32_t type;

    if (ep == 0) {
        return E_CONTROL;
    }

    if (dir == EP_OUT) {
        type = GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPTYPE);
    } else {
        type = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE);
    }

    return type;
}

static void _set_global_out_nak(void)
{
    uint32_t start;
    uint32_t sts;
    uint32_t ep;

    usbc->DCTL |= USBC_SGOUTNAK;

    start = hal_sys_timer_get();
    while (((usbc->GINTSTS & USBC_GOUTNAKEFF) == 0) &&
            (hal_sys_timer_get() - start < MS_TO_TICKS(5))) {
        if (hal_sys_timer_get() - start >= US_TO_TICKS(60)) {
            if ((usbc->GRSTCTL & USBC_DMAREQ) == 0) {
                break;
            }
        }
    }

    if (usbc->GINTSTS & USBC_GOUTNAKEFF) {
        return;
    }

    start = hal_sys_timer_get();
    while ((usbc->GINTSTS & USBC_RXFLVL) &&
            (hal_sys_timer_get() - start < MS_TO_TICKS(5))) {
        sts = usbc->GRXSTSP;

        ep = GET_BITFIELD(sts, USBC_EPNUM);
        ASSERT(ep == 0, "Global OUT NAK: Only ep0 packets can be dropped: ep=%u sts=0x%08X", ep, sts);

        // NOTE:
        // Global OUT NAK pattern cannot be read out from GRXSTSP -- consumed by controller automatically when poping?

        hal_sys_timer_delay(US_TO_TICKS(60));
        if (usbc->GINTSTS & USBC_GOUTNAKEFF) {
            return;
        }
    }

    ASSERT((usbc->GINTSTS & USBC_GOUTNAKEFF), "Global OUT NAK: Failed to recover");
}

static void _disable_out_ep(uint8_t ep, uint32_t set, uint32_t clr)
{
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;
    uint32_t doepctl;

    if (ep >= EPNUM) {
        return;
    }

    if (ep == 0) {
        ctrl = &usbc->DOEPCTL0;
        intr = &usbc->DOEPINT0;
    } else {
        ctrl = &usbc->DOEPnCONFIG[ep - 1].DOEPCTL;
        intr = &usbc->DOEPnCONFIG[ep - 1].DOEPINT;
    }

    doepctl = *ctrl;

    if ((doepctl & (USBC_EPENA | USBC_USBACTEP)) == 0) {
        goto _exit;
    }
    if ((doepctl & (USBC_EPENA | USBC_NAKSTS)) == USBC_NAKSTS && set == USBC_SNAK && clr == 0) {
        goto _exit;
    }

    _set_global_out_nak();

    *ctrl |= USBC_SNAK | USBC_USBACTEP | set;

    if (clr) {
        *ctrl &= ~clr;
    }

    // EP0 out cannot be disabled, but stalled
    if (ep != 0) {
        if (doepctl & USBC_EPENA) {
            *intr = USBC_EPDISBLD;
            *ctrl |= USBC_EPDIS;
            while ((*intr & USBC_EPDISBLD) == 0);
        }

        if ((doepctl & USBC_USBACTEP) == 0) {
            *ctrl &= ~USBC_USBACTEP;
        }
    }

    usbc->DCTL |= USBC_CGOUTNAK;

_exit:
    if (ep > 0) {
        reset_epn_out_transfer(ep);
    }
}

static void _disable_in_ep(uint8_t ep, uint32_t set, uint32_t clr)
{
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;
    uint32_t diepctl;

    if (ep >= EPNUM) {
        return;
    }

    if (ep == 0) {
        ctrl = &usbc->DIEPCTL0;
        intr = &usbc->DIEPINT0;
    } else {
        ctrl = &usbc->DIEPnCONFIG[ep - 1].DIEPCTL;
        intr = &usbc->DIEPnCONFIG[ep - 1].DIEPINT;
    }

    diepctl = *ctrl;

    if ((diepctl & (USBC_EPENA | USBC_USBACTEP)) == 0) {
        goto _exit;
    }
    if ((diepctl & (USBC_EPENA | USBC_NAKSTS)) == USBC_NAKSTS && set == USBC_SNAK && clr == 0) {
        goto _exit;
    }

    *intr = USBC_INEPNAKEFF;
    *ctrl |= USBC_SNAK | USBC_USBACTEP | set;
    if ((diepctl & USBC_EPENA) && (diepctl & USBC_NAKSTS) == 0) {
        while ((*intr & USBC_INEPNAKEFF) == 0);
        *intr = USBC_INEPNAKEFF;
    }

    if (clr) {
        *ctrl &= ~clr;
    }

    if (diepctl & USBC_EPENA) {
        *intr = USBC_EPDISBLD;
        *ctrl |= USBC_EPDIS;
        while ((*intr & USBC_EPDISBLD) == 0);
    }

    usbc->GRSTCTL = USBC_TXFNUM(ep) | USBC_TXFFLSH;
    while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

    if ((diepctl & USBC_USBACTEP) == 0) {
        *ctrl &= ~USBC_USBACTEP;
    }

_exit:
    if (ep > 0) {
        reset_epn_in_transfer(ep);
    }
}

void hal_usb_disable_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(3,14, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (dir == EP_OUT) {
        _disable_out_ep(ep, USBC_SNAK, 0);
    } else {
        _disable_in_ep(ep, USBC_SNAK, 0);
    }
}

void hal_usb_stall_ep(enum EP_DIR dir, uint8_t ep)
{
    uint32_t set;
    uint32_t clr;

    USB_TRACE(3,13, "%s: %d ep%d", __FUNCTION__, dir, ep);

    set = USBC_STALL;
    clr = 0;

    if (dir == EP_OUT) {
        _disable_out_ep(ep, set, clr);
    } else {
        _disable_in_ep(ep, set, clr);
    }
}

void hal_usb_unstall_ep(enum EP_DIR dir, uint8_t ep)
{
    uint32_t set;
    uint32_t clr;
    uint8_t type;

    USB_TRACE(3,12, "%s: %d ep%d", __FUNCTION__, dir, ep);

    set = USBC_SNAK;
    clr = USBC_STALL;

    type = get_ep_type(dir, ep);
    if (type == E_INTERRUPT || type == E_BULK) {
        set |= USBC_SETD0PID;
    }

    if (hal_usb_get_ep_stall_state(dir, ep) == 0) {
        // Ep not in stall state
        if (ep != 0 && (type == E_INTERRUPT || type == E_BULK)) {
            if (dir == EP_OUT) {
                usbc->DOEPnCONFIG[ep - 1].DOEPCTL |= USBC_SETD0PID;
            } else {
                usbc->DIEPnCONFIG[ep - 1].DIEPCTL |= USBC_SETD0PID;
            }
        }
        return;
    }

    if (dir == EP_OUT) {
        _disable_out_ep(ep, set, clr);
    } else {
        _disable_in_ep(ep, set, clr);
    }
}

int hal_usb_get_ep_stall_state(enum EP_DIR dir, uint8_t ep)
{
    volatile uint32_t *ctrl;

    if (ep >= EPNUM) {
        return 0;
    }

    // Select ctl register
    if(ep == 0) {
        if (dir == EP_IN) {
            ctrl = &usbc->DIEPCTL0;
        } else {
            ctrl = &usbc->DOEPCTL0;
        }
    } else {
        if (dir == EP_IN) {
            ctrl = &usbc->DIEPnCONFIG[ep - 1].DIEPCTL;
        } else {
            ctrl = &usbc->DOEPnCONFIG[ep - 1].DOEPCTL;
        }
    }

    return ((*ctrl & USBC_STALL) != 0);
}

void hal_usb_stop_ep(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(3,11, "%s: %d ep%d", __FUNCTION__, dir, ep);

    hal_usb_disable_ep(dir, ep);
}

static void hal_usb_stop_all_out_eps(void)
{
    int i;
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;
    uint32_t doepctl;

    USB_FUNC_ENTRY_TRACE(11);

    // Enable global out nak
    _set_global_out_nak();

    for (i = 0; i < EPNUM; i++) {
        if (i == 0) {
            ctrl = &usbc->DOEPCTL0;
            intr = &usbc->DOEPINT0;
        } else {
            ctrl = &usbc->DOEPnCONFIG[i - 1].DOEPCTL;
            intr = &usbc->DOEPnCONFIG[i - 1].DOEPINT;
        }

        doepctl = *ctrl;

        // BUS RESET will clear USBC_USBACTEP but keep USBC_EPENA in ep ctrl
        if (doepctl & (USBC_EPENA | USBC_USBACTEP)) {
            *ctrl |= USBC_SNAK | USBC_USBACTEP;
            // EP0 out cannot be disabled, but stalled
            if (i != 0) {
                if (doepctl & USBC_EPENA) {
                    *intr = USBC_EPDISBLD;
                    *ctrl |= USBC_EPDIS;
                    while ((*intr & USBC_EPDISBLD) == 0);
                }
                if ((doepctl & USBC_USBACTEP) == 0) {
                    *ctrl &= ~USBC_USBACTEP;
                }
            }
        }
    }

    // Disable global out nak
    usbc->DCTL |= USBC_CGOUTNAK;
}

static void hal_usb_stop_all_in_eps(void)
{
    int i;
    volatile uint32_t *ctrl;
    volatile uint32_t *intr;
    uint32_t diepctl;

    USB_FUNC_ENTRY_TRACE(11);

    usbc->DCTL |= USBC_SGNPINNAK;
    while ((usbc->GINTSTS & USBC_GINNAKEFF) == 0);

    for (i = 0; i < EPNUM; i++) {
        if (i == 0) {
            ctrl = &usbc->DIEPCTL0;
            intr = &usbc->DIEPINT0;
        } else {
            ctrl = &usbc->DIEPnCONFIG[i - 1].DIEPCTL;
            intr = &usbc->DIEPnCONFIG[i - 1].DIEPINT;
        }

        diepctl = *ctrl;

        // BUS RESET will clear USBC_USBACTEP but keep USBC_EPENA in ep ctrl
        if (diepctl & (USBC_EPENA | USBC_USBACTEP)) {
            *intr = USBC_INEPNAKEFF;
            *ctrl |= USBC_SNAK | USBC_USBACTEP;
            if ((diepctl & USBC_EPENA) && (diepctl & USBC_NAKSTS) == 0) {
                while ((*intr & USBC_INEPNAKEFF) == 0);
                *intr = USBC_INEPNAKEFF;
            }
            if (diepctl & USBC_EPENA) {
                *intr = USBC_EPDISBLD;
                *ctrl |= USBC_EPDIS;
                while ((*intr & USBC_EPDISBLD) == 0);
            }
            if ((diepctl & USBC_USBACTEP) == 0) {
                *ctrl &= ~USBC_USBACTEP;
            }
        }
    }

    // Flush Tx Fifo
    usbc->GRSTCTL = USBC_TXFNUM(0x10) | USBC_TXFFLSH | USBC_RXFFLSH;
    while ((usbc->GRSTCTL & (USBC_TXFFLSH | USBC_RXFFLSH)) != 0);

    usbc->DCTL |= USBC_CGNPINNAK;
}

static void hal_usb_flush_tx_fifo(uint8_t ep)
{
    usbc->DCTL |= USBC_SGNPINNAK;
    while ((usbc->GINTSTS & USBC_GINNAKEFF) == 0);

    while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);

    //while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

    usbc->GRSTCTL = USBC_TXFNUM(ep) | USBC_TXFFLSH;
    while ((usbc->GRSTCTL & USBC_TXFFLSH) != 0);

    usbc->DCTL |= USBC_CGNPINNAK;
}

static void hal_usb_flush_all_tx_fifos(void)
{
    hal_usb_flush_tx_fifo(0x10);
}

static void POSSIBLY_UNUSED hal_usb_flush_rx_fifo(void)
{
    _set_global_out_nak();

    usbc->GRSTCTL |= USBC_RXFFLSH;
    while ((usbc->GRSTCTL & USBC_RXFFLSH) != 0);

    usbc->DCTL |= USBC_CGOUTNAK;
}

static void hal_usb_alloc_ep0_fifo(void)
{
    uint32_t ram_size;
    uint32_t ram_avail;

    if ((usbc->GHWCFG2 & USBC_DYNFIFOSIZING) == 0) {
        return;
    }

    // All endpoints have been stopped in hal_usb_irq_reset()

    // Configure FIFOs

    // RX FIFO Calculation
    // -------------------
    // SETUP Packets                : 4 * n + 6
    // Global OUT NAK               : 1
    // DATA Packets + Status Info   : (MPS / 4 + 1) * m
    // OutEp XFER COMPL             : 1 * outEpNum
    // OutEp Disable                : 1 * outEpNum

#ifdef USB_ISO
#define RXFIFOSIZE      ((4 * CTRL_EPNUM + 6) + 1 + (2 * (USB_FIFO_MPS_ISO_RECV / 4 + 1)) + (EPNUM * 2))
#else
#define RXFIFOSIZE      ((4 * CTRL_EPNUM + 6) + 1 + (2 * (USB_MAX_PACKET_SIZE_BULK / 4 + 1)) + (EPNUM * 2))
#endif
#define EP0_TXFIFOSIZE  (2 * (USB_MAX_PACKET_SIZE_CTRL + 3) / 4)

#if (RXFIFOSIZE + EP0_TXFIFOSIZE > SPFIFORAM_SIZE)
#error "Invalid FIFO size configuration"
#endif

    ram_size = GET_BITFIELD(usbc->GDFIFOCFG, USBC_GDFIFOCFG);
    ram_avail = GET_BITFIELD(usbc->GDFIFOCFG, USBC_EPINFOBASEADDR);

    ASSERT(SPFIFORAM_SIZE == ram_size, "Bad dfifo size: %u (should be %u)", ram_size, SPFIFORAM_SIZE);
    ASSERT(RXFIFOSIZE + EP0_TXFIFOSIZE <= ram_avail, "Bad dfifo size cfg: rx=%u ep0=%u avail=%u", RXFIFOSIZE, EP0_TXFIFOSIZE, ram_avail);

    // Rx Fifo Size (and init fifo_addr)
    usbc->GRXFSIZ = USBC_RXFDEP(RXFIFOSIZE);
    fifo_addr = RXFIFOSIZE;

    // EP0 / Non-periodic Tx Fifo Size
    usbc->GNPTXFSIZ = USBC_NPTXFSTADDR(fifo_addr) | USBC_NPTXFDEPS(EP0_TXFIFOSIZE);
    fifo_addr += EP0_TXFIFOSIZE;

    // Flush Tx FIFOs
    hal_usb_flush_all_tx_fifos();
}

static void hal_usb_alloc_epn_fifo(uint8_t ep, uint16_t mps)
{
    uint16_t size;
    uint32_t ram_avail;

    if (ep == 0 || ep >= EPNUM) {
        return;
    }

    size = (mps + 3) / 4 * 2;

    ram_avail = GET_BITFIELD(usbc->GDFIFOCFG, USBC_EPINFOBASEADDR);

    ASSERT(fifo_addr + size <= ram_avail,
        "Fifo overflow: fifo_addr=%u, size=%u, avail=%u",
        fifo_addr, size, ram_avail);

    usbc->DTXFSIZE[ep - 1].DIEPTXFn = USBC_INEPNTXFSTADDR(fifo_addr) | USBC_INEPNTXFDEP(size);
    fifo_addr += size;
}

static void hal_usb_soft_reset(void)
{
    usbc->GRSTCTL |= USBC_CSFTRST;
    while ((usbc->GRSTCTL & USBC_CSFTRST) != 0);
    while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);
}

static void hal_usb_init_phy(void)
{
#ifdef USB_HIGH_SPEED
    usbc->GUSBCFG |= USBC_FORCEDEVMODE | USBC_ULPIAUTORES | USBC_ULPIFSLS | USBC_ULPI_UTMI_SEL;
    usbc->GUSBCFG &= ~(USBC_FSINTF | USBC_PHYIF | USBC_PHYSEL | USBC_USBTRDTIM_MASK);
#else
    usbc->GUSBCFG |= USBC_FORCEDEVMODE | USBC_ULPIAUTORES | USBC_ULPIFSLS |
        USBC_PHYSEL | USBC_ULPI_UTMI_SEL;
    usbc->GUSBCFG &= ~(USBC_FSINTF | USBC_PHYIF | USBC_USBTRDTIM_MASK);
#endif
    // USB turnaround time = 4 * AHB Clock + 1 PHY Clock, in terms of PHY clocks.
    // If AHB Clock >= PHY Clock, time can be set to 5.
    // If AHB Clock * 2 == PHY Clock, time should be set to 9.
    usbc->GUSBCFG |= USBC_USBTRDTIM(5);
}

static void hal_usb_device_init(void)
{
    int i;
    uint8_t speed;

#ifdef CHIP_HAS_USBPHY
    usbphy_open();
#endif

#ifdef USB_HIGH_SPEED
#ifdef CHIP_HAS_USBIF
    usbif->USBIF_00 &= ~(USBIF_00_CFG_DR_SUSPEND | USBIF_00_CFG_REG_SUSPEND);
    usbif->USBIF_08 &= ~USBIF_08_CFG_SEL48M;
#endif
    speed = 0;
#else
#ifdef CHIP_HAS_USBIF
    usbif->USBIF_00 |= USBIF_00_CFG_DR_SUSPEND | USBIF_00_CFG_REG_SUSPEND;
    usbif->USBIF_08 |= USBIF_08_CFG_SEL48M;
#endif
    speed = 3;
#endif

#ifdef CHIP_BEST2000
    if (hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_1) {
        // dr_suspend and reg_suspend are inverted since metal 1
        usbif->USBIF_00 ^= USBIF_00_CFG_DR_SUSPEND | USBIF_00_CFG_REG_SUSPEND;
    }
#endif

#ifdef USB_HIGH_SPEED
    // Wait until usbphy clock is ready
    hal_sys_timer_delay(US_TO_TICKS(60));
#endif

    hal_usb_soft_reset();
    hal_usb_init_phy();
    // Reset after selecting PHY
    hal_usb_soft_reset();
    // Some core cfg (except for PHY selection) will also be reset during soft reset
    hal_usb_init_phy();

    usbc->DCFG    &= ~(USBC_DEVSPD_MASK  | USBC_PERFRINT_MASK);
    usbc->DCFG    |= USBC_DEVSPD(speed) | USBC_NZSTSOUTHSHK | USBC_PERFRINT(0);

    // Clear previous interrupts
    usbc->GINTMSK = 0;
    usbc->GINTSTS = ~0UL;
    usbc->DAINTMSK = 0;
    usbc->DOEPMSK = 0;
    usbc->DIEPMSK = 0;
    for (i = 0; i < EPNUM; ++i) {
        if (i == 0) {
            usbc->DOEPINT0 = ~0UL;
            usbc->DIEPINT0 = ~0UL;
        } else {
            usbc->DOEPnCONFIG[i - 1].DOEPINT = ~0UL;
            usbc->DIEPnCONFIG[i - 1].DIEPINT = ~0UL;
        }
    }
    usbc->GINTMSK = USBC_USBRST | USBC_ENUMDONE | USBC_ERLYSUSP | USBC_USBSUSP;

    usbc->DCTL &= ~USBC_SFTDISCON;
#if (USB_ISO_INTERVAL > 1)
    // Not to check frame number and ignore ISO incomplete interrupts
    usbc->DCTL |= USBC_IGNRFRMNUM;
#endif

    // Enable DMA mode
    // Burst size 16 words
    usbc->GAHBCFG  = USBC_DMAEN | USBC_HBSTLEN(7);
    usbc->GAHBCFG |= USBC_GLBLINTRMSK;
}

static void hal_usb_soft_disconnect(void)
{
    // Disable global interrupt
    usbc->GAHBCFG &= ~USBC_GLBLINTRMSK;
    // Soft disconnection
    usbc->DCTL |= USBC_SFTDISCON;

    hal_usb_device_init();
}

static void enable_usb_irq(void)
{
    NVIC_SetVector(USB_IRQn, (uint32_t)hal_usb_irq_handler);
#ifdef USB_ISO
    NVIC_SetPriority(USB_IRQn, IRQ_PRIORITY_HIGH);
#else
    NVIC_SetPriority(USB_IRQn, IRQ_PRIORITY_NORMAL);
#endif
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);
}

int hal_usb_open(const struct HAL_USB_CALLBACKS *c, enum HAL_USB_API_MODE m)
{
    if (c == NULL) {
        return 1;
    }
    if (c->device_desc == NULL ||
        c->cfg_desc == NULL ||
        c->string_desc == NULL ||
        c->setcfg == NULL) {
        return 2;
    }

    if (device_state != ATTACHED) {
        return 3;
    }
    device_state = POWERED;
    device_sleep_status = USB_SLEEP_NONE;

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, USB_SYS_FREQ);

#if defined(USB_SUSPEND) && (defined(PMU_USB_PIN_CHECK) || defined(USB_LPM))
    if (usbdev_timer == NULL) {
        usbdev_timer = hwtimer_alloc(NULL, NULL);
        ASSERT(usbdev_timer, "Failed to alloc usbdev_timer");
    }
#endif

    hal_cmu_usb_set_device_mode();
    hal_cmu_usb_clock_enable();

    memcpy(&callbacks, c, sizeof(callbacks));

    if (usbc->GAHBCFG & USBC_GLBLINTRMSK) {
        hal_usb_soft_disconnect();
    } else {
        hal_usb_device_init();
    }

    enable_usb_irq();

    if (m == HAL_USB_API_BLOCKING) {
        while (device_state != CONFIGURED);
    }

    return 0;
}

int hal_usb_reopen(const struct HAL_USB_CALLBACKS *c, uint8_t dcfg, uint8_t alt, uint16_t itf)
{
    if (c == NULL) {
        return 1;
    }
    if (c->device_desc == NULL ||
        c->cfg_desc == NULL ||
        c->string_desc == NULL ||
        c->setcfg == NULL) {
        return 2;
    }

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, USB_SYS_FREQ);

    memcpy(&callbacks, c, sizeof(callbacks));

    device_state = CONFIGURED;
    device_cfg = dcfg;
    currentAlternate = alt;
    currentInterface = itf;

    enable_usb_irq();

    return 0;
}

void hal_usb_close(void)
{
#ifdef USB_SUSPEND
#if defined(PMU_USB_PIN_CHECK) || defined(USB_LPM)
    // Stop pin check timer
    hal_usb_stop_usbdev_timer();
#ifdef PMU_USB_PIN_CHECK
    // Disabe PMU pin status check
    pmu_usb_disable_pin_status_check();
#endif
#endif
#endif

    NVIC_DisableIRQ(USB_IRQn);

    device_state = ATTACHED;

    // Disable global interrupt
    usbc->GAHBCFG &= ~USBC_GLBLINTRMSK;
    // Soft disconnection
    usbc->DCTL |= USBC_SFTDISCON;
    // Soft reset
    usbc->GRSTCTL |= USBC_CSFTRST;
    usbc->DCTL |= USBC_SFTDISCON;
    usbc->GRSTCTL |= USBC_CSFTRST;
    //while ((usbc->GRSTCTL & USBC_CSFTRST) != 0);
    //while ((usbc->GRSTCTL & USBC_AHBIDLE) == 0);

#ifdef CHIP_HAS_USBPHY
    usbphy_close();
#endif

    hal_cmu_usb_clock_disable();

    memset(&callbacks, 0, sizeof(callbacks));

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_32K);
}

void hal_usb_detect_disconn(void)
{
    // NOTE:
    // PHY detects the disconnection event by DP/DN voltage level change.
    // But DP/DN voltages are provided by vusb ldo inside chip, which has nothing
    // to do with VBUS. That is why USB controller cannot generate the disconnection
    // interrupt.
    // Meanwhile VBUS detection or charger detection can help to generate USB
    // disconnection event via this function.

    USB_FUNC_ENTRY_TRACE(26);

    if (device_state != ATTACHED && callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_DISCONNECT, 0);
    }
}

int hal_usb_configured(void)
{
    return (device_state == CONFIGURED);
}

int hal_usb_suspended(void)
{
    return (device_sleep_status == USB_SLEEP_SUSPEND);
}

uint32_t hal_usb_get_soffn(void)
{
    return GET_BITFIELD(usbc->DSTS, USBC_SOFFN);
}

#ifdef USB_HIGH_SPEED
uint32_t hal_usb_calc_hshb_ep_mps(uint32_t pkt_size)
{
    // For high speed, high bandwidth endpoints
    if (pkt_size <= USB_MAX_PACKET_SIZE_ISO) {
        return pkt_size;
    } else if (pkt_size > USB_MAX_PACKET_SIZE_ISO && pkt_size <= USB_MAX_PACKET_SIZE_ISO * 2) {
        return ALIGN(pkt_size / 2, 4);
    } else {
        // if (pkt_size > USB_MAX_PACKET_SIZE_ISO * 2 && pkt_size <= USB_MAX_PACKET_SIZE_ISO * 3)
        return ALIGN(pkt_size / 3, 4);
    }
}
#endif

int hal_usb_activate_epn(enum EP_DIR dir, uint8_t ep, uint8_t type, uint16_t mps)
{
    uint32_t fifo_mps;

    USB_TRACE(3,10, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (ep == 0 || ep >= EPNUM) {
        return 1;
    }

    if (dir == EP_OUT) {
        // Stop ep out
        if (usbc->DOEPnCONFIG[ep - 1].DOEPCTL & (USBC_EPENA | USBC_USBACTEP)) {
            hal_usb_stop_ep(dir, ep);
        }
        // Config ep out
        usbc->DOEPnCONFIG[ep - 1].DOEPCTL =
            USBC_EPN_MPS(mps) | USBC_EPTYPE(type) |
            USBC_USBACTEP | USBC_SNAK | USBC_SETD0PID;
        // Unstall ep out
        usbc->DOEPnCONFIG[ep - 1].DOEPCTL &= ~USBC_STALL;
        // Unmask ep out interrupt
        usbc->DAINTMSK |= USBC_OUTEPMSK(1 << ep);
    } else {
        fifo_mps = mps;
#ifdef USB_HIGH_SPEED
        if (type == E_ISOCHRONOUS || type == E_INTERRUPT) {
            if (mps > USB_FIFO_MPS_ISO_SEND) {
                fifo_mps = USB_FIFO_MPS_ISO_SEND;
            }
            epn_in_mc[ep - 1] = 1;
        }
#endif
        // Stop ep in
        if (usbc->DIEPnCONFIG[ep - 1].DIEPCTL & (USBC_EPENA | USBC_USBACTEP)) {
            hal_usb_stop_ep(dir, ep);
        }
        // Config ep in
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL =
            USBC_EPN_MPS(mps) | USBC_EPTYPE(type) |
            USBC_USBACTEP | USBC_EPTXFNUM(ep) | USBC_SNAK | USBC_SETD0PID;
        // Allocate tx fifo
        hal_usb_alloc_epn_fifo(ep, fifo_mps);
        // Unstall ep in
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL &= ~USBC_STALL;
        // Unmask ep in interrupt
        usbc->DAINTMSK |= USBC_INEPMSK(1 << ep);
    }

    return 0;
}

int hal_usb_deactivate_epn(enum EP_DIR dir, uint8_t ep)
{
    USB_TRACE(3,9, "%s: %d ep%d", __FUNCTION__, dir, ep);

    if (ep == 0 || ep >= EPNUM) {
        return 1;
    }

    hal_usb_stop_ep(dir, ep);

    if (dir == EP_OUT) {
        usbc->DOEPnCONFIG[ep - 1].DOEPCTL &= ~USBC_USBACTEP;
        // Mask ep out interrupt
        usbc->DAINTMSK &= ~ USBC_OUTEPMSK(1 << ep);
    } else {
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL &= ~USBC_USBACTEP;
        // Mask ep in interrupt
        usbc->DAINTMSK &= ~ USBC_INEPMSK(1 << ep);
    }

    return 0;
}

// NOTE: Not support send epn mps change, which will involve tx fifo reallocation
int hal_usb_update_recv_epn_mps(uint8_t ep, uint16_t mps)
{
    uint8_t type;

    USB_TRACE(3,9, "%s: ep%d mps=%u", __FUNCTION__, ep, mps);

    if (ep == 0 || ep >= EPNUM) {
        return 1;
    }

    if ((usbc->DOEPnCONFIG[ep - 1].DOEPCTL & USBC_USBACTEP) == 0) {
        return 2;
    }

    hal_usb_stop_ep(EP_OUT, ep);

    usbc->DOEPnCONFIG[ep - 1].DOEPCTL &= ~USBC_USBACTEP;
    // Mask ep out interrupt
    usbc->DAINTMSK &= ~ USBC_OUTEPMSK(1 << ep);
    // Config ep out
    type = GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPTYPE);
    usbc->DOEPnCONFIG[ep - 1].DOEPCTL =
        USBC_EPN_MPS(mps) | USBC_EPTYPE(type) |
        USBC_USBACTEP | USBC_SNAK | USBC_SETD0PID;
    // Unmask ep out interrupt
    usbc->DAINTMSK |= USBC_OUTEPMSK(1 << ep);

    return 0;
}

int hal_usb_update_send_epn_mc(uint8_t ep, uint8_t mc)
{
#ifdef USB_HIGH_SPEED
    uint8_t type;

    USB_TRACE(3,9, "%s: ep%d mc=%u", __FUNCTION__, ep, mc);

    if (ep == 0 || ep >= EPNUM) {
        return 1;
    }

    if (mc < 1 || mc > 3) {
        return 3;
    }

    if ((usbc->DIEPnCONFIG[ep - 1].DIEPCTL & USBC_USBACTEP) == 0) {
        return 2;
    }

    type = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE);
    if (type == E_INTERRUPT || type == E_ISOCHRONOUS) {
        epn_in_mc[ep - 1] = mc;
        return 0;
    }
#endif

    return 4;
}

static void hal_usb_recv_ep0(void)
{
    USB_FUNC_ENTRY_TRACE(8);

    // Enable EP0 to receive a new setup packet
    usbc->DOEPTSIZ0 = USBC_SUPCNT(3) | USBC_OEPXFERSIZE0(USB_MAX_PACKET_SIZE_CTRL) | USBC_OEPPKTCNT0;
    usbc->DOEPDMA0 =  (uint32_t)ep0_out_buffer;
    usbc->DOEPINT0 = usbc->DOEPINT0;
    usbc->DOEPCTL0 |= USBC_CNAK | USBC_EPENA;
}

static void hal_usb_send_ep0(const uint8_t *data, uint16_t size)
{
    USB_TRACE(2,8, "%s: %d", __FUNCTION__, size);
    ASSERT(size <= USB_MAX_PACKET_SIZE_CTRL, "Invalid ep0 send size: %d", size);

    if (data && size) {
        memcpy(ep0_in_buffer, data, size);
    }

    // Enable EP0 to send one packet
    usbc->DIEPTSIZ0 = USBC_IEPXFERSIZE0(size) | USBC_IEPPKTCNT0(1);
    usbc->DIEPDMA0 =  (uint32_t)ep0_in_buffer;
    usbc->DIEPINT0 = usbc->DIEPINT0;
    usbc->DIEPCTL0 |= USBC_CNAK | USBC_EPENA;
}

int hal_usb_recv_epn(uint8_t ep, uint8_t *buffer, uint32_t size)
{
    uint16_t mps;
    uint32_t pkt;
    uint32_t xfer;
    uint32_t fn = 0;
#ifdef USB_ISO
    bool isoEp;
    uint32_t lock;
#endif

    USB_TRACE(3,7, "%s: ep%d %d", __FUNCTION__, ep, size);

    if (device_state != CONFIGURED) {
        return 1;
    }
    if (ep == 0 || ep > EPNUM) {
        return 2;
    }
    if (((uint32_t)buffer & 0x3) != 0) {
        return 3;
    }
    if (epn_out_transfer[ep - 1].data != NULL) {
        return 4;
    }
    if ((usbc->DOEPnCONFIG[ep - 1].DOEPCTL & USBC_USBACTEP) == 0) {
        return 5;
    }
    mps = GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPN_MPS);
    mps = ALIGN(mps, 4);
    if (size < mps) {
        return 6;
    }

    if (size > EPN_MAX_XFERSIZE) {
        return 7;
    }
    pkt = size / mps;
    if (pkt > EPN_MAX_PKTCNT) {
        return 8;
    }
    xfer = pkt * mps;
    if (size != xfer) {
        return 9;
    }

    usbc->DOEPnCONFIG[ep - 1].DOEPTSIZ = USBC_OEPXFERSIZE(xfer) | USBC_OEPPKTCNT(pkt);
    usbc->DOEPnCONFIG[ep - 1].DOEPDMA = (uint32_t)buffer;
    usbc->DOEPnCONFIG[ep - 1].DOEPINT = usbc->DOEPnCONFIG[ep - 1].DOEPINT;

    epn_out_transfer[ep - 1].data = buffer;
    epn_out_transfer[ep - 1].length = xfer;
    epn_out_transfer[ep - 1].enabled = true;

#ifdef USB_ISO
    if (GET_BITFIELD(usbc->DOEPnCONFIG[ep - 1].DOEPCTL, USBC_EPTYPE) == E_ISOCHRONOUS) {
        isoEp = true;
    } else {
        isoEp = false;
    }
    if (isoEp) {
        // Set the frame number in time
        lock = int_lock();
        // Get next frame number
        if (GET_BITFIELD(usbc->DSTS, USBC_SOFFN) & 0x1) {
            fn = USBC_SETD0PID;
        } else {
            fn = USBC_SETD1PID;
        }
    }
#endif

    usbc->DOEPnCONFIG[ep - 1].DOEPCTL |= USBC_EPENA | USBC_CNAK | fn;

#ifdef USB_ISO
    if (isoEp) {
        int_unlock(lock);
    }
#endif

    return 0;
}

int hal_usb_send_epn(uint8_t ep, const uint8_t *buffer, uint32_t size, enum ZLP_STATE zlp)
{
    uint16_t mps;
    uint8_t type;
    uint32_t pkt;
    uint32_t fn = 0;
#ifdef USB_ISO
    bool isoEp;
    uint32_t lock;
#endif

    USB_TRACE(3,6, "%s: ep%d %d", __FUNCTION__, ep, size);

    if (device_state != CONFIGURED) {
        return 1;
    }
    if (ep == 0 || ep > EPNUM) {
        return 2;
    }
    if (((uint32_t)buffer & 0x3) != 0) {
        return 3;
    }
    if (epn_in_transfer[ep - 1].data != NULL) {
        return 4;
    }
    if ((usbc->DIEPnCONFIG[ep - 1].DIEPCTL & USBC_USBACTEP) == 0) {
        return 5;
    }
    if (size > EPN_MAX_XFERSIZE) {
        return 7;
    }
    mps = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPN_MPS);
    if (size <= mps) {
        // Also taking care of 0 size packet
        pkt = 1;
    } else {
        // If mps is not aligned in 4 bytes, application should add padding at the end of each packet to
        // make sure a new packet always starts at 4-byte boundary
        pkt = (size + mps - 1) / mps;
        if (pkt > EPN_MAX_PKTCNT) {
            return 8;
        }
    }

    type = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE);
    if (type == E_INTERRUPT || type == E_ISOCHRONOUS) {
#ifdef USB_HIGH_SPEED
        if (pkt != epn_in_mc[ep - 1] && (pkt % epn_in_mc[ep - 1])) {
            // MC is the pkt cnt must be sent in every (micro)frame.
            // The total pkt cnt should be integral multiple of MC value.
            return 9;
        }
#endif
        // Never send a zero length packet at the end of transfer
        epn_in_transfer[ep - 1].zero_len_pkt = false;
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(size) | USBC_IEPPKTCNT(pkt) |
#ifdef USB_HIGH_SPEED
            USBC_MC(epn_in_mc[ep - 1]);
#else
            USBC_MC(1);
#endif
    } else {
        // Check if a zero length packet is needed at the end of transfer
        if (zlp == ZLP_AUTO) {
            epn_in_transfer[ep - 1].zero_len_pkt = ((size % mps) == 0);
        } else {
            epn_in_transfer[ep - 1].zero_len_pkt = false;
        }
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(size) | USBC_IEPPKTCNT(pkt);
    }
    usbc->DIEPnCONFIG[ep - 1].DIEPDMA = (uint32_t)buffer;
    usbc->DIEPnCONFIG[ep - 1].DIEPINT = usbc->DIEPnCONFIG[ep - 1].DIEPINT;

    epn_in_transfer[ep - 1].data = buffer;
    epn_in_transfer[ep - 1].length = size;
    epn_in_transfer[ep - 1].pkt_cnt = pkt;
    epn_in_transfer[ep - 1].enabled = true;

#ifdef USB_ISO
    if (GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE) == E_ISOCHRONOUS) {
        isoEp = true;
    } else {
        isoEp = false;
    }
    if (isoEp) {
        // Set the frame number in time
        lock = int_lock();
        // Get next frame number
        if (GET_BITFIELD(usbc->DSTS, USBC_SOFFN) & 0x1) {
            fn = USBC_SETD0PID;
        } else {
            fn = USBC_SETD1PID;
        }
    }
#endif

    usbc->DIEPnCONFIG[ep - 1].DIEPCTL |= USBC_EPENA | USBC_CNAK | fn;

#ifdef USB_ISO
    if (isoEp) {
        int_unlock(lock);
    }
#endif

    return 0;
}

static void hal_usb_recv_epn_complete(uint8_t ep, uint32_t statusEp)
{
    uint8_t *data;
    uint32_t size;
    uint32_t doeptsiz;
    enum XFER_COMPL_STATE state = XFER_COMPL_SUCCESS;

    USB_TRACE(3,5, "%s: ep%d 0x%08x", __FUNCTION__, ep, statusEp);

    if (!epn_out_transfer[ep - 1].enabled) {
        return;
    }

    doeptsiz = usbc->DOEPnCONFIG[ep - 1].DOEPTSIZ;

    data = epn_out_transfer[ep - 1].data;
    size = GET_BITFIELD(doeptsiz, USBC_OEPXFERSIZE);
    ASSERT(size <= epn_out_transfer[ep - 1].length,
        "Invalid xfer size: size=%d, len=%d", size, epn_out_transfer[ep - 1].length);
    size = epn_out_transfer[ep - 1].length - size;

#ifdef USB_ISO
    uint32_t doepctl = usbc->DOEPnCONFIG[ep - 1].DOEPCTL;

    if (GET_BITFIELD(doepctl, USBC_EPTYPE) == E_ISOCHRONOUS) {
#if (USB_ISO_INTERVAL == 1)
#if 1
        if (GET_BITFIELD(doeptsiz, USBC_OEPPKTCNT) != 0) {
            state = XFER_COMPL_ERROR;
        }
#else
        uint32_t rxdpid = GET_BITFIELD(doeptsiz, USBC_RXDPID);
        uint32_t pkt = epn_out_transfer[ep - 1].length / GET_BITFIELD(doepctl, USBC_EPN_MPS)
            - GET_BITFIELD(doeptsiz, USBC_OEPPKTCNT);
        if ((rxdpid == DATA_PID_DATA0 && pkt != 1) ||
                (rxdpid == DATA_PID_DATA1 && pkt != 2) ||
                (rxdpid == DATA_PID_DATA2 && pkt != 3)) {
            state = XFER_COMPL_ERROR;
        }
#endif
#else // USB_ISO_INTERVAL != 1
        if (statusEp & USBC_PKTDRPSTS) {
            state = XFER_COMPL_ERROR;
        }
#endif // USB_ISO_INTERVAL != 1
    }
#endif // USB_ISO

    // Clear epn_out_transfer[] before invoking the callback,
    // so that ep can be restarted in the callback
    memset(&epn_out_transfer[ep - 1], 0, sizeof(epn_out_transfer[0]));

    if (callbacks.epn_recv_compl[ep - 1]) {
        callbacks.epn_recv_compl[ep - 1](data, size, state);
    }
}

static void hal_usb_send_epn_complete(uint8_t ep, uint32_t statusEp)
{
    const uint8_t *data;
    uint32_t size;
    uint16_t pkt, mps;
    uint8_t type;
    uint32_t mc;
    enum XFER_COMPL_STATE state = XFER_COMPL_SUCCESS;

    USB_TRACE(3,4, "%s: ep%d 0x%08x", __FUNCTION__, ep, statusEp);

    if (!epn_in_transfer[ep - 1].enabled) {
        return;
    }

    if ((statusEp & USBC_XFERCOMPLMSK) == 0) {
        state = XFER_COMPL_ERROR;
    }

    pkt = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ, USBC_IEPPKTCNT);
    if (pkt != 0) {
        state = XFER_COMPL_ERROR;
        mps = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPN_MPS);
        ASSERT(pkt <= epn_in_transfer[ep - 1].pkt_cnt, "Invalid pkt cnt: pkt=%d, pkt_cnt=%d",
            pkt, epn_in_transfer[ep - 1].pkt_cnt);
        size = (epn_in_transfer[ep - 1].pkt_cnt - pkt) * mps;
    } else {
        size = epn_in_transfer[ep - 1].length;
    }

    if (state == XFER_COMPL_SUCCESS && epn_in_transfer[ep - 1].zero_len_pkt) {
        epn_in_transfer[ep - 1].zero_len_pkt = false;
        // Send the last zero length packet, except for isochronous/interrupt endpoints
        type = GET_BITFIELD(usbc->DIEPnCONFIG[ep - 1].DIEPCTL, USBC_EPTYPE);
        if (type == E_INTERRUPT || type == E_ISOCHRONOUS) {
#ifdef USB_HIGH_SPEED
            mc = USBC_MC(epn_in_mc[ep - 1]);
#else
            mc = USBC_MC(1);
#endif
        } else {
            mc = 0;
        }
        usbc->DIEPnCONFIG[ep - 1].DIEPTSIZ = USBC_IEPXFERSIZE(0) | USBC_IEPPKTCNT(1) | mc;
        usbc->DIEPnCONFIG[ep - 1].DIEPCTL |= USBC_EPENA | USBC_CNAK;
    } else {
        data = epn_in_transfer[ep - 1].data;

        // Clear epn_in_transfer[] before invoking the callback,
        // so that ep can be restarted in the callback
        memset(&epn_in_transfer[ep - 1], 0, sizeof(epn_in_transfer[0]));

        if (state != XFER_COMPL_SUCCESS) {
            // The callback will not be invoked when stopping ep,
            // for epn_in_transfer[] has been cleared
            hal_usb_stop_ep(EP_IN, ep);
        }

        if (callbacks.epn_send_compl[ep - 1]) {
            callbacks.epn_send_compl[ep - 1](data, size, state);
        }
    }

}

static bool requestSetAddress(void)
{
    USB_FUNC_ENTRY_TRACE(25);

    /* Set the device address */
    usbc->DCFG = SET_BITFIELD(usbc->DCFG, USBC_DEVADDR, ep0_transfer.setup_pkt.wValue);
    ep0_transfer.stage = STATUS_IN_STAGE;

    if (ep0_transfer.setup_pkt.wValue == 0)
    {
        device_state = DEFAULT;
    }
    else
    {
        device_state = ADDRESS;
    }

    return true;
}

static bool requestSetConfiguration(void)
{
    USB_FUNC_ENTRY_TRACE(24);

    device_cfg = ep0_transfer.setup_pkt.wValue;
    /* Set the device configuration */
    if (device_cfg == 0)
    {
        /* Not configured */
        device_state = ADDRESS;
    }
    else
    {
        if (callbacks.setcfg && callbacks.setcfg(device_cfg))
        {
            /* Valid configuration */
            device_state = CONFIGURED;
            ep0_transfer.stage = STATUS_IN_STAGE;
        }
        else
        {
            return false;
        }
    }

    return true;
}

static bool requestGetConfiguration(void)
{
    USB_FUNC_ENTRY_TRACE(23);

    /* Send the device configuration */
    ep0_transfer.data = &device_cfg;
    ep0_transfer.length = sizeof(device_cfg);
    ep0_transfer.stage = DATA_IN_STAGE;
    return true;
}

static bool requestGetInterface(void)
{
    USB_FUNC_ENTRY_TRACE(22);

    /* Return the selected alternate setting for an interface */
    if (device_state != CONFIGURED)
    {
        return false;
    }

    /* Send the alternate setting */
    ep0_transfer.setup_pkt.wIndex = currentInterface;
    ep0_transfer.data = &currentAlternate;
    ep0_transfer.length = sizeof(currentAlternate);
    ep0_transfer.stage = DATA_IN_STAGE;
    return true;
}

static bool requestSetInterface(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(21);

    if(callbacks.setitf && callbacks.setitf(ep0_transfer.setup_pkt.wIndex, ep0_transfer.setup_pkt.wValue))
    {
        success = true;
        currentInterface = ep0_transfer.setup_pkt.wIndex;
        currentAlternate = ep0_transfer.setup_pkt.wValue;
        ep0_transfer.stage = STATUS_IN_STAGE;
    }
    return success;
}

static bool requestSetFeature(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(20);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
            if (ep0_transfer.setup_pkt.wValue == TEST_MODE) {
                // TODO: Check test mode
                device_test_mode = (ep0_transfer.setup_pkt.wIndex >> 8);
                success = true;
            }
#ifdef USB_SUSPEND
            else if (ep0_transfer.setup_pkt.wValue == DEVICE_REMOTE_WAKEUP) {
                if (callbacks.set_remote_wakeup) {
                    callbacks.set_remote_wakeup(1);
                    device_pwr_wkup_status |= DEVICE_STATUS_REMOTE_WAKEUP;
                    success = true;
                }
            }
#endif
            break;
        case ENDPOINT_RECIPIENT:
            if (ep0_transfer.setup_pkt.wValue == ENDPOINT_HALT)
            {
                // TODO: Check endpoint number
                hal_usb_stall_ep((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF);
                if (callbacks.state_change) {
                    callbacks.state_change(HAL_USB_EVENT_STALL, ep0_transfer.setup_pkt.wIndex & 0xFF);
                }
                success = true;
            }
            break;
        default:
            break;
    }

    ep0_transfer.stage = STATUS_IN_STAGE;

    return success;
}

static bool requestClearFeature(void)
{
    bool success = false;

    USB_FUNC_ENTRY_TRACE(19);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
#ifdef USB_SUSPEND
            if (ep0_transfer.setup_pkt.wValue == DEVICE_REMOTE_WAKEUP) {
                if (callbacks.set_remote_wakeup) {
                    callbacks.set_remote_wakeup(0);
                    device_pwr_wkup_status &= ~DEVICE_STATUS_REMOTE_WAKEUP;
                    success = true;
                }
            }
#endif
            break;
        case ENDPOINT_RECIPIENT:
            /* TODO: We should check that the endpoint number is valid */
            if (ep0_transfer.setup_pkt.wValue == ENDPOINT_HALT)
            {
                hal_usb_unstall_ep((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF);
                if (callbacks.state_change) {
                    callbacks.state_change(HAL_USB_EVENT_UNSTALL, ep0_transfer.setup_pkt.wIndex & 0xFF);
                }
                success = true;
            }
            break;
        default:
            break;
    }

    ep0_transfer.stage = STATUS_IN_STAGE;

    return success;
}

static bool requestGetStatus(void)
{
    static uint16_t status;
    bool success = false;

    USB_FUNC_ENTRY_TRACE(18);

    if (device_state != CONFIGURED)
    {
        /* Endpoint or interface must be zero */
        if (ep0_transfer.setup_pkt.wIndex != 0)
        {
            return false;
        }
    }

    switch (ep0_transfer.setup_pkt.bmRequestType.recipient)
    {
        case DEVICE_RECIPIENT:
            status = device_pwr_wkup_status;
            success = true;
            break;
        case INTERFACE_RECIPIENT:
            status = 0;
            success = true;
            break;
        case ENDPOINT_RECIPIENT:
            /* TODO: We should check that the endpoint number is valid */
            if (hal_usb_get_ep_stall_state((ep0_transfer.setup_pkt.wIndex & 0x80) ? EP_IN : EP_OUT,
                    ep0_transfer.setup_pkt.wIndex & 0xF))
            {
                status = ENDPOINT_STATUS_HALT;
            }
            else
            {
                status = 0;
            }
            success = true;
            break;
        default:
            break;
    }

    if (success)
    {
        /* Send the status */
        ep0_transfer.data = (uint8_t *)&status; /* Assumes little endian */
        ep0_transfer.length = sizeof(status);
        ep0_transfer.stage = DATA_IN_STAGE;
    }

    return success;
}

static bool requestGetDescriptor(void)
{
    bool success = false;
    uint8_t type;
    uint8_t index;
    uint8_t *desc;

    type = DESCRIPTOR_TYPE(ep0_transfer.setup_pkt.wValue);
    index = DESCRIPTOR_INDEX(ep0_transfer.setup_pkt.wValue);

    USB_TRACE(3,3, "%s: %d %d", __FUNCTION__, type, index);

    switch (type)
    {
        case DEVICE_DESCRIPTOR:
            desc = (uint8_t *)callbacks.device_desc(type);
            if (desc != NULL)
            {
                if ((desc[0] == DEVICE_DESCRIPTOR_LENGTH) \
                    && (desc[1] == DEVICE_DESCRIPTOR))
                {
                    ep0_transfer.length = desc[0];
                    ep0_transfer.data = desc;
                    success = true;
                }
            }
            break;
        case CONFIGURATION_DESCRIPTOR:
            desc = (uint8_t *)callbacks.cfg_desc(index);
            if (desc != NULL)
            {
                if ((desc[0] == CONFIGURATION_DESCRIPTOR_LENGTH) \
                    && (desc[1] == CONFIGURATION_DESCRIPTOR))
                {
                    /* Get wTotalLength */
                    ep0_transfer.length = desc[2] | (desc[3] << 8);
                    ep0_transfer.data = desc;
                    // Get self-powered status
                    if ((desc[7] >> 6) & 0x01) {
                        device_pwr_wkup_status |= DEVICE_STATUS_SELF_POWERED;
                    } else {
                        device_pwr_wkup_status &= ~DEVICE_STATUS_SELF_POWERED;
                    }
                    success = true;
                }
            }
            break;
        case STRING_DESCRIPTOR:
            ep0_transfer.data = (uint8_t *)callbacks.string_desc(index);
            if (ep0_transfer.data) {
                ep0_transfer.length = ep0_transfer.data[0];
                success = true;
            }
            break;
        case INTERFACE_DESCRIPTOR:
        case ENDPOINT_DESCRIPTOR:
            /* TODO: Support is optional, not implemented here */
            break;
        case QUALIFIER_DESCRIPTOR:
#ifdef USB_HIGH_SPEED
            desc = (uint8_t *)callbacks.device_desc(type);
            if (desc != NULL)
            {
                if (desc[0] == QUALIFIER_DESCRIPTOR_LENGTH && desc[1] == type)
                {
                    ep0_transfer.length = desc[0];
                    ep0_transfer.data = desc;
                    success = true;
                }
            }
#endif
            break;
        case BOS_DESCRIPTOR:
#ifdef USB_HIGH_SPEED
            desc = (uint8_t *)callbacks.device_desc(type);
            if (desc != NULL)
            {
                if (desc[0] == BOS_DESCRIPTOR_LENGTH && desc[1] == type)
                {
                    // Total length
                    ep0_transfer.length = desc[2];
                    ep0_transfer.data = desc;
                    success = true;
                }
            }
#endif
            break;
        default:
            // Might be a class or vendor specific descriptor, which
            // should be handled in setuprecv callback
            USB_TRACE(1,0, "*** Error: Unknown desc type: %d", type);
            break;
    }

    if (success) {
        ep0_transfer.stage = DATA_IN_STAGE;
    }

    return success;
}

static void hal_usb_reset_all_transfers(void)
{
    int ep;

    USB_FUNC_ENTRY_TRACE(31);

    for (ep = 1; ep < EPNUM; ep++) {
        reset_epn_out_transfer(ep);
        reset_epn_in_transfer(ep);
    }
}

static bool hal_usb_handle_setup(void)
{
    bool success = false;
    enum DEVICE_STATE old_state;

    USB_FUNC_ENTRY_TRACE(17);

    old_state = device_state;

    /* Process standard requests */
    if (ep0_transfer.setup_pkt.bmRequestType.type == STANDARD_TYPE)
    {
        switch (ep0_transfer.setup_pkt.bRequest)
        {
             case GET_STATUS:
                 success = requestGetStatus();
                 break;
             case CLEAR_FEATURE:
                 success = requestClearFeature();
                 break;
             case SET_FEATURE:
                 success = requestSetFeature();
                 break;
             case SET_ADDRESS:
                success = requestSetAddress();
                 break;
             case GET_DESCRIPTOR:
                 success = requestGetDescriptor();
                 break;
             case SET_DESCRIPTOR:
                 /* TODO: Support is optional, not implemented here */
                 success = false;
                 break;
             case GET_CONFIGURATION:
                 success = requestGetConfiguration();
                 break;
             case SET_CONFIGURATION:
                 success = requestSetConfiguration();
                 break;
             case GET_INTERFACE:
                 success = requestGetInterface();
                 break;
             case SET_INTERFACE:
                 success = requestSetInterface();
                 break;
             default:
                 break;
        }
    }

    if (old_state == CONFIGURED && device_state != CONFIGURED) {
        hal_usb_reset_all_transfers();
    }

    return success;
}

static int hal_usb_ep0_setup_stage(uint32_t statusEp)
{
    uint8_t *data;
    uint8_t setup_cnt;
    uint16_t pkt_len;

    // Skip the check on IN/OUT tokens
    usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
    usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;

    if (statusEp & USBC_BACK2BACKSETUP) {
        data = (uint8_t *)(usbc->DOEPDMA0 - 8);
    } else {
        setup_cnt = GET_BITFIELD(usbc->DOEPTSIZ0, USBC_SUPCNT);
        if (setup_cnt >= 3) {
            setup_cnt = 2;
        }
        data = (uint8_t *)((uint32_t)ep0_out_buffer + 8 * (2 - setup_cnt));
    }
    // Init new transfer
    hal_usb_init_ep0_transfer();
    ep0_transfer.stage = SETUP_STAGE;
    get_setup_packet(data, &ep0_transfer.setup_pkt);

    USB_TRACE(5,2, "Got SETUP type=%d, req=0x%x, val=0x%x, idx=0x%x, len=%d",
        ep0_transfer.setup_pkt.bmRequestType.type,
        ep0_transfer.setup_pkt.bRequest,
        ep0_transfer.setup_pkt.wValue,
        ep0_transfer.setup_pkt.wIndex,
        ep0_transfer.setup_pkt.wLength);

    if (ep0_transfer.setup_pkt.wLength == 0 && ep0_transfer.setup_pkt.bmRequestType.direction != EP_OUT) {
        USB_TRACE(0,0, "*** Error: Ep0 dir should be out if wLength=0");
        return 1;
    }

    if (callbacks.setuprecv == NULL || !callbacks.setuprecv(&ep0_transfer)) {
        if (!hal_usb_handle_setup()) {
            return 1;
        }
    }

#if 0
    if (ep0_transfer.setup_pkt.wLength == 0) {
        ep0_transfer.setup_pkt.bmRequestType.direction = EP_OUT;
    }
#endif

    if (ep0_transfer.stage == DATA_OUT_STAGE) {
        if (ep0_transfer.data == NULL) {
            ep0_transfer.data = (uint8_t *)ep0_out_buffer;
        }
    } else if (ep0_transfer.stage == DATA_IN_STAGE) {
        if (ep0_transfer.length > ep0_transfer.setup_pkt.wLength) {
            ep0_transfer.length = ep0_transfer.setup_pkt.wLength;
        }
        pkt_len = ep0_transfer.length;
        if (pkt_len > USB_MAX_PACKET_SIZE_CTRL) {
            pkt_len = USB_MAX_PACKET_SIZE_CTRL;
        }
        hal_usb_send_ep0(ep0_transfer.data, pkt_len);
    } else if(ep0_transfer.stage == STATUS_IN_STAGE) {
        hal_usb_send_ep0(NULL, 0);
    } else {
        USB_TRACE(1,0, "*** Setup stage switches to invalid stage: %d", ep0_transfer.stage);
        return 1;
    }

    return 0;
}

static int hal_usb_ep0_data_out_stage(void)
{
    uint16_t pkt_len;
    uint16_t reg_size;

    reg_size = GET_BITFIELD(usbc->DOEPTSIZ0, USBC_OEPXFERSIZE0);

    ASSERT(reg_size <= USB_MAX_PACKET_SIZE_CTRL, "Invalid ep0 recv size");

    pkt_len = MIN(USB_MAX_PACKET_SIZE_CTRL - reg_size, (uint16_t)(ep0_transfer.length - ep0_transfer.trx_len));

    ASSERT(ep0_transfer.length >= ep0_transfer.trx_len + pkt_len,
        "Invalid ep0 recv len: length=%u trx_len=%u pkt_len=%u", ep0_transfer.length, ep0_transfer.trx_len, pkt_len);

    memcpy(ep0_transfer.data + ep0_transfer.trx_len, ep0_out_buffer, pkt_len);
    ep0_transfer.trx_len += pkt_len;

    // Always enable setup packet receiving
    // CAUTION: Start ep0 recv before invoking datarecv callbacks to handle all kinds of endpoints.
    //          Some USBC events are triggered by patterns in receive FIFO, only when these
    //          patterns are popped by the core (DMA reading) or the application (usbc->GRXSTSP reading).
    //          E.g., Global OUT NAK (DCTL.SGOUTNak) triggers GINTSTS.GOUTNakEff, but the interrupt will
    //          never raise in data out stage without DMA or usbc->GRXSTSP reading.
    hal_usb_recv_ep0();

    pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
    if (pkt_len == 0) {
        if (callbacks.datarecv == NULL || !callbacks.datarecv(&ep0_transfer)) {
            //hal_usb_stall_ep(EP_OUT, 0);
            //hal_usb_stall_ep(EP_IN, 0);
            //return;
        }
        ep0_transfer.stage = STATUS_IN_STAGE;
        // Check error on IN/OUT tokens
        usbc->DOEPMSK |= USBC_OUTTKNEPDISMSK;
        // Send status packet
        hal_usb_send_ep0(NULL, 0);
    } else {
        // Receive next data packet
    }

    return 0;
}

static int hal_usb_ep0_data_in_stage(void)
{
    uint16_t pkt_len;
    bool zero_len_pkt = false;

    ASSERT(GET_BITFIELD(usbc->DIEPTSIZ0, USBC_IEPPKTCNT0) == 0, "Invalid ep0 sent pkt cnt");
    ASSERT(ep0_transfer.length >= ep0_transfer.trx_len, "Invalid ep0 sent len 1: length=%u trx_len=%u", ep0_transfer.length, ep0_transfer.trx_len);

    pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
    if (pkt_len == 0) {
        // The last zero length packet was sent successfully
        ep0_transfer.stage = STATUS_OUT_STAGE;
        // Receive status packet (receiving is always enabled)
    } else {
        // Update sent count
        if (pkt_len == USB_MAX_PACKET_SIZE_CTRL) {
            zero_len_pkt = true;
        } else if (pkt_len > USB_MAX_PACKET_SIZE_CTRL) {
            pkt_len = USB_MAX_PACKET_SIZE_CTRL;
        }
        ep0_transfer.trx_len += pkt_len;

        ASSERT(ep0_transfer.length >= ep0_transfer.trx_len, "Invalid ep0 sent len 2: length=%u trx_len=%u", ep0_transfer.length, ep0_transfer.trx_len);

        // Send next packet
        pkt_len = ep0_transfer.length - ep0_transfer.trx_len;
        if (pkt_len > USB_MAX_PACKET_SIZE_CTRL) {
            pkt_len = USB_MAX_PACKET_SIZE_CTRL;
        }
        if (pkt_len == 0) {
            if (zero_len_pkt) {
                // Send the last zero length packet
                hal_usb_send_ep0(NULL, 0);
            } else {
                ep0_transfer.stage = STATUS_OUT_STAGE;
                // Check error on IN/OUT tokens
                usbc->DIEPMSK |= USBC_INTKNTXFEMPMSK;
                // Receive status packet (receiving is always enabled)
            }
        } else {
            hal_usb_send_ep0(ep0_transfer.data + ep0_transfer.trx_len, pkt_len);
        }
    }

    return 0;
}

static int hal_usb_ep0_status_stage(void)
{
    // Done with status packet
    ep0_transfer.stage = NONE_STAGE;
    // Skip the check on IN/OUT tokens
    usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
    usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;

    return 0;
}

static void hal_usb_ep0_test_mode_check(void)
{
    if (device_test_mode) {
        usbc->DCTL = SET_BITFIELD(usbc->DCTL, USBC_TSTCTL, device_test_mode);
        device_test_mode = 0;
    }
}

static void hal_usb_handle_ep0_packet(enum EP_DIR dir, uint32_t statusEp)
{
    int ret;

    USB_TRACE(3,16, "%s: dir=%d, statusEp=0x%08x", __FUNCTION__, dir, statusEp);

    if (dir == EP_OUT) {
        if ((statusEp & (USBC_XFERCOMPL | USBC_STSPHSERCVD)) == (USBC_XFERCOMPL | USBC_STSPHSERCVD)) {
            // From Linux driver
            if (GET_BITFIELD(usbc->DOEPTSIZ0, USBC_OEPXFERSIZE0) == USB_MAX_PACKET_SIZE_CTRL &&
                    (usbc->DOEPTSIZ0 & USBC_OEPPKTCNT0)) {
                // Abnormal case
                USB_TRACE(2,0, "*** EP0 OUT empty compl with stsphsercvd: stage=%d, size=0x%08x",
                    ep0_transfer.stage, usbc->DOEPTSIZ0);
                // Always enable setup packet receiving
                hal_usb_recv_ep0();
                return;
            }
        }

        if (statusEp & USBC_XFERCOMPL) {
            // New packet received
            if (statusEp & USBC_SETUP) {
                // Clean previous transfer
                if (ep0_transfer.stage != NONE_STAGE) {
                    USB_TRACE(1,0, "*** Setup stage breaks previous stage %d", ep0_transfer.stage);
                    hal_usb_stop_ep(EP_IN, 0);
                }
                // New setup packet
                ep0_transfer.stage = SETUP_STAGE;
            } else if (statusEp & USBC_STUPPKTRCVD) {
                // Clean previous transfer
                if (ep0_transfer.stage != NONE_STAGE) {
                    USB_TRACE(1,0, "*** Wait setup stage breaks previous stage %d", ep0_transfer.stage);
                    hal_usb_stop_ep(EP_IN, 0);
                }
                // New setup packet received, and wait for USBC h/w state machine finished
                // CAUTION: Enabling data receipt before getting USBC_SETUP interrupt might cause
                //          race condition in h/w, which leads to missing USBC_XFERCOMPL interrupt
                //          after the data has been received and acked
                ep0_transfer.stage = WAIT_SETUP_STAGE;
                return;
            }
        } else {
            // No packet received
            if (statusEp & USBC_SETUP) {
                if (ep0_transfer.stage == WAIT_SETUP_STAGE) {
                    // Previous packet is setup packet
                    ep0_transfer.stage = SETUP_STAGE;
                } else {
                    USB_TRACE(1,0, "*** Setup interrupt occurs in stage %d", ep0_transfer.stage);
                    // The setup packet has been processed
                    return;
                }
            }
        }
    }

    if (ep0_transfer.stage == SETUP_STAGE) {
        ASSERT(dir == EP_OUT, "Invalid dir for setup stage");

        ret = hal_usb_ep0_setup_stage(statusEp);
        if (ret) {
            // Error
            ep0_transfer.stage = NONE_STAGE;
            hal_usb_stall_ep(EP_OUT, 0);
            hal_usb_stall_ep(EP_IN, 0);
            // H/w will unstall EP0 automatically when a setup token is received
            //hal_usb_recv_ep0();
        }
        // Always enable setup packet receiving
        hal_usb_recv_ep0();

        return;
    }

    if (statusEp & USBC_XFERCOMPL) {
        if (dir == EP_OUT) {
            if (ep0_transfer.stage == DATA_OUT_STAGE) {
                hal_usb_ep0_data_out_stage();
            } else if (ep0_transfer.stage == STATUS_OUT_STAGE) {
                hal_usb_ep0_status_stage();
                // Always enable setup packet receiving
                hal_usb_recv_ep0();
            } else {
                // Abnormal case
                USB_TRACE(2,0, "*** EP0 OUT compl in stage %d with size 0x%08x", ep0_transfer.stage, usbc->DOEPTSIZ0);
                // Always enable setup packet receiving
                hal_usb_recv_ep0();
            }
        } else {
            if (ep0_transfer.stage == DATA_IN_STAGE) {
                hal_usb_ep0_data_in_stage();
            } else if (ep0_transfer.stage == STATUS_IN_STAGE) {
                hal_usb_ep0_status_stage();
                hal_usb_ep0_test_mode_check();
            } else {
                // Abnormal case
                USB_TRACE(2,0, "*** EP0 IN compl in stage %d with size 0x%08x", ep0_transfer.stage, usbc->DIEPTSIZ0);
            }
        }
    }
}

static void hal_usb_irq_reset(void)
{
    USB_FUNC_ENTRY_TRACE(2);

    device_state = DEFAULT;
    device_pwr_wkup_status = 0;
    device_sleep_status = USB_SLEEP_NONE;

    usbc->PCGCCTL &= ~USBC_STOPPCLK;
    usbc->DCTL &= ~USBC_RMTWKUPSIG;

    hal_usb_reset_all_transfers();
    hal_usb_stop_all_out_eps();
    hal_usb_stop_all_in_eps();

    // Unmask ep0 interrupts
    usbc->DAINTMSK = USBC_INEPMSK(1 << 0) | USBC_OUTEPMSK(1 << 0);
    usbc->DOEPMSK = USBC_XFERCOMPLMSK | USBC_SETUPMSK;
    usbc->DIEPMSK = USBC_XFERCOMPLMSK | USBC_TIMEOUTMSK;
    usbc->GINTMSK |= USBC_OEPINT | USBC_IEPINT
#if defined(USB_ISO) && (USB_ISO_INTERVAL == 1)
        | USBC_INCOMPISOOUT | USBC_INCOMPISOIN
#endif
        ;
#ifdef USB_SUSPEND
    usbc->GINTMSK &= ~USBC_WKUPINT;
#endif
#ifdef USB_LPM
    usbc->GINTMSK |= USBC_LPM_INT;
    usbc->PCGCCTL |= USBC_ENBL_L1GATING;
    usbc->GLPMCFG = SET_BITFIELD(usbc->GLPMCFG, USBC_HIRD_THRES, USB_L1_LIGHT_SLEEP_BESL) | USBC_HIRD_THRES_BIT4 |
        USBC_LPMCAP | USBC_APPL1RES | USBC_ENBESL; // | USBC_ENBLSLPM;
#ifdef CHIP_HAS_USBIF
    usbif->USBIF_04 = SET_BITFIELD(usbif->USBIF_04, USBIF_04_CFG_SLEEP_THSD, USB_L1_LIGHT_SLEEP_BESL);
#endif
#endif

    // Config ep0 size
    hal_usb_alloc_ep0_fifo();
    // Reset device address
    usbc->DCFG &= ~USBC_DEVADDR_MASK;

    hal_usb_init_ep0_transfer();
    hal_usb_init_epn_transfer();

    if (callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_RESET, 0);
    }
}

static void hal_usb_irq_enum_done(void)
{
    uint8_t speed;
    uint8_t mps = 0;

    USB_FUNC_ENTRY_TRACE(2);

    speed = GET_BITFIELD(usbc->DSTS, USBC_ENUMSPD);
    if (speed == 0) {
        // High speed -- ERROR!
        mps = 0; // 64 bytes
    } else if (speed == 1 || speed == 3) {
        // Full speed
        mps = 0; // 64 bytes
    } else {
        // Low speed -- ERROR!
        mps = 3; // 8 bytes
    }
    // Only support 64-byte MPS !
    mps = 0;
    // Config max packet size
    usbc->DIEPCTL0 = USBC_EP0_MPS(mps) | USBC_USBACTEP | USBC_EPTXFNUM(0) | USBC_SNAK;
    usbc->DOEPCTL0 = USBC_EP0_MPS(mps) | USBC_USBACTEP | USBC_SNAK;

    hal_usb_recv_ep0();
}

#if defined(USB_ISO) && (USB_ISO_INTERVAL == 1)
static void hal_usb_irq_incomp_iso_out(void)
{
    int i;
    uint32_t ctrl;
    uint32_t sof_fn;
    uint32_t statusEp;

    sof_fn = GET_BITFIELD(usbc->DSTS, USBC_SOFFN);

    for (i = 0; i < EPNUM - 1; i++) {
        ctrl = usbc->DOEPnCONFIG[i].DOEPCTL;
        if ((ctrl & USBC_EPENA) && ((ctrl >> USBC_EPDPID_SHIFT) & 1) == (sof_fn & 1) &&
                GET_BITFIELD(ctrl, USBC_EPTYPE) == E_ISOCHRONOUS) {
            statusEp = usbc->DOEPnCONFIG[i].DOEPINT;
            hal_usb_disable_ep(EP_OUT, i + 1);
            break;
        }
    }

    if (i < EPNUM - 1) {
        USB_TRACE(4,17, "%s ep%d: INT=0x%08x, SOF=0x%04x", __FUNCTION__, i + 1, statusEp, sof_fn);
    } else {
        USB_TRACE(1,17, "%s: No valid ISO ep", __FUNCTION__);
    }
}

static void hal_usb_irq_incomp_iso_in(void)
{
    int i;
    uint32_t ctrl;
    uint32_t sof_fn;
    uint32_t statusEp;

    sof_fn = GET_BITFIELD(usbc->DSTS, USBC_SOFFN);

    for (i = 0; i < EPNUM - 1; i++) {
        ctrl = usbc->DIEPnCONFIG[i].DIEPCTL;
        if ((ctrl & USBC_EPENA) && ((ctrl >> USBC_EPDPID_SHIFT) & 1) == (sof_fn & 1) &&
                GET_BITFIELD(ctrl, USBC_EPTYPE) == E_ISOCHRONOUS) {
            statusEp = usbc->DIEPnCONFIG[i].DIEPINT;
            hal_usb_disable_ep(EP_IN, i + 1);
            break;
        }
    }

    if (i < EPNUM - 1) {
        USB_TRACE(4,17, "%s ep%d: INT=0x%08x, SOF=0x%04x", __FUNCTION__, i + 1, statusEp, sof_fn);
    } else {
        USB_TRACE(1,17, "%s: No valid ISO ep", __FUNCTION__);
    }
}
#endif

#ifdef USB_SUSPEND
static void hal_usb_restore_clock(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, USB_SYS_FREQ);

#ifdef PMU_USB_PIN_CHECK
#ifdef CHIP_HAS_USBPHY
    // Enable dig phy clock
    usbphy_wakeup();
#endif

    hal_cmu_clock_enable(HAL_CMU_MOD_H_USBC);
    hal_cmu_clock_enable(HAL_CMU_MOD_O_USB);
#endif

    usbc->PCGCCTL &= ~USBC_STOPPCLK;

#ifdef PMU_USB_PIN_CHECK
    NVIC_ClearPendingIRQ(USB_IRQn);
    NVIC_EnableIRQ(USB_IRQn);
#endif
}

static void hal_usb_stop_clock(void)
{
#ifdef PMU_USB_PIN_CHECK
    // Disable USB IRQ to avoid errors when RESET/RESUME is detected before stopping USB clock
    NVIC_DisableIRQ(USB_IRQn);
#endif

    usbc->PCGCCTL |= USBC_STOPPCLK;

#ifdef PMU_USB_PIN_CHECK
    hal_cmu_clock_disable(HAL_CMU_MOD_O_USB);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_USBC);

#ifdef CHIP_HAS_USBPHY
    // Disable dig phy clock
    usbphy_sleep();
#endif
#endif

    hal_sysfreq_req(HAL_SYSFREQ_USER_USB, HAL_CMU_FREQ_32K);
}

#ifdef PMU_USB_PIN_CHECK
static void hal_usb_pin_status_resume(enum PMU_USB_PIN_CHK_STATUS_T status);

static void hal_usb_pin_check_enable_check(void *param)
{
    pmu_usb_enable_pin_status_check();
}

static void hal_usb_pin_check_cancel_resume(void *param)
{
    USB_TRACE(3,18, "[%X] %s: DSTS=0x%08x", hal_sys_timer_get(), __FUNCTION__, usbc->DSTS);

    if (usbc->DSTS & USBC_SUSPSTS) {
        hal_usb_stop_clock();
        pmu_usb_enable_pin_status_check();
    }
}

static void hal_usb_pin_status_resume(enum PMU_USB_PIN_CHK_STATUS_T status)
{
    USB_TRACE(2,18, "%s: %d", __FUNCTION__, status);

    // Start timer to check resume status, so as to avoid fake pin resume signal
    if (usbdev_timer_active) {
        hwtimer_stop(usbdev_timer);
    }
    hwtimer_update_then_start(usbdev_timer, hal_usb_pin_check_cancel_resume, NULL, PIN_CHECK_WAIT_RESUME_INTERVAL);
    usbdev_timer_active = 1;

    hal_usb_restore_clock();
}
#endif

#if defined(PMU_USB_PIN_CHECK) || defined(USB_LPM)
static void hal_usb_stop_usbdev_timer(void)
{
    if (usbdev_timer_active) {
        hwtimer_stop(usbdev_timer);
        usbdev_timer_active = 0;
    }
}
#endif

static void hal_usb_sleep(enum USB_SLEEP_T cause)
{
    USB_FUNC_ENTRY_TRACE(18);

    device_sleep_status = cause;

    if (callbacks.state_change) {
        callbacks.state_change((cause == USB_SLEEP_SUSPEND) ? HAL_USB_EVENT_SUSPEND : HAL_USB_EVENT_L1_DEEP_SLEEP, 0);
    }

    usbc->GINTMSK |= USBC_WKUPINT;
#ifdef USB_LPM
    usbc->GINTMSK &= ~USBC_LPM_INT;
#endif

    hal_usb_stop_clock();

#ifdef PMU_USB_PIN_CHECK
    pmu_usb_config_pin_status_check(PMU_USB_PIN_CHK_HOST_RESUME, hal_usb_pin_status_resume, false);
    // Stat timer to check current pin status
    if (usbdev_timer_active) {
        hwtimer_stop(usbdev_timer);
    }
    hwtimer_update_then_start(usbdev_timer, hal_usb_pin_check_enable_check, NULL, PIN_CHECK_ENABLE_INTERVAL);
    usbdev_timer_active = 1;
#endif
}

static void hal_usb_wakeup(void)
{
    USB_FUNC_ENTRY_TRACE(18);

    device_sleep_status = USB_SLEEP_NONE;

#ifndef PMU_USB_PIN_CHECK
    hal_usb_restore_clock();
#endif

    usbc->GINTMSK &= ~USBC_WKUPINT;
#ifdef USB_LPM
    usbc->GINTMSK |= USBC_LPM_INT;
#endif

    if (callbacks.state_change) {
        callbacks.state_change(HAL_USB_EVENT_RESUME, 0);
    }
}

static void hal_usb_send_resume_signal(int enable)
{
    if (enable) {
#ifdef PMU_USB_PIN_CHECK
        // Stop pin check timer
        hal_usb_stop_usbdev_timer();
        // Disabe PMU pin status check
        pmu_usb_disable_pin_status_check();
#endif

        // Restore USB clock and IRQ
        hal_usb_restore_clock();

        usbc->GINTMSK &= ~USBC_WKUPINT;

        usbc->DCTL |= USBC_RMTWKUPSIG;
    } else {
        usbc->DCTL &= ~USBC_RMTWKUPSIG;
    }
}
#endif

int hal_usb_remote_wakeup(int signal)
{
#ifdef USB_SUSPEND
    USB_TRACE(2,15, "%s: %d", __FUNCTION__, signal);

    if (signal) {
        if (device_sleep_status != USB_SLEEP_SUSPEND) {
            return 1;
        }
        if ((device_pwr_wkup_status & DEVICE_STATUS_REMOTE_WAKEUP) == 0) {
            return 2;
        }

        hal_usb_send_resume_signal(1);
    } else {
        hal_usb_send_resume_signal(0);

        // USBC will NOT generate resume IRQ in case of remote wakeup, so fake one here
        hal_usb_wakeup();
    }
#endif

    return 0;
}

void hal_usb_lpm_sleep_enable(void)
{
#ifdef USB_LPM
    if (usbc->GLPMCFG & USBC_ENBESL) {
        //usbc->GLPMCFG &= ~USBC_RSTRSLPSTS;
    }
    usbc->GLPMCFG |= (USBC_APPL1RES | USBC_HIRD_THRES_BIT4);

    usbc->PCGCCTL |= USBC_ENBL_L1GATING;
#endif
}

void hal_usb_lpm_sleep_disable(void)
{
#ifdef USB_LPM
    uint32_t cnt;
    uint32_t lock;
    uint32_t lpm;
    bool POSSIBLY_UNUSED rmtWake = false;

    usbc->PCGCCTL &= ~USBC_ENBL_L1GATING;

    usbc->GLPMCFG &= ~(USBC_APPL1RES | USBC_HIRD_THRES_BIT4 | USBC_ENBLSLPM);
    if (usbc->GLPMCFG & USBC_ENBESL) {
        usbc->GLPMCFG |= USBC_RSTRSLPSTS;
    }

    lpm_entry = false;

    // Resume if in L1 state
    if (usbc->GLPMCFG & USBC_SLPSTS) {
        cnt = 0;
        while ((usbc->GLPMCFG & (USBC_BREMOTEWAKE | USBC_SLPSTS | USBC_L1RESUMEOK)) ==
                (USBC_BREMOTEWAKE | USBC_SLPSTS)) {
            hal_sys_timer_delay(US_TO_TICKS(0));
            if (++cnt > 3) {
                break;
            }
        }

        lock = int_lock();

        lpm = usbc->GLPMCFG;

        if (lpm & USBC_SLPSTS) {
            if ((lpm & (USBC_BREMOTEWAKE | USBC_L1RESUMEOK)) == (USBC_BREMOTEWAKE | USBC_L1RESUMEOK)) {
                hal_usb_send_resume_signal(1);
                rmtWake = true;
#if 0
                // Detect race condition between remote wake and L1 state change
                if ((usbc->GLPMCFG & USBC_SLPSTS) == 0) {
                    usbc->DCTL &= ~USBC_RMTWKUPSIG;
                }
#endif
#ifdef USB_SUSPEND
                if (device_sleep_status == USB_SLEEP_L1) {
                    // USBC will NOT generate resume IRQ in case of remote wakeup, so fake one here
                    hal_usb_wakeup();
                }
#endif
            } else if ((lpm & (USBC_BREMOTEWAKE | USBC_L1RESUMEOK)) == USBC_BREMOTEWAKE) {
                TRACE(1,"\n*** ERROR: LPM Disable: Failed to wait L1 resume OK: 0x%08X\n", lpm);
            }
        }

        int_unlock(lock);

        if (rmtWake) {
            USB_TRACE(1,0, "LPM RmtWake: 0x%08X", lpm);
        }
    }
#endif
}

#ifdef USB_LPM
#ifdef USB_SUSPEND
static void hal_usb_lpm_check(void *param)
{
    uint32_t lpm;
    uint32_t lock;

    // TODO: Disable USB irq only (might conflict with hal_usb_stop_clock)
    lock = int_lock();

    lpm = usbc->GLPMCFG;

    if (lpm_entry && GET_BITFIELD(lpm, USBC_HIRD) >= USB_L1_DEEP_SLEEP_BESL) {
        if ((lpm & USBC_SLPSTS) && (lpm & (USBC_BREMOTEWAKE | USBC_L1RESUMEOK)) != USBC_BREMOTEWAKE) {
            lpm_entry = false;
            hal_usb_sleep(USB_SLEEP_L1);
        } else {
            hwtimer_update_then_start(usbdev_timer, hal_usb_lpm_check, NULL, LPM_CHECK_INTERVAL);
            usbdev_timer_active = 1;
        }
    }

    int_unlock(lock);
}
#endif

static void hal_usb_irq_lpm(void)
{
    static const uint32_t lpm_trace_interval = MS_TO_TICKS(2000);
    static uint32_t last_lpm_irq_time;
    static uint32_t cnt = 0;
    uint32_t time;
    uint32_t lpm;

    cnt++;
    lpm = usbc->GLPMCFG;

#ifdef USB_SUSPEND
    if (GET_BITFIELD(lpm, USBC_HIRD) >= USB_L1_DEEP_SLEEP_BESL) {
        // Stat timer to check L1 sleep state
        if (usbdev_timer_active) {
            hwtimer_stop(usbdev_timer);
        }
        hwtimer_update_then_start(usbdev_timer, hal_usb_lpm_check, NULL, LPM_CHECK_INTERVAL);
        usbdev_timer_active = 1;
        lpm_entry = true;
    }
#endif

    time = hal_sys_timer_get();
    if (time - last_lpm_irq_time >= lpm_trace_interval || GET_BITFIELD(lpm, USBC_HIRD) >= USB_L1_DEEP_SLEEP_BESL) {
        last_lpm_irq_time = time;
        USB_TRACE(6,0, "LPM IRQ: 0x%08X rmtWake=%d hird=0x%x l1Res=%d slpSts=%d cnt=%u",
            lpm, !!(lpm & USBC_BREMOTEWAKE), GET_BITFIELD(lpm, USBC_HIRD),
            GET_BITFIELD(lpm, USBC_COREL1RES), !!(lpm & USBC_SLPSTS), cnt);
    }
}
#endif

static void hal_usb_irq_handler(void)
{
    uint32_t status, rawStatus;
    uint32_t statusEp, rawStatusEp;
    uint32_t data;
    uint8_t  i;

    // Store interrupt flag and reset it
    rawStatus = usbc->GINTSTS;
    usbc->GINTSTS = rawStatus;

    status = rawStatus & (usbc->GINTMSK & (USBC_USBRST | USBC_ENUMDONE
#if defined(USB_ISO) && (USB_ISO_INTERVAL == 1)
        | USBC_INCOMPISOOUT | USBC_INCOMPISOIN
#endif
        | USBC_IEPINT | USBC_OEPINT
        | USBC_ERLYSUSP | USBC_USBSUSP | USBC_WKUPINT
        | USBC_LPM_INT));

    USB_TRACE(3,1, "%s: 0x%08x / 0x%08x", __FUNCTION__, status, rawStatus);

#if defined(USB_SUSPEND) && (defined(PMU_USB_PIN_CHECK) || defined(USB_LPM))
    bool stop_timer = true;

#ifdef USB_LPM
    if (lpm_entry && status == USBC_ERLYSUSP && (usbc->DSTS & USBC_ERRTICERR) == 0) {
        // LPM TL1TokenRetry (8 us) timer has expired. L1 state transition will be finished
        // after TL1Residency (50 us) timer from now.
        stop_timer = false;
    } else {
        lpm_entry = false;
    }
#endif

    if (stop_timer) {
        hal_usb_stop_usbdev_timer();
    }
#endif

    if (status & USBC_USBRST) {
        // Usb reset
        hal_usb_irq_reset();

        // We got a reset, and reseted the soft state machine: Discard all other
        // interrupt causes.
        status &= USBC_ENUMDONE;
    }

    if (status & USBC_ENUMDONE) {
        // Enumeration done
        hal_usb_irq_enum_done();
    }

#if defined(USB_ISO) && (USB_ISO_INTERVAL == 1)
    if (status & USBC_INCOMPISOOUT) {
        // Incomplete ISO OUT
        hal_usb_irq_incomp_iso_out();
    }

    if (status & USBC_INCOMPISOIN) {
        // Incomplete ISO IN
        hal_usb_irq_incomp_iso_in();
    }
#endif

    if (status & USBC_IEPINT) {
        // Tx
        data = usbc->DAINT & usbc->DAINTMSK;
        for (i = 0; i < EPNUM; ++i) {
            if (data & USBC_INEPMSK(1 << i)) {
                if (i == 0) {
                    // EP0
                    rawStatusEp = usbc->DIEPINT0;
                    usbc->DIEPINT0 = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DIEPMSK;

                    if ((statusEp & USBC_TIMEOUT) || (statusEp & USBC_INTKNTXFEMP)) {
                        usbc->DIEPMSK &= ~USBC_INTKNTXFEMPMSK;
                        hal_usb_stall_ep(EP_IN, i);
                    } else if (statusEp & USBC_XFERCOMPL) {
                        // Handle ep0 command
                        hal_usb_handle_ep0_packet(EP_IN, rawStatusEp);
                    }
                } else {
                    rawStatusEp = usbc->DIEPnCONFIG[i - 1].DIEPINT;
                    usbc->DIEPnCONFIG[i - 1].DIEPINT = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DIEPMSK;

                    if ((statusEp & USBC_TIMEOUT) || (statusEp & USBC_XFERCOMPL)) {
                        hal_usb_send_epn_complete(i, rawStatusEp);
                    }
                }
            }
        }
    }

    if (status & USBC_OEPINT) {
        // Rx
        data = usbc->DAINT & usbc->DAINTMSK;
        for (i = 0; i < EPNUM; ++i) {
            if (data & USBC_OUTEPMSK(1 << i)) {
                if (i == 0) {
                    rawStatusEp = usbc->DOEPINT0;
                    usbc->DOEPINT0 = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DOEPMSK;

                    if (statusEp & USBC_OUTTKNEPDIS) {
                        usbc->DOEPMSK &= ~USBC_OUTTKNEPDISMSK;
                        hal_usb_stall_ep(EP_OUT, i);
                        if (statusEp & USBC_XFERCOMPL) {
                            // Always enable setup packet receiving
                            hal_usb_recv_ep0();
                        }
                    } else if (statusEp & (USBC_XFERCOMPL | USBC_SETUP)) {
                        // Handle ep0 command
                        hal_usb_handle_ep0_packet(EP_OUT, rawStatusEp);
                    }
                } else {
                    rawStatusEp = usbc->DOEPnCONFIG[i - 1].DOEPINT;
                    usbc->DOEPnCONFIG[i - 1].DOEPINT = rawStatusEp;
                    statusEp = rawStatusEp & usbc->DOEPMSK;

                    if (statusEp & USBC_XFERCOMPL) {
                        hal_usb_recv_epn_complete(i, rawStatusEp);
                    }
                }
            }
        }
    }

#ifdef USB_SUSPEND
    if (status & USBC_ERLYSUSP) {
        if (usbc->DSTS & USBC_ERRTICERR) {
            hal_usb_soft_disconnect();
            return;
        }
    }

    if (status & USBC_USBSUSP) {
        hal_usb_sleep(USB_SLEEP_SUSPEND);
    }

    if (status & USBC_WKUPINT) {
        hal_usb_wakeup();
    }
#endif

#ifdef USB_LPM
    if (status & USBC_LPM_INT) {
        hal_usb_irq_lpm();
    }
#endif
}

#endif // CHIP_HAS_USB
