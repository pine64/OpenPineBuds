
// Google Fast pairing Service
#include "common_apps_imports.h"

#ifdef GFPS_ENABLED
#include "app_gfps.h"
#ifdef GFPS_ENABLED
static void app_tell_battery_info_handler(uint8_t *batteryValueCount,
                                          uint8_t *batteryValue) {
  GFPS_BATTERY_STATUS_E status;
  if (app_battery_is_charging()) {
    status = BATTERY_CHARGING;
  } else {
    status = BATTERY_NOT_CHARGING;
  }

  // TODO: add the charger case's battery level
#ifdef IBRT
  if (app_tws_ibrt_tws_link_connected()) {
    *batteryValueCount = 2;
  } else {
    *batteryValueCount = 1;
  }
#else
  *batteryValueCount = 1;
#endif

  TRACE(2, "%s,*batteryValueCount is %d", __func__, *batteryValueCount);
  if (1 == *batteryValueCount) {
    batteryValue[0] = ((app_battery_current_level() + 1) * 10) | (status << 7);
  } else {
    batteryValue[0] = ((app_battery_current_level() + 1) * 10) | (status << 7);
    batteryValue[1] = ((app_battery_current_level() + 1) * 10) | (status << 7);
  }
}
#endif
#endif

#ifdef GFPS_ENABLED
static void app_tell_battery_info_handler(uint8_t *batteryValueCount,
                                          uint8_t *batteryValue) {
  GFPS_BATTERY_STATUS_E status;
  if (app_battery_is_charging()) {
    status = BATTERY_CHARGING;
  } else {
    status = BATTERY_NOT_CHARGING;
  }

  // TODO: add the charger case's battery level
#ifdef IBRT
  if (app_tws_ibrt_tws_link_connected()) {
    *batteryValueCount = 2;
  } else {
    *batteryValueCount = 1;
  }
#else
  *batteryValueCount = 1;
#endif

  TRACE(2, "%s,*batteryValueCount is %d", __func__, *batteryValueCount);
  if (1 == *batteryValueCount) {
    batteryValue[0] = ((app_battery_current_level() + 1) * 10) | (status << 7);
  } else {
    batteryValue[0] = ((app_battery_current_level() + 1) * 10) | (status << 7);
    batteryValue[1] = ((app_battery_current_level() + 1) * 10) | (status << 7);
  }
}
#endif