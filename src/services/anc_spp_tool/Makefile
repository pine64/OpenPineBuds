cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ANC_TEST_LIB_NAME := libanc_spp_tool

$(ANC_TEST_LIB_NAME)-y := $(src_obj)

obj-y += $(ANC_TEST_LIB_NAME).a

ccflags-y += -Iutils/hexdump    \
        -Iapps/key  \
        -Iservices/bt_app   \
        -Iservices/bt_if/inc  \
        -Iplatform/drivers/ana \
		-Iservices/multimedia/audio/process/anc/include \
		-Iservices/audioflinger \
		-Iservices/nv_section/aud_section \
		-Iservices/nv_section/include \
		-Iservices/audio_eq \
		-Iservices/tota \
		-Iplatform/hal \
		-Iplatform/drivers/norflash \
		-Iservices/multimedia/audio/process/filters/include \
		-Iutils/crc32 \
		-Iutils/boot_struct \
		-Iutils/libc/inc \
		-Iutils/hwtimer_list \
		-Iutils/heap \
        -Iservices/tws/inc \
        -Iutils/cqueue \
        -Iapps/audioplayers


ifeq ($(ANC_FF_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FF_ENABLED
endif

ifeq ($(ANC_FB_ENABLED),1)
CFLAGS_anc_usb_app.o += -DANC_FB_ENABLED
endif

