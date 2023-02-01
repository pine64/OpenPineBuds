#include "plc_utils.h"
#include "hal_trace.h"
#include <stdbool.h>
#include <string.h>

#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402) ||                        \
    defined(CHIP_BEST2300P) || defined(CHIP_BEST2300A)
#define MSBC_MUTE_PATTERN (0x55)
#else
#define MSBC_MUTE_PATTERN (0x00)
#endif

#define MSBC_PKTSIZE (60)

/*
 * msbc frame's last byte is a padding byte, we assume it is zero,
 * but it is not always true.
 * Do not check this by default
 */
//#define ENABLE_PAD_CHECK

/*
 * if msbc frame is filled by 10+ samples in the trail, crc maybe not detect
 * this satuation. Do not check this by default
 */
//#define ENABLE_TRAILING_ZERO_CHECK

/*
 *
 */
#define ENABLE_BLE_CONFLICT_CHECK

#define ENABLE_CRC_CHECK

/* check msbc sequence number */
#define ENABLE_SEQ_CHECK

#ifdef ENABLE_CRC_CHECK
static const uint8_t sbc_crc_tbl[256] = {
    0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF,
    0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E,
    0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
    0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
    0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85,
    0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
    0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9,
    0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
    0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
    0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A,
    0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01,
    0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
    0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24,
    0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2,
    0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
    0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
    0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA,
    0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
    0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95,
    0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
    0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
    0xE3, 0xFE, 0xD9, 0xC4};
#endif

static int sco_parse_synchronization_header(uint8_t *buf, uint8_t *sn) {
  uint8_t sn1, sn2;
#ifdef ENABLE_CRC_CHECK
  uint8_t fcs = 0x0F;
  uint8_t crc = 0;
  uint8_t i, sb, bit, shift;
  uint8_t ind = 6, bitOffset = 24;
#endif
  *sn = 0xff;
#if defined(MSBC_SYNC_HACKER)
  if (((buf[0] != 0x01) && (buf[0] != 0x00)) || ((buf[1] & 0x0f) != 0x08) ||
      (buf[2] != 0xad)) {
    return -1;
  }
#else
  if ((buf[0] != 0x01) || ((buf[1] & 0x0f) != 0x08) || (buf[2] != 0xad)) {
    return -1;
  }
#endif

  sn1 = (buf[1] & 0x30) >> 4;
  sn2 = (buf[1] & 0xc0) >> 6;
  if ((sn1 != 0) && (sn1 != 0x3)) {
    return -2;
  }
  if ((sn2 != 0) && (sn2 != 0x3)) {
    return -3;
  }

#ifdef ENABLE_CRC_CHECK
  fcs = sbc_crc_tbl[fcs ^ buf[3]];
  if (buf[3] != 0x00)
    return -4;
  fcs = sbc_crc_tbl[fcs ^ buf[4]];
  if (buf[4] != 0x00)
    return -4;
  crc = buf[5];
  for (sb = 0; sb < 8; sb++) {
    if (bitOffset % 8) {
      /* Sum the whole byte */
      fcs = sbc_crc_tbl[fcs ^ buf[ind]];
      ind = ind + 1;
    } else {
      if (sb == 7) {
        /* Sum the next 4 bits */

        /* Just sum the most significant 4 bits */
        shift = 7;
        for (i = 0; i < 4; i++) {
          bit = (uint8_t)((0x01 & (buf[ind] >> shift--)) ^ (fcs >> 7));
          if (bit) {
            fcs = (uint8_t)(((fcs << 1) | bit) ^ 0x1C);
          } else {
            fcs = (uint8_t)((fcs << 1));
          }
        }
      }
    }

    bitOffset += 4;
  }
  // TRACE(2,"msbc crc:%d fcs:%d", crc,fcs);
  if (crc != fcs)
    return -4;
#endif

  *sn = (sn1 & 0x01) | (sn2 & 0x02);

#ifdef ENABLE_PAD_CHECK
  // when pad error detected, we should return sn
  if (buf[MSBC_PKTSIZE - 1] != 0x0) {
    return -5;
  }
#endif

  return 0;
}

#ifdef ENABLE_BLE_CONFLICT_CHECK
static bool memcmp_U8(uint8_t *x, uint8_t *y, uint16_t size) {
  for (int i = 0; i < size; i++) {
    if (x[i] != y[i])
      return true;
  }

  return false;
}

// when signal is mute, msbc data remains the same except seq num. We should
// check history flag, otherwise a single conflict may be detected twice
static bool update_ble_sco_conflict(PacketLossState *st, uint8_t *last_pkt,
                                    uint8_t *pkt) {
  // do not check padding byte as it maybe useless when msbc_offset is 1
  bool ret = (st->prev_ble_sco_conflict_flag[1] == false &&
              memcmp_U8(last_pkt, pkt, MSBC_PKTSIZE - 1) == false);

  memcpy(&last_pkt[0], &last_pkt[MSBC_PKTSIZE], MSBC_PKTSIZE);
  memcpy(&last_pkt[MSBC_PKTSIZE], pkt, MSBC_PKTSIZE);

  return ret;
}

static bool check_ble_sco_conflict(PacketLossState *st, bool ret) {
  st->prev_ble_sco_conflict_flag[1] = st->prev_ble_sco_conflict_flag[0];
  st->prev_ble_sco_conflict_flag[0] = ret;

  return ret;
}
#endif

static bool msbc_check_controller_mute_pattern(uint8_t *pkt, uint8_t pattern) {
  // do not check padding byte as it maybe useless when msbc_offset is 1
  for (int i = 0; i < MSBC_PKTSIZE - 1; i++)
    if (pkt[i] != pattern)
      return false;

  return true;
}

#ifdef ENABLE_TRAILING_ZERO_CHECK
static int msbc_check_pkt_trailing_zeros(uint8_t *pkt) {
  int idx = MSBC_PKTSIZE;

  for (int i = MSBC_PKTSIZE - 1; i >= 0; i--) {
    if (pkt[i] != 0) {
      idx = i;
      break;
    }
  }

  return (MSBC_PKTSIZE - 1 - idx);
}
#endif

static uint8_t get_next_sequence_num(uint8_t seq_num) {
  return (seq_num + 1 == 4) ? 0 : (seq_num + 1);
}

void packet_loss_detection_init(PacketLossState *st) {
  st->last_seq_num = 0xff;

  memset(st->last_pkt, 0, sizeof(st->last_pkt));
  memset(st->prev_ble_sco_conflict_flag, 0,
         sizeof(st->prev_ble_sco_conflict_flag));
  memset(st->hist, 0, sizeof(st->hist));
}

plc_type_t packet_loss_detection_process(PacketLossState *st,
                                         uint8_t *sbc_buf) {
  plc_type_t plc_type = PLC_TYPE_PASS;

#ifdef ENABLE_BLE_CONFLICT_CHECK
  bool ble_sco_conflict = update_ble_sco_conflict(st, st->last_pkt, sbc_buf);
#endif

  uint8_t seq_num;
  if (msbc_check_controller_mute_pattern(sbc_buf, MSBC_MUTE_PATTERN) == true) {
    plc_type = PLC_TYPE_CONTROLLER_MUTE;
    st->last_seq_num = 0xff;
  }
#ifdef ENABLE_BLE_CONFLICT_CHECK
  else if (check_ble_sco_conflict(st, ble_sco_conflict) == true) {
    plc_type = PLC_TYPE_BLE_CONFLICT;
    st->last_seq_num = 0xff;
  }
#endif
  else {
    int err = sco_parse_synchronization_header(sbc_buf, &seq_num);
    if (err < 0 && err >= -3) {
      plc_type = PLC_TYPE_HEADER_ERROR;
      st->last_seq_num = 0xff;
    }
#ifdef ENABLE_CRC_CHECK
    else if (err == -4) {
      plc_type = PLC_TYPE_CRC_ERROR;
      st->last_seq_num = 0xff;
    }
#endif
#ifdef ENABLE_PAD_CHECK
    else if (err == -5) {
      plc_type = PLC_TYPE_PAD_ERROR;
      st->last_seq_num = seq_num;
    }
#endif
#ifdef ENABLE_TRAILING_ZERO_CHECK
    else if (msbc_check_pkt_trailing_zeros(sbc_buf) > 10) {
      plc_type = PLC_TYPE_DATA_MISSING;
      st->last_seq_num = 0xff;
    }
#endif
    else {
#ifdef ENABLE_SEQ_CHECK
      if (st->last_seq_num == 0xff) {
        if (seq_num == 0xff) {
          plc_type = PLC_TYPE_SEQUENCE_DISCONTINUE;
        } else {
          plc_type = PLC_TYPE_PASS;
        }
        st->last_seq_num = seq_num;
      } else {
        if (seq_num == 0xff) {
          st->last_seq_num = 0xff;
          plc_type = PLC_TYPE_SEQUENCE_DISCONTINUE;
        } else if (seq_num == get_next_sequence_num(st->last_seq_num)) {
          st->last_seq_num = seq_num;
          plc_type = PLC_TYPE_PASS;
        } else {
          st->last_seq_num = 0xff;
          plc_type = PLC_TYPE_SEQUENCE_DISCONTINUE;
        }
      }
#else
      plc_type = PLC_TYPE_PASS;
#endif
    }
  }

  packet_loss_detection_update_histogram(st, plc_type);

  return plc_type;
}

void packet_loss_detection_update_histogram(PacketLossState *st,
                                            plc_type_t plc_type) {
  if (plc_type < 0 || plc_type >= PLC_TYPE_NUM) {
    TRACE(2, "[%s] plc type %d is invalid", __FUNCTION__, plc_type);
    return;
  }

  // The packet is detected as PLC_TYPE_PASS, but causes a decoder error.
  if (plc_type == PLC_TYPE_DECODER_ERROR) {
    st->hist[0] -= 1;
  }

  st->hist[plc_type] += 1;
}

void packet_loss_detection_report(PacketLossState *st) {
  uint32_t packet_loss_num = 0;

  for (uint8_t i = 1; i < PLC_TYPE_NUM; i++) {
    TRACE(3, "[%s] plc type %d occurs %d times", __FUNCTION__, i, st->hist[i]);
    packet_loss_num += st->hist[i];
  }

  uint32_t packet_total_num = st->hist[0] + packet_loss_num;
  TRACE(4, "[%s] packet loss percent %d/10000(%d/%d)", __FUNCTION__,
        (int32_t)(10000.f * packet_loss_num / packet_total_num),
        packet_loss_num, packet_total_num);
}