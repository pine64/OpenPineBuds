cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

IBRT_OTA_LIB_NAME := lib_ibrt_ota
ifeq ($(BLE), 1)
IBRT_OTA_LIB_NAME := $(IBRT_OTA_LIB_NAME)_ble
endif

ifeq ($(KERNEL),RTX)
IBRT_OTA_LIB_NAME := $(IBRT_OTA_LIB_NAME)_RTX
endif
ifeq ($(KERNEL),RTX5)
IBRT_OTA_LIB_NAME := $(IBRT_OTA_LIB_NAME)_RTX5
endif

$(IBRT_OTA_LIB_NAME)-y := $(src_obj)

obj-y += $(IBRT_OTA_LIB_NAME).a

subdir-ccflags-y += -Iplatform/drivers/ana \
					-Iplatform/hal \
					-Iservices/bluedroid/adapter/src \
					-Iapps/key \
					-Iservices/audio_eq \
					-Iservices/fs/fat \
					-Iservices/fs/sd \
					-Iservices/fs/fat/ChaN \
					$(BT_IF_INCLUDES) \
					$(BT_PROFILES_INCLUDES) \
					-Iservices/overlay \
					-Iservices/nvrecord \
					-Iservices/nv_section/factory_section \
					-Iservices/resources \
					-Iplatform/drivers/uarthci \
					-Iplatform/drivers/ana \
					-Iplatform/drivers/bt \
					-Iplatform/drivers/main \
					-Iutils/cqueue \
					-Iutils/heap \
					-Iutils/retention_ram \
					-Iservices/audioflinger \
                    -Iservices/app_ai/inc \
					-Iutils/lockcqueue \
					-Iutils/hwtimer_list/ \
					-Iutils/intersyshci \
					-Iutils/boot_struct \
					-Iapps/main \
					-Iapps/common \
					-Iapps/audioplayers \
					-Iapps/factory \
					-Iapps/ota \
					-Iservices/ble_app \
					-Iservices/ble_app/app_ota \
					-Iservices/ble_stack/ble_ip \
					-Iservices/ble_stack/hl/api \
					-Iservices/ble_stack/app/api/  \
					-Iservices/ble_stack/common/api/ \
					-Iservices/ble_stack/hl/inc/ \
					-Iservices/ble_stack/ke/api \
					-Iservices/bridge/ \
					-Iservices/ble_stack/hl/src/gap/ \
					-Iservices/ble_stack/hl/src/gap/gapc/ \
					-Iservices/ble_stack/hl/src/gap/gapm/ \
					-Iservices/ble_stack/hl/src/gap/smpc/ \
					-Iservices/ble_stack/ke/src/ \
					-Iservices/ble_stack/hl/src/gatt/attc/ \
					-Iservices/ble_stack/hl/src/gatt/attm/ \
					-Iservices/ble_stack/hl/src/gatt/atts/ \
					-Iservices/ble_stack/hl/src/gatt/gattc/ \
					-Iservices/ble_stack/hl/src/gatt/gattm/ \
					-Iservices/ble_stack/hl/src/gatt/ \
					-Iservices/ble_stack/hl/src/l2c/l2cc/ \
					-Iservices/ble_stack/hl/src/l2c/l2cm/ \
					-Iservices/ble_stack/hci/api/ \
					-Iservices/ble_stack/hci/src/ \
					-Iservices/ble_stack/app/src/  \
					-Iservices/ble_profiles/htp/htpt/api/  \
					-Iservices/ble_profiles/htp/htpt/src/  \
					-Iservices/ble_profiles/htp/	\
					-Iservices/ble_profiles/datapath/datapathps/api/  \
					-Iservices/ble_app/app_main/ \
					-Iservices/ble_app/app_htp/	\
					-Iservices/ble_app/app_datapath/	\
					-Iservices/bt_app \
					-Iservices/bt_app/a2dp_codecs/include \
					-Iservices/ble_app/app_vob \
					-Iservices/multimedia/speech/codec/codecs/adpcm \
					-Iservices/ble_app/app_smartvoice \
					-Iservices/ble_profiles/smartvoice \
					-Iservices/anc_spp_tool \
					-Iutils/crc32 \
					-Ithirdparty/userapi \
					-Iservices/multimedia/audio/codec/sbc/inc \
					-Iservices/multimedia/audio/codec/sbc/src/inc \
					-Iservices/norflash_api

ifeq ($(OTA_ENABLE),1)
subdir-ccflags-y += \
	-Iservices/ibrt_ota/inc
endif

ifeq ($(IBRT),1)
subdir-ccflags-y += \
	-Iservices/ibrt_ui/inc \
	-Iservices/ibrt_core/inc \
	-Iservices/app_ibrt/inc \
	-Iservices/bt_app
endif
		
ifneq ($(OTA_CODE_OFFSET),0)
CFLAGS_ota_control.o += -DOTA_CODE_OFFSET=$(OTA_CODE_OFFSET)
endif			
