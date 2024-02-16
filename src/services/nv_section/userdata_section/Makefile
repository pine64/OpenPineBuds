
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

all_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
all_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

filtout_c :=
filtout_cpp :=

ifneq ($(BISTO_ENABLE),1)
filtout_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*gsound.c))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*gsound.cpp))
endif

ifneq ($(OTA_ENABLE), 1)
filtout_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*ota.c))
filtout_cpp += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*ota.cpp))
endif

obj_c := $(filter-out $(filtout_c),$(all_c))
obj_cpp := $(filter-out $(filtout_cpp),$(all_cpp))

obj-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

ccflags-y := \
    -I$(obj)/../../nv_section/userdata_section \
    -I$(obj)/../../nv_section/include \
    -I$(obj)/../platform/cmsis/inc \
    -Iutils/crc32 \
    $(BT_IF_INCLUDES) \
    -Iutils/hexdump \
    -Iapps/key \
    -Iservices/bt_app \
    -Iplatform/drivers/ana \
    -Iservices/nv_section/customparam_section \
    -Iservices/nv_section/factory_section \
    -Iservices/ble_stack/common/api \
    -Iservices/ble_stack/ble_ip \
    -Iutils/heap \
    -Iservices/nvrecord \
    -Iservices/norflash_api \
    -Iservices/nv_setion/userdata_section \
    -Iservices/voicepath/gsound/gsound_target_api_read_only \
    -Iservices/voicepath/gsound/gsound_custom/inc \
    -Iplatform/drivers/norflash \
    -Iplatform/hal \
    -Iplatform/cmsis/inc \
    -Iservices/osif/ \
    -Iservices/ota \
    -Iutils/cqueue \
    -Iservices/nv_section \
    -Iutils/aes_cmac

ccflags-y += -DAUDIO_OUTPUT_VOLUME_DEFAULT=$(AUDIO_OUTPUT_VOLUME_DEFAULT)

ifeq ($(FLASH_SUSPEND),1)
ccflags-y += -DFLASH_SUSPEND
endif

ifeq ($(TX_IQ_CAL),1)
subdir-ccflags-y += -DTX_IQ_CAL
endif

