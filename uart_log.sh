#!/usr/bin/env sh

rightbud=/dev/serial/by-id/usb-wch.cn_USB_Dual_Serial_0123456789-if00
leftbud=/dev/serial/by-id/usb-wch.cn_USB_Dual_Serial_0123456789-if02

read -p "Which bud do you want to connect to UART for? L/R (default L): " -n 1 -r
ttydev=$leftbud
if [[ $REPLY =~ ^[Rr]$ ]]; then
    ttydev=$rightbud
fi
sudo minicom -D $ttydev -b 2000000
