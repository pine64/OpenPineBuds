#ifndef COMMON_APP_IMPORTS
#define COMMON_APP_IMPORTS

#include "a2dp_api.h"
#include "app_audio.h"
#include "app_battery.h"
#include "app_ble_include.h"
#include "app_bt.h"
#include "app_bt_func.h"
#include "app_bt_media_manager.h"
#include "app_key.h"
#include "app_overlay.h"
#include "app_pwl.h"
#include "app_status_ind.h"
#include "app_thread.h"
#include "app_utils.h"
#include "audioflinger.h"
#include "besbt.h"
#include "bt_drv_interface.h"
#include "bt_if.h"
#include "btapp.h"
#include "cmsis_os.h"
#include "crash_dump_section.h"
#include "factory_section.h"
#include "gapm_task.h"
#include "hal_bootmode.h"
#include "hal_i2c.h"
#include "hal_sleep.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "list.h"
#include "log_section.h"
#include "me_api.h"
#include "norflash_api.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"
#include "os_api.h"
#include "pmu.h"
#include "stdio.h"
#include "string.h"
#include "tgt_hardware.h"
#ifdef __AI_VOICE__
#include "ai_manager.h"
#include "app_ai_if.h"
#include "app_ai_manager_api.h"
#include "app_ai_tws.h"
#endif
#include "app_tws_ibrt_cmd_handler.h"
#include "audio_process.h"

#ifdef __PC_CMD_UART__
#include "app_cmd.h"
#endif

#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#include "app_factory_bt.h"
#endif

#ifdef __INTERCONNECTION__
#include "app_ble_mode_switch.h"
#include "app_interconnection.h"
#include "app_interconnection_ble.h"
#include "app_interconnection_logic_protocol.h"
#endif

#ifdef __INTERACTION__
#include "app_interaction.h"
#endif

#ifdef BISTO_ENABLED
#include "app_ai_manager_api.h"
#include "gsound_custom_actions.h"
#include "gsound_custom_ota.h"
#include "gsound_custom_reset.h"
#include "nvrecord_gsound.h"
#endif

#ifdef IBRT_OTA
#include "ota_bes.h"
#endif

#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#include "resources.h"
#endif

#ifdef VOICE_DATAPATH
#include "app_voicepath.h"
#endif

#ifdef BT_USB_AUDIO_DUAL_MODE
#include "btusb_audio.h"
#include "usbaudio_thread.h"
#endif

#ifdef TILE_DATAPATH
#include "tile_target_ble.h"
#endif

#if defined(IBRT)
#include "app_ibrt_customif_cmd.h"
#include "app_ibrt_customif_ui.h"
#include "app_ibrt_if.h"
#include "app_ibrt_ui_test.h"
#include "app_ibrt_voice_report.h"
#include "app_tws_if.h"
#endif

#endif