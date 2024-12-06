cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

src_obj := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)a2dp/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)a2dp/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)avrcp/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)avrcp/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)conmgr/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)conmgr/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hci/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hci/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)l2cap/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)l2cap/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)me/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)me/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)sdp/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)sdp/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)spp/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)spp/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hfp/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hfp/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)os/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)os/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rfcomm/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)rfcomm/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)sync/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)sync/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)besaud/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)besaud/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dip/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dip/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)map/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)map/*.cpp))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hid/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hid/*.cpp))

ifeq ($(GATT_OVER_BR_EDR),1)
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)btgatt/*.c))
src_obj += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)btgatt/*.cpp))
endif

obj_s := $(filter %.s, $(src_obj))
obj_c := $(filter %.c, $(src_obj))
obj_cpp := $(filter %.cpp, $(src_obj))

ifeq ($(IBRT), 1)
BTAPI_LIB_NAME := ibrt_libbt_api
else
BTAPI_LIB_NAME := $(CHIP)_libbt_api
endif

ifeq ($(HFP_1_6_ENABLE), 1)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_sbc_enc
endif

ifeq ($(SBC_FUNC_IN_ROM), 1)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_sbc_in_rom
endif

ifeq ($(BT_RF_PREFER), 2M)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_2m
endif

ifeq ($(BT_RF_PREFER), 3M)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_3m
endif

ifeq ($(BT_ONE_BRING_TWO), 1)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_1b2
endif

ifeq ($(BLE), 1)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_ble
endif

ifeq ($(KERNEL),RTX)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_RTX
endif
ifeq ($(KERNEL),RTX5)
BTAPI_LIB_NAME := $(BTAPI_LIB_NAME)_RTX5
endif

$(BTAPI_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y += $(BTAPI_LIB_NAME).a

ccflags-y += \
    $(BT_IF_INCLUDES) \
    -Iservices/bt_if_enhanced/conmgr \
    $(BT_PROFILES_INCLUDES) \
    -Iservices/app_ai/inc \
    -Iservices/audio_process \
    -Iservices/overlay \
    -Iservices/nvrecord \
    -Iutils/cqueue \
    -Iutils/heap \
    -Iservices/audioflinger \
    -Iutils/lockcqueue \
    -Iservices/ble_app \
    -Iservices/ble_stack/common/api/ \
    -Iservices/bt_app \
    -Iservices/resources/ \
    -Iservices/osif \
    -Iservices/auto_test/ \
    -Iapps/audioplayers \
    -Iapps/main/ \
    -Iapps/key/ \
    -Iplatform/drivers/ana/ \
    -Iplatform/drivers/bt/ \
    -Iutils/intersyshci \
    -Iservices/ibrt/inc \
    -Iservices/multimedia/audio/codec/sbc/inc \
    -Iservices/multimedia/audio/codec/sbc/src/inc \
    -Iservices/bt_app/a2dp_codecs/include

ifeq ($(GATT_OVER_BR_EDR),1)
ccflags-y += \
    -Iservices/ble_stack/hl/src/l2c/l2cm	\
    -Iservices/ble_stack/ble_ip	\
    -Iservices/ble_stack/hl/api	\
    -Iservices/ble_stack/hl/inc	\
    -Iservices/ble_stack/ke/api
endif

CFLAGS_bt_if.o += -DBESLIB_INFO=$(BESLIB_INFO)

ifeq ($(ANC_APP),1)
CFLAGS_besaud_api.o += -DANC_APP
endif

