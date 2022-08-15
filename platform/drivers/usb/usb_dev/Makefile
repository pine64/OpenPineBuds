cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

src_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c $(cur_dir)src/*.S))
src_obj := $(src_obj:.c=.o)
src_obj := $(src_obj:.S=.o)

cfg_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)cfg/*.c $(cur_dir)cfg/*.S))
cfg_obj := $(cfg_obj:.c=.o)
cfg_obj := $(cfg_obj:.S=.o)

USB_DEV_LIB_NAME := libusbdev

ccflags-y += -Iplatform/drivers/usb/usb_dev/inc

ifeq ($(USB_HIGH_SPEED),1)
ccflags-y += -DUSB_HIGH_SPEED
endif

CFLAGS_usb_cdc.o += -Itests/programmer/inc

CFLAGS_usb_audio.o += -Iutils/hwtimer_list

CFLAGS_usb_audio_sync.o += -Iplatform/drivers/ana

USB_AUDIO_NO_START_ERR ?= 1
ifeq ($(USB_AUDIO_NO_START_ERR),1)
CFLAGS_usb_audio.o += -DUSB_AUDIO_NO_START_ERR
endif

ifneq ($(USYNC_TRACE_VERBOSE),)
CFLAGS_usb_audio_sync.o += -DUSYNC_TRACE_VERBOSE=$(USYNC_TRACE_VERBOSE)
endif

ifneq ($(USB_CDC_VENDOR_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_CDC_VENDOR_ID=$(USB_CDC_VENDOR_ID)
endif

ifneq ($(USB_CDC_PRODUCT_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_CDC_PRODUCT_ID=$(USB_CDC_PRODUCT_ID)
endif

ifneq ($(USB_AUDIO_VENDOR_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_AUDIO_VENDOR_ID=$(USB_AUDIO_VENDOR_ID)
endif

ifneq ($(USB_AUDIO_PRODUCT_ID),)
CFLAGS_usb_dev_desc.o += -DUSB_AUDIO_PRODUCT_ID=$(USB_AUDIO_PRODUCT_ID)
endif

ifeq ($(USB_LPM),1)
CFLAGS_usb_dev_desc.o += -DUSB_LPM
ifeq ($(USB_LPM_DEEP_BESL),1)
CFLAGS_usb_dev_desc.o += -DUSB_LPM_DEEP_BESL
endif
endif

include $(cur_dir)uaud_cfg_flags.mk

$(cur_dir)uaud_cfg_flags.mk: ;

ifeq ($(USB_AUDIO_DYN_CFG),1)

USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_dyn

else # USB_AUDIO_DYN_CFG != 1

ifeq ($(USB_AUDIO_384K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_384k
else ifeq ($(USB_AUDIO_352_8K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_352p8k
else ifeq ($(USB_AUDIO_192K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_192k
else ifeq ($(USB_AUDIO_176_4K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_176p4k
else ifeq ($(USB_AUDIO_96K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_96k
else ifeq ($(USB_AUDIO_44_1K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_44p1k
else ifeq ($(USB_AUDIO_16K),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_16k
endif

ifeq ($(USB_AUDIO_32BIT),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_32b
else
ifeq ($(USB_AUDIO_24BIT),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_24b
endif
endif

endif # USB_AUDIO_DYN_CFG != 1

ifneq ($(USB_AUDIO_SEND_CHAN),)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_s$(USB_AUDIO_SEND_CHAN)
endif

ifeq ($(USB_AUDIO_UAC2),1)
USB_DEV_LIB_NAME := $(USB_DEV_LIB_NAME)_uac2
endif

CFLAGS_usb_audio.o += $(UAUD_CFG_FLAGS)
CFLAGS_usb_dev_desc.o += $(UAUD_CFG_FLAGS)

ifeq ($(SIMU_UAUD_MAX_PKT),1)
CFLAGS_usb_audio.o += -DSIMU_UAUD_MAX_PKT
endif

ifeq ($(ANDROID_ACCESSORY_SPEC),1)
CFLAGS_usb_audio.o += -DANDROID_ACCESSORY_SPEC
ifeq ($(STRICT_ANDROID_ACCESSORY_SPEC),1)
CFLAGS_usb_audio.o += -DSTRICT_ANDROID_ACCESSORY_SPEC
ifeq ($(ANDROID_VOICE_CMD_KEY),1)
CFLAGS_usb_audio.o += -DANDROID_VOICE_CMD_KEY
endif
endif
endif

ifneq ($(USB_AUDIO_RECV_ENABLE),0)
CFLAGS_usb_audio.o += -DUSB_AUDIO_RECV_ENABLE
CFLAGS_usb_audio_sync.o += -DUSB_AUDIO_RECV_ENABLE
endif
ifneq ($(USB_AUDIO_SEND_ENABLE),0)
CFLAGS_usb_audio.o += -DUSB_AUDIO_SEND_ENABLE
CFLAGS_usb_audio_sync.o += -DUSB_AUDIO_SEND_ENABLE
endif

$(USB_DEV_LIB_NAME)-y := $(src_obj)

obj-y := $(USB_DEV_LIB_NAME).a $(cfg_obj)

