
#include "led_control.h"
#include "common_apps_imports.h"
#include "ibrt.h"

/******************************LED_status_timer*********************************************************/
osTimerId LED_statusid = NULL;

osTimerDef(defLED_status, LED_statusfun);

void LED_statusinit(void) {
  LED_statusid = osTimerCreate(osTimer(defLED_status), osTimerOnce, (void *)0);
}

void LED_statusfun(const void *) {
  // TRACE("\n\n!!!!!!enter %s\n\n",__func__);
  if ((Curr_Is_Slave() || app_device_bt_is_connected()) &&
      (!app_battery_is_charging())) {
    app_status_indication_set(APP_STATUS_INDICATION_CONNECTED);
  } else if (!app_device_bt_is_connected() && (!app_battery_is_charging())) {
    app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
  } else if (app_battery_is_charging()) {
    app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
  }
  // unsigned char	firstaddr;
  // I2C_ReadByte(decice_firstreg,&firstaddr);
  // TRACE(3,"0X00 REG = 0x%x",firstaddr);
  startLED_status(1000);
}

void startLED_status(int ms) {
  // TRACE("\n\n !!!!!!!!!!start %s\n\n",__func__);
  osTimerStart(LED_statusid, ms);
}
void stopLED_status(void) {
  // TRACE("\n\n!!!!!!!!!!  stop %s\n\n",__func__);
  osTimerStop(LED_statusid);
}

/********************************LED_status_timer*******************************************************/
