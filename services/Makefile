obj-y := audio_dump/ \
         audioflinger/ \
         audio_process/ \
         hw_dsp/ \
         bt_app/ \
         overlay/ \
         resources/ \
         ../utils/crc32/ \
         ../utils/crc16/ \
         ../utils/heap/ \
         osif/ \
         norflash_api/ \

ifeq ($(BONE_SENSOR_TDM),1)
obj-y += bone_sensor/
endif

ifneq ($(INTERCONNECTION)_$(AI_VOICE),0_0)
obj-y += interconnection/
endif

ifeq ($(ENHANCED_STACK),1)
obj-y += bt_profiles_enhanced/
obj-y += bt_if_enhanced/
else
obj-y += bt_profiles/
obj-y += bt_if/
endif

ifeq ($(MBED),1)
obj-y += fs/
endif

ifneq ($(MBED)_$(AI_VOICE),0_0)
obj-y +=../utils/kfifo/
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
obj-y += voicepath/
obj-y += voicepath/$(VOICE_DATAPATH_TYPE)/
endif

ifneq ($(VOICE_DATAPATH_ENABLED)_$(AI_VOICE),0_0)
obj-y += app_ai/
endif

ifeq ($(BLE),1)
obj-y += ble_app/ \
         bridge/ \
         ble_stack/ \
         ../utils/retention_ram/ \
         ble_profiles/
endif

ifeq ($(AUTO_TEST),1)
obj-y += auto_test/
endif

obj-y += nv_section/

obj-y += nvrecord/

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(APP_ANC_TEST),1)
obj-y += anc_spp_tool/
endif

ifeq ($(TEST_OVER_THE_AIR),1)
obj-y += tota/
endif

ifeq ($(OTA_ENABLE),1)
ifeq ($(IBRT),1)
obj-y += ibrt_ota/
else
endif
ifeq ($(OTA_ENABLE), 1)
obj-y += ota/
endif
endif

ifeq ($(TILE_DATAPATH_ENABLED),1)
obj-y += ../thirdparty/tile/
endif

ifeq ($(AI_VOICE),1)
obj-y += ai_voice/
endif

ifeq ($(CHIP_HAS_CP),1)
obj-y += cp_accel/
endif

ifeq ($(IBRT),1)
obj-y += ibrt_core/
obj-y += app_ibrt/
obj-y += ibrt_ui/
endif

ifeq ($(TWS_SYSTEM_ENABLED),1)
obj-y += app_tws/
endif

ifeq ($(RSA_SHA),1)
obj-y +=../utils/sha256/ ../utils/rsa2048/ 
endif

ifeq ($(AES256_FOR_BIN),1)
obj-y +=../utils/aes256/
endif

obj-y += communication/

ifeq ($(RPC_SUPPORT),1)
obj-y += rpc/
endif

ifeq ($(A2DP_LHDC_ON),1)
obj-y += lhdc_license/
endif

ifeq ($(THROUGH_PUT),1)
obj-y += through_put/
endif
