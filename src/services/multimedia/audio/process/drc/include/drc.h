#ifndef DRC_H
#define DRC_H

#include <stdint.h>

#define MAX_DRC_FILTER_CNT_SUPPORT 2

#define DRC_BAND_NUM (3)

enum
{
    DRC_FREQ_NULL = -1,
    DRC_FREQ_50HZ = 1,
    DRC_FREQ_63HZ,
    DRC_FREQ_79HZ,
    DRC_FREQ_100HZ,
    DRC_FREQ_125HZ,
    DRC_FREQ_158HZ,
    DRC_FREQ_199HZ,
    DRC_FREQ_251HZ,
    DRC_FREQ_316HZ,
    DRC_FREQ_398HZ,
    DRC_FREQ_501HZ,
    DRC_FREQ_630HZ,
    DRC_FREQ_794HZ,
    DRC_FREQ_1000HZ,
    DRC_FREQ_1258HZ,
    DRC_FREQ_1584HZ,
    DRC_FREQ_1995HZ,
    DRC_FREQ_2511HZ,
    DRC_FREQ_3162HZ,
    DRC_FREQ_3981HZ,
    DRC_FREQ_5011HZ,
    DRC_FREQ_6309HZ,
    DRC_FREQ_7943HZ,
    DRC_FREQ_10000HZ,
    DRC_FREQ_12589HZ,
    DRC_FREQ_15848HZ,
    DRC_FREQ_19952HZ,
};

struct DrcBandConfig
{
    int     threshold;
    float   makeup_gain;
    int     ratio;              // set 1, bypass, reduce mips
    int     attack_time;
    int     release_time;
    float   scale_fact;         // invalid
};

typedef struct
{
    int knee;
    int filter_type[MAX_DRC_FILTER_CNT_SUPPORT]; // filter type is choosen from DRC_FREQ_NULL~DRC_FREQ_19952HZ
    int band_num;
    int look_ahead_time;
    struct DrcBandConfig band_settings[DRC_BAND_NUM];
} DrcConfig;

struct DrcState_;

typedef struct DrcState_ DrcState;

#ifdef __cplusplus
extern "C" {
#endif

DrcState *drc_create(int sample_rate, int frame_size, int sample_bit, int ch_num, const DrcConfig *config);

int32_t drc_destroy(DrcState *st);

int32_t drc_set_config(DrcState *st, const DrcConfig *config);

int32_t drc_process(DrcState *st, uint8_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif