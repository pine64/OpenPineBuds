

#ifdef RB_CODEC
extern bool app_rbcodec_check_hfp_active(void);
void app_switch_player_key(APP_KEY_STATUS *status, void *param) {
  TRACE(3, "%s %d,%d", __func__, status->code, status->event);

  if (!rb_ctl_is_init_done()) {
    TRACE(0, "rb ctl not init done");
    return;
  }

  if (app_rbcodec_check_hfp_active()) {
    app_bt_key(status, param);
    return;
  }

  app_rbplay_audio_reset_pause_status();

  if (app_rbplay_mode_switch()) {
    app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
    app_rbcodec_ctr_play_onoff(true);
  } else {
    app_rbcodec_ctr_play_onoff(false);
    app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
  }
  return;
}
#endif
