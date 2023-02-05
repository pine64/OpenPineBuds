#include "ibrt.h"
#include "common_apps_imports.h"
#include "hal_gpio.h"
#include "tgt_hardware.h"
extern struct BT_DEVICE_T app_bt_device;
extern void hal_gpio_pin_set(enum HAL_GPIO_PIN_T pin);

bool Curr_Is_Master(void) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->current_role == IBRT_MASTER)
    return 1;
  else
    return 0;
}

bool Curr_Is_Slave(void) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  if (p_ibrt_ctrl->current_role == IBRT_SLAVE)
    return 1;
  else
    return 0;
}

uint8_t get_curr_role(void) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  return p_ibrt_ctrl->current_role;
}

uint8_t get_nv_role(void) {
  static ibrt_ctrl_t *p_ibrt_ctrl = app_tws_ibrt_get_bt_ctrl_ctx();
  return p_ibrt_ctrl->nv_role;
}
