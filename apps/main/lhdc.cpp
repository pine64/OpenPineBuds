// LHDC needs a licence key
// We dont have one; so probably cant ever officially use this. But its here for
// preservation at the least

#if defined(A2DP_LHDC_ON)
extern "C" {
typedef struct bes_bt_local_info_t {
  uint8_t bt_addr[BTIF_BD_ADDR_SIZE];
  const char *bt_name;
  uint8_t bt_len;
  uint8_t ble_addr[BTIF_BD_ADDR_SIZE];
  const char *ble_name;
  uint8_t ble_len;
} bes_bt_local_info;

typedef int (*LHDC_GET_BT_INFO)(bes_bt_local_info *bt_info);
extern bool lhdcSetLicenseKeyTable(uint8_t *licTable, LHDC_GET_BT_INFO pFunc);
}
extern int bes_bt_local_info_get(bes_bt_local_info *local_info);

void lhdc_license_check() {
  uint8_t lhdc_license_key = 0;
  uint8_t *lhdc_license_data = (uint8_t *)__lhdc_license_start + 0x98;
  TRACE(5, "lhdc_license_data:%p, lhdc license %02x %02x %02x %02x",
        lhdc_license_data, lhdc_license_data[0], lhdc_license_data[1],
        lhdc_license_data[2], lhdc_license_data[3]);

  app_overlay_select(APP_OVERLAY_A2DP_LHDC);
  TRACE(1, "current_overlay = %d", app_get_current_overlay());

  lhdc_license_key =
      lhdcSetLicenseKeyTable(lhdc_license_data, bes_bt_local_info_get);
  TRACE(0, "lhdc_license_key:%d", lhdc_license_key);

  if (lhdc_license_key) {
    TRACE(0, "LHDC OK");
  } else {
    TRACE(0, "LHDC ERROR");
  }
}
#endif
