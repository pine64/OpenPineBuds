/***************************************************************************
 *
 * Copyright 2015-2019 BES.
 * All rights reserved. All unpublished rights reserved.
 *
 * No part of this work may be used or reproduced in any form or by any
 * means, or stored in a database or retrieval system, without prior written
 * permission of BES.
 *
 * Use of this work is governed by a license granted by BES.
 * This work contains confidential and proprietary information of
 * BES. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/
#include "plat_addr_map.h"
#include CHIP_SPECIFIC_HDR(reg_codec)
#include "analog.h"
#include "cmsis.h"
#include "hal_aud.h"
#include "hal_cmu.h"
#include "hal_codec.h"
#include "hal_psc.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "string.h"
#include "tgt_hardware.h"
#ifdef RTOS
#include "cmsis_os.h"
#endif
#include "hal_chipid.h"

#define NO_DAC_RESET

#define SDM_MUTE_NOISE_SUPPRESSION

#ifndef CODEC_OUTPUT_DEV
#define CODEC_OUTPUT_DEV CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV
#endif
#if ((CODEC_OUTPUT_DEV & CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV) == 0)
#ifndef AUDIO_OUTPUT_SWAP
#define AUDIO_OUTPUT_SWAP
#endif
#endif

#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
//#define CODEC_TIMER
#endif

#ifdef CODEC_DSD
#ifdef ANC_APP
#error "ANC_APP conflicts with CODEC_DSD"
#endif
#ifdef AUDIO_ANC_FB_MC
#error "AUDIO_ANC_FB_MC conflicts with CODEC_DSD"
#endif
#ifdef __AUDIO_RESAMPLE__
#error "__AUDIO_RESAMPLE__ conflicts with CODEC_DSD"
#endif
#endif

#define RS_CLOCK_FACTOR 200

// Trigger DMA request when TX-FIFO *empty* count > threshold
#define HAL_CODEC_TX_FIFO_TRIGGER_LEVEL (3)
// Trigger DMA request when RX-FIFO count >= threshold
#define HAL_CODEC_RX_FIFO_TRIGGER_LEVEL (4)

#define MAX_DIG_DBVAL (50)
#define ZERODB_DIG_DBVAL (0)
#define MIN_DIG_DBVAL (-99)

#define MAX_SIDETONE_DBVAL (30)
#define MIN_SIDETONE_DBVAL (-30)
#define SIDETONE_DBVAL_STEP (-2)

#define MAX_SIDETONE_REGVAL (0)
#define MIN_SIDETONE_REGVAL (30)
#define MUTE_SIDETONE_REGVAL (31)

#ifndef MC_DELAY_COMMON
#define MC_DELAY_COMMON 28
#endif

#ifndef CODEC_DIGMIC_PHASE
#define CODEC_DIGMIC_PHASE 7
#endif

#define ADC_IIR_CH_NUM 2

#define MAX_DIG_MIC_CH_NUM 5

#define NORMAL_ADC_CH_NUM 5
// Echo cancel ADC channel number
#define EC_ADC_CH_NUM 2
#define MAX_ADC_CH_NUM (NORMAL_ADC_CH_NUM + EC_ADC_CH_NUM)

#define MAX_DAC_CH_NUM 2

#ifdef CODEC_DSD
#define NORMAL_MIC_MAP                                                         \
  (AUD_CHANNEL_MAP_NORMAL_ALL &                                                \
   ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6 | AUD_CHANNEL_MAP_CH7 |         \
     AUD_CHANNEL_MAP_DIGMIC_CH5 | AUD_CHANNEL_MAP_DIGMIC_CH6 |                 \
     AUD_CHANNEL_MAP_DIGMIC_CH7 | AUD_CHANNEL_MAP_DIGMIC_CH2 |                 \
     AUD_CHANNEL_MAP_DIGMIC_CH3))
#else
#define NORMAL_MIC_MAP                                                         \
  (AUD_CHANNEL_MAP_NORMAL_ALL &                                                \
   ~(AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6 | AUD_CHANNEL_MAP_CH7 |         \
     AUD_CHANNEL_MAP_DIGMIC_CH5 | AUD_CHANNEL_MAP_DIGMIC_CH6 |                 \
     AUD_CHANNEL_MAP_DIGMIC_CH7))
#endif
#define NORMAL_ADC_MAP                                                         \
  (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1 | AUD_CHANNEL_MAP_CH2 |           \
   AUD_CHANNEL_MAP_CH3 | AUD_CHANNEL_MAP_CH4)

#define EC_MIC_MAP (AUD_CHANNEL_MAP_ECMIC_CH0 | AUD_CHANNEL_MAP_ECMIC_CH1)
#define EC_ADC_MAP (AUD_CHANNEL_MAP_CH5 | AUD_CHANNEL_MAP_CH6)

#define VALID_MIC_MAP (NORMAL_MIC_MAP | EC_MIC_MAP)
#define VALID_ADC_MAP (NORMAL_ADC_MAP | EC_ADC_MAP)

#define VALID_SPK_MAP (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)
#define VALID_DAC_MAP VALID_SPK_MAP

#if (CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV & ~VALID_SPK_MAP)
#error "Invalid CFG_HW_AUD_OUTPUT_PATH_SPEAKER_DEV"
#endif
#if (CODEC_OUTPUT_DEV & ~VALID_SPK_MAP)
#error "Invalid CODEC_OUTPUT_DEV"
#endif

#define RSTN_ADC_FREE_RUNNING_CLK CODEC_SOFT_RSTN_ADC(1 << NORMAL_ADC_CH_NUM)

#if defined(SPEECH_SIDETONE) &&                                                \
    (defined(CFG_HW_AUD_SIDETONE_MIC_DEV) && (CFG_HW_AUD_SIDETONE_MIC_DEV)) && \
    defined(CFG_HW_AUD_SIDETONE_GAIN_DBVAL)
#define SIDETONE_ENABLE
#if (CFG_HW_AUD_SIDETONE_GAIN_DBVAL > MAX_SIDETONE_DBVAL) ||                   \
    (CFG_HW_AUD_SIDETONE_GAIN_DBVAL < MIN_SIDETONE_DBVAL) ||                   \
    defined(CFG_HW_AUD_SIDETONE_IIR_INDEX) ||                                  \
    defined(CFG_HW_AUD_SIDETONE_GAIN_RAMP)
#define SIDETONE_DEDICATED_ADC_CHAN
#define SIDETONE_RESERVED_ADC_CHAN
#endif
#endif

enum CODEC_ADC_EN_REQ_T {
  CODEC_ADC_EN_REQ_STREAM,
  CODEC_ADC_EN_REQ_MC,
  CODEC_ADC_EN_REQ_DSD,

  CODEC_ADC_EN_REQ_QTY,
};

enum CODEC_IRQ_TYPE_T {
  CODEC_IRQ_TYPE_BT_TRIGGER,
  CODEC_IRQ_TYPE_VAD,
  CODEC_IRQ_TYPE_ANC_FB_CHECK,

  CODEC_IRQ_TYPE_QTY,
};

struct CODEC_DAC_DRE_CFG_T {
  uint8_t dre_delay;
  uint8_t thd_db_offset;
  uint8_t step_mode;
  uint32_t dre_win;
  uint16_t amp_high;
};

struct CODEC_DAC_SAMPLE_RATE_T {
  enum AUD_SAMPRATE_T sample_rate;
  uint32_t codec_freq;
  uint8_t codec_div;
  uint8_t cmu_div;
  uint8_t dac_up;
  uint8_t bypass_cnt;
  uint8_t mc_delay;
};

static const struct CODEC_DAC_SAMPLE_RATE_T codec_dac_sample_rate[] = {
#ifdef __AUDIO_RESAMPLE__
    {AUD_SAMPRATE_8463, CODEC_FREQ_CRYSTAL, 1, 1, 6, 0, MC_DELAY_COMMON + 160},
    {AUD_SAMPRATE_16927, CODEC_FREQ_CRYSTAL, 1, 1, 3, 0, MC_DELAY_COMMON + 85},
    {AUD_SAMPRATE_50781, CODEC_FREQ_CRYSTAL, 1, 1, 1, 0, MC_DELAY_COMMON + 29},
#endif
    {AUD_SAMPRATE_7350, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     6, 0, MC_DELAY_COMMON + 174},
    {AUD_SAMPRATE_8000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 6,
     0, MC_DELAY_COMMON + 168}, // T
    {AUD_SAMPRATE_14700, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     3, 0, MC_DELAY_COMMON + 71},
    {AUD_SAMPRATE_16000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 3,
     0, MC_DELAY_COMMON + 88}, // T
    {AUD_SAMPRATE_22050, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     2, 0, MC_DELAY_COMMON + 60},
    {AUD_SAMPRATE_24000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 2,
     0, MC_DELAY_COMMON + 58},
    {AUD_SAMPRATE_44100, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 0, MC_DELAY_COMMON + 31}, // T
    {AUD_SAMPRATE_48000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1,
     0, MC_DELAY_COMMON + 30}, // T
    {AUD_SAMPRATE_88200, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 1, MC_DELAY_COMMON + 12},
    {AUD_SAMPRATE_96000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1,
     1, MC_DELAY_COMMON + 12}, // T
    {AUD_SAMPRATE_176400, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 2, MC_DELAY_COMMON + 5},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 2, MC_DELAY_COMMON + 5},
    {AUD_SAMPRATE_352800, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 3, MC_DELAY_COMMON + 2},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 3, MC_DELAY_COMMON + 2},
};

struct CODEC_ADC_SAMPLE_RATE_T {
  enum AUD_SAMPRATE_T sample_rate;
  uint32_t codec_freq;
  uint8_t codec_div;
  uint8_t cmu_div;
  uint8_t adc_down;
  uint8_t bypass_cnt;
};

static const struct CODEC_ADC_SAMPLE_RATE_T codec_adc_sample_rate[] = {
#ifdef __AUDIO_RESAMPLE__
    {AUD_SAMPRATE_8463, CODEC_FREQ_CRYSTAL, 1, 1, 6, 0},
    {AUD_SAMPRATE_16927, CODEC_FREQ_CRYSTAL, 1, 1, 3, 0},
    {AUD_SAMPRATE_50781, CODEC_FREQ_CRYSTAL, 1, 1, 1, 0},
#endif
    {AUD_SAMPRATE_7350, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     6, 0},
    {AUD_SAMPRATE_8000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 6,
     0},
    {AUD_SAMPRATE_14700, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     3, 0},
    {AUD_SAMPRATE_16000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 3,
     0},
    {AUD_SAMPRATE_44100, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 0},
    {AUD_SAMPRATE_48000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1,
     0},
    {AUD_SAMPRATE_88200, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 1},
    {AUD_SAMPRATE_96000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV, 1,
     1},
    {AUD_SAMPRATE_176400, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 2},
    {AUD_SAMPRATE_192000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 2},
    {AUD_SAMPRATE_352800, CODEC_FREQ_44_1K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 3},
    {AUD_SAMPRATE_384000, CODEC_FREQ_48K_SERIES, CODEC_PLL_DIV, CODEC_CMU_DIV,
     1, 3},
};

const CODEC_ADC_VOL_T WEAK codec_adc_vol[TGT_ADC_VOL_LEVEL_QTY] = {
    -99, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
};

static struct CODEC_T *const codec = (struct CODEC_T *)CODEC_BASE;

#define CODEC_FIR_CH0_BASE (CODEC_BASE + 0x9000)
#define CODEC_FIR_CH1_BASE (CODEC_BASE + 0xB000)
#define CODEC_FIR_CH2_BASE (CODEC_BASE + 0xD000)
#define CODEC_FIR_CH3_BASE (CODEC_BASE + 0xF000)

#define CODEC_FIR_HISTORY_CH0_BASE (CODEC_FIR_CH0_BASE - 0x1000)
#define CODEC_FIR_HISTORY_CH1_BASE (CODEC_FIR_CH1_BASE - 0x1000)
#define CODEC_FIR_HISTORY_CH2_BASE (CODEC_FIR_CH2_BASE - 0x1000)
#define CODEC_FIR_HISTORY_CH3_BASE (CODEC_FIR_CH3_BASE - 0x1000)

#ifdef CODEC_MIN_PHASE
static volatile int32_t *const codec_fir_ch0 = (int32_t *)CODEC_FIR_CH0_BASE;
static volatile int32_t *const codec_fir_ch1 = (int32_t *)CODEC_FIR_CH1_BASE;
static volatile int32_t *const codec_fir_ch2 = (int32_t *)CODEC_FIR_CH2_BASE;
static volatile int32_t *const codec_fir_ch3 = (int32_t *)CODEC_FIR_CH3_BASE;
static volatile int32_t *const codec_fir_history0 =
    (int32_t *)CODEC_FIR_HISTORY_CH0_BASE;
static volatile int32_t *const codec_fir_history1 =
    (int32_t *)CODEC_FIR_HISTORY_CH1_BASE;
static volatile int32_t *const codec_fir_history2 =
    (int32_t *)CODEC_FIR_HISTORY_CH2_BASE;
static volatile int32_t *const codec_fir_history3 =
    (int32_t *)CODEC_FIR_HISTORY_CH3_BASE;

#define MAX_FIR_ORDERS (512)

#define FIR_LOW_FREQ

#ifdef FIR_LOW_FREQ
#define FIR_DAC_ORDERS (220)
#define FIR_ADC_ORDERS (220)
#else
#define FIR_DAC_ORDERS (512)
#define FIR_ADC_ORDERS (512)
#endif

STATIC_ASSERT(FIR_DAC_ORDERS <= MAX_FIR_ORDERS, "FIR_DAC_ORDERS too large");
STATIC_ASSERT(FIR_ADC_ORDERS <= MAX_FIR_ORDERS, "FIR_ADC_ORDERS too large");

const static int32_t POSSIBLY_UNUSED fir_coef_linear[FIR_DAC_ORDERS] = {
    524287,
};

const static int32_t POSSIBLY_UNUSED fir_coef_minimun[FIR_DAC_ORDERS] = {
#ifdef FIR_LOW_FREQ
    171,     1077,    4203,    12582,   31473,   68652,   133899,  237244,
    385800,  579602,  807418,  1044189, 1251727, 1383990, 1396843, 1260694,
    972640,  563948,  99101,   -335381, -650923, -780973, -701927, -443652,
    -84578,  268808,  512951,  579186,  455906,  192655,  -116310, -364122,
    -468309, -399443, -190845, 74646,   296174,  392908,  334154,  149860,
    -82526,  -269615, -339392, -269328, -94001,  110604,  259377,  293401,
    203556,  32780,   -143605, -250723, -245944, -136029, 27365,   171442,
    234497,  192404,  67901,   -80955,  -187545, -206431, -132784, -3152,
    122291,  187500,  165597,  69954,   -53253,  -147114, -170196, -114838,
    -9444,   95806,   152719,  137136,  59154,   -42785,  -120684, -139533,
    -93024,  -5397,   80987,   126029,  110487,  44131,   -39952,  -101741,
    -113290, -71331,  2185,    71419,   104055,  86354,   28846,   -39643,
    -86369,  -90279,  -51459,  9830,    63615,   84824,   64943,   15296,
    -39213,  -72539,  -69912,  -34308,  15781,   55890,   67480,   46479,
    4496,    -37461,  -59466,  -52097,  -20389,  19303,   47689,   51882,
    31200,   -3160,   -34103,  -47104,  -36994,  -9893,   20334,   39133,
    38235,   19235,   -7752,   -29414,  -35781,  -24769,  -2693,   19264,
    30682,   26817,   10499,   -9713,   -23968,  -25906,  -15440,  1619,
    16733,   22882,   17788,   4668,    -9693,   -18412,  -17788,  -8810,
    3652,    13454,   16181,   11101,   1226,    -8416,   -13305,  -11533,
    -4481,   4101,    10062,   10834,   6508,    -446,    -6545,   -9016,
    -7036,   -1934,   3627,    7021,    6880,    3613,    -961,    -4595,
    -5701,   -4025,   -619,    2769,    4591,    4179,    1965,    -839,
    -2887,   -3327,   -2140,   -46,     1903,    2850,    2488,    1144,
    -458,    -1565,   -1740,   -1020,   170,     1245,    1754,    1547,
    815,     -53,     -656,    -761,    -376,    288,     927,     1290,
    1257,    878,     307,     -256,    -663,    -840,    -809,    -642,
    -427,    -237,    -103,    -33,
#else
    358,     2134,    7870,    22383,   53324,   110934,  206479,  349044,
    541000,  773159,  1021370, 1246556, 1399740, 1432299, 1309745, 1025462,
    609633,  129002,  -324731, -656532, -794417, -712592, -443054, -70120,
    292346,  535048,  587990,  443750,  160328,  -158614, -400055, -481834,
    -382027, -146176, 130807,  341515,  407598,  309695,  93804,   -148511,
    -318068, -348626, -233320, -26063,  182493,  304084,  290262,  152119,
    -45991,  -215703, -283797, -223953, -67174,  113414,  236243,  248077,
    147280,  -16729,  -167204, -235765, -193530, -63427,  91650,   198950,
    209932,  122330,  -19887,  -148250, -202614, -159367, -41765,  92191,
    178176,  176391,  90055,   -37106,  -142792, -176569, -123965, -12686,
    101744,  163572,  143971,  54527,   -59287,  -141093, -151499, -87091,
    18628,   112527,  148481,  110037,  17958,   -80830,  -137074, -123736,
    -49016,  48438,   119432,  129027,  73750,   -17278,  -97589,  -127045,
    -91900,  -11220,  73360,   119064,  103602,  36056,   -48313,  -106407,
    -109292, -56617,  23738,   90354,   109596,  72600,   -665,    -72105,
    -105274, -83967,  -20135,  52732,   97141,   90877,   38119,   -33174,
    -86035,  -93649,  -52950,  14213,   72765,   92702,   64465,   3511,
    -58099,  -88538,  -72655,  -19509,  42731,   81690,   77624,   33421,
    -27283,  -72714,  -79582,  -45018,  12285,   62151,   78801,   54174,
    1818,    -50528,  -75615,  -60867,  -14676,  38325,   70381,   65146,
    26015,   -25988,  -63481,  -67140,  -35653,  13901,   55293,   67020,
    43473,   -2402,   -46194,  -65008,  -49433,  -8237,   36534,   61346,
    53542,   17793,   -26648,  -56304,  -55870,  -26107,  16829,   50150,
    56518,   33065,   -7346,   -43163,  -55631,  -38609,  -1582,   35606,
    53375,   42719,   9762,    -27735,  -49938,  -45423,  -17053,  19783,
    45514,   46773,   23343,   -11967,  -40312,  -46863,  -28563,  4471,
    34530,   45798,   32673,   2539,    -28370,  -43713,  -35673,  -8936,
    22017,   40746,   37582,   14610,   -15648,  -37053,  -38453,  -19489,
    9418,    32783,   38351,   23519,   -3470,   -28096,  -37370,  -26681,
    -2079,   23137,   35604,   28971,   7128,    -18054,  -33170,  -30413,
    -11603,  12974,   30179,   31045,   15443,   -8023,   -26756,  -30924,
    -18617,  3302,    23015,   30118,   21105,   1094,    -19077,  -28707,
    -22913,  -5093,   15046,   26776,   24054,   8635,    -11031,  -24419,
    -24565,  -11680,  7118,    21726,   24486,   14198,   -3394,   -18792,
    -23876,  -16181,  -76,     15704,   22793,   17629,   3230,    -12551,
    -21309,  -18559,  -6028,   9408,    19490,   18994,   8433,    -6352,
    -17414,  -18974,  -10431,  3440,    15149,   18537,   12008,   -731,
    -12769,  -17737,  -13172,  -1735,   10336,   16622,   13932,   3920,
    -7916,   -15252,  -14311,  -5805,   5559,    13680,   14335,   7371,
    -3319,   -11967,  -14040,  -8615,   1232,    10161,   13463,   9538,
    664,     -8320,   -12648,  -10154,  -2348,   6485,    11634,   10474,
    3797,    -4703,   -10470,  -10527,  -5006,   3007,    9196,    10334,
    5966,    -1433,   -7856,   -9930,   -6686,   1,       6486,    9343,
    7168,    1266,    -5126,   -8610,   -7432,   -2359,   3803,    7762,
    7491,    3267,    -2551,   -6835,   -7369,   -3991,   1387,    5858,
    7087,    4531,    -335,    -4863,   -6673,   -4898,   -597,    3875,
    6149,    5100,    1395,    -2920,   -5545,   -5154,   -2058,   2015,
    4882,    5073,    2582,    -1181,   -4187,   -4878,   -2974,   427,
    3479,    4586,    3237,    234,     -2781,   -4219,   -3383,   -799,
    2107,    3793,    3420,    1262,    -1476,   -3331,   -3363,   -1628,
    894,     2845,    3224,    1895,    -375,    -2357,   -3021,   -2074,
    -79,     1875,    2764,    2167,    459,     -1416,   -2470,   -2186,
    -771,    987,     2151,    2139,    1009,    -599,    -1823,   -2040,
    -1182,   254,     1492,    1894,    1290,    40,      -1172,   -1717,
    -1342,   -286,    869,     1516,    1343,    480,     -592,    -1303,
    -1303,   -626,    342,     1084,    1226,    725,     -127,    -870,
    -1124,   -784,    -55,     664,     1001,    803,     200,     -475,
    -869,    -794,    -313,    303,     729,     756,     392,     -154,
    -592,    -701,    -443,    28,      458,     630,     468,     74,
    -337,    -553,    -473,    -155,    226,     471,     462,     215,
    -130,    -394,    -443,    -263,    45,      319,     420,     302,
    31,      -253,    -404,    -349,    -115,    181,     394,     418,
    237,     -74,     -375,    -536,    -472,    -196,    214,     626,
    935,     1062,    1013,    824,     581,     341,     166,     55,
#endif
};

static uint8_t min_phase_cfg;
#endif

static bool codec_init = false;
static bool codec_opened = false;

static int8_t digdac_gain[MAX_DAC_CH_NUM];
static int8_t digadc_gain[NORMAL_ADC_CH_NUM];

static bool codec_mute[AUD_STREAM_NUM];

#ifdef AUDIO_OUTPUT_SWAP
static bool output_swap = true;
#endif

#ifdef ANC_APP
static float anc_boost_gain_attn;
static int8_t anc_adc_gain_offset[NORMAL_ADC_CH_NUM];
static enum AUD_CHANNEL_MAP_T anc_adc_gain_offset_map;
#endif
#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
static bool codec_nr_enabled;
static int8_t digdac_gain_offset_nr;
#endif
#ifdef AUDIO_OUTPUT_DC_CALIB
static int32_t dac_dc_l;
static int32_t dac_dc_r;
static float dac_dc_gain_attn;
#endif
#ifdef __AUDIO_RESAMPLE__
static uint8_t rs_clk_map;
STATIC_ASSERT(sizeof(rs_clk_map) * 8 >= AUD_STREAM_NUM,
              "rs_clk_map size too small");

static uint32_t resample_clk_freq;
static uint8_t resample_rate_idx[AUD_STREAM_NUM];
#endif
#ifdef CODEC_TIMER
static uint32_t cur_codec_freq;
#endif
// EC
static uint8_t codec_rate_idx[AUD_STREAM_NUM];

// static HAL_CODEC_DAC_RESET_CALLBACK dac_reset_callback;

static uint8_t codec_irq_map;
STATIC_ASSERT(sizeof(codec_irq_map) * 8 >= CODEC_IRQ_TYPE_QTY,
              "codec_irq_map size too small");
static HAL_CODEC_IRQ_CALLBACK codec_irq_callback[CODEC_IRQ_TYPE_QTY];

static enum AUD_CHANNEL_MAP_T codec_dac_ch_map;
static enum AUD_CHANNEL_MAP_T codec_adc_ch_map;
static enum AUD_CHANNEL_MAP_T anc_adc_ch_map;
static enum AUD_CHANNEL_MAP_T codec_mic_ch_map;
static enum AUD_CHANNEL_MAP_T anc_mic_ch_map;
#ifdef SIDETONE_DEDICATED_ADC_CHAN
static enum AUD_CHANNEL_MAP_T sidetone_adc_ch_map;
static int8_t sidetone_adc_gain;
static int8_t sidetone_gain_offset;
#ifdef CFG_HW_AUD_SIDETONE_GAIN_RAMP
static float sidetone_ded_chan_coef;
#endif
#endif

static uint8_t dac_delay_ms;

#ifdef ANC_PROD_TEST
#define OPT_TYPE
#else
#define OPT_TYPE const
#endif

static OPT_TYPE uint8_t codec_digmic_phase = CODEC_DIGMIC_PHASE;

#if defined(AUDIO_ANC_FB_MC) && defined(__AUDIO_RESAMPLE__)
#error "Music cancel cannot work with audio resample"
#endif
#ifdef AUDIO_ANC_FB_MC
static bool mc_enabled;
static bool mc_dual_chan;
static bool mc_16bit;
#endif

#ifdef CODEC_DSD
static bool dsd_enabled;
static uint8_t dsd_rate_idx;
#endif

#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
static uint8_t adc_en_map;
STATIC_ASSERT(sizeof(adc_en_map) * 8 >= CODEC_ADC_EN_REQ_QTY,
              "adc_en_map size too small");
#endif

#ifdef PERF_TEST_POWER_KEY
static enum HAL_CODEC_PERF_TEST_POWER_T cur_perft_power;
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
static int8_t swdac_gain;
static HAL_CODEC_SW_OUTPUT_COEF_CALLBACK sw_output_coef_callback;
#endif

static HAL_CODEC_BT_TRIGGER_CALLBACK bt_trigger_callback = NULL;

#ifdef VOICE_DETECTOR_EN
#define CODEC_VAD_BUF_ADDR 0x40304000
#define CODEC_VAD_BUF_SIZE 0x4000
static enum AUD_VAD_TYPE_T vad_type;
static AUD_VAD_CALLBACK vad_handler;
static bool vad_enabled;
static uint32_t vad_data_cnt;
static uint32_t vad_addr_cnt;
#endif

#if defined(DAC_CLASSG_ENABLE)
static struct dac_classg_cfg _dac_classg_cfg = {
    .thd2 = 0xC0,
    .thd1 = 0x10,
    .thd0 = 0x10,
    .lr = 1,
    .step_4_3n = 0,
    .quick_down = 1,
    .wind_width = 0x400,
};
#endif

#ifdef DAC_DRE_ENABLE
static const struct CODEC_DAC_DRE_CFG_T dac_dre_cfg = {
    .dre_delay = 8,
    .thd_db_offset = 0xF, // 5,
    .step_mode = 0,
    .dre_win = 0x6000,
    .amp_high = 2, // 0x10,
};
#endif

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map,
                                       int32_t gain);
static void hal_codec_set_dig_dac_gain(enum AUD_CHANNEL_MAP_T map,
                                       int32_t gain);
static void hal_codec_restore_dig_dac_gain(void);
static void hal_codec_set_dac_gain_value(enum AUD_CHANNEL_MAP_T map,
                                         uint32_t val);
static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val);
static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map,
                                            uint32_t cnt);
static uint32_t hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map);
#ifdef AUDIO_OUTPUT_SW_GAIN
static void hal_codec_set_sw_gain(int32_t gain);
#endif
#ifdef __AUDIO_RESAMPLE__
static float get_playback_resample_phase(void);
static float get_capture_resample_phase(void);
static uint32_t resample_phase_float_to_value(float phase);
static float resample_phase_value_to_float(uint32_t value);
#endif

static void hal_codec_reg_update_delay(void) { hal_sys_timer_delay_us(2); }

#if defined(DAC_CLASSG_ENABLE)
void hal_codec_classg_config(const struct dac_classg_cfg *cfg) {
  _dac_classg_cfg = *cfg;
}

static void hal_codec_classg_enable(bool en) {
  struct dac_classg_cfg *config;

  if (en) {
    config = &_dac_classg_cfg;

    codec->REG_0B4 =
        SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD2, config->thd2);
    codec->REG_0B4 =
        SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD1, config->thd1);
    codec->REG_0B4 =
        SET_BITFIELD(codec->REG_0B4, CODEC_CODEC_CLASSG_THD0, config->thd0);

    // Make class-g set the lowest gain after several samples.
    // Class-g gain will have impact on dc.
    codec->REG_0B0 = SET_BITFIELD(codec->REG_0B0, CODEC_CODEC_CLASSG_WINDOW, 6);

    if (config->lr)
      codec->REG_0B0 |= CODEC_CODEC_CLASSG_LR;
    else
      codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_LR;

    if (config->step_4_3n)
      codec->REG_0B0 |= CODEC_CODEC_CLASSG_STEP_3_4N;
    else
      codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_STEP_3_4N;

    if (config->quick_down)
      codec->REG_0B0 |= CODEC_CODEC_CLASSG_QUICK_DOWN;
    else
      codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;

    codec->REG_0B0 |= CODEC_CODEC_CLASSG_EN;

    // Restore class-g window after the gain has been updated
    hal_codec_reg_update_delay();
    codec->REG_0B0 = SET_BITFIELD(codec->REG_0B0, CODEC_CODEC_CLASSG_WINDOW,
                                  config->wind_width);
  } else {
    codec->REG_0B0 &= ~CODEC_CODEC_CLASSG_QUICK_DOWN;
  }
}
#endif

#if defined(AUDIO_OUTPUT_DC_CALIB) || defined(SDM_MUTE_NOISE_SUPPRESSION)
static void hal_codec_dac_dc_offset_enable(int32_t dc_l, int32_t dc_r) {
  codec->REG_0E0 &= CODEC_CODEC_DAC_DC_UPDATE_CH0;
  hal_codec_reg_update_delay();
  codec->REG_0E8 = SET_BITFIELD(codec->REG_0E8, CODEC_CODEC_DAC_DC_CH1, dc_r);
  codec->REG_0E0 = SET_BITFIELD(codec->REG_0E0, CODEC_CODEC_DAC_DC_CH0, dc_l) |
                   CODEC_CODEC_DAC_DC_UPDATE_CH0;
}
#endif

#ifdef CODEC_MIN_PHASE
static void hal_codec_min_phase_init(void) {
  int i;

  // SYS clock should be 52M or above

  // DAC
  codec->REG_108 &= ~(CODEC_STREAM0_FIR1_CH0);
  codec->REG_108 = SET_BITFIELD(codec->REG_108, CODEC_FIR_MODE_CH0, 0);
  codec->REG_108 =
      SET_BITFIELD(codec->REG_108, CODEC_FIR_ORDER_CH0, FIR_DAC_ORDERS);

  codec->REG_10C = SET_BITFIELD(codec->REG_10C, CODEC_FIR_BURST_LENGTH_CH0, 4);
  codec->REG_10C = SET_BITFIELD(codec->REG_10C, CODEC_FIR_GAIN_SEL_CH0, 4);

  codec->REG_110 &= ~(CODEC_STREAM0_FIR1_CH1);
  codec->REG_110 = SET_BITFIELD(codec->REG_110, CODEC_FIR_MODE_CH1, 0);
  codec->REG_110 =
      SET_BITFIELD(codec->REG_110, CODEC_FIR_ORDER_CH1, FIR_DAC_ORDERS);

  codec->REG_114 = SET_BITFIELD(codec->REG_114, CODEC_FIR_BURST_LENGTH_CH1, 4);
  codec->REG_114 = SET_BITFIELD(codec->REG_114, CODEC_FIR_GAIN_SEL_CH1, 4);

  // ADC
  codec->REG_118 &= ~(CODEC_STREAM0_FIR1_CH2);
  codec->REG_118 = SET_BITFIELD(codec->REG_118, CODEC_FIR_MODE_CH2, 0);
  codec->REG_118 =
      SET_BITFIELD(codec->REG_118, CODEC_FIR_ORDER_CH2, FIR_ADC_ORDERS);

  // codec->REG_11C = SET_BITFIELD(codec->REG_11C,CODEC_FIR_BURST_LENGTH_CH2,4);
  codec->REG_11C = SET_BITFIELD(codec->REG_11C, CODEC_FIR_GAIN_SEL_CH2, 6);

  codec->REG_120 &= ~(CODEC_STREAM0_FIR1_CH3);
  codec->REG_120 = SET_BITFIELD(codec->REG_120, CODEC_FIR_MODE_CH3, 0);
  codec->REG_120 =
      SET_BITFIELD(codec->REG_120, CODEC_FIR_ORDER_CH3, FIR_ADC_ORDERS);

  // codec->REG_124 = SET_BITFIELD(codec->REG_124,CODEC_FIR_BURST_LENGTH_CH3,4);
  codec->REG_124 = SET_BITFIELD(codec->REG_124, CODEC_FIR_GAIN_SEL_CH3, 6);

  // DAC
  codec->REG_100 |= (CODEC_FIR_UPSAMPLE_CH0 | CODEC_FIR_UPSAMPLE_CH1);

  for (i = 0; i < FIR_DAC_ORDERS; i++) {
    codec_fir_ch0[i] = fir_coef_minimun[i];
    codec_fir_ch1[i] = fir_coef_minimun[i];
  }

  // ADC
  codec->REG_100 &= ~(CODEC_FIR_UPSAMPLE_CH2 | CODEC_FIR_UPSAMPLE_CH3);

  for (i = 0; i < FIR_DAC_ORDERS; i++) {
    codec_fir_ch2[i] = fir_coef_minimun[i];
    codec_fir_ch3[i] = fir_coef_minimun[i];
  }

  // Init history buffer
  for (i = 0; i < MAX_FIR_ORDERS; i++) {
    codec_fir_history0[i] = 0;
    codec_fir_history1[i] = 0;
    codec_fir_history2[i] = 0;
    codec_fir_history3[i] = 0;
  }
}

static void hal_codec_min_phase_term(void) {
  // Release SYS clock request
}
#endif

int hal_codec_open(enum HAL_CODEC_ID_T id) {
  int i;
  bool first_open;

#ifdef CODEC_POWER_DOWN
  first_open = true;
#else
  first_open = !codec_init;
#endif

  analog_aud_pll_open(ANA_AUD_PLL_USER_CODEC);

  if (!codec_init) {
    for (i = 0; i < CFG_HW_AUD_INPUT_PATH_NUM; i++) {
      if (cfg_audio_input_path_cfg[i].cfg & AUD_CHANNEL_MAP_ALL &
          ~VALID_MIC_MAP) {
        ASSERT(false, "Invalid input path cfg: i=%d io_path=%d cfg=0x%X", i,
               cfg_audio_input_path_cfg[i].io_path,
               cfg_audio_input_path_cfg[i].cfg);
      }
    }
#ifdef ANC_APP
    anc_boost_gain_attn = 1.0f;
#endif
    codec_init = true;
  }
  if (first_open) {
    // Codec will be powered down first
    hal_psc_codec_enable();
  }
  hal_cmu_codec_clock_enable();
  hal_cmu_codec_reset_clear();

  codec_opened = true;

  codec->REG_060 |= CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA_MASK |
                    CODEC_POL_ADC_ANA_MASK | CODEC_POL_DAC_OUT;
  codec->REG_064 |= CODEC_SOFT_RSTN_32K | CODEC_SOFT_RSTN_IIR |
                    CODEC_SOFT_RSTN_RS | CODEC_SOFT_RSTN_DAC |
                    CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK;
  codec->REG_000 = 0;
  codec->REG_04C &= ~CODEC_MC_ENABLE;
  codec->REG_004 = ~0UL;
  hal_codec_reg_update_delay();
  codec->REG_004 = 0;
  codec->REG_000 |= CODEC_CODEC_IF_EN;

  codec->REG_054 |= CODEC_FAULT_MUTE_DAC_ENABLE;

#ifdef AUDIO_OUTPUT_SWAP
  if (output_swap) {
    codec->REG_0A0 |= CODEC_CODEC_DAC_OUT_SWAP;
  } else {
    codec->REG_0A0 &= ~CODEC_CODEC_DAC_OUT_SWAP;
  }
#endif

  if (first_open) {
#ifdef AUDIO_ANC_FB_MC
    codec->REG_04C = CODEC_DMA_CTRL_MC;
    codec->REG_230 |= CODEC_MC_EN_SEL | CODEC_MC_RATE_SRC_SEL;
#endif

    // ANC zero-crossing
    codec->REG_0D4 |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 |
                      CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
    codec->REG_0D8 |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 |
                      CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;

    // Enable ADC zero-crossing gain adjustment
    for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
      *(&codec->REG_084 + i) |= CODEC_CODEC_ADC_GAIN_SEL_CH0;
    }

    // DRE ini gain and offset
    uint8_t max_gain, ini_gain, dre_offset;
    max_gain = analog_aud_get_max_dre_gain();
    if (max_gain < 0xF) {
      ini_gain = 0xF - max_gain;
    } else {
      ini_gain = 0;
    }
    if (max_gain > 0xF) {
      dre_offset = max_gain - 0xF;
    } else {
      dre_offset = 0;
    }
    codec->REG_0C0 = CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(ini_gain) |
                     CODEC_CODEC_DRE_GAIN_OFFSET_CH0(dre_offset);
    codec->REG_0C8 = CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(ini_gain) |
                     CODEC_CODEC_DRE_GAIN_OFFSET_CH1(dre_offset);
    codec->REG_0E0 = CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH0(0);
    codec->REG_0E8 = CODEC_CODEC_DAC_ANA_GAIN_UPDATE_DELAY_CH1(0);

#ifdef ANC_PROD_TEST
#ifdef AUDIO_ANC_FB_MC
    // Enable ADC + music cancel.
    codec->REG_130 |= CODEC_CODEC_FB_CHECK_KEEP_CH0;
    codec->REG_134 |= CODEC_CODEC_FB_CHECK_KEEP_CH1;
#elif defined(AUDIO_ANC_FB_MC_HW)
    // Enable ADC + music cancel.
    codec->REG_130 |= CODEC_CODEC_FB_CHECK_KEEP_CH0;
#endif
#endif

#if defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL)
    const CODEC_ADC_VOL_T *adc_gain_db;

    adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
    if (adc_gain_db) {
      hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
#ifdef SIDETONE_DEDICATED_ADC_CHAN
      sidetone_adc_gain = *adc_gain_db;
#endif
    }
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
    hal_codec_dac_dc_offset_enable(dac_dc_l, dac_dc_r);
#elif defined(SDM_MUTE_NOISE_SUPPRESSION)
    hal_codec_dac_dc_offset_enable(1, 1);
#endif

#ifdef AUDIO_OUTPUT_SW_GAIN
    const struct CODEC_DAC_VOL_T *vol_tab_ptr;

    // Init gain settings
    vol_tab_ptr = hal_codec_get_dac_volume(0);
    if (vol_tab_ptr) {
      analog_aud_set_dac_gain(vol_tab_ptr->tx_pa_gain);
      hal_codec_set_dig_dac_gain(VALID_DAC_MAP, ZERODB_DIG_DBVAL);
    }
#else
    // Enable DAC zero-crossing gain adjustment
    codec->REG_09C |= CODEC_CODEC_DAC_GAIN_SEL_CH0;
    codec->REG_0A0 |= CODEC_CODEC_DAC_GAIN_SEL_CH1;
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
    // Reset SDM
    hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
    codec->REG_098 |= CODEC_CODEC_DAC_SDM_CLOSE;
#endif

#ifdef SDM_MUTE_NOISE_SUPPRESSION
    codec->REG_098 =
        SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_DITHER_GAIN, 0x10);
#endif

#ifdef __AUDIO_RESAMPLE__
    codec->REG_0E4 &=
        ~(CODEC_CODEC_RESAMPLE_DAC_ENABLE | CODEC_CODEC_RESAMPLE_ADC_ENABLE |
          CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE |
          CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);
#endif

#ifdef CODEC_DSD
    for (i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
      if (codec_adc_sample_rate[i].sample_rate == AUD_SAMPRATE_44100) {
        break;
      }
    }
    hal_codec_set_adc_down((AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3),
                           codec_adc_sample_rate[i].adc_down);
    hal_codec_set_adc_hbf_bypass_cnt(
        (AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3),
        codec_adc_sample_rate[i].bypass_cnt);
#endif

    // Mute DAC when cpu fault occurs
    hal_cmu_codec_set_fault_mask(0x3F);

#ifdef CODEC_TIMER
    // Disable sync stamp auto clear to avoid impacting codec timer capture
    codec->REG_054 &= ~CODEC_STAMP_CLR_USED;
#else
    // Enable sync stamp auto clear
    codec->REG_054 |= CODEC_STAMP_CLR_USED;
#endif
  }

#ifdef CODEC_MIN_PHASE
  if (min_phase_cfg) {
    hal_codec_min_phase_init();
  }
#endif

  return 0;
}

int hal_codec_close(enum HAL_CODEC_ID_T id) {
#ifdef CODEC_MIN_PHASE
  if (min_phase_cfg) {
    hal_codec_min_phase_term();
  }
#endif

  codec->REG_054 &= ~CODEC_FAULT_MUTE_DAC_ENABLE;

  codec->REG_000 &= ~CODEC_CODEC_IF_EN;
  codec->REG_064 &= ~(CODEC_SOFT_RSTN_32K | CODEC_SOFT_RSTN_IIR |
                      CODEC_SOFT_RSTN_RS | CODEC_SOFT_RSTN_DAC |
                      CODEC_SOFT_RSTN_ADC_MASK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
  codec->REG_060 &= ~(CODEC_EN_CLK_ADC_MASK | CODEC_EN_CLK_ADC_ANA_MASK);

  codec_opened = false;

#ifdef CODEC_POWER_DOWN
  hal_cmu_codec_reset_set();
  hal_cmu_codec_clock_disable();
  hal_psc_codec_disable();
#else
  // NEVER reset or power down CODEC registers, for the CODEC driver expects
  // that last configurations still exist in the next stream setup
  hal_cmu_codec_clock_disable();
#endif

  analog_aud_pll_close(ANA_AUD_PLL_USER_CODEC);

  return 0;
}

void hal_codec_crash_mute(void) {
  if (codec_opened) {
    codec->REG_000 &= ~CODEC_CODEC_IF_EN;
  }
}

#ifdef DAC_DRE_ENABLE
static void hal_codec_dac_dre_enable(void) {
  codec->REG_0C0 =
      (codec->REG_0C0 & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH0 |
                          CODEC_CODEC_DRE_DELAY_CH0_MASK |
                          CODEC_CODEC_DRE_INI_ANA_GAIN_CH0_MASK |
                          CODEC_CODEC_DRE_THD_DB_OFFSET_CH0_MASK |
                          CODEC_CODEC_DRE_STEP_MODE_CH0_MASK)) |
      CODEC_CODEC_DRE_DELAY_CH0(dac_dre_cfg.dre_delay) |
      CODEC_CODEC_DRE_INI_ANA_GAIN_CH0(0xF) |
      CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.thd_db_offset) |
      CODEC_CODEC_DRE_THD_DB_OFFSET_CH0(dac_dre_cfg.step_mode) |
      CODEC_CODEC_DRE_ENABLE_CH0;
  codec->REG_0C4 = (codec->REG_0C4 & ~(CODEC_CODEC_DRE_WINDOW_CH0_MASK |
                                       CODEC_CODEC_DRE_AMP_HIGH_CH0_MASK)) |
                   CODEC_CODEC_DRE_WINDOW_CH0(dac_dre_cfg.dre_win) |
                   CODEC_CODEC_DRE_AMP_HIGH_CH0(dac_dre_cfg.amp_high);

  codec->REG_0C8 =
      (codec->REG_0C8 & ~(CODEC_CODEC_DRE_THD_DB_OFFSET_SIGN_CH1 |
                          CODEC_CODEC_DRE_DELAY_CH1_MASK |
                          CODEC_CODEC_DRE_INI_ANA_GAIN_CH1_MASK |
                          CODEC_CODEC_DRE_THD_DB_OFFSET_CH1_MASK |
                          CODEC_CODEC_DRE_STEP_MODE_CH1_MASK)) |
      CODEC_CODEC_DRE_DELAY_CH1(dac_dre_cfg.dre_delay) |
      CODEC_CODEC_DRE_INI_ANA_GAIN_CH1(0xF) |
      CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.thd_db_offset) |
      CODEC_CODEC_DRE_THD_DB_OFFSET_CH1(dac_dre_cfg.step_mode) |
      CODEC_CODEC_DRE_ENABLE_CH1;
  codec->REG_0CC = (codec->REG_0CC & ~(CODEC_CODEC_DRE_WINDOW_CH1_MASK |
                                       CODEC_CODEC_DRE_AMP_HIGH_CH1_MASK)) |
                   CODEC_CODEC_DRE_WINDOW_CH1(dac_dre_cfg.dre_win) |
                   CODEC_CODEC_DRE_AMP_HIGH_CH1(dac_dre_cfg.amp_high);
}

static void hal_codec_dac_dre_disable(void) {
  codec->REG_0C0 &= ~CODEC_CODEC_DRE_ENABLE_CH0;
  codec->REG_0C8 &= ~CODEC_CODEC_DRE_ENABLE_CH1;
}
#endif

#ifdef PERF_TEST_POWER_KEY
static void hal_codec_update_perf_test_power(void) {
  int32_t nominal_vol;
  uint32_t ini_ana_gain;
  int32_t dac_vol;

  if (!codec_opened) {
    return;
  }

  dac_vol = 0;
  if (cur_perft_power == HAL_CODEC_PERF_TEST_30MW) {
    nominal_vol = 0;
    ini_ana_gain = 0;
  } else if (cur_perft_power == HAL_CODEC_PERF_TEST_10MW) {
    nominal_vol = -5;
    ini_ana_gain = 6;
  } else if (cur_perft_power == HAL_CODEC_PERF_TEST_5MW) {
    nominal_vol = -8;
    ini_ana_gain = 0xA;
  } else if (cur_perft_power == HAL_CODEC_PERF_TEST_M60DB) {
    nominal_vol = -60;
    ini_ana_gain = 0xF; // about -11 dB
    dac_vol = -49;
  } else {
    return;
  }

  if (codec->REG_0C0 & CODEC_CODEC_DRE_ENABLE_CH0) {
    dac_vol = nominal_vol;
  } else {
    codec->REG_0C0 = SET_BITFIELD(
        codec->REG_0C0, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
    codec->REG_0C8 = SET_BITFIELD(
        codec->REG_0C8, CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);
  }

#ifdef AUDIO_OUTPUT_SW_GAIN
  hal_codec_set_sw_gain(dac_vol);
#else
  hal_codec_set_dig_dac_gain(VALID_DAC_MAP, dac_vol);
#endif

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
  if (codec_nr_enabled) {
    codec_nr_enabled = false;
    hal_codec_set_noise_reduction(true);
  }
#endif
}

void hal_codec_dac_gain_m60db_check(enum HAL_CODEC_PERF_TEST_POWER_T type) {
  cur_perft_power = type;

  if (!codec_opened || (codec->REG_098 & CODEC_CODEC_DAC_EN) == 0) {
    return;
  }

  hal_codec_update_perf_test_power();
}
#endif

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
void hal_codec_set_noise_reduction(bool enable) {
  uint32_t ini_ana_gain;

  if (codec_nr_enabled == enable) {
    // Avoid corrupting digdac_gain_offset_nr or using an invalid one
    return;
  }

  codec_nr_enabled = enable;

  if (!codec_opened) {
    return;
  }

  // ini_ana_gain=0   -->   0dB
  // ini_ana_gain=0xF --> -11dB
  if (enable) {
    ini_ana_gain =
        GET_BITFIELD(codec->REG_0C0, CODEC_CODEC_DRE_INI_ANA_GAIN_CH0);
    digdac_gain_offset_nr = ((0xF - ini_ana_gain) * 11 + 0xF / 2) / 0xF;
    ini_ana_gain = 0xF;
  } else {
    ini_ana_gain = 0xF - (digdac_gain_offset_nr * 0xF + 11 / 2) / 11;
    digdac_gain_offset_nr = 0;
  }

  codec->REG_0C0 = SET_BITFIELD(codec->REG_0C0,
                                CODEC_CODEC_DRE_INI_ANA_GAIN_CH0, ini_ana_gain);
  codec->REG_0C8 = SET_BITFIELD(codec->REG_0C8,
                                CODEC_CODEC_DRE_INI_ANA_GAIN_CH1, ini_ana_gain);

#ifdef AUDIO_OUTPUT_SW_GAIN
  hal_codec_set_sw_gain(swdac_gain);
#else
  hal_codec_restore_dig_dac_gain();
#endif
}
#endif

void hal_codec_stop_playback_stream(enum HAL_CODEC_ID_T id) {
#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) &&  \
    (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
  // Disable PA
  analog_aud_codec_speaker_enable(false);
#endif

  codec->REG_098 &=
      ~(CODEC_CODEC_DAC_EN | CODEC_CODEC_DAC_EN_CH0 | CODEC_CODEC_DAC_EN_CH1);

#ifdef CODEC_TIMER
  // Reset codec timer
  codec->REG_054 &= ~CODEC_EVENT_FOR_CAPTURE;
#endif

#ifdef DAC_DRE_ENABLE
  hal_codec_dac_dre_disable();
#endif

#if defined(DAC_CLASSG_ENABLE)
  hal_codec_classg_enable(false);
#endif

#ifndef NO_DAC_RESET
  // Reset DAC
  // Avoid DAC outputing noise after it is disabled
  codec->REG_064 &= ~CODEC_SOFT_RSTN_DAC;
  codec->REG_064 |= CODEC_SOFT_RSTN_DAC;
#endif
  codec->REG_060 &= ~CODEC_EN_CLK_DAC;
}

void hal_codec_start_playback_stream(enum HAL_CODEC_ID_T id) {
  codec->REG_060 |= CODEC_EN_CLK_DAC;
#ifndef NO_DAC_RESET
  // Reset DAC
  codec->REG_064 &= ~CODEC_SOFT_RSTN_DAC;
  codec->REG_064 |= CODEC_SOFT_RSTN_DAC;
#endif

#ifdef DAC_DRE_ENABLE
  if (
  //(codec->REG_044 & CODEC_MODE_16BIT_DAC) == 0 &&
#ifdef ANC_APP
      anc_adc_ch_map == 0 &&
#endif
      1) {
    hal_codec_dac_dre_enable();
  }
#endif

#ifdef PERF_TEST_POWER_KEY
  hal_codec_update_perf_test_power();
#endif

#if defined(DAC_CLASSG_ENABLE)
  hal_codec_classg_enable(true);
#endif

#ifdef CODEC_TIMER
  // Enable codec timer and record time by bt event instead of gpio event
  codec->REG_054 =
      (codec->REG_054 & ~CODEC_EVENT_SEL) | CODEC_EVENT_FOR_CAPTURE;
#endif

  if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
    codec->REG_098 |= CODEC_CODEC_DAC_EN_CH0;
  } else {
    codec->REG_098 &= ~CODEC_CODEC_DAC_EN_CH0;
  }
  if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
    codec->REG_098 |= CODEC_CODEC_DAC_EN_CH1;
  } else {
    codec->REG_098 &= ~CODEC_CODEC_DAC_EN_CH1;
  }

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) &&  \
    (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
#if 0
    uint32_t cfg_en;
    uint32_t anc_ff_gain, anc_fb_gain;

    cfg_en = codec->REG_000 & CODEC_DAC_ENABLE;
    anc_ff_gain = codec->REG_0D4;
    anc_fb_gain = codec->REG_0D8;

    if (cfg_en) {
        codec->REG_000 &= ~cfg_en;
    }
    if (anc_ff_gain) {
        codec->REG_0D4 = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
        anc_ff_gain |= CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FF_CH1;
    }
    if (anc_fb_gain) {
        codec->REG_0D8 = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;
        anc_fb_gain = CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH0 | CODEC_CODEC_ANC_MUTE_GAIN_PASS0_FB_CH1;
    }
    osDelay(1);
#endif
#endif

  codec->REG_098 |= CODEC_CODEC_DAC_EN;

#if (defined(AUDIO_OUTPUT_DC_CALIB_ANA) || defined(AUDIO_OUTPUT_DC_CALIB)) &&  \
    (!(defined(__TWS__) || defined(IBRT)) || defined(ANC_APP))
#ifdef AUDIO_OUTPUT_DC_CALIB
  // At least delay 4ms for 8K-sample-rate mute data to arrive at DAC PA
  osDelay(5);
#endif

#if 0
    if (cfg_en) {
        codec->REG_000 |= cfg_en;
    }
    if (anc_ff_gain) {
        codec->REG_0D4 = anc_ff_gain;
    }
    if (anc_fb_gain) {
        codec->REG_0D8 = anc_fb_gain;
    }
#endif

  // Enable PA
  analog_aud_codec_speaker_enable(true);

#ifdef AUDIO_ANC_FB_MC
  if (mc_enabled) {
    uint32_t lock;
    lock = int_lock();
    // MC FIFO and DAC FIFO must be started at the same time
    codec->REG_04C |= CODEC_MC_ENABLE;
    codec->REG_000 |= CODEC_DAC_ENABLE;
    int_unlock(lock);
  }
#endif
#endif
}

#ifdef AF_ADC_I2S_SYNC
static bool _hal_codec_capture_enable_delay = false;

void hal_codec_capture_enable_delay(void) {
  _hal_codec_capture_enable_delay = true;
}

void hal_codec_capture_enable(void) { codec->REG_080 |= CODEC_CODEC_ADC_EN; }
#endif

int hal_codec_start_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream) {
  if (stream == AUD_STREAM_PLAYBACK) {
    // Reset and start DAC
    hal_codec_start_playback_stream(id);
  } else {
#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
    adc_en_map |= (1 << CODEC_ADC_EN_REQ_STREAM);
    if (adc_en_map == (1 << CODEC_ADC_EN_REQ_STREAM))
#endif
    {
      // Reset ADC ANA
      codec->REG_064 &= ~CODEC_SOFT_RSTN_ADC_ANA_MASK;
      codec->REG_064 |= CODEC_SOFT_RSTN_ADC_ANA_MASK;

#ifdef AF_ADC_I2S_SYNC
      if (_hal_codec_capture_enable_delay) {
        _hal_codec_capture_enable_delay = false;
      } else {
        hal_codec_capture_enable();
      }
#else
      codec->REG_080 |= CODEC_CODEC_ADC_EN;
#endif
    }
  }

  return 0;
}

int hal_codec_stop_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream) {
  if (stream == AUD_STREAM_PLAYBACK) {
    // Stop and reset DAC
    hal_codec_stop_playback_stream(id);
  } else {
#if defined(AUDIO_ANC_FB_MC) || defined(CODEC_DSD)
    adc_en_map &= ~(1 << CODEC_ADC_EN_REQ_STREAM);
    if (adc_en_map == 0)
#endif
    {
      codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
#ifdef AF_ADC_I2S_SYNC
      _hal_codec_capture_enable_delay = false;
#endif
    }
  }

  return 0;
}

#ifdef CODEC_DSD
void hal_codec_dsd_enable(void) { dsd_enabled = true; }

void hal_codec_dsd_disable(void) { dsd_enabled = false; }

static void hal_codec_dsd_cfg_start(void) {
#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
  uint32_t vol;
  const CODEC_ADC_VOL_T *adc_gain_db;

  vol = hal_codec_get_mic_chan_volume_level(AUD_CHANNEL_MAP_DIGMIC_CH2);
  adc_gain_db = hal_codec_get_adc_volume(vol);
  if (adc_gain_db) {
    hal_codec_set_dig_adc_gain((AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3),
                               *adc_gain_db);
  }
#endif

  codec->REG_004 |= CODEC_DSD_RX_FIFO_FLUSH | CODEC_DSD_TX_FIFO_FLUSH;
  hal_codec_reg_update_delay();
  codec->REG_004 &= ~(CODEC_DSD_RX_FIFO_FLUSH | CODEC_DSD_TX_FIFO_FLUSH);

  codec->REG_0B8 = CODEC_CODEC_DSD_ENABLE_L | CODEC_CODEC_DSD_ENABLE_R |
                   CODEC_CODEC_DSD_SAMPLE_RATE(dsd_rate_idx);
  codec->REG_048 = CODEC_DSD_IF_EN | CODEC_DSD_ENABLE | CODEC_DSD_DUAL_CHANNEL |
                   CODEC_MODE_24BIT_DSD |
                   /* CODEC_DMA_CTRL_RX_DSD | */ CODEC_DMA_CTRL_TX_DSD |
                   CODEC_DSD_IN_16BIT;

  codec->REG_080 = (codec->REG_080 & ~(CODEC_CODEC_LOOP_SEL_L_MASK |
                                       CODEC_CODEC_LOOP_SEL_R_MASK)) |
                   CODEC_CODEC_ADC_LOOP | CODEC_CODEC_LOOP_SEL_L(2) |
                   CODEC_CODEC_LOOP_SEL_R(3);

  codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, 2);
  codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
  codec->REG_0A8 = SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, 3);
  codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;

  codec->REG_064 &=
      ~(CODEC_SOFT_RSTN_ADC(1 << 2) | CODEC_SOFT_RSTN_ADC(1 << 3));
  codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << 2) | CODEC_SOFT_RSTN_ADC(1 << 3);
  codec->REG_080 |= CODEC_CODEC_ADC_EN_CH2 | CODEC_CODEC_ADC_EN_CH3;

  if (adc_en_map == 0) {
    // Reset ADC free running clock and ADC ANA
    codec->REG_064 &=
        ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
    codec->REG_064 |=
        (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
    codec->REG_080 |= CODEC_CODEC_ADC_EN;
  }
  adc_en_map |= (1 << CODEC_ADC_EN_REQ_DSD);
}

static void hal_codec_dsd_cfg_stop(void) {
  adc_en_map &= ~(1 << CODEC_ADC_EN_REQ_DSD);
  if (adc_en_map == 0) {
    codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
  }

  codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH2 | CODEC_CODEC_ADC_EN_CH3);
  codec->REG_0A4 &=
      ~(CODEC_CODEC_PDM_ADC_SEL_CH2 | CODEC_CODEC_PDM_ADC_SEL_CH3);
  codec->REG_048 = 0;
  codec->REG_0B8 = 0;

  codec->REG_080 &= ~CODEC_CODEC_ADC_LOOP;
}
#endif

#ifdef __AUDIO_RESAMPLE__
void hal_codec_resample_clock_enable(enum AUD_STREAM_T stream) {
  uint32_t clk;
  bool set = false;

  // 192K-24BIT requires 52M clock, and 384K-24BIT requires 104M clock
  if (stream == AUD_STREAM_PLAYBACK) {
    clk = codec_dac_sample_rate[resample_rate_idx[AUD_STREAM_PLAYBACK]]
              .sample_rate *
          RS_CLOCK_FACTOR;
  } else {
    clk = codec_adc_sample_rate[resample_rate_idx[AUD_STREAM_CAPTURE]]
              .sample_rate *
          RS_CLOCK_FACTOR;
  }

  if (rs_clk_map == 0) {
    set = true;
  } else {
    if (resample_clk_freq < clk) {
      set = true;
    }
  }

  if (set) {
    resample_clk_freq = clk;
    hal_cmu_codec_rs_enable(clk);
  }

  rs_clk_map |= (1 << stream);
}

void hal_codec_resample_clock_disable(enum AUD_STREAM_T stream) {
  if (rs_clk_map == 0) {
    return;
  }
  rs_clk_map &= ~(1 << stream);
  if (rs_clk_map == 0) {
    hal_cmu_codec_rs_disable();
  }
}
#endif

static void hal_codec_enable_dig_mic(enum AUD_CHANNEL_MAP_T mic_map) {
  uint32_t phase = 0;
  uint32_t line_map = 0;

  phase = codec->REG_0A8;
  for (int i = 0; i < MAX_DIG_MIC_CH_NUM; i++) {
    if (mic_map & (AUD_CHANNEL_MAP_DIGMIC_CH0 << i)) {
      line_map |= (1 << (i / 2));
    }
    phase = (phase & ~(CODEC_CODEC_PDM_CAP_PHASE_CH0_MASK << (i * 2))) |
            (CODEC_CODEC_PDM_CAP_PHASE_CH0(codec_digmic_phase) << (i * 2));
  }
  codec->REG_0A8 = phase;
  codec->REG_0A4 |= CODEC_CODEC_PDM_ENABLE;
  hal_iomux_set_dig_mic(line_map);
}

static void hal_codec_disable_dig_mic(void) {
  codec->REG_0A4 &= ~CODEC_CODEC_PDM_ENABLE;
}

static void hal_codec_set_ec_down_sel(bool dac_rate_valid) {
  uint8_t dac_factor;
  uint8_t adc_factor;
  uint8_t d, a;
  uint8_t val;
  uint8_t sel = 0;
  bool err = false;

  if (dac_rate_valid) {
    d = codec_rate_idx[AUD_STREAM_PLAYBACK];
    if (codec_dac_sample_rate[d].sample_rate < AUD_SAMPRATE_44100) {
      dac_factor = (6 / codec_dac_sample_rate[d].dac_up) *
                   (codec_dac_sample_rate[d].bypass_cnt + 1);
    } else {
      // SINC to 48K/44.1K
      dac_factor = (6 / 1) * (0 + 1);
    }
    a = codec_rate_idx[AUD_STREAM_CAPTURE];
    adc_factor = (6 / codec_adc_sample_rate[a].adc_down) *
                 (codec_adc_sample_rate[a].bypass_cnt + 1);

    val = dac_factor / adc_factor;
    if (val * adc_factor == dac_factor) {
      if (val == 3) {
        sel = 0;
      } else if (val == 6) {
        sel = 1;
      } else if (val == 1) {
        sel = 2;
      } else if (val == 2) {
        sel = 3;
      } else {
        err = true;
      }
    } else {
      err = true;
    }

    ASSERT(!err, "%s: Invalid EC sample rate: play=%u cap=%u", __FUNCTION__,
           codec_dac_sample_rate[d].sample_rate,
           codec_adc_sample_rate[a].sample_rate);
  } else {
    sel = 0;
  }

  uint32_t ec_mask, ec_val;

  ec_mask = 0;
  ec_val = 0;
  if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
    ec_mask |= CODEC_CODEC_DOWN_SEL_MC_CH0_MASK;
    ec_val |= CODEC_CODEC_DOWN_SEL_MC_CH0(sel);
  }
  if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
    ec_mask |= CODEC_CODEC_DOWN_SEL_MC_CH1_MASK;
    ec_val |= CODEC_CODEC_DOWN_SEL_MC_CH1(sel);
  }
  codec->REG_228 = (codec->REG_228 & ~ec_mask) | ec_val;
}

static void hal_codec_ec_enable(void) {
  uint32_t ec_val;
  bool dac_rate_valid;
  uint8_t a;

  dac_rate_valid = !!(codec->REG_000 & CODEC_DAC_ENABLE);

  hal_codec_set_ec_down_sel(dac_rate_valid);

  // If no normal ADC chan, ADC0 must be enabled
  if ((codec_adc_ch_map & ~EC_ADC_MAP) == 0) {
    a = codec_rate_idx[AUD_STREAM_CAPTURE];
    hal_codec_set_adc_down(AUD_CHANNEL_MAP_CH0,
                           codec_adc_sample_rate[a].adc_down);
    hal_codec_set_adc_hbf_bypass_cnt(AUD_CHANNEL_MAP_CH0,
                                     codec_adc_sample_rate[a].bypass_cnt);
    codec->REG_080 |= CODEC_CODEC_ADC_EN_CH0;
  }

  ec_val = 0;
  if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
    ec_val |= CODEC_CODEC_MC_ENABLE_CH0;
  }
  if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
    ec_val |= CODEC_CODEC_MC_ENABLE_CH1;
  }
  if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
    ec_val |= CODEC_CODEC_RESAMPLE_MC_ENABLE;
    if ((codec_adc_ch_map & EC_ADC_MAP) == EC_ADC_MAP) {
      ec_val |= CODEC_CODEC_RESAMPLE_MC_DUAL_CH;
    }
  }
  codec->REG_228 |= ec_val;
}

static void hal_codec_ec_disable(void) {
  codec->REG_228 &=
      ~(CODEC_CODEC_MC_ENABLE_CH0 | CODEC_CODEC_MC_ENABLE_CH1 |
        CODEC_CODEC_RESAMPLE_MC_ENABLE | CODEC_CODEC_RESAMPLE_MC_DUAL_CH);
  if ((codec_adc_ch_map & ~EC_ADC_MAP) == 0 &&
      (anc_adc_ch_map & AUD_CHANNEL_MAP_CH0) == 0) {
    codec->REG_080 &= ~CODEC_CODEC_ADC_EN_CH0;
  }
}

int hal_codec_start_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream,
                              int dma) {
  uint32_t fifo_flush = 0;

  if (stream == AUD_STREAM_PLAYBACK) {
#ifdef CODEC_DSD
    if (dsd_enabled) {
      hal_codec_dsd_cfg_start();
    }
#endif
#ifdef CODEC_MIN_PHASE
    if (min_phase_cfg & (1 << AUD_STREAM_PLAYBACK)) {
      if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_100 |= CODEC_FIR_STREAM_ENABLE_CH0;
        codec->REG_098 |= CODEC_CODEC_DAC_L_FIR_UPSAMPLE;
      }
      if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_100 |= CODEC_FIR_STREAM_ENABLE_CH1;
        codec->REG_098 |= CODEC_CODEC_DAC_R_FIR_UPSAMPLE;
      }
    }
#endif
#ifdef __AUDIO_RESAMPLE__
    if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
      hal_codec_resample_clock_enable(stream);
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
      enum HAL_CODEC_SYNC_TYPE_T sync_type;

      sync_type =
          GET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL);
      if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
        codec->REG_0E4 =
            SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL,
                         HAL_CODEC_SYNC_TYPE_NONE);
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        hal_codec_reg_update_delay();
        codec->REG_0F4 = resample_phase_float_to_value(1.0f);
        hal_codec_reg_update_delay();
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        hal_codec_reg_update_delay();
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        codec->REG_0E4 = SET_BITFIELD(
            codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, sync_type);
        hal_codec_reg_update_delay();
        codec->REG_0F4 =
            resample_phase_float_to_value(get_playback_resample_phase());
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
      }
#endif
      codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_ENABLE;
    }
#endif
    if ((codec->REG_000 & CODEC_ADC_ENABLE) &&
        (codec_adc_ch_map & EC_ADC_MAP)) {
      hal_codec_set_ec_down_sel(true);
    }
#ifdef AUDIO_ANC_FB_MC
    fifo_flush |= CODEC_MC_FIFO_FLUSH;
#endif
    fifo_flush |= CODEC_TX_FIFO_FLUSH;
    codec->REG_004 |= fifo_flush;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~fifo_flush;
    if (dma) {
      codec->REG_008 = SET_BITFIELD(codec->REG_008, CODEC_CODEC_TX_THRESHOLD,
                                    HAL_CODEC_TX_FIFO_TRIGGER_LEVEL);
      codec->REG_000 |= CODEC_DMACTRL_TX;
      // Delay a little time for DMA to fill the TX FIFO before sending
      for (volatile int i = 0; i < 50; i++)
        ;
    }
#ifdef AUDIO_ANC_FB_MC
    if (mc_dual_chan) {
      codec->REG_04C |= CODEC_DUAL_CHANNEL_MC;
    } else {
      codec->REG_04C &= ~CODEC_DUAL_CHANNEL_MC;
    }
    if (mc_16bit) {
      codec->REG_04C |= CODEC_MODE_16BIT_MC;
    } else {
      codec->REG_04C &= ~CODEC_MODE_16BIT_MC;
    }
    if (adc_en_map == 0) {
      // Reset ADC free running clock and ADC ANA
      codec->REG_064 &=
          ~(RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
      codec->REG_064 |=
          (RSTN_ADC_FREE_RUNNING_CLK | CODEC_SOFT_RSTN_ADC_ANA_MASK);
      codec->REG_080 |= CODEC_CODEC_ADC_EN;
    }
    adc_en_map |= (1 << CODEC_ADC_EN_REQ_MC);
    // If codec function has been enabled, start FIFOs directly;
    // otherwise, start FIFOs after PA is enabled
    if (codec->REG_098 & CODEC_CODEC_DAC_EN) {
      uint32_t lock;
      lock = int_lock();
      // MC FIFO and DAC FIFO must be started at the same time
      codec->REG_04C |= CODEC_MC_ENABLE;
      codec->REG_000 |= CODEC_DAC_ENABLE;
      int_unlock(lock);
    }
    mc_enabled = true;
#else
    codec->REG_000 |= CODEC_DAC_ENABLE;
#endif
  } else {
#ifdef VOICE_DETECTOR_EN
    if ((codec_adc_ch_map & AUD_CHANNEL_MAP_CH4) &&
        (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG)) {
      // Stop vad buffering
      hal_codec_vad_stop();
    }
#endif
#ifdef CODEC_MIN_PHASE
    if (min_phase_cfg & (1 << AUD_STREAM_CAPTURE)) {
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH2) {
        codec->REG_100 |= CODEC_FIR_STREAM_ENABLE_CH2;
        codec->REG_0D0 |= CODEC_CODEC_ADC_FIR_DS_EN_CH2;
      }
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH3) {
        codec->REG_100 |= CODEC_FIR_STREAM_ENABLE_CH3;
        codec->REG_0D0 |= CODEC_CODEC_ADC_FIR_DS_EN_CH3;
      }
    }
#endif
#ifdef __AUDIO_RESAMPLE__
    if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
      hal_codec_resample_clock_enable(stream);
#if (defined(__TWS__) || defined(IBRT)) && defined(ANC_APP)
      enum HAL_CODEC_SYNC_TYPE_T sync_type;

      sync_type =
          GET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL);
      if (sync_type != HAL_CODEC_SYNC_TYPE_NONE) {
        codec->REG_0E4 =
            SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL,
                         HAL_CODEC_SYNC_TYPE_NONE);
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        hal_codec_reg_update_delay();
        codec->REG_0F8 = resample_phase_float_to_value(1.0f);
        hal_codec_reg_update_delay();
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        hal_codec_reg_update_delay();
        codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        codec->REG_0E4 = SET_BITFIELD(
            codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, sync_type);
        hal_codec_reg_update_delay();
        codec->REG_0F8 =
            resample_phase_float_to_value(get_capture_resample_phase());
        codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
      }
#endif
      codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_ENABLE;
    }
#endif
    if (codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
      hal_codec_enable_dig_mic(codec_mic_ch_map);
    }
    if (codec_adc_ch_map & EC_ADC_MAP) {
      hal_codec_ec_enable();
    }
    for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
      if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
        if (i < NORMAL_ADC_CH_NUM &&
            (codec->REG_080 & (CODEC_CODEC_ADC_EN_CH0 << i)) == 0) {
          // Reset ADC channel
          codec->REG_064 &= ~CODEC_SOFT_RSTN_ADC(1 << i);
          codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << i);
          codec->REG_080 |= (CODEC_CODEC_ADC_EN_CH0 << i);
        }
        codec->REG_000 |= (CODEC_ADC_ENABLE_CH0 << i);
      }
    }
    fifo_flush = CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 |
                 CODEC_RX_FIFO_FLUSH_CH2 | CODEC_RX_FIFO_FLUSH_CH3 |
                 CODEC_RX_FIFO_FLUSH_CH4 | CODEC_RX_FIFO_FLUSH_CH5 |
                 CODEC_RX_FIFO_FLUSH_CH6;
    codec->REG_004 |= fifo_flush;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~fifo_flush;
    if (dma) {
      codec->REG_008 = SET_BITFIELD(codec->REG_008, CODEC_CODEC_RX_THRESHOLD,
                                    HAL_CODEC_RX_FIFO_TRIGGER_LEVEL);
      codec->REG_000 |= CODEC_DMACTRL_RX;
    }
    codec->REG_000 |= CODEC_ADC_ENABLE;
  }

  return 0;
}

int hal_codec_stop_interface(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream) {
  uint32_t fifo_flush = 0;

  if (stream == AUD_STREAM_PLAYBACK) {
    codec->REG_000 &= ~CODEC_DAC_ENABLE;
    codec->REG_000 &= ~CODEC_DMACTRL_TX;
#ifdef __AUDIO_RESAMPLE__
    codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_ENABLE;
    hal_codec_resample_clock_disable(stream);
#endif
#ifdef CODEC_MIN_PHASE
    if (min_phase_cfg & (1 << AUD_STREAM_PLAYBACK)) {
      if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
        codec->REG_100 &= ~CODEC_FIR_STREAM_ENABLE_CH0;
        codec->REG_098 &= ~CODEC_CODEC_DAC_L_FIR_UPSAMPLE;
      }
      if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
        codec->REG_100 &= ~CODEC_FIR_STREAM_ENABLE_CH1;
        codec->REG_098 &= ~CODEC_CODEC_DAC_R_FIR_UPSAMPLE;
      }
    }
#endif
#ifdef CODEC_DSD
    hal_codec_dsd_cfg_stop();
    dsd_enabled = false;
#endif
#ifdef AUDIO_ANC_FB_MC
    mc_enabled = false;
    codec->REG_04C &= ~CODEC_MC_ENABLE;
    adc_en_map &= ~(1 << CODEC_ADC_EN_REQ_MC);
    if (adc_en_map == 0) {
      codec->REG_080 &= ~CODEC_CODEC_ADC_EN;
    }
    fifo_flush |= CODEC_MC_FIFO_FLUSH;
#endif
    fifo_flush |= CODEC_TX_FIFO_FLUSH;
    codec->REG_004 |= fifo_flush;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~fifo_flush;
    // Cancel dac sync request
    hal_codec_sync_dac_disable();
    hal_codec_sync_dac_resample_rate_disable();
    hal_codec_sync_dac_gain_disable();
#ifdef NO_DAC_RESET
    // Clean up DAC intermediate states
    osDelay(dac_delay_ms);
#endif
  } else {
    codec->REG_000 &=
        ~(CODEC_ADC_ENABLE | CODEC_ADC_ENABLE_CH0 | CODEC_ADC_ENABLE_CH1 |
          CODEC_ADC_ENABLE_CH2 | CODEC_ADC_ENABLE_CH3 | CODEC_ADC_ENABLE_CH4 |
          CODEC_ADC_ENABLE_CH5 | CODEC_ADC_ENABLE_CH6);
    codec->REG_000 &= ~CODEC_DMACTRL_RX;
    for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
      if (i < NORMAL_ADC_CH_NUM &&
          (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) &&
          (anc_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) == 0) {
        codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << i);
      }
    }
    if (codec_adc_ch_map & EC_ADC_MAP) {
      hal_codec_ec_disable();
    }
    if ((codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) &&
        (anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0) {
      hal_codec_disable_dig_mic();
    }
#ifdef __AUDIO_RESAMPLE__
    codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_ENABLE;
    hal_codec_resample_clock_disable(stream);
#endif
#ifdef CODEC_MIN_PHASE
    if (min_phase_cfg & (1 << AUD_STREAM_CAPTURE)) {
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH2) {
        codec->REG_100 &= ~CODEC_FIR_STREAM_ENABLE_CH2;
        codec->REG_0D0 &= ~CODEC_CODEC_ADC_FIR_DS_EN_CH2;
      }
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH3) {
        codec->REG_100 &= ~CODEC_FIR_STREAM_ENABLE_CH3;
        codec->REG_0D0 &= ~CODEC_CODEC_ADC_FIR_DS_EN_CH3;
      }
    }
#endif
    fifo_flush = CODEC_RX_FIFO_FLUSH_CH0 | CODEC_RX_FIFO_FLUSH_CH1 |
                 CODEC_RX_FIFO_FLUSH_CH2 | CODEC_RX_FIFO_FLUSH_CH3 |
                 CODEC_RX_FIFO_FLUSH_CH4 | CODEC_RX_FIFO_FLUSH_CH5 |
                 CODEC_RX_FIFO_FLUSH_CH6;
    codec->REG_004 |= fifo_flush;
    hal_codec_reg_update_delay();
    codec->REG_004 &= ~fifo_flush;
    // Cancel adc sync request
    hal_codec_sync_adc_disable();
    hal_codec_sync_adc_resample_rate_disable();
    hal_codec_sync_adc_gain_disable();
  }

  return 0;
}

static void hal_codec_set_dac_gain_value(enum AUD_CHANNEL_MAP_T map,
                                         uint32_t val) {
  codec->REG_09C &= ~CODEC_CODEC_DAC_GAIN_UPDATE;
  hal_codec_reg_update_delay();
  if (map & AUD_CHANNEL_MAP_CH0) {
    codec->REG_09C =
        SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_CH0, val);
  }
  if (map & AUD_CHANNEL_MAP_CH1) {
    codec->REG_0A0 =
        SET_BITFIELD(codec->REG_0A0, CODEC_CODEC_DAC_GAIN_CH1, val);
  }
  codec->REG_09C |= CODEC_CODEC_DAC_GAIN_UPDATE;
}

void hal_codec_get_dac_gain(float *left_gain, float *right_gain) {
  struct DAC_GAIN_T {
    int32_t v : 20;
  };

  struct DAC_GAIN_T left;
  struct DAC_GAIN_T right;

  left.v = GET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_CH0);
  right.v = GET_BITFIELD(codec->REG_0A0, CODEC_CODEC_DAC_GAIN_CH1);

  *left_gain = left.v;
  *right_gain = right.v;

  // Gain format: 6.14
  *left_gain /= (1 << 14);
  *right_gain /= (1 << 14);
}

void hal_codec_dac_mute(bool mute) {
  codec_mute[AUD_STREAM_PLAYBACK] = mute;

#ifdef AUDIO_OUTPUT_SW_GAIN
  hal_codec_set_sw_gain(swdac_gain);
#else
  if (mute) {
    hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
  } else {
    hal_codec_restore_dig_dac_gain();
  }
#endif
}

static float db_to_amplitude_ratio(int32_t db) {
  float coef;

  if (db == ZERODB_DIG_DBVAL) {
    coef = 1;
  } else if (db <= MIN_DIG_DBVAL) {
    coef = 0;
  } else {
    if (db > MAX_DIG_DBVAL) {
      db = MAX_DIG_DBVAL;
    }
    coef = db_to_float(db);
  }

  return coef;
}

static float digdac_gain_to_float(int32_t gain) {
  float coef;

#if defined(NOISE_GATING) && defined(NOISE_REDUCTION)
  gain += digdac_gain_offset_nr;
#endif

  coef = db_to_amplitude_ratio(gain);

#ifdef AUDIO_OUTPUT_DC_CALIB
  coef *= dac_dc_gain_attn;
#endif

#ifdef ANC_APP
  coef *= anc_boost_gain_attn;
#endif

#if 0
    static const float thd_attn = 0.982878873; // -0.15dB

    // Ensure that THD is good at max gain
    if (coef > thd_attn) {
        coef = thd_attn;
    }
#endif

  return coef;
}

static void hal_codec_set_dig_dac_gain(enum AUD_CHANNEL_MAP_T map,
                                       int32_t gain) {
  uint32_t val;
  float coef;
  bool mute;

  if (map & AUD_CHANNEL_MAP_CH0) {
    digdac_gain[0] = gain;
  }
  if (map & AUD_CHANNEL_MAP_CH1) {
    digdac_gain[1] = gain;
  }

#ifdef AUDIO_OUTPUT_SW_GAIN
  mute = false;
#else
  mute = codec_mute[AUD_STREAM_PLAYBACK];
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
  if (codec->REG_098 & CODEC_CODEC_DAC_SDM_CLOSE) {
    mute = true;
  }
#endif

  if (mute) {
    val = 0;
  } else {
    coef = digdac_gain_to_float(gain);

    // Gain format: 6.14
    int32_t s_val = (int32_t)(coef * (1 << 14));
    val = __SSAT(s_val, 20);
  }

  hal_codec_set_dac_gain_value(map, val);
}

static void hal_codec_restore_dig_dac_gain(void) {
  if (digdac_gain[0] == digdac_gain[1]) {
    hal_codec_set_dig_dac_gain(VALID_DAC_MAP, digdac_gain[0]);
  } else {
    hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH0, digdac_gain[0]);
    hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH1, digdac_gain[1]);
  }
}

#ifdef AUDIO_OUTPUT_SW_GAIN
static void hal_codec_set_sw_gain(int32_t gain) {
  float coef;
  bool mute;

  swdac_gain = gain;

  mute = codec_mute[AUD_STREAM_PLAYBACK];

  if (mute) {
    coef = 0;
  } else {
    coef = digdac_gain_to_float(gain);
  }

  if (sw_output_coef_callback) {
    sw_output_coef_callback(coef);
  }
}

void hal_codec_set_sw_output_coef_callback(
    HAL_CODEC_SW_OUTPUT_COEF_CALLBACK callback) {
  sw_output_coef_callback = callback;
}
#endif

static void hal_codec_set_adc_gain_value(enum AUD_CHANNEL_MAP_T map,
                                         uint32_t val) {
  uint32_t gain_update = 0;

  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
      *(&codec->REG_084 + i) =
          SET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_GAIN_CH0, val);
      gain_update |= (CODEC_CODEC_ADC_GAIN_UPDATE_CH0 << i);
    }
  }
  codec->REG_09C &= ~gain_update;
  hal_codec_reg_update_delay();
  codec->REG_09C |= gain_update;
}

static void hal_codec_set_dig_adc_gain(enum AUD_CHANNEL_MAP_T map,
                                       int32_t gain) {
  uint32_t val;
  float coef;
  bool mute;
  int i;
  int32_t s_val;

  for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (1 << i)) {
      digadc_gain[i] = gain;
    }
  }

  mute = codec_mute[AUD_STREAM_CAPTURE];

  if (mute) {
    val = 0;
  } else {
#ifdef ANC_APP
    enum AUD_CHANNEL_MAP_T adj_map;
    int32_t anc_gain;

    adj_map = map & anc_adc_gain_offset_map;
    while (adj_map) {
      i = get_msb_pos(adj_map);
      adj_map &= ~(1 << i);
      anc_gain = gain + anc_adc_gain_offset[i];
      coef = db_to_amplitude_ratio(anc_gain);
      coef /= anc_boost_gain_attn;
      // Gain format: 8.12
      s_val = (int32_t)(coef * (1 << 12));
      val = __SSAT(s_val, 20);
      hal_codec_set_adc_gain_value((1 << i), val);
    }

    map &= ~anc_adc_gain_offset_map;
#endif

    if (map) {
      coef = db_to_amplitude_ratio(gain);
#ifdef ANC_APP
      coef /= anc_boost_gain_attn;
#endif
      // Gain format: 8.12
      s_val = (int32_t)(coef * (1 << 12));
      val = __SSAT(s_val, 20);
    } else {
      val = 0;
    }
  }

  hal_codec_set_adc_gain_value(map, val);
}

static void hal_codec_restore_dig_adc_gain(void) {
  int i;

  for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    hal_codec_set_dig_adc_gain((1 << i), digadc_gain[i]);
  }
}

static void POSSIBLY_UNUSED hal_codec_get_adc_gain(enum AUD_CHANNEL_MAP_T map,
                                                   float *gain) {
  struct ADC_GAIN_T {
    int32_t v : 20;
  };

  struct ADC_GAIN_T adc_val;

  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
      adc_val.v =
          GET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_GAIN_CH0);

      *gain = adc_val.v;
      // Gain format: 8.12
      *gain /= (1 << 12);
      return;
    }
  }

  *gain = 0;
}

void hal_codec_adc_mute(bool mute) {
  codec_mute[AUD_STREAM_CAPTURE] = mute;

  if (mute) {
    hal_codec_set_adc_gain_value(NORMAL_ADC_MAP, 0);
  } else {
    hal_codec_restore_dig_adc_gain();
  }
}

int hal_codec_set_chan_vol(enum AUD_STREAM_T stream,
                           enum AUD_CHANNEL_MAP_T ch_map, uint8_t vol) {
  if (stream == AUD_STREAM_PLAYBACK) {
#ifdef AUDIO_OUTPUT_SW_GAIN
    ASSERT(false, "%s: Cannot set play chan vol with AUDIO_OUTPUT_SW_GAIN",
           __func__);
#else
#ifdef SINGLE_CODEC_DAC_VOL
    ASSERT(false, "%s: Cannot set play chan vol with SINGLE_CODEC_DAC_VOL",
           __func__);
#else
    const struct CODEC_DAC_VOL_T *vol_tab_ptr;

    vol_tab_ptr = hal_codec_get_dac_volume(vol);
    if (vol_tab_ptr) {
      if (ch_map & AUD_CHANNEL_MAP_CH0) {
        hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH0,
                                   vol_tab_ptr->sdac_volume);
      }
      if (ch_map & AUD_CHANNEL_MAP_CH1) {
        hal_codec_set_dig_dac_gain(AUD_CHANNEL_MAP_CH1,
                                   vol_tab_ptr->sdac_volume);
      }
    }
#endif
#endif
  } else {
#ifdef SINGLE_CODEC_ADC_VOL
    ASSERT(false, "%s: Cannot set cap chan vol with SINGLE_CODEC_ADC_VOL",
           __func__);
#else
    uint8_t mic_ch, adc_ch;
    enum AUD_CHANNEL_MAP_T map;
    const CODEC_ADC_VOL_T *adc_gain_db;

    adc_gain_db = hal_codec_get_adc_volume(vol);
    if (adc_gain_db) {
      map = ch_map & ~EC_MIC_MAP;
      while (map) {
        mic_ch = get_lsb_pos(map);
        map &= ~(1 << mic_ch);
        adc_ch = hal_codec_get_adc_chan(1 << mic_ch);
        ASSERT(adc_ch < NORMAL_ADC_CH_NUM, "%s: Bad cap ch_map=0x%X (ch=%u)",
               __func__, ch_map, mic_ch);
        hal_codec_set_dig_adc_gain((1 << adc_ch), *adc_gain_db);
      }
    }
#endif
  }

  return 0;
}

static int hal_codec_set_dac_hbf_bypass_cnt(uint32_t cnt) {
  uint32_t bypass = 0;
  uint32_t bypass_mask = CODEC_CODEC_DAC_HBF1_BYPASS |
                         CODEC_CODEC_DAC_HBF2_BYPASS |
                         CODEC_CODEC_DAC_HBF3_BYPASS;

  if (cnt == 0) {
  } else if (cnt == 1) {
    bypass = CODEC_CODEC_DAC_HBF3_BYPASS;
  } else if (cnt == 2) {
    bypass = CODEC_CODEC_DAC_HBF2_BYPASS | CODEC_CODEC_DAC_HBF3_BYPASS;
  } else if (cnt == 3) {
    bypass = CODEC_CODEC_DAC_HBF1_BYPASS | CODEC_CODEC_DAC_HBF2_BYPASS |
             CODEC_CODEC_DAC_HBF3_BYPASS;
  } else {
    ASSERT(false, "%s: Invalid dac bypass cnt: %u", __FUNCTION__, cnt);
  }

  // OSR is fixed to 512
  // codec->REG_098 = SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_OSR_SEL, 2);

  codec->REG_098 = (codec->REG_098 & ~bypass_mask) | bypass;
  return 0;
}

static int hal_codec_set_dac_up(uint32_t val) {
  uint32_t sel = 0;

  if (val == 2) {
    sel = 0;
  } else if (val == 3) {
    sel = 1;
  } else if (val == 4) {
    sel = 2;
  } else if (val == 6) {
    sel = 3;
  } else if (val == 1) {
    sel = 4;
  } else {
    ASSERT(false, "%s: Invalid dac up: %u", __FUNCTION__, val);
  }
  codec->REG_098 = SET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_UP_SEL, sel);
  return 0;
}

static uint32_t POSSIBLY_UNUSED hal_codec_get_dac_up(void) {
  uint32_t sel;

  sel = GET_BITFIELD(codec->REG_098, CODEC_CODEC_DAC_UP_SEL);
  if (sel == 0) {
    return 2;
  } else if (sel == 1) {
    return 3;
  } else if (sel == 2) {
    return 4;
  } else if (sel == 3) {
    return 6;
  } else {
    return 1;
  }
}

static int hal_codec_set_adc_down(enum AUD_CHANNEL_MAP_T map, uint32_t val) {
  uint32_t sel = 0;

  if (val == 3) {
    sel = 0;
  } else if (val == 6) {
    sel = 1;
  } else if (val == 1) {
    sel = 2;
  } else {
    ASSERT(false, "%s: Invalid adc down: %u", __FUNCTION__, val);
  }
  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
      *(&codec->REG_084 + i) = SET_BITFIELD(*(&codec->REG_084 + i),
                                            CODEC_CODEC_ADC_DOWN_SEL_CH0, sel);
    }
  }
  return 0;
}

static int hal_codec_set_adc_hbf_bypass_cnt(enum AUD_CHANNEL_MAP_T map,
                                            uint32_t cnt) {
  uint32_t bypass = 0;
  uint32_t bypass_mask = CODEC_CODEC_ADC_HBF1_BYPASS_CH0 |
                         CODEC_CODEC_ADC_HBF2_BYPASS_CH0 |
                         CODEC_CODEC_ADC_HBF3_BYPASS_CH0;

  if (cnt == 0) {
  } else if (cnt == 1) {
    bypass = CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
  } else if (cnt == 2) {
    bypass = CODEC_CODEC_ADC_HBF2_BYPASS_CH0 | CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
  } else if (cnt == 3) {
    bypass = CODEC_CODEC_ADC_HBF1_BYPASS_CH0 | CODEC_CODEC_ADC_HBF2_BYPASS_CH0 |
             CODEC_CODEC_ADC_HBF3_BYPASS_CH0;
  } else {
    ASSERT(false, "%s: Invalid bypass cnt: %u", __FUNCTION__, cnt);
  }
  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
      *(&codec->REG_084 + i) = (*(&codec->REG_084 + i) & ~bypass_mask) | bypass;
    }
  }
  return 0;
}

#ifdef __AUDIO_RESAMPLE__
static float get_playback_resample_phase(void) {
  return (float)codec_dac_sample_rate[resample_rate_idx[AUD_STREAM_PLAYBACK]]
             .codec_freq /
         hal_cmu_get_crystal_freq();
}

static float get_capture_resample_phase(void) {
  return (float)hal_cmu_get_crystal_freq() /
         codec_adc_sample_rate[resample_rate_idx[AUD_STREAM_CAPTURE]]
             .codec_freq;
}

static uint32_t resample_phase_float_to_value(float phase) {
  if (phase >= 4.0) {
    return (uint32_t)-1;
  } else {
    // Phase format: 2.30
    return (uint32_t)(phase * (1 << 30));
  }
}

static float POSSIBLY_UNUSED resample_phase_value_to_float(uint32_t value) {
  // Phase format: 2.30
  return (float)value / (1 << 30);
}
#endif

#ifdef SIDETONE_ENABLE
static void hal_codec_set_sidetone_adc_chan(enum AUD_CHANNEL_MAP_T chan_map) {
  if (chan_map == AUD_CHANNEL_MAP_CH0) {
    codec->REG_080 &= ~CODEC_CODEC_SIDE_TONE_MIC_SEL;
    codec->REG_078 &= ~CODEC_CODEC_SIDE_TONE_CH_SEL;
  } else if (chan_map == AUD_CHANNEL_MAP_CH2) {
    codec->REG_080 &= ~CODEC_CODEC_SIDE_TONE_MIC_SEL;
    codec->REG_078 |= CODEC_CODEC_SIDE_TONE_CH_SEL;
  } else if (chan_map == AUD_CHANNEL_MAP_CH4) {
    codec->REG_080 |= CODEC_CODEC_SIDE_TONE_MIC_SEL;
  }
}
#endif

int hal_codec_setup_stream(enum HAL_CODEC_ID_T id, enum AUD_STREAM_T stream,
                           const struct HAL_CODEC_CONFIG_T *cfg) {
  int i;
  int rate_idx;
  uint32_t ana_dig_div;
  enum AUD_SAMPRATE_T sample_rate;

  if (stream == AUD_STREAM_PLAYBACK) {
    if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) &
        cfg->set_flag) {
      if (cfg->channel_num == AUD_CHANNEL_NUM_2) {
        if (cfg->channel_map != (AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) {
          TRACE(2, "\n!!! WARNING:%s: Bad play stereo ch map: 0x%X\n", __func__,
                cfg->channel_map);
        }
        codec->REG_044 |= CODEC_DUAL_CHANNEL_DAC;
      } else {
        ASSERT(cfg->channel_num == AUD_CHANNEL_NUM_1, "%s: Bad play ch num: %u",
               __func__, cfg->channel_num);
        // Allow to DMA one channel but output 2 channels
        ASSERT((cfg->channel_map &
                ~(AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1)) == 0,
               "%s: Bad play mono ch map: 0x%X", __func__, cfg->channel_map);
        codec->REG_044 &= ~CODEC_DUAL_CHANNEL_DAC;
      }
      codec_dac_ch_map = AUD_CHANNEL_MAP_CH0 | AUD_CHANNEL_MAP_CH1;
    }

    if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
      if (cfg->bits == AUD_BITS_16) {
        codec->REG_044 =
            (codec->REG_044 & ~CODEC_MODE_32BIT_DAC) | CODEC_MODE_16BIT_DAC;
        codec->REG_04C =
            (codec->REG_04C & ~CODEC_MODE_32BIT_MC) | CODEC_MODE_16BIT_MC;
      } else if (cfg->bits == AUD_BITS_24) {
        codec->REG_044 &= ~(CODEC_MODE_16BIT_DAC | CODEC_MODE_32BIT_DAC);
        codec->REG_04C &= ~(CODEC_MODE_16BIT_MC | CODEC_MODE_32BIT_MC);
      } else if (cfg->bits == AUD_BITS_32) {
        codec->REG_044 =
            (codec->REG_044 & ~CODEC_MODE_16BIT_DAC) | CODEC_MODE_32BIT_DAC;
        codec->REG_04C =
            (codec->REG_04C & ~CODEC_MODE_16BIT_MC) | CODEC_MODE_32BIT_MC;
      } else {
        ASSERT(false, "%s: Bad play bits: %u", __func__, cfg->bits);
      }
    }

    if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
      sample_rate = cfg->sample_rate;
#ifdef CODEC_DSD
      if (dsd_enabled) {
        if (sample_rate == AUD_SAMPRATE_176400) {
          dsd_rate_idx = 0;
        } else if (sample_rate == AUD_SAMPRATE_352800) {
          dsd_rate_idx = 1;
        } else if (sample_rate == AUD_SAMPRATE_705600) {
          dsd_rate_idx = 2;
        } else {
          ASSERT(false, "%s: Bad DSD sample rate: %u", __func__, sample_rate);
        }
        sample_rate = AUD_SAMPRATE_44100;
      }
#endif

      for (i = 0; i < ARRAY_SIZE(codec_dac_sample_rate); i++) {
        if (codec_dac_sample_rate[i].sample_rate == sample_rate) {
          break;
        }
      }
      ASSERT(i < ARRAY_SIZE(codec_dac_sample_rate),
             "%s: Invalid playback sample rate: %u", __func__, sample_rate);
      rate_idx = i;
      ana_dig_div = codec_dac_sample_rate[rate_idx].codec_div /
                    codec_dac_sample_rate[rate_idx].cmu_div;
      ASSERT(ana_dig_div * codec_dac_sample_rate[rate_idx].cmu_div ==
                 codec_dac_sample_rate[rate_idx].codec_div,
             "%s: Invalid playback div for rate %u: codec_div=%u cmu_div=%u",
             __func__, sample_rate, codec_dac_sample_rate[rate_idx].codec_div,
             codec_dac_sample_rate[rate_idx].cmu_div);

      TRACE(2, "[%s] playback sample_rate=%d", __func__, sample_rate);

#ifdef CODEC_TIMER
      cur_codec_freq = codec_dac_sample_rate[rate_idx].codec_freq;
#endif

      codec_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;

#ifdef __AUDIO_RESAMPLE__
      uint32_t mask, val;

      if (hal_cmu_get_audio_resample_status() &&
          codec_dac_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
#ifdef CODEC_TIMER
        cur_codec_freq = CODEC_FREQ_CRYSTAL;
#endif
        if ((codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) == 0 ||
            resample_rate_idx[AUD_STREAM_PLAYBACK] != rate_idx) {
          resample_rate_idx[AUD_STREAM_PLAYBACK] = rate_idx;
          codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
          hal_codec_reg_update_delay();
          codec->REG_0F4 =
              resample_phase_float_to_value(get_playback_resample_phase());
          hal_codec_reg_update_delay();
          codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        }

        mask = CODEC_CODEC_RESAMPLE_DAC_L_ENABLE |
               CODEC_CODEC_RESAMPLE_DAC_R_ENABLE;
        val = 0;
        if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH0) {
          val |= CODEC_CODEC_RESAMPLE_DAC_L_ENABLE;
        }
        if (codec_dac_ch_map & AUD_CHANNEL_MAP_CH1) {
          val |= CODEC_CODEC_RESAMPLE_DAC_R_ENABLE;
        }
      } else {
        mask = CODEC_CODEC_RESAMPLE_DAC_L_ENABLE |
               CODEC_CODEC_RESAMPLE_DAC_R_ENABLE |
               CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
        val = 0;
      }
      codec->REG_0E4 = (codec->REG_0E4 & ~mask) | val;
#endif

      // 8K -> 4ms, 16K -> 2ms, ...
      dac_delay_ms =
          4 / ((sample_rate + AUD_SAMPRATE_8000 / 2) / AUD_SAMPRATE_8000);
      if (dac_delay_ms < 2) {
        dac_delay_ms = 2;
      }

#ifdef __AUDIO_RESAMPLE__
      if (!hal_cmu_get_audio_resample_status())
#endif
      {
#ifdef __AUDIO_RESAMPLE__
        ASSERT(codec_dac_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL,
               "%s: playback sample rate %u is for resample only", __func__,
               sample_rate);
#endif
        analog_aud_freq_pll_config(codec_dac_sample_rate[rate_idx].codec_freq,
                                   codec_dac_sample_rate[rate_idx].codec_div);
        hal_cmu_codec_dac_set_div(codec_dac_sample_rate[rate_idx].cmu_div *
                                  CODEC_FREQ_EXTRA_DIV);
      }
      hal_codec_set_dac_up(codec_dac_sample_rate[rate_idx].dac_up);
      hal_codec_set_dac_hbf_bypass_cnt(
          codec_dac_sample_rate[rate_idx].bypass_cnt);
#ifdef AUDIO_ANC_FB_MC
      codec->REG_04C = SET_BITFIELD(codec->REG_04C, CODEC_MC_DELAY,
                                    codec_dac_sample_rate[rate_idx].mc_delay);
#endif
    }

    if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
      const struct CODEC_DAC_VOL_T *vol_tab_ptr;

      vol_tab_ptr = hal_codec_get_dac_volume(cfg->vol);
      if (vol_tab_ptr) {
#ifdef AUDIO_OUTPUT_SW_GAIN
        hal_codec_set_sw_gain(vol_tab_ptr->sdac_volume);
#else
        analog_aud_set_dac_gain(vol_tab_ptr->tx_pa_gain);
        hal_codec_set_dig_dac_gain(VALID_DAC_MAP, vol_tab_ptr->sdac_volume);
#endif
#ifdef PERF_TEST_POWER_KEY
        // Update performance test power after applying new dac volume
        hal_codec_update_perf_test_power();
#endif
      }
    }
  } else {
    enum AUD_CHANNEL_MAP_T mic_map;
    enum AUD_CHANNEL_MAP_T reserv_map;
    uint8_t cnt;
    uint8_t ch_idx;
    uint32_t cfg_set_mask;
    uint32_t cfg_clr_mask;
#ifdef VOICE_DETECTOR_EN
    uint32_t adc_channel_en = 0;
#endif

    mic_map = 0;
    if ((HAL_CODEC_CONFIG_CHANNEL_MAP | HAL_CODEC_CONFIG_CHANNEL_NUM) &
        cfg->set_flag) {
      codec_adc_ch_map = 0;
      codec_mic_ch_map = 0;
      mic_map = cfg->channel_map;
    }

    if (mic_map) {
      codec_mic_ch_map = mic_map;
      reserv_map = 0;

#ifdef ANC_APP
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
#ifdef ANC_PROD_TEST
      if ((ANC_FF_MIC_CH_L & ~NORMAL_MIC_MAP) ||
          (ANC_FF_MIC_CH_L & (ANC_FF_MIC_CH_L - 1))) {
        ASSERT(false, "Invalid ANC_FF_MIC_CH_L: 0x%04X", ANC_FF_MIC_CH_L);
      }
      if ((ANC_FF_MIC_CH_R & ~NORMAL_MIC_MAP) ||
          (ANC_FF_MIC_CH_R & (ANC_FF_MIC_CH_R - 1))) {
        ASSERT(false, "Invalid ANC_FF_MIC_CH_R: 0x%04X", ANC_FF_MIC_CH_R);
      }
      if (ANC_FF_MIC_CH_L & ANC_FF_MIC_CH_R) {
        ASSERT(
            false,
            "Conflicted ANC_FF_MIC_CH_L (0x%04X) and ANC_FF_MIC_CH_R (0x%04X)",
            ANC_FF_MIC_CH_L, ANC_FF_MIC_CH_R);
      }
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
      if ((ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_L) ||
          (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_R) ||
          (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_L) ||
          (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_R)) {
        ASSERT(false,
               "Conflicted FF MIC (0x%04X/0x%04X) and FB MIC (0x%04X/0x%04X)",
               ANC_FF_MIC_CH_L, ANC_FF_MIC_CH_R, ANC_FB_MIC_CH_L,
               ANC_FB_MIC_CH_R);
      }
#endif
#ifdef VOICE_DETECTOR_EN
      if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_CH4) {
        ASSERT(false, "Conflicted ANC_FF_MIC_CH_L and VAD MIC");
      }
      if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_CH4) {
        ASSERT(false, "Conflicted ANC_FF_MIC_CH_R and VAD MIC");
      }
#endif
#else // !ANC_PROD_TEST
#if (ANC_FF_MIC_CH_L & ~NORMAL_MIC_MAP) ||                                     \
    (ANC_FF_MIC_CH_L & (ANC_FF_MIC_CH_L - 1))
#error "Invalid ANC_FF_MIC_CH_L"
#endif
#if (ANC_FF_MIC_CH_R & ~NORMAL_MIC_MAP) ||                                     \
    (ANC_FF_MIC_CH_R & (ANC_FF_MIC_CH_R - 1))
#error "Invalid ANC_FF_MIC_CH_R"
#endif
#if (ANC_FF_MIC_CH_L & ANC_FF_MIC_CH_R)
#error "Conflicted ANC_FF_MIC_CH_L and ANC_FF_MIC_CH_R"
#endif
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
#if (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_L) ||                                     \
    (ANC_FF_MIC_CH_L & ANC_FB_MIC_CH_R) ||                                     \
    (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_L) || (ANC_FF_MIC_CH_R & ANC_FB_MIC_CH_R)
#error                                                                         \
    "Conflicted ANC_FF_MIC_CH_L and ANC_FF_MIC_CH_R, ANC_FB_MIC_CH_L, ANC_FB_MIC_CH_R"
#endif
#endif
#ifdef VOICE_DETECTOR_EN
#if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_CH4)
#error "Conflicted ANC_FF_MIC_CH_L and VAD MIC"
#endif
#if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_CH4)
#error "Conflicted ANC_FF_MIC_CH_R and VAD MIC"
#endif
#endif
#endif // !ANC_PROD_TEST
      if (mic_map & ANC_FF_MIC_CH_L) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH0;
        mic_map &= ~ANC_FF_MIC_CH_L;
        ch_idx = get_msb_pos(ANC_FF_MIC_CH_L);
        if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
          ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
          codec->REG_0A8 =
              SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH0, ch_idx);
          codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0;
        } else {
          codec->REG_084 =
              SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
          codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH0;
        }
      } else if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_ALL) {
        reserv_map |= AUD_CHANNEL_MAP_CH0;
      }
      if (mic_map & ANC_FF_MIC_CH_R) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH1;
        mic_map &= ~ANC_FF_MIC_CH_R;
        ch_idx = get_msb_pos(ANC_FF_MIC_CH_R);
        if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
          ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
          codec->REG_0A8 =
              SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH1, ch_idx);
          codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH1;
        } else {
          codec->REG_088 =
              SET_BITFIELD(codec->REG_088, CODEC_CODEC_ADC_IN_SEL_CH1, ch_idx);
          codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH1;
        }
      } else if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_ALL) {
        reserv_map |= AUD_CHANNEL_MAP_CH1;
      }
#if defined(SIDETONE_ENABLE) && !defined(SIDETONE_DEDICATED_ADC_CHAN)
      if (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FF_MIC_CH_L) {
        hal_codec_set_sidetone_adc_chan(AUD_CHANNEL_MAP_CH0);
      }
#ifdef ANC_PROD_TEST
      if (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FF_MIC_CH_R) {
        ASSERT(false, "SIDETONE MIC cannot be ANC_FF_MIC_CH_R: 0x%X",
               ANC_FF_MIC_CH_R);
      }
#elif (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FF_MIC_CH_R)
#error "SIDETONE MIC cannot be ANC_FF_MIC_CH_R"
#endif
#endif
#endif

#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
#ifdef ANC_PROD_TEST
      if ((ANC_FB_MIC_CH_L & ~NORMAL_MIC_MAP) ||
          (ANC_FB_MIC_CH_L & (ANC_FB_MIC_CH_L - 1))) {
        ASSERT(false, "Invalid ANC_FB_MIC_CH_L: 0x%04X", ANC_FB_MIC_CH_L);
      }
      if ((ANC_FB_MIC_CH_R & ~NORMAL_MIC_MAP) ||
          (ANC_FB_MIC_CH_R & (ANC_FB_MIC_CH_R - 1))) {
        ASSERT(false, "Invalid ANC_FB_MIC_CH_R: 0x%04X", ANC_FB_MIC_CH_R);
      }
      if (ANC_FB_MIC_CH_L & ANC_FB_MIC_CH_R) {
        ASSERT(
            false,
            "Conflicted ANC_FB_MIC_CH_L (0x%04X) and ANC_FB_MIC_CH_R (0x%04X)",
            ANC_FB_MIC_CH_L, ANC_FB_MIC_CH_R);
      }
#ifdef VOICE_DETECTOR_EN
      if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_CH4) {
        ASSERT(false, "Conflicted ANC_FB_MIC_CH_L and VAD MIC");
      }
      if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_CH4) {
        ASSERT(false, "Conflicted ANC_FB_MIC_CH_R and VAD MIC");
      }
#endif
#else // !ANC_PROD_TEST
#if (ANC_FB_MIC_CH_L & ~NORMAL_MIC_MAP) ||                                     \
    (ANC_FB_MIC_CH_L & (ANC_FB_MIC_CH_L - 1))
#error "Invalid ANC_FB_MIC_CH_L"
#endif
#if (ANC_FB_MIC_CH_R & ~NORMAL_MIC_MAP) ||                                     \
    (ANC_FB_MIC_CH_R & (ANC_FB_MIC_CH_R - 1))
#error "Invalid ANC_FB_MIC_CH_R"
#endif
#if (ANC_FB_MIC_CH_L & ANC_FB_MIC_CH_R)
#error "Conflicted ANC_FB_MIC_CH_L and ANC_FB_MIC_CH_R"
#endif
#ifdef VOICE_DETECTOR_EN
#if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_CH4)
#error "Conflicted ANC_FB_MIC_CH_L and VAD MIC"
#endif
#if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_CH4)
#error "Conflicted ANC_FB_MIC_CH_R and VAD MIC"
#endif
#endif
#endif // !ANC_PROD_TEST
      if (mic_map & ANC_FB_MIC_CH_L) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH2;
        mic_map &= ~ANC_FB_MIC_CH_L;
        ch_idx = get_msb_pos(ANC_FB_MIC_CH_L);
        if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
          ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
          codec->REG_0A8 =
              SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, ch_idx);
          codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
        } else {
          codec->REG_08C =
              SET_BITFIELD(codec->REG_08C, CODEC_CODEC_ADC_IN_SEL_CH2, ch_idx);
          codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH2;
        }
      } else if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_ALL) {
        reserv_map |= AUD_CHANNEL_MAP_CH2;
      }
      if (mic_map & ANC_FB_MIC_CH_R) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH3;
        mic_map &= ~ANC_FB_MIC_CH_R;
        ch_idx = get_msb_pos(ANC_FB_MIC_CH_R);
        if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
          ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
          codec->REG_0A8 =
              SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, ch_idx);
          codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;
        } else {
          codec->REG_090 =
              SET_BITFIELD(codec->REG_090, CODEC_CODEC_ADC_IN_SEL_CH3, ch_idx);
          codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH3;
        }
      } else if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_ALL) {
        reserv_map |= AUD_CHANNEL_MAP_CH3;
      }
#if defined(SIDETONE_ENABLE) && !defined(SIDETONE_DEDICATED_ADC_CHAN)
      if (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FB_MIC_CH_L) {
        hal_codec_set_sidetone_adc_chan(AUD_CHANNEL_MAP_CH2);
      }
#ifdef ANC_PROD_TEST
      if (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FB_MIC_CH_R) {
        ASSERT(false, "SIDETONE MIC cannot be ANC_FB_MIC_CH_R: 0x%X",
               ANC_FB_MIC_CH_R);
      }
#elif (CFG_HW_AUD_SIDETONE_MIC_DEV == ANC_FB_MIC_CH_R)
#error "SIDETONE MIC cannot be ANC_FB_MIC_CH_R"
#endif
#endif
#endif
#endif // ANC_APP

#ifdef CODEC_DSD
      reserv_map |= AUD_CHANNEL_MAP_CH2 | AUD_CHANNEL_MAP_CH3;
#endif

      if (mic_map & AUD_CHANNEL_MAP_CH4) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH4;
        mic_map &= ~AUD_CHANNEL_MAP_CH4;
        codec->REG_094 =
            SET_BITFIELD(codec->REG_094, CODEC_CODEC_ADC_IN_SEL_CH4, 4);
        codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH4;
#if defined(SIDETONE_ENABLE) && !defined(SIDETONE_DEDICATED_ADC_CHAN)
        if (CFG_HW_AUD_SIDETONE_MIC_DEV == AUD_CHANNEL_MAP_CH4) {
          hal_codec_set_sidetone_adc_chan(AUD_CHANNEL_MAP_CH4);
        }
#endif
      }
      if (mic_map & AUD_CHANNEL_MAP_ECMIC_CH0) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH5;
        mic_map &= ~AUD_CHANNEL_MAP_ECMIC_CH0;
        codec->REG_228 &= ~CODEC_CODEC_MC_SEL_CH0;
      }
      if (mic_map & AUD_CHANNEL_MAP_ECMIC_CH1) {
        codec_adc_ch_map |= AUD_CHANNEL_MAP_CH6;
        mic_map &= ~AUD_CHANNEL_MAP_ECMIC_CH1;
        codec->REG_228 |= CODEC_CODEC_MC_SEL_CH1;
      }

      reserv_map |= codec_adc_ch_map;

#ifdef CODEC_MIN_PHASE
      if (min_phase_cfg & (1 << AUD_STREAM_CAPTURE)) {
        if (mic_map && (reserv_map & AUD_CHANNEL_MAP_CH2) == 0) {
          codec_adc_ch_map |= AUD_CHANNEL_MAP_CH2;
          reserv_map |= codec_adc_ch_map;
          ch_idx = get_lsb_pos(mic_map);
          mic_map &= ~(1 << ch_idx);
          if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
            ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
            codec->REG_0A8 =
                SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, ch_idx);
            codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
          } else {
            codec->REG_08C = SET_BITFIELD(codec->REG_08C,
                                          CODEC_CODEC_ADC_IN_SEL_CH2, ch_idx);
            codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH2;
          }
        }
        if (mic_map && (reserv_map & AUD_CHANNEL_MAP_CH3) == 0) {
          codec_adc_ch_map |= AUD_CHANNEL_MAP_CH3;
          reserv_map |= codec_adc_ch_map;
          ch_idx = get_lsb_pos(mic_map);
          mic_map &= ~(1 << ch_idx);
          if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
            ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
            codec->REG_0A8 =
                SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, ch_idx);
            codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;
          } else {
            codec->REG_090 = SET_BITFIELD(codec->REG_090,
                                          CODEC_CODEC_ADC_IN_SEL_CH3, ch_idx);
            codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH3;
          }
        }
      }
#endif

#ifdef SIDETONE_ENABLE
#if defined(SIDETONE_DEDICATED_ADC_CHAN) || defined(SIDETONE_RESERVED_ADC_CHAN)
      if (mic_map & CFG_HW_AUD_SIDETONE_MIC_DEV) {
        enum AUD_CHANNEL_MAP_T st_map = 0;

        // Alloc sidetone adc chan
        if ((reserv_map & AUD_CHANNEL_MAP_CH0) == 0) {
          st_map = AUD_CHANNEL_MAP_CH0;
        } else if ((reserv_map & AUD_CHANNEL_MAP_CH2) == 0) {
          st_map = AUD_CHANNEL_MAP_CH2;
        } else if ((reserv_map & AUD_CHANNEL_MAP_CH4) == 0) {
          st_map = AUD_CHANNEL_MAP_CH4;
        } else {
          ASSERT(false,
                 "%s: Cannot alloc dedicated sidetone adc: reserv_map=0x%X",
                 __func__, reserv_map);
        }
        // Associate mic and sidetone adc
        hal_codec_set_sidetone_adc_chan(st_map);
        ch_idx = get_lsb_pos(CFG_HW_AUD_SIDETONE_MIC_DEV);
        i = get_lsb_pos(st_map);
        if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
          ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
          codec->REG_0A8 =
              (codec->REG_0A8 & ~(CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) |
              (CODEC_CODEC_PDM_MUX_CH0(ch_idx) << (3 * i));
          codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0 << i;
        } else {
          *(&codec->REG_084 + i) = SET_BITFIELD(
              *(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
          codec->REG_0A4 &= ~(CODEC_CODEC_PDM_ADC_SEL_CH0 << i);
        }
#ifdef SIDETONE_DEDICATED_ADC_CHAN
        sidetone_adc_ch_map = st_map;
#else
        mic_map &= ~(1 << ch_idx);
        codec_adc_ch_map |= st_map;
#endif
        // Mark sidetone adc as used
        reserv_map |= st_map;
      }
#endif
#endif

      i = 0;
      while (mic_map && i < NORMAL_ADC_CH_NUM) {
        ASSERT(i < MAX_ANA_MIC_CH_NUM || (mic_map & AUD_CHANNEL_MAP_DIGMIC_ALL),
               "%s: Not enough ana cap chan: mic_map=0x%X adc_map=0x%X "
               "reserv_map=0x%X",
               __func__, mic_map, codec_adc_ch_map, reserv_map);
        ch_idx = get_lsb_pos(mic_map);
        mic_map &= ~(1 << ch_idx);
        while ((reserv_map & (AUD_CHANNEL_MAP_CH0 << i)) &&
               i < NORMAL_ADC_CH_NUM) {
          i++;
        }
#if defined(SIDETONE_ENABLE) && !(defined(SIDETONE_DEDICATED_ADC_CHAN) ||      \
                                  defined(SIDETONE_RESERVED_ADC_CHAN))
        if (CFG_HW_AUD_SIDETONE_MIC_DEV == (1 << ch_idx)) {
          if ((reserv_map & AUD_CHANNEL_MAP_CH0) == 0) {
            i = 0;
          } else if ((reserv_map & AUD_CHANNEL_MAP_CH2) == 0) {
            i = 2;
          } else if ((reserv_map & AUD_CHANNEL_MAP_CH4) == 0) {
            i = 4;
          } else {
            ASSERT(false,
                   "%s: No sidetone adc: reserv_map=0x%X. Try "
                   "SIDETONE_RESERVED_ADC_CHAN",
                   __func__, reserv_map);
          }
          hal_codec_set_sidetone_adc_chan((1 << i));
        }
#endif
        if (i < NORMAL_ADC_CH_NUM) {
          codec_adc_ch_map |= (AUD_CHANNEL_MAP_CH0 << i);
          reserv_map |= codec_adc_ch_map;
          if ((1 << ch_idx) & AUD_CHANNEL_MAP_DIGMIC_ALL) {
            ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
            codec->REG_0A8 =
                (codec->REG_0A8 & ~(CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) |
                (CODEC_CODEC_PDM_MUX_CH0(ch_idx) << (3 * i));
            codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0 << i;
          } else {
            *(&codec->REG_084 + i) = SET_BITFIELD(
                *(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
            codec->REG_0A4 &= ~(CODEC_CODEC_PDM_ADC_SEL_CH0 << i);
          }
          i++;
        }
      }

#if defined(SIDETONE_ENABLE) && !(defined(SIDETONE_DEDICATED_ADC_CHAN) ||      \
                                  defined(SIDETONE_RESERVED_ADC_CHAN))
      if (mic_map) {
        if (reserv_map + 1 < (1 << NORMAL_ADC_CH_NUM)) {
          ASSERT(false,
                 "%s: No adc due to sidetone mic_map=0x%X reserv_map=0x%X. Try "
                 "SIDETONE_RESERVED_ADC_CHAN",
                 __func__, mic_map, reserv_map);
        }
      }
#endif
      ASSERT(mic_map == 0, "%s: Bad cap chan map: 0x%X reserv_map=0x%X",
             __func__, mic_map, reserv_map);
    }

    if (HAL_CODEC_CONFIG_BITS & cfg->set_flag) {
      cfg_set_mask = 0;
      cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0 | CODEC_MODE_16BIT_ADC_CH1 |
                     CODEC_MODE_16BIT_ADC_CH2 | CODEC_MODE_16BIT_ADC_CH3 |
                     CODEC_MODE_16BIT_ADC_CH4 | CODEC_MODE_16BIT_ADC_CH5 |
                     CODEC_MODE_16BIT_ADC_CH6 | CODEC_MODE_24BIT_ADC |
                     CODEC_MODE_32BIT_ADC;
      if (cfg->bits == AUD_BITS_16) {
        cfg_set_mask |= CODEC_MODE_16BIT_ADC_CH0 | CODEC_MODE_16BIT_ADC_CH1 |
                        CODEC_MODE_16BIT_ADC_CH2 | CODEC_MODE_16BIT_ADC_CH3 |
                        CODEC_MODE_16BIT_ADC_CH4 | CODEC_MODE_16BIT_ADC_CH5 |
                        CODEC_MODE_16BIT_ADC_CH6;
      } else if (cfg->bits == AUD_BITS_24) {
        cfg_set_mask |= CODEC_MODE_24BIT_ADC;
      } else if (cfg->bits == AUD_BITS_32) {
        cfg_set_mask |= CODEC_MODE_32BIT_ADC;
      } else {
        ASSERT(false, "%s: Bad cap bits: %d", __func__, cfg->bits);
      }
#ifdef VOICE_DETECTOR_EN
      for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
        adc_channel_en |= (CODEC_ADC_ENABLE_CH0 << i);
      }

      if (((codec->REG_000 & adc_channel_en) != 0) &&
          ((codec->REG_040 & cfg_set_mask) == 0)) {
        ASSERT(false, "%s: Cap bits conflict: %d", __func__, cfg->bits);
      } else
#endif
        codec->REG_040 = (codec->REG_040 & ~cfg_clr_mask) | cfg_set_mask;
    }

    cnt = 0;
    for (i = 0; i < MAX_ADC_CH_NUM; i++) {
      if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
        cnt++;
      }
    }
    ASSERT(cnt == cfg->channel_num,
           "%s: Invalid capture stream chan cfg: map=0x%X num=%u", __func__,
           codec_adc_ch_map, cfg->channel_num);

    if (HAL_CODEC_CONFIG_SAMPLE_RATE & cfg->set_flag) {
      sample_rate = cfg->sample_rate;

      for (i = 0; i < ARRAY_SIZE(codec_adc_sample_rate); i++) {
        if (codec_adc_sample_rate[i].sample_rate == sample_rate) {
          break;
        }
      }
      ASSERT(i < ARRAY_SIZE(codec_adc_sample_rate),
             "%s: Invalid capture sample rate: %d", __func__, sample_rate);
      rate_idx = i;
      ana_dig_div = codec_adc_sample_rate[rate_idx].codec_div /
                    codec_adc_sample_rate[rate_idx].cmu_div;
      ASSERT(ana_dig_div * codec_adc_sample_rate[rate_idx].cmu_div ==
                 codec_adc_sample_rate[rate_idx].codec_div,
             "%s: Invalid catpure div for rate %u: codec_div=%u cmu_div=%u",
             __func__, sample_rate, codec_adc_sample_rate[rate_idx].codec_div,
             codec_adc_sample_rate[rate_idx].cmu_div);

      TRACE(2, "[%s] capture sample_rate=%d", __func__, sample_rate);

#ifdef CODEC_TIMER
      cur_codec_freq = codec_adc_sample_rate[rate_idx].codec_freq;
#endif

      codec_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;

      if (codec_adc_ch_map & EC_ADC_MAP) {
        // If EC enabled, init resample-adc-ch0 to adc0
        codec->REG_0E4 =
            SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_CH0_SEL, 0);
      }

      uint32_t normal_chan_num;

      normal_chan_num = cfg->channel_num;
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH5) {
        normal_chan_num--;
      }
      if (codec_adc_ch_map & AUD_CHANNEL_MAP_CH6) {
        normal_chan_num--;
      }

#ifdef __AUDIO_RESAMPLE__
      uint32_t mask, val;

      if (hal_cmu_get_audio_resample_status() &&
          codec_adc_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL) {
        ASSERT(normal_chan_num <= AUD_CHANNEL_NUM_2,
               "%s: Invalid capture resample chan num: %d/%d map=0x%X",
               __func__, normal_chan_num, cfg->channel_num, cfg->channel_map);
#ifdef CODEC_TIMER
        cur_codec_freq = CODEC_FREQ_CRYSTAL;
#endif
        if ((codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0 ||
            resample_rate_idx[AUD_STREAM_CAPTURE] != rate_idx) {
          resample_rate_idx[AUD_STREAM_CAPTURE] = rate_idx;
          codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
          hal_codec_reg_update_delay();
          codec->REG_0F8 =
              resample_phase_float_to_value(get_capture_resample_phase());
          hal_codec_reg_update_delay();
          codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        }

        mask = CODEC_CODEC_RESAMPLE_ADC_DUAL_CH |
               CODEC_CODEC_RESAMPLE_ADC_CH0_SEL_MASK |
               CODEC_CODEC_RESAMPLE_ADC_CH1_SEL_MASK;
        val = 0;
        cnt = 0;
        for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
          if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
            if (cnt == 0) {
              val |= CODEC_CODEC_RESAMPLE_ADC_CH0_SEL(i);
            } else {
              val |= CODEC_CODEC_RESAMPLE_ADC_CH1_SEL(i);
            }
            cnt++;
          }
        }
        if (normal_chan_num == AUD_CHANNEL_NUM_2) {
          val |= CODEC_CODEC_RESAMPLE_ADC_DUAL_CH;
        }
      } else {
        mask = CODEC_CODEC_RESAMPLE_ADC_DUAL_CH |
               CODEC_CODEC_RESAMPLE_ADC_CH0_SEL_MASK |
               CODEC_CODEC_RESAMPLE_ADC_CH1_SEL_MASK |
               CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
        val = 0;
      }
      codec->REG_0E4 = (codec->REG_0E4 & ~mask) | val;
#endif

      // Echo cancel channels will check the enable signal of resample ADC CH0,
      // even when resample is disabled
      if (codec_adc_ch_map & EC_ADC_MAP) {
        if (normal_chan_num &&
            (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) == 0) {
          for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
            if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
              codec->REG_0E4 = SET_BITFIELD(
                  codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_CH0_SEL, i);
              break;
            }
          }
        }
      }

#ifdef __AUDIO_RESAMPLE__
      if (!hal_cmu_get_audio_resample_status())
#endif
      {
#ifdef __AUDIO_RESAMPLE__
        ASSERT(codec_adc_sample_rate[rate_idx].codec_freq != CODEC_FREQ_CRYSTAL,
               "%s: capture sample rate %u is for resample only", __func__,
               sample_rate);
#endif
        analog_aud_freq_pll_config(codec_adc_sample_rate[rate_idx].codec_freq,
                                   codec_adc_sample_rate[rate_idx].codec_div);
        hal_cmu_codec_adc_set_div(codec_adc_sample_rate[rate_idx].cmu_div *
                                  CODEC_FREQ_EXTRA_DIV);
      }
      hal_codec_set_adc_down(codec_adc_ch_map,
                             codec_adc_sample_rate[rate_idx].adc_down);
      hal_codec_set_adc_hbf_bypass_cnt(
          codec_adc_ch_map, codec_adc_sample_rate[rate_idx].bypass_cnt);
    }

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
    if (HAL_CODEC_CONFIG_VOL & cfg->set_flag) {
#ifdef SINGLE_CODEC_ADC_VOL
      const CODEC_ADC_VOL_T *adc_gain_db;
      adc_gain_db = hal_codec_get_adc_volume(cfg->vol);
      if (adc_gain_db) {
        hal_codec_set_dig_adc_gain(NORMAL_ADC_MAP, *adc_gain_db);
#ifdef SIDETONE_DEDICATED_ADC_CHAN
        sidetone_adc_gain = *adc_gain_db;
        hal_codec_set_dig_adc_gain(sidetone_adc_ch_map,
                                   sidetone_adc_gain + sidetone_gain_offset);
#endif
      }
#else // !SINGLE_CODEC_ADC_VOL
      uint32_t vol;

      mic_map = codec_mic_ch_map;
      while (mic_map) {
        ch_idx = get_lsb_pos(mic_map);
        mic_map &= ~(1 << ch_idx);
        vol = hal_codec_get_mic_chan_volume_level(1 << ch_idx);
        hal_codec_set_chan_vol(AUD_STREAM_CAPTURE, (1 << ch_idx), vol);
      }
#ifdef SIDETONE_DEDICATED_ADC_CHAN
      if (codec_mic_ch_map & CFG_HW_AUD_SIDETONE_MIC_DEV) {
        const CODEC_ADC_VOL_T *adc_gain_db;

        vol = hal_codec_get_mic_chan_volume_level(CFG_HW_AUD_SIDETONE_MIC_DEV);
        adc_gain_db = hal_codec_get_adc_volume(vol);
        if (adc_gain_db) {
          sidetone_adc_gain = *adc_gain_db;
          hal_codec_set_dig_adc_gain(sidetone_adc_ch_map,
                                     sidetone_adc_gain + sidetone_gain_offset);
        }
      }
#endif
#endif // !SINGLE_CODEC_ADC_VOL
    }
#endif
  }

  return 0;
}

int hal_codec_anc_adc_enable(enum ANC_TYPE_T type) {
#ifdef ANC_APP
  enum AUD_CHANNEL_MAP_T map;
  enum AUD_CHANNEL_MAP_T mic_map;
  uint8_t ch_idx;

  map = 0;
  mic_map = 0;
  if (type == ANC_FEEDFORWARD) {
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
    if (ANC_FF_MIC_CH_L) {
      ch_idx = get_msb_pos(ANC_FF_MIC_CH_L);
      if (ANC_FF_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
        ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
        codec->REG_0A8 =
            SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH0, ch_idx);
        codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH0;
      } else {
        codec->REG_084 =
            SET_BITFIELD(codec->REG_084, CODEC_CODEC_ADC_IN_SEL_CH0, ch_idx);
        codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH0;
      }
      map |= AUD_CHANNEL_MAP_CH0;
      mic_map |= ANC_FF_MIC_CH_L;
    }
    if (ANC_FF_MIC_CH_R) {
      ch_idx = get_msb_pos(ANC_FF_MIC_CH_R);
      if (ANC_FF_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
        ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
        codec->REG_0A8 =
            SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH1, ch_idx);
        codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH1;
      } else {
        codec->REG_088 =
            SET_BITFIELD(codec->REG_088, CODEC_CODEC_ADC_IN_SEL_CH1, ch_idx);
        codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH1;
      }
      map |= AUD_CHANNEL_MAP_CH1;
      mic_map |= ANC_FF_MIC_CH_R;
    }
#else
    ASSERT(false, "No ana adc ff ch defined");
#endif
  } else if (type == ANC_FEEDBACK) {
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
    if (ANC_FB_MIC_CH_L) {
      ch_idx = get_msb_pos(ANC_FB_MIC_CH_L);
      if (ANC_FB_MIC_CH_L & AUD_CHANNEL_MAP_DIGMIC_ALL) {
        ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
        codec->REG_0A8 =
            SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH2, ch_idx);
        codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH2;
      } else {
        codec->REG_08C =
            SET_BITFIELD(codec->REG_08C, CODEC_CODEC_ADC_IN_SEL_CH2, ch_idx);
        codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH2;
      }
      map |= AUD_CHANNEL_MAP_CH2;
      mic_map |= ANC_FB_MIC_CH_L;
    }
    if (ANC_FB_MIC_CH_R) {
      ch_idx = get_msb_pos(ANC_FB_MIC_CH_R);
      if (ANC_FB_MIC_CH_R & AUD_CHANNEL_MAP_DIGMIC_ALL) {
        ch_idx = hal_codec_get_digmic_hw_index(ch_idx);
        codec->REG_0A8 =
            SET_BITFIELD(codec->REG_0A8, CODEC_CODEC_PDM_MUX_CH3, ch_idx);
        codec->REG_0A4 |= CODEC_CODEC_PDM_ADC_SEL_CH3;
      } else {
        codec->REG_090 =
            SET_BITFIELD(codec->REG_090, CODEC_CODEC_ADC_IN_SEL_CH3, ch_idx);
        codec->REG_0A4 &= ~CODEC_CODEC_PDM_ADC_SEL_CH3;
      }
      map |= AUD_CHANNEL_MAP_CH3;
      mic_map |= ANC_FB_MIC_CH_R;
    }
#else
    ASSERT(false, "No ana adc fb ch defined");
#endif
  }
  anc_adc_ch_map |= map;
  anc_mic_ch_map |= mic_map;

  if (anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) {
    hal_codec_enable_dig_mic(anc_mic_ch_map);
  }

  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (map & (AUD_CHANNEL_MAP_CH0 << i)) {
      if ((codec->REG_080 & (CODEC_CODEC_ADC_EN_CH0 << i)) == 0) {
        // Reset ADC channel
        codec->REG_064 &= ~CODEC_SOFT_RSTN_ADC(1 << i);
        codec->REG_064 |= CODEC_SOFT_RSTN_ADC(1 << i);
        codec->REG_080 |= (CODEC_CODEC_ADC_EN_CH0 << i);
      }
    }
  }

#ifdef DAC_DRE_ENABLE
  if (anc_adc_ch_map && (codec->REG_098 & CODEC_CODEC_DAC_EN)) {
    hal_codec_dac_dre_disable();
  }
#endif
#endif

  return 0;
}

int hal_codec_anc_adc_disable(enum ANC_TYPE_T type) {
#ifdef ANC_APP
  enum AUD_CHANNEL_MAP_T map;
  enum AUD_CHANNEL_MAP_T mic_map;

  map = 0;
  mic_map = 0;
  if (type == ANC_FEEDFORWARD) {
#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
    if (ANC_FF_MIC_CH_L) {
      map |= AUD_CHANNEL_MAP_CH0;
      mic_map |= ANC_FF_MIC_CH_L;
    }
    if (ANC_FF_MIC_CH_R) {
      map |= AUD_CHANNEL_MAP_CH1;
      mic_map |= ANC_FF_MIC_CH_R;
    }
#endif
  } else if (type == ANC_FEEDBACK) {
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
    if (ANC_FB_MIC_CH_L) {
      map |= AUD_CHANNEL_MAP_CH2;
      mic_map |= ANC_FB_MIC_CH_L;
    }
    if (ANC_FB_MIC_CH_R) {
      map |= AUD_CHANNEL_MAP_CH3;
      mic_map |= ANC_FB_MIC_CH_R;
    }
#endif
  }
  anc_adc_ch_map &= ~map;
  anc_mic_ch_map &= ~mic_map;

  if ((anc_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0 &&
      ((codec_mic_ch_map & AUD_CHANNEL_MAP_DIGMIC_ALL) == 0 ||
       (codec->REG_000 & CODEC_ADC_ENABLE) == 0)) {
    hal_codec_disable_dig_mic();
  }

  for (int i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if ((map & (AUD_CHANNEL_MAP_CH0 << i)) == 0) {
      continue;
    }
    if (codec->REG_000 & CODEC_ADC_ENABLE) {
      if (codec_adc_ch_map & (AUD_CHANNEL_MAP_CH0 << i)) {
        continue;
      }
      if (i == 0 &&
          (codec->REG_228 &
           (CODEC_CODEC_MC_ENABLE_CH0 | CODEC_CODEC_MC_ENABLE_CH1)) &&
          (codec_adc_ch_map & ~EC_ADC_MAP) == 0) {
        continue;
      }
    }
    codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << i);
  }

#ifdef DAC_DRE_ENABLE
  if (anc_adc_ch_map == 0 && (codec->REG_098 & CODEC_CODEC_DAC_EN) &&
      //(codec->REG_044 & CODEC_MODE_16BIT_DAC) == 0 &&
      1) {
    hal_codec_dac_dre_enable();
  }
#endif
#endif

  return 0;
}

enum AUD_SAMPRATE_T hal_codec_anc_convert_rate(enum AUD_SAMPRATE_T rate) {
  if (hal_cmu_get_audio_resample_status()) {
    return AUD_SAMPRATE_50781;
  } else if (CODEC_FREQ_48K_SERIES / rate * rate == CODEC_FREQ_48K_SERIES) {
    return AUD_SAMPRATE_48000;
  } else /* if (CODEC_FREQ_44_1K_SERIES / rate * rate ==
            CODEC_FREQ_44_1K_SERIES) */
  {
    return AUD_SAMPRATE_44100;
  }
}

int hal_codec_anc_dma_enable(enum HAL_CODEC_ID_T id) { return 0; }

int hal_codec_anc_dma_disable(enum HAL_CODEC_ID_T id) { return 0; }

int hal_codec_aux_mic_dma_enable(enum HAL_CODEC_ID_T id) { return 0; }

int hal_codec_aux_mic_dma_disable(enum HAL_CODEC_ID_T id) { return 0; }

uint32_t hal_codec_get_alg_dac_shift(void) { return 0; }

#ifdef ANC_APP
void hal_codec_set_anc_boost_gain_attn(float attn) {
  anc_boost_gain_attn = attn;

#ifdef AUDIO_OUTPUT_SW_GAIN
  hal_codec_set_sw_gain(swdac_gain);
#else
  hal_codec_restore_dig_dac_gain();
#endif
  hal_codec_restore_dig_adc_gain();
}

void hal_codec_apply_anc_adc_gain_offset(enum ANC_TYPE_T type, int8_t offset_l,
                                         int8_t offset_r) {
  enum AUD_CHANNEL_MAP_T map_l, map_r;
  enum AUD_CHANNEL_MAP_T ch_map;
  uint8_t ch_idx;

  if (analog_debug_get_anc_calib_mode()) {
    return;
  }

  map_l = 0;
  map_r = 0;

#if defined(ANC_FF_MIC_CH_L) || defined(ANC_FF_MIC_CH_R)
  if (type & ANC_FEEDFORWARD) {
    if (ANC_FF_MIC_CH_L) {
      map_l |= AUD_CHANNEL_MAP_CH0;
    }
    if (ANC_FF_MIC_CH_R) {
      map_r |= AUD_CHANNEL_MAP_CH1;
    }
  }
#endif
#if defined(ANC_FB_MIC_CH_L) || defined(ANC_FB_MIC_CH_R)
  if (type & ANC_FEEDBACK) {
    if (ANC_FB_MIC_CH_L) {
      map_l |= AUD_CHANNEL_MAP_CH2;
    }
    if (ANC_FB_MIC_CH_R) {
      map_r |= AUD_CHANNEL_MAP_CH3;
    }
  }
#endif

  if (map_l) {
    ch_map = map_l;
    while (ch_map) {
      ch_idx = get_msb_pos(ch_map);
      ch_map &= ~(1 << ch_idx);
      anc_adc_gain_offset[ch_idx] = offset_l;
    }
    if (offset_l) {
      anc_adc_gain_offset_map |= map_l;
    } else {
      anc_adc_gain_offset_map &= ~map_l;
    }
  }
  if (map_r) {
    ch_map = map_r;
    while (ch_map) {
      ch_idx = get_msb_pos(ch_map);
      ch_map &= ~(1 << ch_idx);
      anc_adc_gain_offset[ch_idx] = offset_r;
    }
    if (offset_r) {
      anc_adc_gain_offset_map |= map_r;
    } else {
      anc_adc_gain_offset_map &= ~map_r;
    }
  }
  if (map_l || map_r) {
    hal_codec_restore_dig_adc_gain();
  }
}
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB
void hal_codec_set_dac_dc_gain_attn(float attn) { dac_dc_gain_attn = attn; }

void hal_codec_set_dac_dc_offset(int16_t dc_l, int16_t dc_r) {
  // DC calib values are based on 16-bit, but hardware compensation is based on
  // 24-bit
  dac_dc_l = dc_l << 8;
  dac_dc_r = dc_r << 8;
#ifdef SDM_MUTE_NOISE_SUPPRESSION
  if (dac_dc_l == 0) {
    dac_dc_l = 1;
  }
  if (dac_dc_r == 0) {
    dac_dc_r = 1;
  }
#endif
}
#endif

void hal_codec_set_dac_reset_callback(HAL_CODEC_DAC_RESET_CALLBACK callback) {
  // dac_reset_callback = callback;
}

static uint32_t POSSIBLY_UNUSED
hal_codec_get_adc_chan(enum AUD_CHANNEL_MAP_T mic_map) {
  uint8_t adc_ch;
  uint8_t mic_ch;
  uint8_t digmic_ch0;
  uint8_t en_ch;
  bool digmic;
  int i;

  adc_ch = MAX_ADC_CH_NUM;

  mic_ch = get_lsb_pos(mic_map);

  if (((1 << mic_ch) & codec_mic_ch_map) == 0) {
    return adc_ch;
  }

  digmic_ch0 = get_lsb_pos(AUD_CHANNEL_MAP_DIGMIC_CH0);

  if (mic_ch >= digmic_ch0) {
    mic_ch -= digmic_ch0;
    digmic = true;
  } else {
    digmic = false;
  }

  for (i = 0; i < NORMAL_ADC_CH_NUM; i++) {
    if (codec_adc_ch_map & (1 << i)) {
      if (digmic ^ !!(codec->REG_0A4 & (CODEC_CODEC_PDM_ADC_SEL_CH0 << i))) {
        continue;
      }
      if (digmic) {
        en_ch = (codec->REG_0A8 & (CODEC_CODEC_PDM_MUX_CH0_MASK << (3 * i))) >>
                (CODEC_CODEC_PDM_MUX_CH0_SHIFT + 3 * i);
      } else {
        en_ch =
            GET_BITFIELD(*(&codec->REG_084 + i), CODEC_CODEC_ADC_IN_SEL_CH0);
      }
      if (mic_ch == en_ch) {
        adc_ch = i;
        break;
      }
    }
  }

  return adc_ch;
}

void hal_codec_sidetone_enable(void) {
#ifdef SIDETONE_ENABLE
#if (CFG_HW_AUD_SIDETONE_MIC_DEV & (CFG_HW_AUD_SIDETONE_MIC_DEV - 1))
#error "Invalid CFG_HW_AUD_SIDETONE_MIC_DEV: only 1 mic can be defined"
#endif
#if (CFG_HW_AUD_SIDETONE_MIC_DEV == 0) ||                                      \
    (CFG_HW_AUD_SIDETONE_MIC_DEV & ~NORMAL_MIC_MAP)
#error "Invalid CFG_HW_AUD_SIDETONE_MIC_DEV: bad mic channel"
#endif
  int gain = CFG_HW_AUD_SIDETONE_GAIN_DBVAL;
  uint32_t val;

#ifdef SIDETONE_DEDICATED_ADC_CHAN
  sidetone_gain_offset = 0;
  if (gain > MAX_SIDETONE_DBVAL) {
    sidetone_gain_offset = gain - MAX_SIDETONE_DBVAL;
  } else if (gain < MIN_SIDETONE_DBVAL) {
    sidetone_gain_offset = gain - MIN_SIDETONE_DBVAL;
  }
#endif

  if (gain > MAX_SIDETONE_DBVAL) {
    gain = MAX_SIDETONE_DBVAL;
  } else if (gain < MIN_SIDETONE_DBVAL) {
    gain = MIN_SIDETONE_DBVAL;
  }

  val = MIN_SIDETONE_REGVAL + (gain - MIN_SIDETONE_DBVAL) / SIDETONE_DBVAL_STEP;

  codec->REG_080 =
      SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN, val);

#ifdef SIDETONE_DEDICATED_ADC_CHAN
  uint8_t adc_ch;

  adc_ch = get_lsb_pos(sidetone_adc_ch_map);
  if (adc_ch >= NORMAL_ADC_CH_NUM) {
    return;
  }

  hal_codec_set_dig_adc_gain(sidetone_adc_ch_map,
                             sidetone_adc_gain + sidetone_gain_offset);
#ifdef CFG_HW_AUD_SIDETONE_GAIN_RAMP
  hal_codec_get_adc_gain(sidetone_adc_ch_map, &sidetone_ded_chan_coef);
  hal_codec_set_dig_adc_gain(sidetone_adc_ch_map, MIN_DIG_DBVAL);
#endif
  codec->REG_080 |= (CODEC_CODEC_ADC_EN_CH0 << adc_ch);

#ifdef CFG_HW_AUD_SIDETONE_IIR_INDEX
#if (CFG_HW_AUD_SIDETONE_IIR_INDEX >= ADC_IIR_CH_NUM + 0UL)
#error "Invalid CFG_HW_AUD_SIDETONE_IIR_INDEX"
#endif
  uint32_t mask;

  if (CFG_HW_AUD_SIDETONE_IIR_INDEX == 0) {
    mask = CODEC_CODEC_ADC_IIR_CH0_SEL_MASK;
    val = CODEC_CODEC_ADC_IIR_CH0_SEL(adc_ch);
  } else {
    mask = CODEC_CODEC_ADC_IIR_CH1_SEL_MASK;
    val = CODEC_CODEC_ADC_IIR_CH1_SEL(adc_ch);
  }
  codec->REG_078 = (codec->REG_078 & ~mask) | val;
#endif
#endif
#endif
}

void hal_codec_sidetone_disable(void) {
#ifdef SIDETONE_ENABLE
  codec->REG_080 = SET_BITFIELD(codec->REG_080, CODEC_CODEC_SIDE_TONE_GAIN,
                                MUTE_SIDETONE_REGVAL);
#ifdef SIDETONE_DEDICATED_ADC_CHAN
  if (sidetone_adc_ch_map) {
    uint8_t adc_ch;

    adc_ch = get_lsb_pos(sidetone_adc_ch_map);
    codec->REG_080 &= ~(CODEC_CODEC_ADC_EN_CH0 << adc_ch);
  }
#endif
#endif
}

int hal_codec_sidetone_gain_ramp_up(float step) {
  int ret = 0;
#ifdef CFG_HW_AUD_SIDETONE_GAIN_RAMP
  float coef;
  uint32_t val;

  hal_codec_get_adc_gain(sidetone_adc_ch_map, &coef);
  coef += step;
  if (coef >= sidetone_ded_chan_coef) {
    coef = sidetone_ded_chan_coef;
    ret = 1;
  }
  // Gain format: 8.12
  int32_t s_val = (int32_t)(coef * (1 << 12));
  val = __SSAT(s_val, 20);
  hal_codec_set_adc_gain_value(sidetone_adc_ch_map, val);

#endif
  return ret;
}

int hal_codec_sidetone_gain_ramp_down(float step) {
  int ret = 0;
#ifdef CFG_HW_AUD_SIDETONE_GAIN_RAMP
  float coef;
  uint32_t val;

  hal_codec_get_adc_gain(sidetone_adc_ch_map, &coef);
  coef -= step;
  if (coef <= 0) {
    coef = 0;
    ret = 1;
  }

  // Gain format: 8.12
  int32_t s_val = (int32_t)(coef * (1 << 12));
  val = __SSAT(s_val, 20);
  hal_codec_set_adc_gain_value(sidetone_adc_ch_map, val);
#endif
  return ret;
}

void hal_codec_select_adc_iir_mic(uint32_t index,
                                  enum AUD_CHANNEL_MAP_T mic_map) {
  uint32_t mask, val;
  uint8_t adc_ch;

  ASSERT(index < ADC_IIR_CH_NUM, "%s: Bad index=%u", __func__, index);
  ASSERT(mic_map && (mic_map & (mic_map - 1)) == 0, "%s: Bad mic_map=0x%X",
         __func__, mic_map);
#ifdef CFG_HW_AUD_SIDETONE_IIR_INDEX
  ASSERT(index != CFG_HW_AUD_SIDETONE_IIR_INDEX,
         "%s: Adc iir index conflicts with sidetone", __func__);
#endif

  adc_ch = hal_codec_get_adc_chan(mic_map);
  if (index == 0) {
    mask = CODEC_CODEC_ADC_IIR_CH0_SEL_MASK;
    val = CODEC_CODEC_ADC_IIR_CH0_SEL(adc_ch);
  } else {
    mask = CODEC_CODEC_ADC_IIR_CH1_SEL_MASK;
    val = CODEC_CODEC_ADC_IIR_CH1_SEL(adc_ch);
  }
  codec->REG_078 = (codec->REG_078 & ~mask) | val;
}

void hal_codec_min_phase_mode_enable(enum AUD_STREAM_T stream) {
#ifdef CODEC_MIN_PHASE
  if (min_phase_cfg == 0 && codec_opened) {
    hal_codec_min_phase_init();
  }

  min_phase_cfg |= (1 << stream);
#endif
}

void hal_codec_min_phase_mode_disable(enum AUD_STREAM_T stream) {
#ifdef CODEC_MIN_PHASE
  min_phase_cfg &= ~(1 << stream);

  if (min_phase_cfg == 0 && codec_opened) {
    hal_codec_min_phase_term();
  }
#endif
}

void hal_codec_sync_dac_enable(enum HAL_CODEC_SYNC_TYPE_T type) {
#if defined(ANC_APP)
  // hal_codec_sync_dac_resample_rate_enable(type);
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_DAC_ENABLE_SEL, type);
#else
  codec->REG_054 =
      SET_BITFIELD(codec->REG_054, CODEC_CODEC_DAC_ENABLE_SEL, type);
#endif
}

void hal_codec_sync_dac_disable(void) {
#if defined(ANC_APP)
  // hal_codec_sync_dac_resample_rate_disable();
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_DAC_ENABLE_SEL,
                                HAL_CODEC_SYNC_TYPE_NONE);
#else
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_DAC_ENABLE_SEL,
                                HAL_CODEC_SYNC_TYPE_NONE);
#endif
}

void hal_codec_sync_adc_enable(enum HAL_CODEC_SYNC_TYPE_T type) {
#if defined(ANC_APP)
  // hal_codec_sync_adc_resample_rate_enable(type);
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_ADC_ENABLE_SEL, type);
#else
  codec->REG_054 =
      SET_BITFIELD(codec->REG_054, CODEC_CODEC_ADC_ENABLE_SEL, type);
#endif
}

void hal_codec_sync_adc_disable(void) {
#if defined(ANC_APP)
  // hal_codec_sync_adc_resample_rate_disable();
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_ADC_ENABLE_SEL,
                                HAL_CODEC_SYNC_TYPE_NONE);
#else
  codec->REG_054 = SET_BITFIELD(codec->REG_054, CODEC_CODEC_ADC_ENABLE_SEL,
                                HAL_CODEC_SYNC_TYPE_NONE);
#endif
}

void hal_codec_sync_dac_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type) {
  codec->REG_0E4 =
      SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_resample_rate_disable(void) {
  codec->REG_0E4 =
      SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_DAC_TRIGGER_SEL,
                   HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_resample_rate_enable(enum HAL_CODEC_SYNC_TYPE_T type) {
  codec->REG_0E4 =
      SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL, type);
}

void hal_codec_sync_adc_resample_rate_disable(void) {
  codec->REG_0E4 =
      SET_BITFIELD(codec->REG_0E4, CODEC_CODEC_RESAMPLE_ADC_TRIGGER_SEL,
                   HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_dac_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type) {
  codec->REG_09C =
      SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL, type);
}

void hal_codec_sync_dac_gain_disable(void) {
  codec->REG_09C =
      SET_BITFIELD(codec->REG_09C, CODEC_CODEC_DAC_GAIN_TRIGGER_SEL,
                   HAL_CODEC_SYNC_TYPE_NONE);
}

void hal_codec_sync_adc_gain_enable(enum HAL_CODEC_SYNC_TYPE_T type) {}

void hal_codec_sync_adc_gain_disable(void) {}

void hal_codec_gpio_trigger_debounce_enable(void) {
  if (codec_opened) {
    codec->REG_054 |= CODEC_GPIO_TRIGGER_DB_ENABLE;
  }
}

void hal_codec_gpio_trigger_debounce_disable(void) {
  if (codec_opened) {
    codec->REG_054 &= ~CODEC_GPIO_TRIGGER_DB_ENABLE;
  }
}

#ifdef CODEC_TIMER
uint32_t hal_codec_timer_get(void) {
  if (codec_opened) {
    return codec->REG_050;
  }

  return 0;
}

uint32_t hal_codec_timer_ticks_to_us(uint32_t ticks) {
  uint32_t timer_freq;

  timer_freq = cur_codec_freq / 4 / CODEC_FREQ_EXTRA_DIV;

  return (uint32_t)((float)ticks * 1000000 / timer_freq);
}

void hal_codec_timer_trigger_read(void) {
  if (codec_opened) {
    codec->REG_078 ^= CODEC_GET_CNT_TRIG;
    hal_codec_reg_update_delay();
  }
}
#endif

#ifdef AUDIO_OUTPUT_DC_CALIB_ANA
int hal_codec_dac_sdm_reset_set(void) {
  if (codec_opened) {
    hal_codec_set_dac_gain_value(VALID_DAC_MAP, 0);
    if (codec->REG_098 & CODEC_CODEC_DAC_EN) {
      osDelay(dac_delay_ms);
    }
    for (int i = 0x200; i >= 0; i -= 0x100) {
      hal_codec_dac_dc_offset_enable(i, i);
      osDelay(1);
    }
    codec->REG_098 |= CODEC_CODEC_DAC_SDM_CLOSE;
    osDelay(1);
  }

  return 0;
}

int hal_codec_dac_sdm_reset_clear(void) {
  if (codec_opened) {
    osDelay(1);
    codec->REG_098 &= ~CODEC_CODEC_DAC_SDM_CLOSE;
    for (int i = 0x100; i <= 0x300; i += 0x100) {
      hal_codec_dac_dc_offset_enable(i, i);
      osDelay(1);
    }
    hal_codec_restore_dig_dac_gain();
  }

  return 0;
}
#endif

void hal_codec_tune_resample_rate(enum AUD_STREAM_T stream, float ratio) {
#ifdef __AUDIO_RESAMPLE__
  uint32_t val;

  if (!codec_opened) {
    return;
  }

  if (stream == AUD_STREAM_PLAYBACK) {
    if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE) {
      codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
      hal_codec_reg_update_delay();
      val = resample_phase_float_to_value(get_playback_resample_phase());
      val += (int)(val * ratio);
      codec->REG_0F4 = val;
      hal_codec_reg_update_delay();
      codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
    }
  } else {
    if (codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE) {
      codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
      hal_codec_reg_update_delay();
      val = resample_phase_float_to_value(get_capture_resample_phase());
      val -= (int)(val * ratio);
      codec->REG_0F8 = val;
      hal_codec_reg_update_delay();
      codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
    }
  }
#endif
}

void hal_codec_tune_both_resample_rate(float ratio) {
#ifdef __AUDIO_RESAMPLE__
  bool update[2];
  uint32_t val[2];
  uint32_t lock;

  if (!codec_opened) {
    return;
  }

  update[0] = !!(codec->REG_0E4 & CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE);
  update[1] = !!(codec->REG_0E4 & CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE);

  val[0] = val[1] = 0;

  if (update[0]) {
    codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
    val[0] = resample_phase_float_to_value(get_playback_resample_phase());
    val[0] += (int)(val[0] * ratio);
  }
  if (update[1]) {
    codec->REG_0E4 &= ~CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
    val[1] = resample_phase_float_to_value(get_capture_resample_phase());
    val[1] -= (int)(val[1] * ratio);
  }

  hal_codec_reg_update_delay();

  if (update[0]) {
    codec->REG_0F4 = val[0];
  }
  if (update[1]) {
    codec->REG_0F8 = val[1];
  }

  hal_codec_reg_update_delay();

  lock = int_lock();
  if (update[0]) {
    codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_DAC_PHASE_UPDATE;
  }
  if (update[1]) {
    codec->REG_0E4 |= CODEC_CODEC_RESAMPLE_ADC_PHASE_UPDATE;
  }
  int_unlock(lock);
#endif
}

int hal_codec_select_clock_out(uint32_t cfg) {
  uint32_t lock;
  int ret = 1;

  lock = int_lock();

  if (codec_opened) {
    codec->REG_060 = SET_BITFIELD(codec->REG_060, CODEC_CFG_CLK_OUT, cfg);
    ret = 0;
  }

  int_unlock(lock);

  return ret;
}

#ifdef AUDIO_ANC_FB_MC
void hal_codec_setup_mc(enum AUD_CHANNEL_NUM_T channel_num,
                        enum AUD_BITS_T bits) {
  if (channel_num == AUD_CHANNEL_NUM_2) {
    mc_dual_chan = true;
  } else {
    mc_dual_chan = false;
  }

  if (bits <= AUD_BITS_16) {
    mc_16bit = true;
  } else {
    mc_16bit = false;
  }
}
#endif

void hal_codec_swap_output(bool swap) {
#ifdef AUDIO_OUTPUT_SWAP
  output_swap = swap;

  if (codec_opened) {
    if (output_swap) {
      codec->REG_0A0 |= CODEC_CODEC_DAC_OUT_SWAP;
    } else {
      codec->REG_0A0 &= ~CODEC_CODEC_DAC_OUT_SWAP;
    }
  }
#endif
}

int hal_codec_config_digmic_phase(uint8_t phase) {
#ifdef ANC_PROD_TEST
  codec_digmic_phase = phase;
#endif
  return 0;
}

static void hal_codec_general_irq_handler(void) {
  uint32_t status;

  status = codec->REG_00C;
  codec->REG_00C = status;

  status &= codec->REG_010;

  for (int i = 0; i < CODEC_IRQ_TYPE_QTY; i++) {
    if (codec_irq_callback[i]) {
      codec_irq_callback[i](status);
    }
  }
}

static void hal_codec_set_irq_handler(enum CODEC_IRQ_TYPE_T type,
                                      HAL_CODEC_IRQ_CALLBACK cb) {
  uint32_t lock;

  ASSERT(type < CODEC_IRQ_TYPE_QTY, "%s: Bad type=%d", __func__, type);

  lock = int_lock();

  codec_irq_callback[type] = cb;

  if (cb) {
    if (codec_irq_map == 0) {
      NVIC_SetVector(CODEC_IRQn, (uint32_t)hal_codec_general_irq_handler);
      NVIC_SetPriority(CODEC_IRQn, IRQ_PRIORITY_HIGHPLUSPLUS);
      NVIC_ClearPendingIRQ(CODEC_IRQn);
      NVIC_EnableIRQ(CODEC_IRQn);
    }
    codec_irq_map |= (1 << type);
  } else {
    codec_irq_map &= ~(1 << type);
    if (codec_irq_map == 0) {
      NVIC_DisableIRQ(CODEC_IRQn);
      NVIC_ClearPendingIRQ(CODEC_IRQn);
    }
  }

  int_unlock(lock);
}

void hal_codec_anc_fb_check_set_irq_handler(HAL_CODEC_IRQ_CALLBACK cb) {
  hal_codec_set_irq_handler(CODEC_IRQ_TYPE_ANC_FB_CHECK, cb);
}

/* AUDIO CODEC VOICE ACTIVE DETECTION DRIVER */
#ifdef VOICE_DETECTOR_EN

//#define CODEC_VAD_DEBUG

static inline void hal_codec_vad_set_udc(int v) {
  codec->REG_14C &= ~CODEC_VAD_U_DC(0xf);
  codec->REG_14C |= CODEC_VAD_U_DC(v);
}

static inline void hal_codec_vad_set_upre(int v) {
  codec->REG_14C &= ~CODEC_VAD_U_PRE(0x7);
  codec->REG_14C |= CODEC_VAD_U_PRE(v);
}

static inline void hal_codec_vad_set_frame_len(int v) {
  codec->REG_14C &= ~CODEC_VAD_FRAME_LEN(0xff);
  codec->REG_14C |= CODEC_VAD_FRAME_LEN(v);
}

static inline void hal_codec_vad_set_mvad(int v) {
  codec->REG_14C &= ~CODEC_VAD_MVAD(0xf);
  codec->REG_14C |= CODEC_VAD_MVAD(v);
}

static inline void hal_codec_vad_set_pre_gain(int v) {
  codec->REG_14C &= ~CODEC_VAD_PRE_GAIN(0x3f);
  codec->REG_14C |= CODEC_VAD_PRE_GAIN(v);
}

static inline void hal_codec_vad_set_sth(int v) {
  codec->REG_14C &= ~CODEC_VAD_STH(0x3f);
  codec->REG_14C |= CODEC_VAD_STH(v);
}

static inline void hal_codec_vad_set_frame_th1(int v) {
  codec->REG_150 &= ~CODEC_VAD_FRAME_TH1(0xff);
  codec->REG_150 |= CODEC_VAD_FRAME_TH1(v);
}

static inline void hal_codec_vad_set_frame_th2(int v) {
  codec->REG_150 &= ~CODEC_VAD_FRAME_TH2(0x3ff);
  codec->REG_150 |= CODEC_VAD_FRAME_TH2(v);
}

static inline void hal_codec_vad_set_frame_th3(int v) {
  codec->REG_150 &= ~CODEC_VAD_FRAME_TH3(0x3fff);
  codec->REG_150 |= CODEC_VAD_FRAME_TH3(v);
}

static inline void hal_codec_vad_set_range1(int v) {
  codec->REG_154 &= ~CODEC_VAD_RANGE1(0x1f);
  codec->REG_154 |= CODEC_VAD_RANGE1(v);
}

static inline void hal_codec_vad_set_range2(int v) {
  codec->REG_154 &= ~CODEC_VAD_RANGE2(0x7f);
  codec->REG_154 |= CODEC_VAD_RANGE2(v);
}

static inline void hal_codec_vad_set_range3(int v) {
  codec->REG_154 &= ~CODEC_VAD_RANGE3(0x1ff);
  codec->REG_154 |= CODEC_VAD_RANGE3(v);
}

static inline void hal_codec_vad_set_range4(int v) {
  codec->REG_154 &= ~CODEC_VAD_RANGE4(0x3ff);
  codec->REG_154 |= CODEC_VAD_RANGE4(v);
}

static inline void hal_codec_vad_set_psd_th1(int v) {
  codec->REG_158 &= ~CODEC_VAD_PSD_TH1(0x7ffffff);
  codec->REG_158 |= CODEC_VAD_PSD_TH1(v);
}

static inline void hal_codec_vad_set_psd_th2(int v) {
  codec->REG_15C &= ~CODEC_VAD_PSD_TH2(0x7ffffff);
  codec->REG_15C |= CODEC_VAD_PSD_TH2(v);
}

static inline void hal_codec_vad_en(int enable) {
  if (enable) {
    codec->REG_148 |= CODEC_VAD_EN; // enable vad
  } else {
    codec->REG_148 &= ~CODEC_VAD_EN; // disable vad
    codec->REG_148 |= CODEC_VAD_FINISH;
  }
}

static inline void hal_codec_vad_bypass_ds(int bypass) {
  if (bypass)
    codec->REG_148 |= CODEC_VAD_DS_BYPASS; // bypass ds
  else
    codec->REG_148 &= ~CODEC_VAD_DS_BYPASS; // not bypass ds
}

static inline void hal_codec_vad_bypass_dc(int bypass) {
  if (bypass)
    codec->REG_148 |= CODEC_VAD_DC_CANCEL_BYPASS; // bypass dc
  else
    codec->REG_148 &= ~CODEC_VAD_DC_CANCEL_BYPASS; // not bypass dc
}

static inline void hal_codec_vad_bypass_pre(int bypass) {
  if (bypass)
    codec->REG_148 |= CODEC_VAD_PRE_BYPASS; // bypass pre
  else
    codec->REG_148 &= ~CODEC_VAD_PRE_BYPASS; // not bypass pre
}

static inline void hal_codec_vad_dig_mode(int enable) {
  if (enable)
    codec->REG_148 |= CODEC_VAD_DIG_MODE; // digital mode
  else
    codec->REG_148 &= ~CODEC_VAD_DIG_MODE; // not digital mode
}

static inline void hal_codec_vad_adc_en(int enable) {
  if (enable) {
    codec->REG_080 |= (CODEC_CODEC_ADC_EN | CODEC_CODEC_ADC_EN_CH4);
  } else {
    uint32_t val;

    val = codec->REG_080;
    val &= ~CODEC_CODEC_ADC_EN_CH4;
    if ((val & (CODEC_CODEC_ADC_EN_CH0 | CODEC_CODEC_ADC_EN_CH1 |
                CODEC_CODEC_ADC_EN_CH2 | CODEC_CODEC_ADC_EN_CH3)) == 0) {
      val &= ~CODEC_CODEC_ADC_EN;
    }
    codec->REG_080 = val;
  }
}

static inline void hal_codec_vad_irq_en(int enable) {
  if (enable) {
    codec->REG_010 |= (CODEC_VAD_FIND_MSK | CODEC_VAD_NOT_FIND_MSK);
  } else {
    codec->REG_010 &= ~(CODEC_VAD_FIND_MSK | CODEC_VAD_NOT_FIND_MSK);
  }

  codec->REG_00C = CODEC_VAD_FIND | CODEC_VAD_NOT_FIND;
}

static inline void hal_codec_vad_adc_if_en(int enable) {
  if (enable) {
    codec->REG_000 |=
        (CODEC_DMACTRL_RX | CODEC_ADC_ENABLE_CH4 | CODEC_ADC_ENABLE);
  } else {
    codec->REG_000 &=
        ~(CODEC_DMACTRL_RX | CODEC_ADC_ENABLE_CH4 | CODEC_ADC_ENABLE);
  }
}

static inline void hal_codec_vad_adc_down(int v) {
  unsigned int regval = codec->REG_094;

  regval &= ~CODEC_CODEC_ADC_DOWN_SEL_CH4(0x3);
  regval |= CODEC_CODEC_ADC_DOWN_SEL_CH4(v);
  codec->REG_094 = regval;
}

#ifdef CODEC_VAD_DEBUG
void hal_codec_vad_reg_dump(void) {
  TRACE(1, "codec base = %8x\n", (int)&(codec->REG_000));
  TRACE(1, "codec->REG_000 = %x\n", codec->REG_000);
  TRACE(1, "codec->REG_00C = %x\n", codec->REG_00C);
  TRACE(1, "codec->REG_010 = %x\n", codec->REG_010);
  TRACE(1, "codec->REG_060 = %x\n", codec->REG_060);
  TRACE(1, "codec->REG_064 = %x\n", codec->REG_064);
  TRACE(1, "codec->REG_080 = %x\n", codec->REG_080);
  TRACE(1, "codec->REG_094 = %x\n", codec->REG_094);
  TRACE(1, "codec->REG_148 = %x\n", codec->REG_148);
  TRACE(1, "codec->REG_14C = %x\n", codec->REG_14C);
  TRACE(1, "codec->REG_150 = %x\n", codec->REG_150);
  TRACE(1, "codec->REG_154 = %x\n", codec->REG_154);
  TRACE(1, "codec->REG_158 = %x\n", codec->REG_158);
  TRACE(1, "codec->REG_15C = %x\n", codec->REG_15C);
}
#endif

static inline void hal_codec_vad_data_info(uint32_t *data_cnt,
                                           uint32_t *addr_cnt) {
  uint32_t regval = codec->REG_160;

  *data_cnt = GET_BITFIELD(regval, CODEC_VAD_MEM_DATA_CNT) * 2;
  if (*data_cnt >=
      ((CODEC_VAD_MEM_DATA_CNT_MASK >> CODEC_VAD_MEM_DATA_CNT_SHIFT) - 1) * 2) {
    *data_cnt =
        ((CODEC_VAD_MEM_DATA_CNT_MASK >> CODEC_VAD_MEM_DATA_CNT_SHIFT) + 1) * 2;
  }
  *addr_cnt = GET_BITFIELD(regval, CODEC_VAD_MEM_ADDR_CNT) * 2;
}

uint32_t hal_codec_vad_recv_data(uint8_t *dst, uint32_t dst_size) {
  uint8_t *src = (uint8_t *)CODEC_VAD_BUF_ADDR;
  const uint32_t src_size = CODEC_VAD_BUF_SIZE;
  uint32_t len;
  uint32_t start_pos;

  TRACE(5, "%s, dst=%x, dst_size=%d, vad_data_cnt=%d, vad_addr_cnt=%d",
        __func__, (uint32_t)dst, dst_size, vad_data_cnt, vad_addr_cnt);

  if (vad_data_cnt > src_size || vad_addr_cnt >= src_size) {
    return 0;
  }

  if (dst == NULL) {
    return vad_data_cnt;
  }

  if (vad_addr_cnt >= vad_data_cnt) {
    start_pos = vad_addr_cnt - vad_data_cnt;
  } else {
    // In this case (src_size == vad_data_cnt)
    start_pos = vad_addr_cnt + src_size - vad_data_cnt;
  }

  len = MIN(dst_size, vad_data_cnt);

  if (start_pos + len <= src_size) {
    memcpy(dst, src + start_pos, len);
  } else {
    uint32_t len1, len2;
    len1 = src_size - start_pos;
    len2 = len - len1;
    memcpy(dst, src + start_pos, len1);
    memcpy(dst + len1, src, len2);
  }

  TRACE(2, "%s, len=%d", __func__, len);
  return len;
}

void hal_codec_get_vad_data_info(struct CODEC_VAD_BUF_INFO_T *vad_buf_info) {
  vad_buf_info->base_addr = CODEC_VAD_BUF_ADDR;
  vad_buf_info->buf_size = CODEC_VAD_BUF_SIZE;
  vad_buf_info->data_count = vad_data_cnt;
  vad_buf_info->addr_count = vad_addr_cnt;
}

static void hal_codec_vad_isr(uint32_t irq_status) {
  if ((irq_status & (CODEC_VAD_FIND | CODEC_VAD_NOT_FIND)) == 0) {
    return;
  }

  TRACE(2, "%s VAD_FIND=%d", __func__, !!(irq_status & CODEC_VAD_FIND));

  if (vad_handler) {
    vad_handler(!!(irq_status & CODEC_VAD_FIND));
  }
}

int hal_codec_vad_config(const struct AUD_VAD_CONFIG_T *conf) {
  unsigned int adc_channel_en = 0;
  unsigned int cfg_set_mask = 0;
  unsigned int cfg_clr_mask = 0;

  if (!conf)
    return -1;

  vad_handler = conf->handler;

  hal_codec_vad_en(0);
  hal_codec_vad_irq_en(0);

  hal_codec_vad_set_udc(conf->udc);
  hal_codec_vad_set_upre(conf->upre);
  hal_codec_vad_set_frame_len(conf->frame_len);
  hal_codec_vad_set_mvad(conf->mvad);
  hal_codec_vad_set_pre_gain(conf->pre_gain);
  hal_codec_vad_set_sth(conf->sth);
  hal_codec_vad_set_frame_th1(conf->frame_th[0]);
  hal_codec_vad_set_frame_th2(conf->frame_th[1]);
  hal_codec_vad_set_frame_th3(conf->frame_th[2]);
  hal_codec_vad_set_range1(conf->range[0]);
  hal_codec_vad_set_range2(conf->range[1]);
  hal_codec_vad_set_range3(conf->range[2]);
  hal_codec_vad_set_range4(conf->range[3]);
  hal_codec_vad_set_psd_th1(conf->psd_th[0]);
  hal_codec_vad_set_psd_th2(conf->psd_th[1]);
  hal_codec_vad_dig_mode(0);
  hal_codec_vad_bypass_dc(0);
  hal_codec_vad_bypass_pre(0);

  if (conf->sample_rate == AUD_SAMPRATE_8000) {
    // select adc down 8KHz
    hal_codec_vad_adc_down(1);
    hal_codec_vad_bypass_ds(1);
  } else if (conf->sample_rate == AUD_SAMPRATE_16000) {
    // select adc down 16KHz
    hal_codec_vad_adc_down(0);
    hal_codec_vad_bypass_ds(0);
  } else {
    ASSERT(false, "%s: Bad sample rate: %u", __func__, conf->sample_rate);
  }

  cfg_clr_mask = CODEC_MODE_16BIT_ADC_CH0 | CODEC_MODE_16BIT_ADC_CH1 |
                 CODEC_MODE_16BIT_ADC_CH2 | CODEC_MODE_16BIT_ADC_CH3 |
                 CODEC_MODE_16BIT_ADC_CH4 | CODEC_MODE_16BIT_ADC_CH5 |
                 CODEC_MODE_16BIT_ADC_CH6 | CODEC_MODE_24BIT_ADC |
                 CODEC_MODE_32BIT_ADC;

  if (conf->bits == AUD_BITS_16) {
    cfg_set_mask |= CODEC_MODE_16BIT_ADC_CH0 | CODEC_MODE_16BIT_ADC_CH1 |
                    CODEC_MODE_16BIT_ADC_CH2 | CODEC_MODE_16BIT_ADC_CH3 |
                    CODEC_MODE_16BIT_ADC_CH4 | CODEC_MODE_16BIT_ADC_CH5 |
                    CODEC_MODE_16BIT_ADC_CH6;
  } else if (conf->bits == AUD_BITS_24) {
    cfg_set_mask |= CODEC_MODE_24BIT_ADC;
  } else if (conf->bits == AUD_BITS_32) {
    cfg_set_mask |= CODEC_MODE_32BIT_ADC;
  } else {
    ASSERT(false, "%s: Bad cap bits: %d", __func__, conf->bits);
  }

  for (int i = 0; i < MAX_ADC_CH_NUM; i++) {
    adc_channel_en |= (CODEC_ADC_ENABLE_CH0 << i);
  }

  if (((codec->REG_000 & adc_channel_en) != 0) &&
      ((codec->REG_040 & cfg_set_mask) == 0)) {
    ASSERT(false, "%s: Cap bits conflict: %d", __func__, conf->bits);
  } else {
    codec->REG_040 = (codec->REG_040 & ~cfg_clr_mask) | cfg_set_mask;
  }

  codec->REG_220 = 320;
  codec->REG_224 = 32000 * 3; // vad timeout value
#ifdef I2C_VAD
  codec->REG_230 |= CODEC_VAD_EXT_EN | CODEC_VAD_SRC_SEL;
#endif

#if !(defined(FIXED_CODEC_ADC_VOL) && defined(SINGLE_CODEC_ADC_VOL))
  const CODEC_ADC_VOL_T *adc_gain_db;

#ifdef SINGLE_CODEC_ADC_VOL
  adc_gain_db = hal_codec_get_adc_volume(CODEC_SADC_VOL);
#else
  adc_gain_db = hal_codec_get_adc_volume(
      hal_codec_get_mic_chan_volume_level(AUD_CHANNEL_MAP_CH4));
#endif
  if (adc_gain_db) {
    hal_codec_set_dig_adc_gain(AUD_CHANNEL_MAP_CH4, *adc_gain_db);
  }
#endif

  return 0;
}

int hal_codec_vad_open(const struct AUD_VAD_CONFIG_T *conf) {
  vad_type = conf->type;

  // open analog vad
  analog_aud_vad_adc_enable(true);

  // enable vad clock
  hal_cmu_codec_vad_clock_enable(1);

  hal_codec_vad_config(conf);

  return 0;
}

int hal_codec_vad_close(void) {
#ifdef I2C_VAD
  codec->REG_230 &= ~(CODEC_VAD_EXT_EN | CODEC_VAD_SRC_SEL);
#endif

  // disable vad clock
  hal_cmu_codec_vad_clock_enable(0);

  // close analog vad
  analog_aud_vad_adc_enable(false);

  vad_type = AUD_VAD_TYPE_NONE;

  return 0;
}

int hal_codec_vad_start(void) {
  if (vad_enabled) {
    return 0;
  }
  vad_enabled = true;
  vad_data_cnt = 0;
  vad_addr_cnt = 0;

  hal_codec_vad_irq_en(1);
  hal_codec_set_irq_handler(CODEC_IRQ_TYPE_VAD, hal_codec_vad_isr);

  if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG) {
    // digital vad
    hal_codec_vad_en(1);
    // enable adc if
    hal_codec_vad_adc_if_en(1);
    // enable adc
    hal_codec_vad_adc_en(1);
  }

  analog_aud_vad_enable(vad_type, true);

  return 0;
}

int hal_codec_vad_stop(void) {
  if (!vad_enabled) {
    return 0;
  }
  vad_enabled = false;
  hal_codec_vad_data_info(&vad_data_cnt, &vad_addr_cnt);

  analog_aud_vad_enable(vad_type, false);

  hal_codec_vad_irq_en(0);
  hal_codec_set_irq_handler(CODEC_IRQ_TYPE_VAD, NULL);

  if (vad_type == AUD_VAD_TYPE_MIX || vad_type == AUD_VAD_TYPE_DIG) {
    hal_codec_vad_en(0);
    hal_codec_vad_adc_if_en(0);
    hal_codec_vad_adc_en(0);
  }

  return 0;
}

#endif

//********************BT trigger functions: START********************
static void hal_codec_bt_trigger_isr(uint32_t irq_status) {
  if ((irq_status & CODEC_BT_TRIGGER) == 0) {
    return;
  }

  if (bt_trigger_callback) {
    TRACE(1, "[%s] bt_trigger_callback Start...", __func__);
    bt_trigger_callback();
  } else {
    TRACE(1, "[%s] bt_trigger_callback = NULL", __func__);
  }
}

static inline void hal_codec_bt_trigger_irq_en(int enable) {
  if (enable)
    codec->REG_010 |= CODEC_BT_TRIGGER_MSK;
  else
    codec->REG_010 &= ~CODEC_BT_TRIGGER_MSK;

  codec->REG_00C = CODEC_BT_TRIGGER;
}

void hal_codec_set_bt_trigger_callback(HAL_CODEC_BT_TRIGGER_CALLBACK callback) {
  bt_trigger_callback = callback;
}

int hal_codec_bt_trigger_start(void) {
  uint32_t lock;

  TRACE(1, "[%s] Start", __func__);

  lock = int_lock();

  hal_codec_set_irq_handler(CODEC_IRQ_TYPE_BT_TRIGGER,
                            hal_codec_bt_trigger_isr);
  hal_codec_bt_trigger_irq_en(1);

  int_unlock(lock);

  return 0;
}

int hal_codec_bt_trigger_stop(void) {
  uint32_t lock;

  TRACE(1, "[%s] Stop", __func__);

  lock = int_lock();

  hal_codec_bt_trigger_irq_en(0);
  hal_codec_set_irq_handler(CODEC_IRQ_TYPE_BT_TRIGGER, NULL);

  int_unlock(lock);

  return 0;
}
//********************BT trigger functions: END********************
