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

#ifdef RB_CODEC
#define HEAP_SECTION_SIZE			0x1000
#else
#define HEAP_SECTION_SIZE			0x1000
#endif

#define STACK_SECTION_SIZE			0xD00
#define CP_STACK_SECTION_SIZE		0x1000

#if defined(ROM_UTILS_ON)
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
#define ROM_UTILS_RESV_RAM_SIZE     0x64
#else
#define ROM_UTILS_RESV_RAM_SIZE     0x0
#endif
#endif

#ifdef RB_CODEC
#define OVERLAY_DATA_SECTION_SIZE	0xA000
#else
#define OVERLAY_DATA_SECTION_SIZE	0x8000
#endif

#if (RAM_SIZE >= 0x48000)
#define FAST_XRAM_SECTION_SIZE		0x78000
#endif

/* TODO: Add FLASH_REGION_OFFSET if there are multiple levels of boot loaders */
#if !defined(OTA_BOOT_SIZE) && defined(OTA_CODE_OFFSET)
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#else
#define FLASH_REGION_BASE			(FLASH_BASE)
#endif

#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif

#define FLASH_NC_REGION_BASE		FLASH_C_TO_NC(FLASH_REGION_BASE)
#define FLASHX_REGION_BASE			FLASH_TO_FLASHX(FLASH_REGION_BASE)

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
#define OVERLAY_FOR_CP
#endif

#ifdef OVERLAY_FOR_CP
#define REGION_OVERLAY_TEXT			RAMCPX
#define REGION_OVERLAY_DATA			RAMCP
#else
#define REGION_OVERLAY_TEXT			FRAMX
#define REGION_OVERLAY_DATA			RAM
#endif

/* Linker script to configure memory regions. */
/* See plat_addr_map.h and common.mk for the macro definitions */
MEMORY
{
	ROM			(rx)	: ORIGIN = ROM_BASE,				LENGTH = ROM_SIZE
	FLASH 		(r)		: ORIGIN = FLASH_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
	FLASH_NC	(r)		: ORIGIN = FLASH_NC_REGION_BASE,	LENGTH = FLASH_REGION_SIZE
	FLASHX		(rx)	: ORIGIN = FLASHX_REGION_BASE,		LENGTH = FLASH_REGION_SIZE
	RAM			(rwx)	: ORIGIN = RAM_BASE,				LENGTH = RAM_SIZE - FAST_XRAM_SECTION_SIZE
	RAMX		(rx)	: ORIGIN = RAMX_BASE,				LENGTH = RAM_SIZE - FAST_XRAM_SECTION_SIZE
	FRAMX		(rwx)	: ORIGIN = RAMX_BASE + RAM_SIZE - FAST_XRAM_SECTION_SIZE,	LENGTH = FAST_XRAM_SECTION_SIZE
#ifdef PSRAM_BASE
	PSRAM 		(rwx)	: ORIGIN = PSRAM_BASE,				LENGTH = PSRAM_SIZE
	PSRAM_NC	(rwx)	: ORIGIN = PSRAM_NC_BASE,			LENGTH = PSRAM_SIZE
	PSRAMX		(rx)	: ORIGIN = PSRAMX_BASE,				LENGTH = PSRAM_SIZE
#endif
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	RAMCP 		(rwx)	: ORIGIN = RAMCP_BASE,				LENGTH = RAMCP_SIZE
	RAMCPX 		(rwx)	: ORIGIN = RAMCPX_BASE,				LENGTH = RAMCPX_SIZE
#endif
}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 *   __factory_start
 *   __factory_end
 *   __calib_start
 *   __calib_end
 *   __flash_start
 *   __flash_end
 *   __free_flash
 *   __boot_sram_start_flash__
 *   __boot_sram_end_flash__
 *   __boot_sram_start__
 *   __boot_bss_sram_start__
 *   __boot_bss_sram_end__
 *   __sram_text_data_start_flash__
 *   __sram_text_data_end_flash__
 *   __fast_sram_text_data_start__
 *   __fast_sram_text_data_end__
 *   __fast_sram_text_data_start_flash__
 *   __fast_sram_text_data_end_flash__
 *   __sram_text_data_start__
 *   __sram_bss_start__
 *   __sram_bss_end__
 *   __overlay_text_start__
 *   __overlay_text_exec_start__
 *   __overlay_data_start__
 */
ENTRY(Boot_Loader)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_INFO_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

	.boot_struct (ORIGIN(FLASH)) :
	{
		__flash_start = .;
		KEEP(*(.boot_struct))
		. = ALIGN(4);
	} > FLASH

	. = FLASH_TO_FLASHX(.);

	.boot_text_flash (.) : AT (FLASHX_TO_FLASH(ADDR(.boot_text_flash)))
	{
		*(.boot_loader)
		*(.boot_text_flash*)
		*(.boot_rodata*)
		. = ALIGN(4);
	} > FLASHX

#if defined(__BES_OTA_MODE__)
	.ota_boot_info (ALIGN(4096)) :
	{
		*(.ota_boot_info)
		. = (0x1000);
	} > FLASHX

	.ota_boot_rev (ALIGN(4096)) :
	{
		*(.ota_boot_rev)
		. = (0x1000);
	} > FLASHX
#endif

	. = FLASHX_TO_FLASH(.);

	.got_info (.) :
	{
		__got_info_start = .;
		__got_start = .;
		*(.got)
		. = ALIGN(4);
		__got_end = .;
		__got_plt_start = .;
		*(.got.plt)
		. = ALIGN(4);
		__igot_plt_start = .;
		*(.igot.plt)
		. = ALIGN(4);
		__dynamic_start = .;
		*(.dynamic)
		. = ALIGN(4);
		__got_info_end = .;
	} > FLASH

	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.reboot_param (.) (NOLOAD) :
	{
		KEEP(*(.reboot_param))
		. = REBOOT_PARAM_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.userdata_pool (.) (NOLOAD) :
	{
		*(.userdata_pool)
		. = ALIGN(4);
		__userdata_pool_end__ = .;
	} > RAM

	__boot_sram_start_flash__ = LOADADDR(.got_info) + SIZEOF(.got_info);
	__boot_sram_start__ = __userdata_pool_end__;

	.boot_text_sram (RAM_TO_RAMX(__boot_sram_start__)) : AT (__boot_sram_start_flash__)
	{
		/* memcpy.o or libc_nano.a(lib_a-memcpy.o/lib_a-memcpy-stub.o) */
		*:memcpy.o(.text*)
		*:lib_a-memcpy*.o(.text*)
		*:libc_rom.o(.text*)

		/* memset.o or libc_nano.a(lib_a-memset.o) */
		*:memset.o(.text*)
		*:lib_a-memset*.o(.text*)

		*:hal_norflash*.o(.text*)
		*:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .text*)

#ifdef BOOT_CODE_IN_RAM
		*:hal_cmu_*.o(.text*)
		*:hal_iomux_*.o(.text*)
		*:hal_analogif_*.o(.text*)
		*:hal_spi.o(.text*)
		*:hal_timer.o(.text*)
		*:hal_sysfreq.o(.text*)
#endif
            *:hal_iomux_best2300p.o(.text* .rodata*)
            *:hal_analogif_best2300p.o(.text* .rodata*)
            *:hal_timer.o(.text* .rodata*)
            *:hal_spi.o(.text* .rodata*)
            *:hal_sysfreq.o(.text* .rodata*)
            *:hal_cmu_best2300p.o(.text* .rodata*)
            *:hal_cmu_common.o(.text* .rodata*)
            *:hal_chipid.o (.text* .rodata)
            *:hal_cache.o (.text* .rodata)
            *:hal_btpcm.o (.text* .rodata)
            *:hal_bootmode.o (.text* .rodata)

		*(.boot_text_sram*)
		. = ALIGN(4);
		__boot_text_sram_end = .;
	} > RAMX

	.boot_data_sram (RAMX_TO_RAM(ADDR(.boot_text_sram) + SIZEOF(.boot_text_sram))) :
					AT (__boot_sram_start_flash__ + SIZEOF(.boot_text_sram))
	{
		*:hal_norflash*.o(.data* .rodata*)
		*:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .data* EXCLUDE_FILE(*:norflash_api.o) .rodata*)
		*:hal_psram.o(.data* .rodata*)
		*(.boot_data*)
		. = ALIGN(4);
		__boot_data_sram_end = .;
	} > RAM

	__boot_sram_end_flash__ = __boot_sram_start_flash__ +
								SIZEOF(.boot_text_sram) + SIZEOF(.boot_data_sram);

	.boot_bss_sram (.) (NOLOAD) :
	{
		__boot_bss_sram_start__ = .;
		*:hal_norflash*.o(.bss*)
		*:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .bss*)
		*:hal_psram.o(.bss*)
		*(.boot_bss*)
		. = ALIGN(4);
		__boot_bss_sram_end__ = .;
	} > RAM

	__sram_text_data_start_flash__ = __boot_sram_end_flash__;
	__sram_text_data_start__ = __boot_bss_sram_end__;

	.sram_text (RAM_TO_RAMX(__sram_text_data_start__)) : AT (__sram_text_data_start_flash__)
	{
		*:norflash_api.o(.text*)
		*(.sram_text*)
		. = ALIGN(4);
	} > RAMX

	.sram_data (RAMX_TO_RAM(ADDR(.sram_text) + SIZEOF(.sram_text))) :
				AT (__sram_text_data_start_flash__ + SIZEOF(.sram_text))
	{
#if defined(GLOBAL_SRAM_KISS_FFT)
			*:fftwrap.o(.data* .rodata*)
			*:kiss_fft.o(.data* .rodata*)
			*:kiss_fftr.o(.data* .rodata*)
#endif

#if defined(GLOBAL_SRAM_CMSIS_FFT)
		*:arm_common_tables.o(.rodata.armBitRevIndexTable256)
		*:arm_common_tables.o(.rodata.twiddleCoef_256)
		*:arm_common_tables.o(.rodata.twiddleCoef_rfft_512)
		*:arm_common_tables.o(.rodata.armBitRevIndexTable128)
		*:arm_common_tables.o(.rodata.twiddleCoef_128)
		*:arm_common_tables.o(.rodata.twiddleCoef_rfft_256)
#endif

#if defined(BTUSB_AUDIO_MODE)
		*:hal_usb.o(.rodata*)
#endif
		*:utils.o(.data* .rodata*)
		*:cqueue.o(.data* .rodata*)
		*:iir_process.o(.data* .rodata*)
#if defined(HFP_1_6_ENABLE)
		*:sbcplc.o(.data* .rodata*)
		*:sbc.o(.data* .rodata*)
		*:sbc_math.o(.data* .rodata*)
#endif
#if defined(A2DP_AAC_ON)
		*:fixpoint_math.o(.data* .rodata*)
		*:FDK_bitbuffer.o(.data* .rodata*)
#endif
		*:crossover_filter.o(.data* .rodata*)
		*:exp_func_tables.o(.data* .rodata*)
#if defined(KWS_BES)
		*:cnn.o(.data* .rodata.second_conv_wt)
		*:command_recognition.o(.data* .rodata*)
		*:arm_bitreversal_16.o(.data* .rodata*)
		*:arm_cfft_radix8_f32.o(.data* .rodata*)
		*:arm_bitreversal2.o(.data* .rodata*)
		*:arm_mult_f32.o(.data* .rodata*)
		*:arm_cmplx_mag_squared_f32.o(.data* .rodata*)
		*:arm_cmplx_mult_cmplx_f32.o(.data* .rodata*)
		*:arm_abs_f32.o(.data* .rodata*)
		*:arm_cos_f32.o(.data* .rodata*)
		*:arm_sin_f32.o(.data* .rodata*)
#endif
		*:norflash_api.o(.rodata*)
		*(.sram_data*)
		. = ALIGN(4);
	} > RAM

	__sram_text_data_end_flash__ = __sram_text_data_start_flash__ +
								SIZEOF(.sram_text) + SIZEOF(.sram_data);

	.sram_bss (.) (NOLOAD) :
	{
		__sram_bss_start__ = .;
		*:utils.o(.bss*)
		*(.sram_bss*)
		. = ALIGN(4);
		__sram_bss_end__ = .;
	} > RAM

	__fast_sram_text_data_start_flash__ = __sram_text_data_end_flash__;

	.fast_text_sram (ORIGIN(FRAMX)) : AT (__fast_sram_text_data_start_flash__)
	{
		__fast_sram_text_exec_start__ = .;

#if defined(GLOBAL_SRAM_KISS_FFT)
		*:fftwrap.o(.text*)
		*:kiss_fft.o(.text*)
		*:kiss_fftr.o(.text*)
#endif

#if defined(GLOBAL_SRAM_CMSIS_FFT)
		*:cmsis_fftwrap.o(.text*)
		*:arm_rfft_fast_f32.o(.text*)
		*:arm_cfft_f32.o(.text*)
#endif


#if defined(ROM_UTILS_ON)
		*:rom_utils.o(.text*)
#endif

#if defined(BTUSB_AUDIO_MODE)
		*:hal_dma.o(.text*)
		*:hal_uart.o(.text*)
		*:hal_trace.o(.text*)
#endif

#if defined(BTUSB_AUDIO_MODE)
		*:hal_usb.o(.text*)

		*:usb_audio.o(.text*)
		*:usb_audio_sync.o(.text*)
		*:usb_audio_cdc.o(.text*)

		*:usbaudio_entry.o(.text*)
		*:usb_audio_app.o(.text*)
#endif

		/* memmove.o or libc_nano.a(lib_a-memmove.o) */
		*:memmove.o(.text*)
		*:lib_a-memmove.o(.text*)
		*:utils.o(.text*)
		*:cqueue.o(.text*)
		*:audio_resample_ex.o(.text*)
#if defined(MIX_MIC_DURING_MUSIC)
		*:app_bt_stream.o(.text*)
#endif
#if defined(HFP_1_6_ENABLE)
#if defined(__SBC_FUNC_IN_ROM__)
		*:sbc.o(.text*)
#else
		*:sbc.o(.text.SbcIsValidStreamInfo)
		*:sbc.o(.text.SBC_FrameLen)
		*:sbc.o(.text.SbcCrcSum4)
		*:sbc.o(.text.SbcMonoBitAlloc)
		*:sbc.o(.text.SbcStereoBitAlloc)
		*:sbc.o(.text.SbcParseHeader)
		*:sbc.o(.text.SbcParseScaleFactors)
		*:sbc.o(.text.SbcUnpackSample)
		*:sbc.o(.text.SbcBuildSubbandSamples)
		*:sbc.o(.text.SbcJointProcessing)
		*:sbc.o(.text.SbcSynthesisFilter4)
		*:sbc.o(.text.SbcSynthesisFilter8)
		*:sbc.o(.text.SbcResetDecoderState)
		*:sbc.o(.text.SbcMuteFrame)
		*:sbc.o(.text.SBC_DecodeFrames)
		*:sbc.o(.text.SbcQuantizeSamples)
		*:sbc.o(.text.SBC_InitDecoder)
		*:sbc.o(.text.SBC_InitEncoder)
		*:sbc.o(.text.SBC_EncodeFrames)
#endif
		*:sbc_math.o(.text*)
#endif
#if defined(A2DP_AAC_ON)
		*:fixpoint_math.o(.text*)
		*:FDK_bitbuffer.o(.text*)
		*:FDK_tools_rom.o(.text*FDKgetWindowSlope*)
		*:FDK_tools_rom.o(.text*getBitstreamElementList*)
		*:dct.o(.text*dct_IV*)
#if !defined(__SBC_FUNC_IN_ROM_VBEST2000__)
		*:fft.o(.text._Z3fftiPlPi)
		*:fft_rad2.o(.text*dit_fft*)
		*:dct.o(.text*getTables*)
#endif
#ifdef VOICE_DATAPATH
		*:opus_encoder.o(.text*)
#else
#if defined(__AI_VOICE__) && (RAM_SIZE >= 0x48000)
		*:voice_opus.o(.text*)
		/**:opus_encoder.o(.text*)*/
		*:opus_encoder.o(.text.frame_size_select)
		*:opus_encoder.o(.text.opus_encode_native)
		*:opus_encoder.o(.text.user_bitrate_to_bitrate)
		*:opus_encoder.o(.text.compute_equiv_rate)
		*:opus_encoder.o(.text.decide_fec)
		*:opus_encoder.o(.text.dc_reject)
		*:opus_encoder.o(.text.gen_toc)
		*:opus_encoder.o(.text.opus_encode)
		*:opus_encoder.o(.text.opus_encoder_create)
		*:opus_encoder.o(.text.opus_encoder_get_size)
		*:opus_encoder.o(.text.opus_encoder_init)
		*:opus_encoder.o(.text.opus_encoder_ctl)
		*:analysis.o(.text*)
		/**:celt_encoder.o(.text*)*/
		*:celt_encoder.o(.text.opus_custom_encoder_ctl)
		*:celt_encoder.o(.text.celt_encoder_get_size)
		*:celt_encoder.o(.text.opus_custom_encoder_get_size)
		*:celt_encoder.o(.text.celt_encoder_init)
		*:celt_encoder.o(.text.opus_custom_encoder_init_arch)
		/**:repacketizer.o(.text*)*/
		*:repacketizer.o(.text.opus_packet_pad)
		*:repacketizer.o(.text.opus_repacketizer_init)
		*:repacketizer.o(.text.opus_repacketizer_cat)
		*:repacketizer.o(.text.opus_repacketizer_cat_impl)
		*:repacketizer.o(.text.opus_repacketizer_out_range_impl)
		*:opus.o(.text*)
		*:enc_API.o(.text*)
		/**:entenc.o(.text*)*/
		*:entenc.o(.text.ec_enc_init)
		*:entenc.o(.text.ec_enc_icdf)
		*:entenc.o(.text.ec_enc_normalize)
		*:entenc.o(.text.ec_enc_carry_out)
		*:entenc.o(.text.ec_write_byte)
		*:entenc.o(.text.ec_enc_patch_initial_bits)
		*:entenc.o(.text.ec_enc_done)
		*:control_audio_bandwidth.o(.text*)
		*:encode_pulses.o(.text*)
		*:LPC_analysis_filter.o(.text*)
		*:burg_modified_FIX.o(.text.silk_burg_modified_c)
		*:vector_ops_FIX.o(.text.silk_scale_copy_vector16)
		*:vector_ops_FIX.o(.text.silk_inner_prod_aligned)
		*:vector_ops_FIX.o(.text.silk_inner_prod16_aligned_64_c)
		*:NLSF_del_dec_quant.o(.text.silk_NLSF_del_dec_quant)
		*:encode_frame_FIX.o(.text.silk_encode_frame_FIX)
		*:encode_frame_FIX.o(.text.silk_encode_do_VAD_FIX)
		*:encode_frame_FIX.o(.text.silk_LBRR_encode_FIX)
		/**:pitch.o(.text*)*/
		*:pitch.o(.text.celt_pitch_xcorr_c)
		*:resampler.o(.text*)
		*:VAD.o(.text*)
		*:encode_indices.o(.text*)
		/**:control_codec.o(.text*)*/
		*:control_codec.o(.text.silk_control_encoder)
		*:control_codec.o(.text.silk_setup_resamplers)
		*:control_codec.o(.text.silk_setup_fs)
		*:control_codec.o(.text.silk_setup_complexity)
		*:control_codec.o(.text.silk_setup_LBRR)
		/**:celt_lpc.o(.text*)*/
		*:celt_lpc.o(.text._celt_autocorr)
		/**:shell_coder.o(.text*)*/
		*:shell_coder.o(.text.combine_pulses)
		*:shell_coder.o(.text.silk_shell_encoder)
		*:shell_coder.o(.text.encode_split)
		*:init_encoder.o(.text*)
		*:log2lin.o(.text*)
		*:biquad_alt.o(.text*)
		*:stereo_encode_pred.o(.text*)
		*:NLSF_unpack.o(.text*)
		*:code_signs.o(.text*)
		*:lin2log.o(.text*)
		*:HP_variable_cutoff.o(.text*)
		*:sum_sqr_shift.o(.text*)
		*:inner_prod_aligned.o(.text*)
		*:stereo_quant_pred.o(.text*)
		*:entcode.o(.text*)
		*:celt.o(.text*)
		*:laplace.o(.text*)
		/**:NSQ.o(.text*)*/ /*complexity 1*/
		/**:NSQ_del_dec.o(.text*)*/ /*complexity 2*/
#endif
#endif
#endif

#if defined(__AUDIO_SPECTRUM__)
		*:spectrum_fix.o(.text*)
		*:audio_spectrum.o(.text*)
#endif
		*:mdct.o(.text*)
		*:crossover_filter.o(.text*)
		*:data_compressor.o(.text*)
		*:drc.o(.text*)
		*:limiter.o(.text*)
#if defined(KWS_BES)
		*:arm_*.o(.text*)
		*:mfcc.o(.text*)
		*:cnn.o(.text*)
		*:command_recognition.o(.text*)
		*:arm_radix8_butterfly_f32.o(.text*)
		*:arm_bitreversal_32.o(.text*)
		*:arm_bitreversal_16.o(.text*)
		*:dct.o(.text*dct_IV*)
		*:fft.o(.text._Z3fftiPlPi)
		*:fft_rad2.o(.text*dit_fft*)
		*:dct.o(.text*getTables*)
		*:arm_cfft_radix8_f32.o(.text*)
		*:arm_bitreversal2.o(.text*)
		*:arm_mult_f32.o(.text*)
		*:arm_cmplx_mag_squared_f32.o(.text*)
		*:arm_cmplx_mult_cmplx_f32.o(.text*)
		*:lib_a-wf_sqrt.o(.text*)
		*:arm_abs_f32.o(.text*)
		*:arm_cos_f32.o(.text*)
		*:arm_sin_f32.o(.text*)
#endif
#if !defined(SCO_OPTIMIZE_FOR_RAM)
		:audio_prompt_sbc.o(.text*)
#endif


        /*apps*/

*:app_battery.o(.text* .rodata*)
*:app_pwl.o(.text* .rodata*)
*:app_key.o(.text* .rodata*)
*:app_status_ind.o(.text* .rodata*)
*:apps.o(.text* .rodata*)
*:app_spec_ostimer.o(.text* .rodata*)
*:app_utils.o(.text* .rodata*)
*:app_thread.o(.text* .rodata*)
*:app_audio.o(.text* .rodata*)
*:plc_utils.o(.text* .rodata*)
*:a2dp_decoder.o(.text* .rodata*)
*:a2dp_decoder_sbc.o(.text* .rodata*)
*:a2dp_decoder_aac_lc.o(.text* .rodata*)
*:a2dp_decoder_cp.o(.text* .rodata*)

/*cq*/
*:cqueue.o(.text* .rodata*)
/*heap api*/
*:heap_api.o(.text* .rodata*)
*:multi_heap.o(.text* .rodata*)
*:pool_api.o(.text* .rodata*)
/*hwtimer*/
*:hwtimer_list.o(.text* .rodata*)
/*kfifo*/
*:kfifo.o(.text* .rodata*)
/*list*/
*:list.o(.text* .rodata*)


*:sbcaac_dummy.o(.text* .rodata*)

*:system_cp.o(.text* .rodata*)
*:system_ARMCM.o(.text* .rodata*)
*:patch.o(.text* .rodata*)
*:cmsis_nvic.o(.text* .rodata*)
*:arm_var_q31.o(.text* .rodata*)
*:arm_var_q15.o(.text* .rodata*)
*:arm_var_f32.o(.text* .rodata*)
*:arm_std_q31.o(.text* .rodata*)
*:arm_std_q15.o(.text* .rodata*)
*:arm_std_f32.o(.text* .rodata*)
*:arm_rms_q31.o(.text* .rodata*)
*:arm_rms_q15.o(.text* .rodata*)
*:arm_rms_f32.o(.text* .rodata*)
*:arm_power_q7.o(.text* .rodata*)
*:arm_power_q31.o(.text* .rodata*)
*:arm_power_q15.o(.text* .rodata*)
*:arm_power_f32.o(.text* .rodata*)
*:arm_min_q7.o(.text* .rodata*)
*:arm_min_q31.o(.text* .rodata*)
*:arm_min_q15.o(.text* .rodata*)
*:arm_min_f32.o(.text* .rodata*)
*:arm_mean_q7.o(.text* .rodata*)
*:arm_mean_q31.o(.text* .rodata*)
*:arm_mean_q15.o(.text* .rodata*)
*:arm_mean_f32.o(.text* .rodata*)
*:arm_max_q7.o(.text* .rodata*)
*:arm_max_q31.o(.text* .rodata*)
*:arm_max_q15.o(.text* .rodata*)
*:arm_max_f32.o(.text* .rodata*)
*:arm_const_structs.o(.text* .rodata*)
*:arm_common_tables.o(.text* .rodata*)
*:arm_dot_prod_q15.o(.text* .rodata*)
*:arm_biquad_cascade_df1_init_f32.o(.text* .rodata*)
*:arm_fir_init_q15.o(.text* .rodata*)
*:arm_fir_fast_q15.o(.text* .rodata*)
*:arm_bitreversal2.o(.text* .rodata*)
*:arm_cfft_q15.o(.text* .rodata*)
*:arm_cfft_radix4_q15.o(.text* .rodata*)
*:arm_cfft_radix8_f32.o(.text* .rodata*)
*:arm_rfft_fast_init_f32.o(.text* .rodata*)
*:arm_rfft_init_q15.o(.text* .rodata*)
*:arm_rfft_q15.o(.text* .rodata*)
*:arm_copy_q31.o(.text* .rodata*)
*:arm_fill_q31.o(.text* .rodata*)
*:arm_q15_to_float.o(.text* .rodata*)
*:arm_dot_prod_q7.o(.text* .rodata*)
*:arm_add_q15.o(.text* .rodata*)
*:arm_shift_q15.o(.text* .rodata*)
*:arm_negate_q15.o(.text* .rodata*)
*:arm_offset_q15.o(.text* .rodata*)
*:arm_mult_q15.o(.text* .rodata*)

#if 1
                    /*bt app*/
                    *:a2dp_codec_scalable.o(.text* .rodata*)
                    *:a2dp_codec_sbc.o(.text* .rodata*)
                    *:a2dp_codec_opus.o(.text* .rodata*)
                    *:codec_lhdc.o(.text* .rodata*)
                    *:a2dp_codec_lhdc.o(.text* .rodata*)
                    *:a2dp_codec_ldac.o(.text* .rodata*)
                    *:a2dp_codec_aac.o(.text* .rodata*)
                    *:app_a2dp_codecs.o(.text* .rodata*)
                    *:app_a2dp.o(.text* .rodata*)
                    *:app_bt.o(.text* .rodata*)
                    *:app_bt_func.o(.text* .rodata*)
                    *:app_bt_media_manager.o(.text* .rodata*)
                    *:app_bt_stream.o(.text* .rodata*)
                    *:app_keyhandle.o(.text* .rodata*)
                    *:app_ring_merge.o(.text* .rodata*)
                    *:app_sec.o(.text* .rodata*)
                    *:app_spp.o(.text* .rodata*)
                    *:audio_prompt_sbc.o(.text* .rodata*)
                    *:besmain.o(.text* .rodata*)


                    /*bt if*/
                    *:bt_status_conv.o(.text* .rodata*)
                    *:bt_if.o(.text* .rodata*)
                    *:a2dp_api.o(.text* .rodata*)
                    *:codec_sbc.o(.text* .rodata*)
                    *:avrcp_api.o(.text* .rodata*)
                    *:cmgr_api.o(.text* .rodata*)
                    *:hci_api.o(.text* .rodata*)
                    *:l2cap_api.o(.text* .rodata*)
                    *:me_api.o(.text* .rodata*)
                    *:utils.o(.text* .rodata*)
                    *:sdp_api.o(.text* .rodata*)
                    *:spp.o(.text* .rodata*)
                    *:spp_api.o(.text* .rodata*)
                    *:hfp_api.o(.text* .rodata*)
                    *:os_api.o(.text* .rodata*)
                    *:rfcomm_api.o(.text* .rodata*)
                    *:bt_xtal_sync.o(.text* .rodata*)
                    *:besaud_api.o(.text* .rodata*)
#if 0
                    /*bt prf*/
                    *:a2dp.o(.text*)
                    *:sbc.o(.text*)
                    *:avctp.o(.text*)
                    *:avrcp.o(.text*)
                    *:besaud.o(.text*)
                    *:besaudalloc.o(.text*)
                    *:besaudcon.o(.text*)
                    *:btm.o(.text*)
                    *:btm_devicedb.o(.text*)
                    *:btm_handle_hcievent.o(.text*)
                    *:btm_sco_interface.o(.text*)
                    *:btm_security.o(.text*)
                    *:avdtp.o(.text*)
                    *:connect.o(.text*)
                    *:hfp.o(.text*)
                    *:hshf.o(.text*)
                    *:hsp.o(.text*)
                    *:l2cap.o(.text*)
                    *:l2cap_er.o(.text*)
                    *:bt_common.o(.text*)
                    *:co_lib.o(.text*)
                    *:co_ppbuff.o(.text*)
                    *:md5.o(.text*)
                    *:obex.o(.text*)
                    *:pbap.o(.text*)
                    *:rfcomm.o(.text*)
                    *:sco.o(.text*)
                    *:sdp.o(.text*)
                    *:co_printf.o(.text*)
                    *:co_queue.o(.text*)
                    *:co_timer.o(.text*)
                    *:cobuf.o(.text*)
                    *:bes_os.o(.text*)
                    *:hci.o(.text*)
                    *:hci_buff.o(.text*)
                    *:bt_schedule.o(.text*)
#else
*:a2dp.o(.text* .rodata*)
*:sbc.o(.text* .rodata*)
*:avctp.o(.text* .rodata*)
*:avrcp.o(.text* .rodata*)
*:besaud.o(.text* .rodata*)
*:besaudalloc.o(.text* .rodata*)
*:besaudcon.o(.text* .rodata*)
*:btm.o(.text* .rodata*)
*:btm_devicedb.o(.text* .rodata*)
*:btm_handle_hcievent.o(.text* .rodata*)
*:btm_sco_interface.o(.text* .rodata*)
*:btm_security.o(.text* .rodata*)
*:avdtp.o(.text* .rodata*)
*:connect.o(.text* .rodata*)
*:hfp.o(.text* .rodata*)
*:hshf.o(.text* .rodata*)
*:hsp.o(.text* .rodata*)
*:l2cap.o(.text* .rodata*)
*:l2cap_er.o(.text* .rodata*)
*:bt_common.o(.text* .rodata*)
*:co_lib.o(.text* .rodata*)
*:co_ppbuff.o(.text* .rodata*)
*:md5.o(.text* .rodata*)
*:obex.o(.text* .rodata*)
*:pbap.o(.text* .rodata*)
*:rfcomm.o(.text* .rodata*)
*:sco.o(.text* .rodata*)
*:sdp.o(.text* .rodata*)
*:co_printf.o(.text* .rodata*)
*:co_queue.o(.text* .rodata*)
*:co_timer.o(.text* .rodata*)
*:cobuf.o(.text* .rodata*)
*:bes_os.o(.text* .rodata*)
*:hci.o(.text* .rodata*)
*:hci_buff.o(.text* .rodata*)
*:bt_schedule.o(.text* .rodata*)

#endif

#endif

#if 1
            /*af */
            *:audioflinger.o(.text* .rodata*)

            /*intersys*/
            *:hal_intersys.o(.text* .rodata*)
            *:intersyshci.o(.text* .rodata*)
            *:trans_adapt_v2.o(.text* .rodata*)

            /*tws ibrt*/

            *:app_ibrt_bt_profile_sync.o(.text* .rodata*)
            *:app_ibrt_ui.o(.text* .rodata*)
            *:app_tws_besaud.o(.text* .rodata*)
            *:app_tws_ctrl_thread.o(.text* .rodata*)
            *:app_tws_ibrt.o(.text* .rodata*)
            *:app_tws_ibrt_audio_analysis.o(.text* .rodata*)
            *:app_tws_ibrt_audio_sync.o(.text* .rodata*)
            *:app_tws_ibrt_cmd_handler.o(.text* .rodata*)
            *:app_tws_ibrt_mock.o(.text* .rodata*)
            *:app_tws_ibrt_queue.o(.text* .rodata*)
            *:app_tws_ibrt_trace.o(.text* .rodata*)
            *:app_tws_profile_sync.o(.text* .rodata*)
            *:app_vendor_cmd_evt.o(.text* .rodata*)


                    /*app ibrt*/
                    *:app_ibrt_customif_cmd.o(.text* .rodata*)
                    *:app_ibrt_customif_ui.o(.text* .rodata*)
                    *:app_ibrt_if.o(.text* .rodata*)
                    *:app_ibrt_keyboard.o(.text* .rodata*)
                    *:app_ibrt_nvrecord.o(.text* .rodata*)
                    *:app_ibrt_ota_update.o(.text* .rodata*)
                    *:app_ibrt_peripheral_manager.o(.text* .rodata*)
                    *:app_ibrt_search_pair_ui.o(.text* .rodata*)
                    *:app_ibrt_ui_test.o(.text* .rodata*)
                    *:app_ibrt_voice_report.o(.text* .rodata*)
#endif
                    /*rtos*/
                    *:SVC_Table.o(.text* .rodata*)
                    *:HAL_CM4.o(.text* .rodata*)
                    *:RTX_Conf_CM.o(.text* .rodata*)
                    *:rt_Time.o(.text* .rodata*)
                    *:rt_Task.o(.text* .rodata*)
                    *:rt_System.o(.text* .rodata*)
                    *:rt_Semaphore.o(.text* .rodata*)
                    *:rt_Robin.o(.text* .rodata*)
                    *:rt_Mutex.o(.text* .rodata*)
                    *:rt_MemBox.o(.text* .rodata*)
                    *:rt_Mailbox.o(.text* .rodata*)
                    *:rt_List.o(.text* .rodata*)
                    *:rt_Event.o(.text* .rodata*)
                    *:rt_CMSIS.o(.text* .rodata*)
                    *:HAL_CM.o(.text* .rodata*)

/*services multimedia*/

        *:g711.o(.text* .rodata*)
        *:g726.o(.text* .rodata*)
        *:g726_16.o(.text* .rodata*)
        *:g726_24.o(.text* .rodata*)
        *:g726_32.o(.text* .rodata*)
        *:g72x.o(.text* .rodata*)
        *:armfirfilt.o(.text* .rodata*)
        *:buffer_manager.o(.text* .rodata*)
        *:exp_func_tables.o(.text* .rodata*)
        *:fastmath_const.o(.text* .rodata*)
        *:firfilt.o(.text* .rodata*)
        *:fraction_delay.o(.text* .rodata*)
        *:frame_resize.o(.text* .rodata*)
        *:fftfilt.o(.text* .rodata*)
        *:speech_iir_calibration.o(.text* .rodata*)
        *:speech_aec.o(.text* .rodata*)
        *:codec_op_cpx.o(.text* .rodata*)
        *:codec_op_etsi.o(.text* .rodata*)
        *:codec_op_float.o(.text* .rodata*)
        *:codec_op_lib.o(.text* .rodata*)
        *:codec_op_netsi.o(.text* .rodata*)
        *:codec_op_vec.o(.text* .rodata*)
        *:med_aec_delay.o(.text* .rodata*)
        *:med_aec_hf_af.o(.text* .rodata*)
        *:med_aec_hf_nlp.o(.text* .rodata*)
        *:med_aec_hpf.o(.text* .rodata*)
        *:med_aec_main.o(.text* .rodata*)
        *:med_emph_filter.o(.text* .rodata*)
        *:med_fft.o(.text* .rodata*)
        *:med_fft_tab.o(.text* .rodata*)
        *:med_kissfft.o(.text* .rodata*)
        *:mdf1.o(.text* .rodata*)
        *:digital_agc.o(.text* .rodata*)
        *:triple_mic_denoise3.o(.text* .rodata*)
        *:buffer.o(.text* .rodata*)
        *:jitter.o(.text* .rodata*)
        *:scal.o(.text* .rodata*)
        *:smallft.o(.text* .rodata*)
        *:speech_ns.o(.text* .rodata*)
        *:cng.o(.text* .rodata*)
        *:ns3f.o(.text* .rodata*)
        *:wnr.o(.text* .rodata*)
        *:table.o(.text* .rodata*)
        *:dtw.o(.text* .rodata*)
        *:mfcc.o(.text* .rodata*)
        *:recognition.o(.text* .rodata*)
        *:vad.o(.text* .rodata*)
        *:resample.o(.text* .rodata*)
        *:spectrum.o(.text* .rodata*)
        *:spectrum_fix.o(.text* .rodata*)
        *:window_generator.o(.text* .rodata*)
        *:speech_dc_filter.o(.text* .rodata*)
        *:iirfilt_design.o(.text* .rodata*)
        *:genericStds.o(.text* .rodata*)
        *:conv_string.o(.text* .rodata*)
        *:cmdl_parser.o(.text* .rodata*)
        *:tran_det.o(.text* .rodata*)
        *:ton_corr.o(.text* .rodata*)
        *:sbrenc_freq_sca.o(.text* .rodata*)
        *:sbr_rom.o(.text* .rodata*)
        *:sbr_ram.o(.text* .rodata*)
        *:sbr_misc.o(.text* .rodata*)
        *:sbr_encoder.o(.text* .rodata*)
        *:resampler.o(.text* .rodata*)
        *:ps_main.o(.text* .rodata*)
        *:ps_encode.o(.text* .rodata*)
        *:ps_bitenc.o(.text* .rodata*)
        *:nf_est.o(.text* .rodata*)
        *:mh_det.o(.text* .rodata*)
        *:invf_est.o(.text* .rodata*)
        *:fram_gen.o(.text* .rodata*)
        *:env_est.o(.text* .rodata*)
        *:env_bit.o(.text* .rodata*)
        *:code_env.o(.text* .rodata*)
        *:bit_sbr.o(.text* .rodata*)
        *:sbrdecoder.o(.text* .rodata*)
        *:sbrdec_freq_sca.o(.text* .rodata*)
        *:sbrdec_drc.o(.text* .rodata*)
        *:sbr_rom.o(.text* .rodata*)
        *:sbr_ram.o(.text* .rodata*)
        *:sbr_dec.o(.text* .rodata*)
        *:sbr_deb.o(.text* .rodata*)
        *:sbr_crc.o(.text* .rodata*)
        *:psdec_hybrid.o(.text* .rodata*)
        *:psdec.o(.text* .rodata*)
        *:psbitdec.o(.text* .rodata*)
        *:lpp_tran.o(.text* .rodata*)
        *:huff_dec.o(.text* .rodata*)
        *:env_extr.o(.text* .rodata*)
        *:env_dec.o(.text* .rodata*)
        *:env_calc.o(.text* .rodata*)
        *:pcmutils_lib.o(.text* .rodata*)
        *:limiter.o(.text* .rodata*)
        *:tpenc_lib.o(.text* .rodata*)
        *:tpenc_latm.o(.text* .rodata*)
        *:tpenc_asc.o(.text* .rodata*)
        *:tpenc_adts.o(.text* .rodata*)
        *:tpenc_adif.o(.text* .rodata*)
        *:tpdec_drm.o(.text* .rodata*)
        *:tpdec_adts.o(.text* .rodata*)
        *:tpdec_adif.o(.text* .rodata*)
        *:scale.o(.text* .rodata*)
        *:qmf.o(.text* .rodata*)
        *:mdct.o(.text* .rodata*)
        *:fixpoint_math.o(.text* .rodata*)
        *:fft_rad2.o(.text* .rodata*)
        *:fft.o(.text* .rodata*)
        *:FDK_trigFcts.o(.text* .rodata*)
        *:FDK_tools_rom.o(.text* .rodata*)
        *:FDK_hybrid.o(.text* .rodata*)
        *:FDK_crc.o(.text* .rodata*)
        *:FDK_core.o(.text* .rodata*)
        *:FDK_bitbuffer.o(.text* .rodata*)
        *:dct.o(.text* .rodata*)
        *:autocorr2nd.o(.text* .rodata*)
        *:transform.o(.text* .rodata*)
        *:tonality.o(.text* .rodata*)
        *:spreading.o(.text* .rodata*)
        *:sf_estim.o(.text* .rodata*)
        *:quantize.o(.text* .rodata*)
        *:qc_main.o(.text* .rodata*)
        *:psy_main.o(.text* .rodata*)
        *:psy_configuration.o(.text* .rodata*)
        *:pre_echo_control.o(.text* .rodata*)
        *:pnsparam.o(.text* .rodata*)
        *:noisedet.o(.text* .rodata*)
        *:ms_stereo.o(.text* .rodata*)
        *:metadata_main.o(.text* .rodata*)
        *:metadata_compressor.o(.text* .rodata*)
        *:line_pe.o(.text* .rodata*)
        *:intensity.o(.text* .rodata*)
        *:grp_data.o(.text* .rodata*)
        *:dyn_bits.o(.text* .rodata*)
        *:chaosmeasure.o(.text* .rodata*)
        *:channel_map.o(.text* .rodata*)
        *:block_switch.o(.text* .rodata*)
        *:bitenc.o(.text* .rodata*)
        *:bit_cnt.o(.text* .rodata*)
        *:bandwidth.o(.text* .rodata*)
        *:band_nrg.o(.text* .rodata*)
        *:adj_thr.o(.text* .rodata*)
        *:aacenc_tns.o(.text* .rodata*)
        *:aacEnc_rom.o(.text* .rodata*)
        *:aacEnc_ram.o(.text* .rodata*)
        *:aacenc_pns.o(.text* .rodata*)
        *:aacenc_lib.o(.text* .rodata*)
        *:aacenc.o(.text* .rodata*)
        *:rvlcconceal.o(.text* .rodata*)
        *:rvlcbit.o(.text* .rodata*)
        *:rvlc.o(.text* .rodata*)
        *:ldfiltbank.o(.text* .rodata*)
        *:conceal.o(.text* .rodata*)
        *:aacdec_hcrs.o(.text* .rodata*)
        *:aacdec_hcr_bit.o(.text* .rodata*)
        *:aacdec_hcr.o(.text* .rodata*)
        *:aacdec_drc.o(.text* .rodata*)
        *:aac_rom.o(.text* .rodata*)
        *:aac_ram.o(.text* .rodata*)
        *:floatlimiter.o(.text* .rodata*)
        *:audio_resample_ex_32bit.o(.text* .rodata*)
        *:audio_resample_ex.o(.text* .rodata*)
        *:audio_resample.o(.text* .rodata*)
        *:limiter.o(.text* .rodata*)
        *:hw_iir_process_best2300p.o(.text* .rodata*)
        *:hw_codec_iir_process_best2300p.o(.text* .rodata*)
        *:fir_process_best2300p.o(.text* .rodata*)
        *:dsd_process_best2300p.o(.text* .rodata*)
        *:crossover_filter.o(.text* .rodata*)
        *:audio_memory.o(.text* .rodata*)
        *:anc_process_best2300p.o(.text* .rodata*)
        *:adp_smallft.o(.text* .rodata*)
        *:adp_filter.o(.text* .rodata*)
        *:adp_fftwrap.o(.text* .rodata*)
        *:fft128dot.o(.text* .rodata*)
        *:resample_coef.o(.text* .rodata*)
        *:eq_cfg.o(.text* .rodata*)

/*platform*/
#if 1
        *:main.o (.text* .rodata)
        *:hwtimer_list.o (.text* .rodata)
        *:hal_mcu2cp_cp.o (.text* .rodata)
        *:hal_mcu2cp_mcu.o (.text* .rodata)
        *:hal_sleep_mcu_pd.o (.text* .rodata)
        *:hal_wdt.o (.text* .rodata)
        *:hal_usbhost.o (.text* .rodata)
        *:hal_usb.o (.text* .rodata)
        *:hal_uart.o (.text* .rodata)
        *:hal_transq.o (.text* .rodata)
        *:hal_trace.o (.text* .rodata)
        *:hal_tdm.o (.text* .rodata)
        *:hal_sleep.o (.text* .rodata)
        *:hal_sdmmc.o (.text* .rodata)
        *:hal_rtc.o (.text* .rodata)
        *:hal_pwm.o (.text* .rodata)
        *:hal_overlay.o (.text* .rodata)
        *:hal_key.o (.text* .rodata)
        *:hal_i2s_tdm.o (.text* .rodata)
        *:hal_i2s.o (.text* .rodata)
        *:hal_i2c.o (.text* .rodata)
        *:hal_gpio.o (.text* .rodata)
        *:hal_gpadc.o (.text* .rodata)
        *:hal_dma.o (.text* .rodata)
        *:hal_codec_common.o (.text* .rodata)
        *:hal_cmd.o (.text* .rodata)

*:hal_intersys.o (.text* .rodata)
*:hal_psram.o (.text* .rodata)
*:hal_memsc.o (.text* .rodata)
*:hal_spdif.o (.text* .rodata)
*:hal_phyif.o (.text* .rodata)
#else

#endif
/*
        *:hal_cmu_common.o (.text* .rodata)
        *:hal_timer.o (.text* .rodata)
            *:hal_sysfreq.o (.text* .rodata)
            *:hal_spi.o (.text* .rodata)

        *:hal_analogif_best2300p.o (.text* .rodata)
        *:hal_cmu_best2300p.o (.text* .rodata)
        *:hal_iomux_best2300p.o (.text* .rodata)
        */
        #if 1
        *:hal_psc_best2300p.o (.text* .rodata)
        *:hal_sensor_eng_best2300p.o (.text* .rodata)
        *:usbphy_best2300p.o (.text* .rodata)
        *:pmu_best2300p.o (.text* .rodata)
        *:analog_best2300p.o (.text* .rodata)
        *:codec_best2300p.o (.text* .rodata)
        *:codec_tlv32aic32.o (.text* .rodata)
    *:hal_codec_best2300p.o (.text* .rodata)
        *:bt_drv_reg_op.o (.text* .rodata)
        *:bt_drv_calibration.o (.text* .rodata)
        *:bt_drv.o (.text* .rodata)
        *:iqcorrect.o (.text* .rodata)
        *:bt_drv_uart_bridge_intsys.o (.text* .rodata)
        *:bt_drv_rfconfig.o (.text* .rodata)
        *:bt_drv_patch.o (.text* .rodata)
        *:bt_drv_config.o (.text* .rodata)
        *:bt_drv_common.o (.text* .rodata)
        *:btpcm.o (.text* .rodata)
        *:sbcaac_dummy.o (.text* .rodata)
        *:usb_cdc.o (.text* .rodata)
        *:usb_audio_sync.o (.text* .rodata)
        *:usb_audio.o (.text* .rodata)
        *:usb_dev_desc.o (.text* .rodata)
        #endif
/*
*:system_utils.o (.text* .rodata)
        */
        *:system_cp.o (.text* .rodata)
        *:system_ARMCM.o (.text* .rodata)
        *:patch.o (.text* .rodata)
        *:mpu.o (.text* .rodata)
        *:cmsis_nvic.o (.text* .rodata)
        *:arm_var_q31.o (.text* .rodata)
        *:arm_var_q15.o (.text* .rodata)
        *:arm_var_f32.o (.text* .rodata)
        *:arm_std_q31.o (.text* .rodata)
        *:arm_std_q15.o (.text* .rodata)
        *:arm_std_f32.o (.text* .rodata)
        *:arm_rms_q31.o (.text* .rodata)
        *:arm_rms_q15.o (.text* .rodata)
        *:arm_rms_f32.o (.text* .rodata)
        *:arm_power_q7.o (.text* .rodata)
        *:arm_power_q31.o (.text* .rodata)
        *:arm_power_q15.o (.text* .rodata)
        *:arm_power_f32.o (.text* .rodata)
        *:arm_min_q7.o (.text* .rodata)
        *:arm_min_q31.o (.text* .rodata)
        *:arm_min_q15.o (.text* .rodata)
        *:arm_min_f32.o (.text* .rodata)
        *:arm_mean_q7.o (.text* .rodata)
        *:arm_mean_q31.o (.text* .rodata)
        *:arm_mean_q15.o (.text* .rodata)
        *:arm_mean_f32.o (.text* .rodata)
        *:arm_max_q7.o (.text* .rodata)
        *:arm_max_q31.o (.text* .rodata)
        *:arm_max_q15.o (.text* .rodata)
        *:arm_max_f32.o (.text* .rodata)
        *:arm_const_structs.o (.text* .rodata)
        *:arm_common_tables.o (.text* .rodata)
        *:arm_dot_prod_q15.o (.text* .rodata)
        *:arm_biquad_cascade_df1_init_f32.o (.text* .rodata)
        *:arm_fir_init_q15.o (.text* .rodata)
        *:arm_fir_fast_q15.o (.text* .rodata)
        *:arm_bitreversal2.o (.text* .rodata)
        *:arm_cfft_q15.o (.text* .rodata)
        *:arm_cfft_radix4_q15.o (.text* .rodata)
        *:arm_cfft_radix8_f32.o (.text* .rodata)
        *:arm_rfft_fast_init_f32.o (.text* .rodata)
        *:arm_rfft_init_q15.o (.text* .rodata)
        *:arm_rfft_q15.o (.text* .rodata)
        *:arm_copy_q31.o (.text* .rodata)
        *:arm_fill_q31.o (.text* .rodata)
        *:arm_q15_to_float.o (.text* .rodata)
        *:arm_dot_prod_q7.o (.text* .rodata)
        *:arm_add_q15.o (.text* .rodata)
        *:arm_shift_q15.o (.text* .rodata)
        *:arm_negate_q15.o (.text* .rodata)
        *:arm_offset_q15.o (.text* .rodata)
        *:arm_mult_q15.o (.text* .rodata)


		*(.fast_text_sram*)
		. = ALIGN(4);
		__fast_sram_text_exec_end__ = .;
	} > FRAMX

	__fast_sram_text_data_end_flash__ = __fast_sram_text_data_start_flash__ + SIZEOF(.fast_text_sram);
	__fast_sram_text_data_start__ = RAMX_TO_RAM(__fast_sram_text_exec_start__);
	__fast_sram_text_data_end__ = RAMX_TO_RAM(__fast_sram_text_exec_end__);

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	__cp_text_sram_start_flash__ = __fast_sram_text_data_end_flash__;

	.cp_text_sram (ORIGIN(RAMCPX)) : AT (__cp_text_sram_start_flash__)
	{
		__cp_text_sram_exec_start__ = .;
		*(.cp_text_sram*)
		. = ALIGN(4);
		__cp_text_sram_exec_end__ = .;
	} > RAMCPX

	__cp_text_sram_start = RAMX_TO_RAM(__cp_text_sram_exec_start__);
	__cp_text_sram_end = RAMX_TO_RAM(__cp_text_sram_exec_end__);

	.vector_table_cp (ORIGIN(RAMCP)) (NOLOAD) :
	{
		KEEP(*(.vector_table_cp))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAMCP

	__cp_data_sram_start_flash__ = __cp_text_sram_start_flash__ + SIZEOF(.cp_text_sram);

	.cp_data_sram (.) : AT (__cp_data_sram_start_flash__)
	{
		__cp_data_sram_start = .;
		*(.cp_data*)
		. = ALIGN(4);
		__cp_data_sram_end = .;
	} > RAMCP

	__cp_sram_end_flash__ = __cp_data_sram_start_flash__ + SIZEOF(.cp_data_sram);

	.cp_bss_sram (.) (NOLOAD) :
	{
		__cp_bss_sram_start = .;
		*(.cp_bss*)
		. = ALIGN(4);
		__cp_bss_sram_end = .;
	} > RAMCP

	__overlay_text_start_flash__ = __cp_sram_end_flash__;
#else
	__overlay_text_start_flash__ = __fast_sram_text_data_end_flash__;
#endif

#ifdef OVERLAY_FOR_CP
	__overlay_text_start__ = __cp_text_sram_end;
#else
	__overlay_text_start__ = __fast_sram_text_data_end__;
#endif
	__overlay_text_exec_start__ = RAM_TO_RAMX(__overlay_text_start__);

	OVERLAY (__overlay_text_exec_start__) : NOCROSSREFS AT (__overlay_text_start_flash__)
	{
#ifndef NO_OVERLAY
		.overlay_text0
		{
			LONG(0);

#if !defined(GLOBAL_SRAM_KISS_FFT)
			*:kiss_fft.o(.text*)
			*:kiss_fftr.o(.text*)
			*:fftwrap.o(.text*)
#endif

#if !defined(GLOBAL_SRAM_CMSIS_FFT)
			*:cmsis_fftwrap.o(.text*)
			*:arm_rfft_fast_f32.o(.text*)
			*:arm_cfft_f32.o(.text*)
#endif

#if defined(SCO_OPTIMIZE_FOR_RAM)
			*:arm_biquad_cascade_df1_f32.o(.text*  .rodata*)
			*:integer_delay.o(.text* .rodata*)
			*:lc_mmse_ns_float.o(.text.speech_ns2float_process .rodata*)
			*:coherence.o(.text* .rodata*)
			*:echo_suppression.o(.text.float_cmp .rodata*)
			*:echo_suppression.o(.text.es_process .rodata*)
			*:lcmmse.o(.text.lcmmse_noise_estimate .rodata*)
			*:lcmmse.o(.text.lcmmse_get_speech_prob .rodata*)
			*:logmmse.o(.text.filterbank_compute_bank_pflt .rodata*)
			*:logmmse.o(.text.filterbank_compute_psd_pflt .rodata*)
			*:logmmse.o(.text.logmmse_analysis .rodata*)
			*:logmmse.o(.text.logmmse_process .rodata*)
#else
			/* common */
			*:ae_common.o(.text* .rodata*)
			*:ae_rand.o(.text* .rodata*)
			*:fastmath.o(.text* .rodata*)
			*:integer_delay.o(.text* .rodata*)
			*:iirfilt.o(.text* .rodata*)
			*:ringbuf.o(.text* .rodata*)
			*:scale_int16.o(.text* .rodata*)
			*:speech_conv.o(.text* .rodata*)
			*:speech_trans_buf.o(.text* .rodata*)

			*:preprocess.o(.text* .rodata*)
			*:filterbank.o(.text* .rodata*)
			*:mdf.o(.text* .rodata*)
			*:plc_8000.o(.text* .rodata*)
#if defined(HFP_1_6_ENABLE)
			*:dspfns.o(.text* .rodata*)
			*:lpc_plc.o(.text* .rodata*)
			*:plc_16000.o(.text* .rodata*)
#endif
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
			*:Pcm8k_Cvsd.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_2MIC_NS)
			*:webrtc_vad.o(.text.WebRtcVad_Process)
			*:webrtc_vad.o(.text.WebRtcVad_ValidRateAndFrameLength)
			*:webrtc_vad.o(.text.WebRtcVad_CalcVad16khz)
			*:webrtc_vad.o(.text.WebRtcVad_Downsampling)
			*:webrtc_vad.o(.text.WebRtcVad_CalcVad8khz)
			*:vad_filterbank.o(.text.*)
			*:energy.o(.text.*)
			*:get_scaling_square.o(.text.*)
			*:dual_mic_denoise.o(.text*)
			*:cross_correlation.o(.text*)
			*:min_max_operations.o(.text*)
			*:resample_by_2_internal.o(.text*)
			*:division_operations.o(.text*)
			*:downsample_fast.o(.text*)
			*:resample_fractional.o(.text*)
			*:resample_48khz.o(.text*)
			*:vad_sp.o(.text*)
			*:vad_core.o(.text*)
			*:webrtc_vad.o(.text*)
			*:vad_gmm.o(.text*)
#endif
#if defined(SPEECH_TX_2MIC_NS2)
			*:speech_2mic_ns2_denoise.o(.text*)
#endif
#if defined(SPEECH_TX_2MIC_NS5)
            *:leftright_denoise.o(.text*)
#endif
#if defined(SPEECH_TX_NS2) || defined(SPEECH_RX_NS2)
			*:lc_mmse_ns.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_NS2FLOAT) || defined(SPEECH_RX_NS2FLOAT)
			*:lc_mmse_ns_float.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_NS3) || defined(SPEECH_RX_NS3)
			*:ns3.o(.text* .rodata*)
			*:noise_suppression_x.o(.text* .rodata*)
			*:nsx_core.o(.text* .rodata*)
			*:nsx_core_c.o(.text* .rodata*)
			*:copy_set_operations.o(.text* .rodata*)
			*:cross_correlation.o(.text* .rodata*)
			*:division_operations.o(.text* .rodata*)
			*:dot_product_with_scale.o(.text* .rodata*)
			*:downsample_fast.o(.text* .rodata*)
			*:energy.o(.text* .rodata*)
			*:get_scaling_square.o(.text* .rodata*)
			*:min_max_operations.o(.text* .rodata*)
			*:real_fft.o(.text* .rodata*)
			*:resample_by_2.o(.text* .rodata*)
			*:resample_by_2_internal.o(.text* .rodata*)
			*:spl_init.o(.text* .rodata*)
			*:spl_sqrt.o(.text* .rodata*)
			*:spl_sqrt_floor.o(.text* .rodata*)
			*:vector_scaling_operations.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_AEC2FLOAT)
			*:coherence.o(.text* .rodata*)
			*:echo_canceller.o(.text* .rodata*)
			*:echo_suppression.o(.text* .rodata*)
			*:lcmmse.o(.text* .rodata*)
			*:logmmse.o(.text* .rodata*)
			*:mdf2.o(.text* .rodata*)
			*:noise_generator.o(.text* .rodata*)
			*:pink_noise_gen.o(.text* .rodata*)
			*:white_noise_gen.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_3MIC_NS)
			*:speech_3mic_ns.o(.text* .rodata*)
#endif
#if defined(MSBC_8K_SAMPLE_RATE)
			*:iir_resample.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_COMPEXP)
			*:compexp.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_EQ) || defined(SPEECH_RX_EQ) || defined(MSBC_8K_SAMPLE_RATE)
			*:speech_eq.o(.text* .rodata*)
			*:speech_arm_eq.o(.text* .rodata*)
			*:arm_biquad_cascade_df1_f32.o(.text* .rodata*)
#endif
#if defined(SPEECH_TX_POST_GAIN) || defined(SPEECH_RX_POST_GAIN)
			*:speech_gain.o(.text* .rodata*)
#endif
#if defined(__SMARTVOICE__)
#if defined(SPEECH_TX_2MIC_NS2) || defined(SPEECH_TX_NS2)
			*:app_smartvoice.o(.text._Z23mic_data_process_deinitv)
			*:app_smartvoice.o(.text._Z21mic_data_process_initv)
			*:app_smartvoice.o(.text._Z20mic_data_process_runPsm)
#endif
#endif

#endif // #if !defined(SCO_OPTIMIZE_FOR_RAM)
			*(.overlay_text0)
			. = ALIGN(4);
		}
		.overlay_text1
		{
			LONG(0);
#if !defined(RB_CODEC)
#if !defined(HFP_1_6_ENABLE)
			*:sbc.o(.text* .rodata*)
			*:sbc_math.o(.text* .rodata*)
#endif
			*:fixedpoint.o(.text* .rodata*)
			*:replaygain.o(.text* .rodata*)
			*:dsp_core.o(.text* .rodata*)
			*:dsp_misc.o(.text* .rodata*)
			*:dsp_filter.o(.text* .rodata*)
			*:dsp_sample_input.o(.text* .rodata*)
			*:dsp_sample_output.o(.text* .rodata*)
			*:eq.o(.text* .rodata*)
			*:pga.o(.text* .rodata*)
#if !defined(A2DP_AAC_ON) && !defined(A2DP_LHDC_ON)
			*:fir_process.o(.text* .rodata*)
			*:iir_process.o(.text.iir_run .text.*iir_run_per_* .rodata*)
#endif
#endif
			*(.overlay_a2dp_sbc*)
			*(.overlay_text1)
			. = ALIGN(4);
		}
		.overlay_text2
		{
			LONG(0);
			*:fmdec.o(.text* .data* .rodata* .bss*)
			*(.overlay_text2)
			. = ALIGN(4);
		}
		.overlay_text3
		{
			LONG(0);
			*:fft128dot.o(.text*)
			*(.overlay_text3)
			. = ALIGN(4);
		}
		.overlay_text4
		{
			LONG(0);
#if defined(A2DP_AAC_ON)
			*:block.o(.text.*CBlock_GetEscape*  .rodata*)
			*:tpdec_lib.o(.text.*transportDec_GetBitstream* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_GetAuBitsRemaining* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_GetValue* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_Read* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_FillData* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_ReadAccessUnit* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_readHeader* .rodata*)
			*:block.o(.text.*ApplyTools* .rodata*)
			*:aacdec_pns.o(.text.*CPns_Apply* .rodata*)
			*:aacdec_tns.o(.text.*CTns_Apply* .rodata*)
			*:aacdec_tns.o(.text.*CTns_Reset* .rodata*)
			*:aacdec_pns.o(.text.*CPns_ResetData* .rodata*)
			*:pulsedata.o(.text.*CPulseData_Read* .rodata*)
#if !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:channel.o(.text.*CChannelElement_Read* .rodata*)
			*:tpdec_asc.o(.text.*CProgramConfig_Reset* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_CrcCheck* .rodata*)
			*:block.o(.text.*CBlock_FrequencyToTime* .rodata*)
			*:block.o(.text.*CBlock_ReadSectionData* .rodata*)
			*:channel.o(.text.*CChannelElement_Decode* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_CrcEndReg* .rodata*)
			*:aacdecoder.o(.text.*CAacDecoder_DecodeFrame* .rodata*)
#endif

#if !defined(__SBC_FUNC_IN_ROM_VBEST2000__) && !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:block.o(.text.*CBlock_ReadSpectralData* .rodata*)
			*:block.o(.text.*CBlock_ScaleSpectralData* .rodata*)
			*:aacdec_tns.o(.text.*CTns_ReadDataPresentFlag* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_CrcStartReg* .rodata*)
			*:block.o(.text.*CBlock_ReadScaleFactorData* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_EndAccessUnit* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_GetNrOfSubFrames* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_GetAuBitsTotal* .rodata*)
			*:tpdec_asc.o(.text.*CProgramConfig_LookupElement* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_GetFrameLengthInBits* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_ReadPayloadLengthInfo* .rodata*)
			*:block.o(.text.*CBlock_InverseQuantizeSpectralData* .rodata*)
			*:channelinfo.o(.text.*IcsRead* .rodata*)
			*:aacdec_tns.o(.text.*CTns_Read* .rodata*)
			*:tpdec_lib.o(.text.*transportDec_AdjustEndOfAccessUnit* .rodata*)
			*:pulsedata.o(.text.*CPulseData_Apply* .rodata*)
			*:tpdec_asc.o(.text.*AudioSpecificConfig_Init* .rodata*)
			*:tpdec_asc.o(.text.*AudioSpecificConfig_Parse* .rodata*)
			*:tpdec_latm.o(.text.*CLatmDemux_ReadStreamMuxConfig* .rodata*)
			*:tpdec_asc.o(.text.*getSampleRate* .rodata*)
			*:tpdec_asc.o(.text.*getAOT* .rodata*)
			*:aacdecoder.o(.text.*CAacDecoder_Init* .rodata*)
			*:tpdec_asc.o(.text.*CProgramConfig_Init* .rodata*)
			*:aacdecoder_lib.o(.text.*aacDecoder_ConfigCallback* .rodata*)
			*:aacdecoder.o(.text.*CAacDecoder_ExtPayloadParse* .rodata*)
			*:stereo.o(.text.*CJointStereo_Read* .rodata*)
#endif
#endif
			*(.overlay_a2dp_aac*)
			*(.overlay_text4)
			. = ALIGN(4);
		}
		.overlay_text5
		{
			LONG(0);
			*:mpa.o(.text*)
			*:bit.o(.text*)
			*:frame.o(.text*)
			*:huffman.o(.text*)
			*:layer12.o(.text*)
			*:layer3.o(.text*)
			*:stream.o(.text*)
			*:synth.o(.text*)
			*(.overlay_text5)
			. = ALIGN(4);
		}
		.overlay_text6
		{
			LONG(0);
#if defined(A2DP_SCALABLE_ON)
			*:Audio_asm.o(.text*)
			*:vq.o(.text*)
			*:util.o(.text*)
			*:rate.o(.text*)
			*:quant_bands.o(.text*)
			*:modes.o(.text*)
			*:mathops.o(.text*)
			*:laplace.o(.text*)
			*:entdec.o(.text*)
			*:entcode.o(.text*)
			*:cwrs.o(.text*)
			*:audio_int32.o(.text*)
			*:audio_decoder.o(.text*)
			*:audio.o(.text*)
			*(.overlay_a2dp_ssc*)
#endif

#if defined(A2DP_LHDC_ON)
			*:lhdcUtil.o(.text*)
			*:dec.o(.text*)
			*:cirbuf.o(.text*)
			*(.overlay_a2dp_lhdc*)
#endif
#if defined(A2DP_LDAC_ON)
			*:ldacBT_api.o(.text*)
			*:decode_ldac.o(.text*)
			*:dequant_ldac.o(.text*)
			*:imdct_ldac.o(.text*)
			*:ldacBT_internal.o(.text*)
			*:ldaclib_api.o(.text*)
			*:memory_ldac.o(.text*)
			*:setpcm_ldac.o(.text*)
			*:sigana_ldac.o(.text*)
			*:tables_ldac.o(.text*)
			*:tables_sigproc_ldac.o(.text*)
			*:unpack_ldac.o(.text*)
			*(.overlay_a2dp_ldac*)
#endif

			*(.overlay_text6)
			. = ALIGN(4);
		}
		.overlay_text7
		{
			LONG(0);
			*(.overlay_text7)
			. = ALIGN(4);
		}
#endif
		.overlay_text_last
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > REGION_OVERLAY_TEXT

	.dummy_overlay_text_end (.) (NOLOAD) :
	{
		__overlay_text_exec_end__ = .;
	} > REGION_OVERLAY_TEXT

#ifdef OVERLAY_FOR_CP
	__fast_sram_end__ = __fast_sram_text_data_end__;
#else
	__fast_sram_end__ = RAMX_TO_RAM(__overlay_text_exec_end__);
#endif

	ASSERT(__fast_sram_end__ - __fast_sram_text_data_start__ <= FAST_XRAM_SECTION_SIZE, "fast xram sections too large")
	__free_fram = FAST_XRAM_SECTION_SIZE - (__fast_sram_end__ - __fast_sram_text_data_start__);

#ifdef OVERLAY_FOR_CP
	__overlay_data_start__ = __cp_bss_sram_end;
#else
	__overlay_data_start__ = __sram_bss_end__;
#endif

	OVERLAY (__overlay_data_start__) : NOCROSSREFS AT (__load_stop_overlay_text_last)
	{
#ifndef NO_OVERLAY
		.overlay_data0
		{
			/* Explicitly place 4 bytes at section start to avoid "section type changed to PROGBITS" warnings */
			LONG(0);
#if !defined(SCO_OPTIMIZE_FOR_RAM)

#if !defined(GLOBAL_SRAM_KISS_FFT)
			*:fftwrap.o(.data* .rodata*)
			*:kiss_fft.o(.data* .rodata*)
			*:kiss_fftr.o(.data* .rodata*)
#endif

#if !defined(GLOBAL_SRAM_CMSIS_FFT)
			*:arm_common_tables.o(.rodata.armBitRevIndexTable256)
			*:arm_common_tables.o(.rodata.twiddleCoef_256)
			*:arm_common_tables.o(.rodata.twiddleCoef_rfft_512)
			*:arm_common_tables.o(.rodata.armBitRevIndexTable128)
			*:arm_common_tables.o(.rodata.twiddleCoef_128)
			*:arm_common_tables.o(.rodata.twiddleCoef_rfft_256)
#endif

			/* common */
			*:fastmath_const.o(.data* .rodata*)

			*:preprocess.o(.data* .rodata*)
			*:filterbank.o(.data* .rodata*)
			*:mdf.o(.data* .rodata*)
			*:plc_8000.o(.data* .rodata*)
#if defined(HFP_1_6_ENABLE)
			*:table.o(.data* .rodata*)
			*:plc_16000.o(.data* .rodata*)
#endif
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
			*:Pcm8k_Cvsd.o(.data* .rodata* .bss*)
#endif
#if defined(SPEECH_TX_2MIC_NS)
			*:dual_mic_denoise.o(.data* .rodata*)
			*:vad_filterbank.o(.data* .rodata*)
			*:energy.o(.data* .rodata*)
			*:get_scaling_square.o(.data* .rodata*)

			*:cross_correlation.o(.data* .rodata*)
			*:min_max_operations.o(.data* .rodata*)
			*:resample_by_2_internal.o(.data* .rodata*)
			*:division_operations.o(.data* .rodata*)
			*:downsample_fast.o(.data* .rodata*)
			*:resample_fractional.o(.data* .rodata*)
			*:resample_48khz.o(.data* .rodata*)
			*:vad_sp.o(.data* .rodata*)
			*:vad_core.o(.data* .rodata*)
			*:webrtc_vad.o(.data* .rodata*)
			*:vad_gmm.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_2MIC_NS2)
			*:speech_2mic_ns2_denoise.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_2MIC_NS5)
            *:leftright_denoise.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS2) || defined(SPEECH_RX_NS2)
			*:lc_mmse_ns.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS3) || defined(SPEECH_RX_NS3)
			*:nsx_core.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS2FLOAT) || defined(SPEECH_RX_NS2FLOAT)
			*:lc_mmse_ns_float.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_3MIC_NS)
			*:speech_3mic_ns.o(.data* .rodata*)
#endif
#if defined(MSBC_8K_SAMPLE_RATE)
			*:iir_resample.o(.data* .rodata*)
#endif

#endif // #if !defined(SCO_OPTIMIZE_FOR_RAM)
			*(.overlay_data0 .overlay_rodata0)
			. = ALIGN(4);
		}
		.overlay_data1
		{
			LONG(0);
#if !defined(RB_CODEC)
#if !defined(HFP_1_6_ENABLE)
			*:sbc.o(.data* .rodata*)
			*:sbc_math.o(.data* .rodata*)
#endif
			*:fixedpoint.o(.data* .rodata*)
			*:replaygain.o(.data* .rodata*)
			*:dsp_core.o(.data* .rodata*)
			*:dsp_misc.o(.data* .rodata*)
			*:dsp_filter.o(.data* .rodata*)
			*:dsp_sample_input.o(.data* .rodata*)
			*:dsp_sample_output.o(.data* .rodata*)
			*:eq.o(.data* .rodata*)
			*:pga.o(.data* .rodata*)
#if !defined(A2DP_AAC_ON)
			*:fir_process.o(.data* .rodata*)
#endif
#endif
			*(.overlay_data1 .overlay_rodata1)
			. = ALIGN(4);
		}
		.overlay_data2
		{
			LONG(0);
			*(.overlay_data2 .overlay_rodata2)
			. = ALIGN(4);
		}
		.overlay_data3
		{
			LONG(0);
			*:fft128dot.o(.data* .rodata*)
			*(.overlay_data3 .overlay_rodata3)
			. = ALIGN(4);
		}
		.overlay_data4
		{
			LONG(0);
#if defined(A2DP_AAC_ON)
			*:block.o(.data* .rodata*)
			*:tpdec_lib.o(.data* .rodata*)
			*:tpdec_latm.o(.data* .rodata*)
			*:aacdec_pns.o(.data* .rodata*)
			*:aacdec_tns.o(.data* .rodata*)
			*:pulsedata.o(.data* .rodata*)
			*:channel.o(.data* .rodata*)
			*:tpdec_asc.o(.data* .rodata*)
			*:aacdecoder.o(.data* .rodata*)
			*:channelinfo.o(.data* .rodata*)
			*:aacdecoder_lib.o(.data* .rodata*)
			*:stereo.o(.data* .rodata*)
#endif
			*(.overlay_data4 .overlay_rodata4)
			. = ALIGN(4);
		}
		.overlay_data5
		{
			LONG(0);
			*:mpa.o(.data* .bss*)
			*:bit.o(.data* .bss*)
			*:frame.o(.data* .bss*)
			*:huffman.o(.data* .bss*)
			*:layer12.o(.data* .bss*)
			*:layer3.o(.data* .bss*)
			*:stream.o(.data* .bss*)
			*:synth.o(.data* .bss*)
			*(.overlay_data5 .overlay_rodata5)
			. = ALIGN(4);
		}
		.overlay_data6
		{
			LONG(0);
#if defined(A2DP_SCALABLE_ON)
			*(.constdata)
			*:Audio_asm.o(.data* .rodata*)
			*:vq.o(.data* .rodata*)
			*:util.o(.data* .rodata*)
			*:rate.o(.data* .rodata*)
			*:quant_bands.o(.data* .rodata*)
			*:modes.o(.data* .rodata*)
			*:mathops.o(.data* .rodata*)
			*:laplace.o(.data* .rodata*)
			*:entdec.o(.data* .rodata*)
			*:entcode.o(.data* .rodata*)
			*:cwrs.o(.data* .rodata*)
			*:audio_int32.o(.data* .rodata*)
			*:audio_decoder.o(.data* .rodata*)
			*:audio.o(.data* .rodata*)
#endif
#if defined(A2DP_LHDC_ON)
			*:lhdcUtil.o(.data* .rodata*)
			*:dec.o(.data* .rodata*)
			*:cirbuf.o(.data* .rodata*)
#endif
#if defined(A2DP_LDAC_ON)
			*:ldacBT_api.o(.data* .rodata*)
			*:decode_ldac.o(.data* .rodata*)
			*:dequant_ldac.o(.data* .rodata*)
			*:imdct_ldac.o(.data* .rodata*)
			*:ldacBT_internal.o(.data* .rodata*)
			*:ldaclib_api.o(.data* .rodata*)
			*:memory_ldac.o(.data* .rodata*)
			*:setpcm_ldac.o(.data* .rodata*)
			*:sigana_ldac.o(.data* .rodata*)
			*:tables_ldac.o(.data* .rodata*)
			*:tables_sigproc_ldac.o(.data* .rodata*)
			*:unpack_ldac.o(.data* .rodata*)
#endif
			*(.overlay_data6 .overlay_rodata6)
			. = ALIGN(4);
		}
		.overlay_data7
		{
			LONG(0);
			*(.overlay_data7 .overlay_rodata7)
			. = ALIGN(4);
		}
#endif
		.overlay_data_last
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > REGION_OVERLAY_DATA

	.dummy_overlay_data_end (.) (NOLOAD) :
	{
		__overlay_data_end__ = .;
	} > REGION_OVERLAY_DATA

	ASSERT(__overlay_data_end__ - __overlay_data_start__ <= OVERLAY_DATA_SECTION_SIZE, "overlay sections too large")

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	.cp_ramx_last_dummy (NOLOAD) :
	{
		__cp_ramx_last_dummy_start = .;
	} > RAMCPX

	.cp_ram_last_dummy (NOLOAD) :
	{
		. = ALIGN(8);
		__cp_ram_last_dummy_start = .;
	} > RAMCP

	__cp_stack_top = ORIGIN(RAMCP) + LENGTH(RAMCP);
	__cp_stack_limit = __cp_stack_top - CP_STACK_SECTION_SIZE;

	__free_ramcpx = ORIGIN(RAMCPX) + LENGTH(RAMCPX) - __cp_ramx_last_dummy_start;
	ASSERT(__cp_stack_limit >= __cp_ram_last_dummy_start, "region RAMCP overflowed with stack")
	__free_ramcp = __cp_stack_limit - __cp_ram_last_dummy_start;
#endif

	.text (FLASH_TO_FLASHX(__load_stop_overlay_data_last)) : AT (FLASHX_TO_FLASH(ADDR(.text)))
	{
		*(.text*)
		*(.flash_text*)

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)
	} > FLASHX

	.ARM.extab (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.extab)))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASHX

	__exidx_start = .;
	/* .ARM.exidx contains R_ARM_PREL31 (+-0x40000000) offset to functions, which means
	 * the session location cannot be too far away from the function addresses */
	.ARM.exidx (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.exidx)))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASHX
	__exidx_end = .;

	. = FLASHX_TO_FLASH(.);

	.rodata (.) :
	{
		*(.rodata*)
		*(.flash_rodata*)
		KEEP(*(.eh_frame*))
		*(.note.gnu.build-id)
	} > FLASH

#ifdef TRACE_STR_SECTION
	.trc_str (.) :
	{
		*(.trc_str*)
	} > FLASH
#endif

	.ai.gatt.server.table (.) :
	{
	. = ALIGN(4);
	__ai_gatt_server_table_start = .;
		KEEP(*(.ai_gatt_server_table))
	__ai_gatt_server_table_end = .;
	} > FLASH

	.ai.ble.handler.table (.) :
	{
	. = ALIGN(4);
	__ai_ble_handler_table_start = .;
		KEEP(*(.ai_ble_handler_table))
	__ai_ble_handler_table_end = .;
	} > FLASH

	.ai.spp.register.table (.) :
	{
	. = ALIGN(4);
	__ai_spp_register_table_start = .;
		KEEP(*(.ai_spp_register_table))
	__ai_spp_register_table_end = .;
	} > FLASH

	.tota.cmd.table (.) :
	{
		. = ALIGN(4);
		__tota_handler_table_start = .;
		KEEP(*(.tota_handler_table))
		__tota_handler_table_end = .;
	} > FLASH

	.ai.handler.function.table (.) :
	{
	. = ALIGN(4);
	__ai_handler_function_table_start = .;
		KEEP(*(.ai_handler_function_table))
	__ai_handler_function_table_end = .;
	} > FLASH

	.custom.cmd.table (.) :
	{
		. = ALIGN(4);
		__custom_handler_table_start = .;
		KEEP(*(.custom_handler_table))
		__custom_handler_table_end = .;
	} > FLASH

	.thirdparty.event.table (.) :
	{
		. = ALIGN(4);
		__thirdparty_handler_table_start = .;
		KEEP(*(.thirdparty_handler_table))
		__thirdparty_handler_table_end = .;
	} > FLASH

#if defined(FIRMWARE_REV)
	.firmware_rev (.) :
	{
		. = ALIGN(64);
		*(.fw_rev)
	} > FLASH
#endif

	/* To copy multiple FLASH to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	/*
	.copy.table (.) :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__data2_start__)
		LONG (__data2_end__ - __data2_start__)
		__copy_table_end__ = .;
	} > FLASH
	*/

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	/*
	.zero.table (.) :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > FLASH
	*/

	/* Location counter can end up 2byte aligned with narrow Thumb code but
	   __etext is assumed by startup code to be the LMA of a section in RAM
	   which must be 4byte aligned */
	__etext = ALIGN(4);

	/* The VMA is either the end of overlay_data or the end of sram bss */

	.data : AT (__etext)
	{
		__data_start__ = .;

		*(vtable)
		. = ALIGN(4);
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))

		/* All data end */
		. = ALIGN(4);
		__data_end__ = .;

	} > RAM

	.bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.heap (.) (NOLOAD) :
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. += HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (.) (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
#if defined(ROM_UTILS_ON)
	__StackTop = ORIGIN(RAM) + LENGTH(RAM) - ROM_UTILS_RESV_RAM_SIZE;
#else
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
#endif
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;

	. = __etext + SIZEOF(.data);

	.system_info (.) :
	{
		KEEP(*(.system_info_list_size))
		KEEP(*(.system_info_list))
		. = ALIGN(4);
	}  > FLASH

	.build_info (.) :
	{
		KEEP(*(.build_info))
		. = ALIGN(4);
	} > FLASH = 0x00000000

#ifdef SLAVE_BIN_FLASH_OFFSET
	.slave_code (ORIGIN(FLASH) + SLAVE_BIN_FLASH_OFFSET):
	{
		KEEP(*(.slave_code_flash_rodata))
	} > FLASH
#endif

	/* The following section MUST be the last loaded section */
	.code_start_addr (.) :
	{
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;

	.coredump_section (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE - OTA_UPGRADE_LOG_SIZE - CORE_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__coredump_section_start = .;
		. = CORE_DUMP_SECTION_SIZE;
		__coredump_section_end = .;
	} > FLASH_NC

	.ota_upgrade_log (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE - OTA_UPGRADE_LOG_SIZE) (NOLOAD) :
	{
		__ota_upgrade_log_start = .;
		. = OTA_UPGRADE_LOG_SIZE;
		__ota_upgrade_log_end = .;
	} > FLASH_NC

	.log_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__log_dump_start = .;
		. = LOG_DUMP_SECTION_SIZE;
		__log_dump_end = .;
	} > FLASH_NC

	.crash_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE -  USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		CRASH_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__crash_dump_start = .;
		. = CRASH_DUMP_SECTION_SIZE;
		__crash_dump_end = .;
	} > FLASH_NC

	.custom_parameter (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
	 AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE) (NOLOAD) :
	{
		__custom_parameter_start = .;
		. = CUSTOM_PARAMETER_SECTION_SIZE;
		__custom_parameter_end = .;
	} > FLASH_NC

	.userdata (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2) (NOLOAD) :
	{
		__userdata_start = .;
		. = USERDATA_SECTION_SIZE*2;
		__userdata_end = .;
	} > FLASH_NC

	.audio (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE) (NOLOAD) :
	{
		__aud_start = .;
		. = AUD_SECTION_SIZE;
		__aud_end = .;
	} > FLASH_NC

	.reserved (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE - RESERVED_SECTION_SIZE) (NOLOAD) :
	{
		__reserved_start = .;
		. = RESERVED_SECTION_SIZE;
		__reserved_end = .;
	} > FLASH_NC

	.factory (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE) (NOLOAD) :
	{
		__factory_start = .;
		. = FACTORY_SECTION_SIZE;
		__factory_end = .;
	} > FLASH_NC

#ifdef OTA_BOOT_SIZE
#if (OTA_BOOT_SIZE > FLASH_SIZE)
#error "OTA_BOOT_SIZE should <= FLASH_SIZE"
#endif
#if defined(OTA_CODE_OFFSET) && (OTA_BOOT_SIZE > OTA_CODE_OFFSET)
#error "OTA_BOOT_SIZE should <= OTA_CODE_OFFSET"
#endif
	__tail_section_start = FLASH_BASE + OTA_BOOT_SIZE;
#else
	__tail_section_start = __ota_upgrade_log_start;
#endif

	ASSERT(FLASH_NC_TO_C(__tail_section_start) >= __flash_end, "region FLASH overflowed")
	__free_flash = FLASH_NC_TO_C(__tail_section_start) - __flash_end;

#if !defined(OTA_BOOT_SIZE) && defined(OTA_REMAP_OFFSET)
	ASSERT((FLASH_BASE & 0x03FFFFFF) == 0, "bad FLASH_BASE")
	ASSERT((FLASH_SIZE & (FLASH_SIZE - 1)) == 0, "bad FLASH_SIZE")
#ifdef CHIP_BEST2300P
	ASSERT(__flash_end <= FLASH_BASE + OTA_REMAP_OFFSET, "flash code size too large to remap")
	ASSERT(__flash_start - FLASH_BASE >= FLASH_NC_BASE + FLASH_SIZE - __tail_section_start, "flash code conflicted with tail sections in remap")
	ASSERT(__flash_start - FLASH_BASE >= FLASH_SIZE / (1 << (4 + 1)), "flash code cannot remap within 4 regions (max code size is half of flash size)")
	ASSERT((__flash_start - FLASH_BASE) % (FLASH_SIZE / (1 << (4 + 1))) == 0, "flash code start address not aligned with remap region boundary")
#else
	ASSERT(((OTA_REMAP_OFFSET) & (0x1000 - 1)) == 0, "OTA_REMAP_OFFSET not aligned with 4K sector boundary")
	ASSERT(__flash_end <= FLASH_BASE + (OTA_REMAP_OFFSET), "flash code size too large to remap")
	ASSERT((__tail_section_start & (0x1000 - 1)) == 0, "__tail_section_start not aligned with 4K sector boundary")
	ASSERT(__flash_end + (OTA_REMAP_OFFSET) <= __tail_section_start, "flash code conflicted with tail sections in remap")
#endif
#endif
}

