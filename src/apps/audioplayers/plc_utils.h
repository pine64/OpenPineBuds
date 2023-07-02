#ifndef PLC_UTILS_H
#define PLC_UTILS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum plc_type
{
    PLC_TYPE_PASS = 0,
    PLC_TYPE_CONTROLLER_MUTE,
    PLC_TYPE_HEADER_ERROR,
    PLC_TYPE_CRC_ERROR,
    PLC_TYPE_PAD_ERROR,
    PLC_TYPE_DATA_MISSING,
    PLC_TYPE_SEQUENCE_DISCONTINUE,
    PLC_TYPE_BLE_CONFLICT,
    PLC_TYPE_DECODER_ERROR,
    PLC_TYPE_NUM,
} plc_type_t;

typedef struct
{
    uint8_t last_seq_num;
    uint8_t last_pkt[60 * 2];

    // for ble sco conflict
    bool prev_ble_sco_conflict_flag[2];

    // histogram
    uint32_t hist[PLC_TYPE_NUM];
} PacketLossState;

#ifdef __cplusplus
extern "C" {
#endif

void packet_loss_detection_init(PacketLossState *st);

plc_type_t packet_loss_detection_process(PacketLossState *st, uint8_t *sbc_buf);

/*
 * Update plc type histogram
 * Normally this function is called at the end of packet_loss_detection_process,
 * but if a decoder error occurs outside, it should be updated maually.
 */
void packet_loss_detection_update_histogram(PacketLossState *st, plc_type_t plc_type);

void packet_loss_detection_report(PacketLossState *st);

#ifdef __cplusplus
}
#endif

#endif