cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

src_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
src_obj := $(src_obj:.c=.o)
src_obj := $(src_obj:.cpp=.o)
src_obj := $(src_obj:.S=.o)

libusbhost-y := $(src_obj)

obj-y := libusbhost.a

ccflags-y := -Iplatform-Iplatform/drivers/usb/usb_host/inc -Iplatform/drivers/ana

ifeq ($(USB_WAKEUP_OPEN),1)
CFLAGS_USBHost.o += -DUSB_WAKEUP_OPEN
endif

ifeq ($(USB_WAKEUP_RESET),1)
CFLAGS_USBHost.o += -DUSB_WAKEUP_RESET
endif
