cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))
auth_type := -DLICE_KEY_TYPE_KEYPRO
# -DHAVE_TEST_KEY

ifeq ($(A2DP_LHDC_LARC), 1)
auth_type += -DHAVE_LARC
endif

ifneq ($(GEN_LIB), 1)
auth_type += -DHAVE_TEST_KEY
endif

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.S))
obj_s += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)dec_src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c))
obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.cpp))

ifeq ($(A2DP_DECODER_VER),2)
ifeq ($(A2DP_LHDC_V3),1)
LHDC_LIB_NAME := BEST2300P_LibLHDC_V2_V3_3_1_0_SAVI_KEYPRO_UUID
else
LHDC_LIB_NAME := BEST2300P_LibLHDC_V2_V3_3_1_0_SAVI_KEYPRO_UUID
endif
else
LHDC_LIB_NAME := liblhdc-dec
endif

$(LHDC_LIB_NAME)-y := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

obj-y += $(LHDC_LIB_NAME).a

#CFLAGS_dec.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_cirbuf.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_lhdcUtil.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_encdec.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_lic_check.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
#CFLAGS_licdecrypt.o += -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant
ccflags-y +=  -lc -O3 -Wchar-subscripts -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant $(DEF)

subdir-ccflags-y += \
    -Iservices/tws/inc \
    -Iutils/heap \
    -Iutils/cqueue \
    -Iapps/audioplayers \
    -Iservices/bt_app \
    -Iplatform \
    -Ithirdparty/audio_codec_lib/liblhdc-dec/dec_src \
    -Ithirdparty/audio_codec_lib/liblhdc-dec \
    -Iservices/ble_stack/ke/api \
    -Iservices/ble_stack/ble_ip \
    $(BT_PROFILES_INCLUDES) \
    -Iservices/nvrecord \
    -Iservices/bt_if/inc \
    -Iconfig \
    -Iservices/ble_stack/hl/api
