cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

#obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y :=

ifeq ($(ROM_BUILD),1)
obj-y += ../../tests/rom/startup_ARMCM.S
else
obj-y += ../../platform/main/startup_main.S
endif

obj-y += main.c
ifeq ($(ANC_APP),1)
obj-y += anc_usb_app.c
endif
obj-y += usb_audio_app.c
obj-y += dualadc_audio_app.c
obj-y += adda_loop_app.c
obj-y += safe_queue.c
obj-y += memutils.c
ifeq ($(MIC_KEY),1)
obj-y += mic_key.c
endif
ifeq ($(VENDOR_MSG_SUPPT),1)
obj-y += usb_vendor_msg.c
endif

obj-y += ../../services/audio_process/
obj-y += ../../services/multimedia/
obj-y += ../../services/nv_section/
obj-y += ../../utils/crc32/

ifeq ($(USB_AUDIO_SPEECH),1)
obj-y += speech_process.c
obj-y += ../../apps/audioplayers/bt_sco_chain.c
obj-y += ../../apps/audioplayers/bt_sco_chain_cfg_default.c
obj-y += ../../apps/audioplayers/bt_sco_chain_tuning.cpp
obj-y += ../../services/audio_dump/
obj-y += ../../utils/heap/

ccflags-y += \
	-Iapps/audioplayers \
	-Iapps/common
endif

obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

ifeq ($(INTSRAM_RUN),1)
asflags-y += -DINTSRAM_RUN
ccflags-y += -DINTSRAM_RUN
endif

ccflags-y +=  \
	-Iutils/boot_struct \
	-Iutils/crc32 \
	-Iutils/heap \
	-Iutils/hexdump \
	-Iutils/hwtimer_list \
	-Iplatform/drivers/usb/usb_dev/inc	\
	-Iplatform/drivers/ana \
	-Iservices/multimedia/audio/process/adp/include \
	-Iservices/multimedia/audio/process/anc/include \
	-Iservices/multimedia/audio/process/filters/include \
	-Iservices/multimedia/audio/process/resample/include \
	-Iservices/audio_dump/include \
	-Iservices/multimedia/speech/inc \
	-Iservices/audio_process \
	-Iservices/nv_section/aud_section \
	-Iservices/nv_section/include

CFLAGS_usb_audio_app.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)
CFLAGS_adda_loop_app.o += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

ifneq ($(DEBUG_PORT),)
CFLAGS_main.o += -DDEBUG_PORT=$(DEBUG_PORT)
endif

ifeq ($(USB_AUDIO_SPEECH),1)
CFLAGS_main.o += -DUSB_AUDIO_SPEECH
endif

ANC_USB_CFG_FLAGS :=

# ANC option
ifeq ($(ANC_APP),1)
ANC_USB_CFG_FLAGS += -DANC_APP
endif

# USB audio option
ifeq ($(USB_AUDIO_APP),1)
ANC_USB_CFG_FLAGS += -DUSB_AUDIO_APP
endif

ifeq ($(USB_I2S_APP),1)
ANC_USB_CFG_FLAGS += -DUSB_I2S_APP
ifneq ($(USB_I2S_ID),)
ANC_USB_CFG_FLAGS += -DUSB_I2S_ID=$(USB_I2S_ID)
endif
endif

ifeq ($(USB_HIGH_SPEED),1)
ANC_USB_CFG_FLAGS += -DUSB_HIGH_SPEED
endif

ifeq ($(AUDIO_RESAMPLE),1)
ANC_USB_CFG_FLAGS += -D__AUDIO_RESAMPLE__
endif

ifeq ($(ADC_CH_SEP_BUFF),1)
ANC_USB_CFG_FLAGS += -DADC_CH_SEP_BUFF
endif

include platform/drivers/usb/usb_dev/uaud_cfg_flags.mk

platform/drivers/usb/usb_dev/uaud_cfg_flags.mk: ;

ANC_USB_CFG_FLAGS += $(UAUD_CFG_FLAGS)

# USB audio configuration
ifeq ($(USB_AUDIO_DYN_CFG),1)
ifneq ($(USB_AUDIO_RECV_ENABLE),0)
ifneq ($(AUDIO_RESAMPLE),1)
SW_CAPTURE_RESAMPLE ?= 1
endif
endif
endif

ifeq ($(AUDIO_PLAYBACK_24BIT),1)
ANC_USB_CFG_FLAGS += -DAUDIO_PLAYBACK_24BIT
endif

# DSD configuration
ifeq ($(HW_FIR_DSD_PROCESS),1)
ifeq ($(HW_FIR_DSD_BUF_MID_ADDR),)
$(error HW_FIR_DSD_BUF_MID_ADDR must be defined with HW_FIR_DSD_PROCESS)
endif
ANC_USB_CFG_FLAGS += -D__HW_FIR_DSD_PROCESS__ -DHW_FIR_DSD_BUF_MID_ADDR=$(HW_FIR_DSD_BUF_MID_ADDR)
endif

ifeq ($(CODEC_DSD),1)
ANC_USB_CFG_FLAGS += -DCODEC_DSD
endif

# EQ configuration
ifeq ($(HW_FIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_FIR_EQ_PROCESS__
endif

ifeq ($(HW_IIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_IIR_EQ_PROCESS__
endif

ifeq ($(SW_IIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__SW_IIR_EQ_PROCESS__
endif

ifeq ($(HW_DAC_IIR_EQ_PROCESS),1)
ANC_USB_CFG_FLAGS += -D__HW_DAC_IIR_EQ_PROCESS__
endif

ifeq ($(AUDIO_RESAMPLE),1)
ifeq ($(SW_PLAYBACK_RESAMPLE),1)
ANC_USB_CFG_FLAGS += -DSW_PLAYBACK_RESAMPLE
endif
endif

ifeq ($(SW_CAPTURE_RESAMPLE),1)
ANC_USB_CFG_FLAGS += -DSW_CAPTURE_RESAMPLE
endif

CFLAGS_main.o += $(ANC_USB_CFG_FLAGS)
CFLAGS_usb_audio_app.o += $(ANC_USB_CFG_FLAGS)
CFLAGS_anc_usb_app.o += $(ANC_USB_CFG_FLAGS)

ifeq ($(ANC_KEY_DOUBLE_CLICK_ON_OFF),1)
CFLAGS_anc_usb_app.o += -DANC_KEY_DOUBLE_CLICK_ON_OFF
endif

ifeq ($(ANC_FF_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FF_ENABLED
endif

ifeq ($(ANC_FB_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FB_ENABLED
endif

ifeq ($(AUDIO_SECTION_SUPPT),1)
CFLAGS_anc_usb_app.o += -D__AUDIO_SECTION_SUPPT__
endif

ifeq ($(ANC_INIT_OFF),1)
CFLAGS_anc_usb_app.o += -DANC_INIT_OFF
endif

ifeq ($(ADDA_LOOP_APP),1)
CFLAGS_main.o += -DADDA_LOOP_APP
endif

ifeq ($(PC_CMD_UART),1)
CFLAGS_main.o += -D__PC_CMD_UART__
endif

ifeq ($(JTAG_ENABLE),1)
CFLAGS_main.o += -DJTAG_ENABLE
endif

ifeq ($(DEBUG_MODE_USB_DOWNLOAD),1)
CFLAGS_main.o += -DDEBUG_MODE_USB_DOWNLOAD
endif

ifeq ($(DELAY_STREAM_OPEN),1)
CFLAGS_usb_audio_app.o += -DDELAY_STREAM_OPEN
endif

ifeq ($(NOISE_GATING),1)
CFLAGS_usb_audio_app.o += -DNOISE_GATING
endif

ifeq ($(NOISE_REDUCTION),1)
CFLAGS_usb_audio_app.o += -DNOISE_REDUCTION
endif

ifeq ($(ANC_L_R_MISALIGN_WORKAROUND),1)
CFLAGS_usb_audio_app.o += -DANC_L_R_MISALIGN_WORKAROUND
endif

ifeq ($(ANDROID_ACCESSORY_SPEC),1)
CFLAGS_usb_audio_app.o += -DANDROID_ACCESSORY_SPEC
ifeq ($(ANDROID_VOICE_CMD_KEY),1)
CFLAGS_usb_audio_app.o += -DANDROID_VOICE_CMD_KEY
endif
endif

ifeq ($(DUAL_AUX_MIC_MORE_FILTER),1)
CFLAGS_usb_audio_app.o += -DDUAL_AUX_MIC_MORE_FILTER
endif

ifeq ($(FREQ_RESP_EQ),1)
CFLAGS_usb_audio_app.o += -DFREQ_RESP_EQ
endif

ifeq ($(KEEP_SAME_LATENCY),1)
CFLAGS_usb_audio_app.o += -DKEEP_SAME_LATENCY
CFLAGS_speech_process.o += -DKEEP_SAME_LATENCY
endif

ifeq ($(USB_AUDIO_PWRKEY_TEST),1)
CFLAGS_usb_audio_app.o += -DUSB_AUDIO_PWRKEY_TEST
endif

ifeq ($(AUDIO_RESAMPLE),1)
# If neither best1000 nor best2000
ifeq ($(filter best1000 best2000,$(CHIP)),)
PLL_TUNE_SAMPLE_RATE ?= 1
endif
ifeq ($(PLL_TUNE_SAMPLE_RATE),1)
CFLAGS_usb_audio_app.o += -DPLL_TUNE_SAMPLE_RATE
endif
ifeq ($(PLL_TUNE_XTAL),1)
CFLAGS_usb_audio_app.o += -DPLL_TUNE_XTAL
endif
endif

ifeq ($(TARGET_TO_MAX_DIFF),1)
CFLAGS_usb_audio_app.o += -DTARGET_TO_MAX_DIFF
endif

ifneq ($(USB_AUDIO_RECV_ENABLE),0)
CFLAGS_usb_audio_app.o += -DUSB_AUDIO_RECV_ENABLE
endif
ifneq ($(USB_AUDIO_SEND_ENABLE),0)
CFLAGS_usb_audio_app.o += -DUSB_AUDIO_SEND_ENABLE
endif

ifeq ($(USB_EQ_TUNING), 1)
CFLAGS_main.o += -DUSB_EQ_TUNING
CFLAGS_usb_vendor_msg.o += -DUSB_EQ_TUNING
endif

ifeq ($(VENDOR_MSG_SUPPT), 1)
CFLAGS_main.o += -D_VENDOR_MSG_SUPPT_
CFLAGS_usb_vendor_msg.o += -D_VENDOR_MSG_SUPPT_
CFLAGS_usb_audio_app.o += -D_VENDOR_MSG_SUPPT_
endif

ifeq ($(CHIP_HAS_DCO), 1)
CFLAGS_main.o += -DCHIP_HAS_DCO
CFLAGS_usb_vendor_msg.o += -DCHIP_HAS_DCO
CFLAGS_usb_audio_app.o += -DCHIP_HAS_DCO
endif