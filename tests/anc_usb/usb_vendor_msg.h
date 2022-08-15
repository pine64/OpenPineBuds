#ifndef _USB_VENDOR_MSG_H_
#define _USB_VENDOR_MSG_H_

#include "usb_audio.h"

#ifdef USB_ANC_MC_EQ_TUNING    
#define VENDOR_RX_BUF_SZ        5000
#else
#define VENDOR_RX_BUF_SZ        240
#endif
extern uint8_t vendor_msg_rx_buf[VENDOR_RX_BUF_SZ];

int usb_vendor_callback (struct USB_AUDIO_VENDOR_MSG_T *msg);

void vendor_info_init (void);

#endif // _USB_VENDOR_MSG_H_
