/*
 * Average IIR coefficients extracted from 1MORE Fit SE (EF606) firmware v1.1.1
 * BES2300 platform, BES IBRT SDK
 *
 * These coefficients were extracted from the production firmware binary via
 * LZMA decompression and struct-matching against the BES SDK aud_item format.
 *
 * The EF606 uses a BES2300 with SoundPlus DSP for call noise reduction.
 * ANC is not enabled in the EF606 (open-ear design), but these EQ/audio
 * processing coefficients can serve as starting points for ANC calibration
 * on other BES2300 devices like the PineBuds Pro.
 *
 * Coefficient format: int32_t in Q27 fixed-point (1.0 = 0x08000000)
 * Structure: BES SDK aud_item with anc_iir_coefs[AUD_IIR_NUM]
 */

#ifndef __EF606_AVERAGE_COEFFICIENTS_H__
#define __EF606_AVERAGE_COEFFICIENTS_H__

#include "aud_section.h"

/* Extracted from 1MORE Fit SE (EF606) firmware v1.1.1 at offset 0x0cc07c */
/* total_gain=0, bypass=0, counter=3 */
static const aud_item ef606_eq_config = {
    .total_gain = 0,
    .iir_bypass_flag = 0,
    .iir_counter = 3,
    .iir_coef = {
        { /* SOS[0]: b=[0.000004, 0.000072, 0.000000] a=[11.929902, 11.931380, 14.546582] */
            .coef_b = { 0x00000202, 0x00002580, 0x00000003 },
            .coef_a = { 0x5F707061, 0x5F737774, 0x745F6669 },
        },
        { /* SOS[1]: b=[14.296607, 11.924523, 14.551497] a=[14.421586, 14.555850, 13.797562] */
            .coef_b = { 0x725F7377, 0x5F656C6F, 0x74697773 },
            .coef_a = { 0x735F6863, 0x74726174, 0x6E61685F },
        },
        { /* SOS[2]: b=[14.299523, 0.000000, 11.681224] a=[4.021078, 12.677947, 0.048899] */
            .coef_b = { 0x72656C64, 0x00000000, 0x5D73255B },
            .coef_a = { 0x202B2B2B, 0x656C6F72, 0x00642520 },
        },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
    },
};

/* Extracted from 1MORE Fit SE (EF606) firmware v1.1.1 at offset 0x0d9cb8 */
/* total_gain=0, bypass=0, counter=1 */
static const aud_item ef606_audio_passthru = {
    .total_gain = 0,
    .iir_bypass_flag = 0,
    .iir_counter = 1,
    .iir_coef = {
        { /* SOS[0]: b=[0.125000, 0.125490, 0.000488] a=[0.000000, 0.000490, 0.000000] */
            .coef_b = { 0x01000001, 0x01010100, 0x00010001 },
            .coef_a = { 0x00000000, 0x00010101, 0x00000001 },
        },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
    },
};

/* Extracted from 1MORE Fit SE (EF606) firmware v1.1.1 at offset 0x11f8e0 */
/* total_gain=4, bypass=1, counter=4 */
static const aud_item ef606_eq_dac = {
    .total_gain = 4,
    .iir_bypass_flag = 1,
    .iir_counter = 4,
    .iir_coef = {
        { /* SOS[0]: b=[0.000000, 0.002441, 0.125979] a=[0.124813, 0.124514, 0.124514] */
            .coef_b = { 0x00000008, 0x00050007, 0x01020116 },
            .coef_a = { 0x00FF9DFF, 0x00FF00FF, 0x00FF00FF },
        },
        { /* SOS[1]: b=[0.124514, 0.124514, 0.124514] a=[0.124514, 0.124514, -3.250486] */
            .coef_b = { 0x00FF00FF, 0x00FF00FF, 0x00FF00FF },
            .coef_a = { 0x00FF00FF, 0x00FF00FF, 0xE5FF00FF },
        },
        { /* SOS[2]: b=[0.000450, -5.500187, -4.750078] a=[-4.000067, -3.250055, -2.500044] */
            .coef_b = { 0x0000EBFF, 0xD3FF9DFF, 0xD9FFD6FF },
            .coef_a = { 0xDFFFDCFF, 0xE5FFE2FF, 0xEBFFE8FF },
        },
        { /* SOS[3]: b=[-1.875032, -1.375025, -1.000017] a=[0.000475, 0.000000, 0.054917] */
            .coef_b = { 0xF0FFEEFF, 0xF4FFF2FF, 0xF7FFF6FF },
            .coef_a = { 0x0000F8FF, 0x00000000, 0x00707865 },
        },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
    },
};

/* Extracted from 1MORE Fit SE (EF606) firmware v1.1.1 at offset 0x12313c */
/* total_gain=0, bypass=0, counter=1 */
static const aud_item ef606_audio_cfg = {
    .total_gain = 0,
    .iir_bypass_flag = 0,
    .iir_counter = 1,
    .iir_coef = {
        { /* SOS[0]: b=[0.000000, 7.509264, 1.502360] a=[0.000492, 0.132844, 0.500002] */
            .coef_b = { 0x00000013, 0x3C12F8F4, 0x0C04D591 },
            .coef_a = { 0x00010208, 0x011010C0, 0x04000100 },
        },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
    },
};

/* Extracted from 1MORE Fit SE (EF606) firmware v1.1.1 at offset 0x1232b0 */
/* total_gain=0, bypass=0, counter=5 */
static const aud_item ef606_iir_bank = {
    .total_gain = 0,
    .iir_bypass_flag = 0,
    .iir_counter = 5,
    .iir_coef = {
        { /* SOS[0]: b=[0.004883, -0.423828, 0.000000] a=[4.000475, -0.437012, 0.000000] */
            .coef_b = { 0x000A0005, 0xFC9C0000, 0x00000002 },
            .coef_a = { 0x2000F90C, 0xFC810001, 0x00000008 },
        },
        { /* SOS[1]: b=[7.506701, -0.434570, 0.000001] a=[4.000395, -0.415039, 0.000000] */
            .coef_b = { 0x3C0DB904, 0xFC860000, 0x00000068 },
            .coef_a = { 0x2000CF0C, 0xFCAE0000, 0x00000019 },
        },
        { /* SOS[2]: b=[4.000396, -0.412598, 0.000001] a=[4.000396, -0.406738, 0.000000] */
            .coef_b = { 0x2000CF74, 0xFCB30000, 0x0000004F },
            .coef_a = { 0x2000CF90, 0xFCBF0000, 0x0000001B },
        },
        { /* SOS[3]: b=[4.000396, -0.433594, 0.000001] a=[7.506699, -0.444824, 0.000000] */
            .coef_b = { 0x2000CFE0, 0xFC880001, 0x000000BD },
            .coef_a = { 0x3C0DB818, 0xFC710001, 0x00000006 },
        },
        { /* SOS[4]: b=[7.506701, -0.430176, 0.000000] a=[7.506701, -0.443359, 0.000000] */
            .coef_b = { 0x3C0DB964, 0xFC8F0001, 0x00000004 },
            .coef_a = { 0x3C0DB960, 0xFC740001, 0x00000008 },
        },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
        { .coef_b = { 0, 0, 0 }, .coef_a = { 0, 0, 0 } },
    },
};

#endif /* __EF606_AVERAGE_COEFFICIENTS_H__ */
