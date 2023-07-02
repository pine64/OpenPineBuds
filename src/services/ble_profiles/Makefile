
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)datapath/datapathps/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hrp/hrps/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hogp/hogpbh/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hogp/hogpd/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)hogp/hogrh/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)bas/bass/src/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)prf/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)voicepath/$(VOICE_DATAPATH_TYPE)/*.c))
ifeq ($(TILE_DATAPATH_ENABLED),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)/tile/*.c))
endif
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ota/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)anc/ancc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)ams/amsc/*.c))
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gfps/gfps_provider/src/*.c))
#obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)gfps/gfps_crypto/*.c))

ifeq ($(TEST_OVER_THE_AIR),1)
obj_c += $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)tota/*.c))
subdir-ccflags-y +=-Iservices/tota
endif
obj-y := $(obj_c:.c=.o)
ifeq ($(GFPS_ENABLE),1)
obj-y += gfps/
endif
subdir-ccflags-y += \
	-Iservices/audio_process \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	$(BT_IF_INCLUDES) \
	$(BT_PROFILES_INCLUDES) \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/retention_ram \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/factory \
	-Iservices/ble_app \
	-Iservices/ble_stack/ble_ip \
	-Iservices/ble_stack/hl/api \
	-Iservices/ble_stack/app/api/  \
	-Iservices/ble_stack/common/api/ \
	-Iservices/ble_stack/ke/api \
	-Iservices/bridge/ \
	-Iservices/ble_stack/hl/inc/ \
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
	-Iservices/ble_stack/profiles/htp/htpt/api/ \
	-Iservices/ble_stack/profiles/htp/htpt/src/ \
	-Iservices/ble_stack/profiles/htp/ \
	-Iservices/ble_profiles/datapath/datapathps/api/ \
	-Iservices/ble_app/app_main/ \
	-Iservices/ble_app/app_voice \
	-Iservices/ble_app/app_htp/ \
	-Iservices/ble_app/app_datapath/ \
	-Iservices/ble_profiles/hrp/hrps/src/ \
	-Iservices/ble_profiles/hrp/hrps/api \
	-Iservices/ble_profiles/hrp/ \
	-Iservices/ble_app/app_hrps/ \
	-Iservices/ble_app/app_amsc/ \
	-Iservices/ble_app/app_ancc/ \
	-Iservices/ble_profiles/hogp/ \
	-Iservices/ble_profiles/hogp/hogpd/api/ \
	-Iservices/ble_profiles/bas/bass/src/ \
	-Iservices/ble_profiles/bas/bass/api/ \
	-Iservices/ble_profiles/anc/ \
	-Iservices/ble_profiles/anc/ancc/ \
	-Iservices/ble_profiles/ams/ \
	-Iservices/ble_profiles/ams/amsc/ \
	-Iservices/ble_profiles/voicepath/gsound \
	-Iservices/voicepath/$(VOICE_DATAPATH_TYPE)/ \
	-Iservices/voicepath/gsound/gsound_target/ \
	-Iservices/voicepath/gsound/gsound_custom/inc \
	-Iservices/voicepath/gsound/gsound_target_api_read_only \
	-Iservices/ble_profiles/gfps/gfps_provider/src/ \
	-Iservices/ble_profiles/gfps/gfps_provider/api/ \
	-Iservices/ble_profiles/gfps/gfps_crypto/ \
	-Iservices/ble_profiles/gfps/api/ \
	-Iservices/ai_voice/manager \
    -Iservices/app_ai/inc \
	-Iservices/ble_profiles/tile \
	-Ithirdparty/tile/tile_target/ \
	-Ithirdparty/tile/tile_common/tile_assert \
	-Ithirdparty/tile/tile_common/tile_features \
	-Ithirdparty/tile/tile_common/tile_gatt_db \
	-Ithirdparty/tile/tile_common/tile_player \
	-Ithirdparty/tile/tile_common/tile_service \
	-Ithirdparty/tile/tile_common/tile_storage \
	-Iservices/bt_app
	
ifeq ($(OTA_ENABLE),1)
ifeq ($(IBRT),1)
ccflags-y += -Iservices/ibrt_ota/inc
else
ccflags-y += -Iservices/ota
endif
endif

