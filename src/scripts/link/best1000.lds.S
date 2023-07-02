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

#define STACK_SECTION_SIZE			0x1000
#define CP_STACK_SECTION_SIZE		0x1000

#if defined(ROM_UTILS_ON)
#if defined(CHIP_BEST1400) || defined(CHIP_BEST1402)
#define ROM_UTILS_RESV_RAM_SIZE     0x64
#else
#define ROM_UTILS_RESV_RAM_SIZE     0x0
#endif
#endif

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
#define OVERLAY_DATA_SECTION_SIZE	0x10000
#elif defined(RB_CODEC)
#define OVERLAY_DATA_SECTION_SIZE	0xA000
#else
#define OVERLAY_DATA_SECTION_SIZE	0x8000
#endif

#if !defined(FAST_XRAM_SECTION_SIZE)
#if (RAM_SIZE >= 0x48000)
#define RTOS_IN_RAM
#if defined(BTUSB_AUDIO_MODE)
#define FAST_XRAM_SECTION_SIZE		0x20000
#elif (defined(CHIP_BEST1402) && defined(SPEECH_TX_2MIC_NS2))
#define FAST_XRAM_SECTION_SIZE		0xF800
#else
#define FAST_XRAM_SECTION_SIZE		0x16000
#endif
#elif (RAM_SIZE >= 0x40000)
#if defined(CHIP_BEST1402) || defined(CHIP_BEST1400)
#define FAST_XRAM_SECTION_SIZE		0x5200
#else
#define FAST_XRAM_SECTION_SIZE		0x3800
#endif
#else
#define FAST_XRAM_SECTION_SIZE		0x6000
#endif
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
		*(.boot_rodata_flash*)
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
		*:libc_rom.o(.rodata*)

		*(.boot_rodata_sram*)
		*(.boot_data*)
		. = ALIGN(4);
		__boot_data_sram_end = .;
	} > RAM

	__boot_sram_end_flash__ = __boot_sram_start_flash__ +
								SIZEOF(.boot_text_sram) + SIZEOF(.boot_data_sram);

	.boot_bss_sram (.) (NOLOAD) : AT (ADDR(.boot_bss_sram))
	{
		__boot_bss_sram_start__ = .;
		*:hal_norflash*.o(.bss*)
		*:norflash_*.o(EXCLUDE_FILE(*:norflash_api.o) .bss*)
		*:hal_psram.o(.bss*)
		*(.boot_bss*)
		. = ALIGN(32);
		__boot_bss_sram_end__ = .;
	} > RAM

	__sram_text_data_start_flash__ = __boot_sram_end_flash__;
	__sram_text_data_start__ = __boot_bss_sram_end__;

	.sram_text (RAM_TO_RAMX(__sram_text_data_start__)) : AT (__sram_text_data_start_flash__)
	{
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
		*:hal_memsc.o(.text*)
		*:hal_trace.o(.text*)
		*:hal_trace_mod.o(.text*)
		*:mpu_armv*.o(.text*)
		*:codec_sbc.o(.text*)
		*:multi_heap.o(.text*)
		*:pool_api.o(.text*)
		*libgcc.a:(.text*)

		/* for LIBC_ROM=0 */
		*libc_nano.a:(.text*)

		/* for SCO_CP_ACCEL=1 */
		*:cmsis_dsp_lib.o(.text*)
		*libm.a:(.text*)
#endif

#ifdef KWS_IN_RAM
		*:kws_lib.o(.text*)
#endif

#if defined(GSOUND_HOTWORD_EXTERNAL)
		*:voice_sbc.o(.text*)
		*:gsound_custom_audio.o(.text*)
		*:gsound_custom_hotword_external.o(.text*)
		*:libspeech*.o(.text*)
#endif

		*:norflash_api.o(.text*)
		*(.sram_text*)
		/*
		   If you want protect one section by armv8's mpu, the section should
		   be aligned to 32bytes
		 */
		. = ALIGN(32);
		__sram_text_end__ = .;
	} > RAMX

	.sram_data (RAMX_TO_RAM(ADDR(.sram_text) + SIZEOF(.sram_text))) :
				AT (__sram_text_data_start_flash__ + SIZEOF(.sram_text))
	{
#ifdef RTOS_IN_RAM
		*:rtos_lib.o(.data* .rodata*)
#endif

#if defined(GLOBAL_SRAM_KISS_FFT)
		*:fftwrap.o(.data* .rodata*)
		*:kiss_fft.o(.data* .rodata*)
		*:kiss_fftr.o(.data* .rodata*)
#endif

#if defined(GLOBAL_SRAM_CMSIS_FFT) || defined(AI_CAPTURE_DATA_AEC)
		*:CommonTables.o(.rodata.armBitRevIndexTable256)
		*:CommonTables.o(.rodata.twiddleCoef_256)
		*:CommonTables.o(.rodata.twiddleCoef_rfft_512)
		*:CommonTables.o(.rodata.armBitRevIndexTable128)
		*:CommonTables.o(.rodata.twiddleCoef_128)
		*:CommonTables.o(.rodata.twiddleCoef_rfft_256)
#endif

#if defined(BTUSB_AUDIO_MODE)
		*:hal_usb.o(.rodata*)
#endif

#if defined(SCO_CP_ACCEL)
		*:plc_8000.o(.data* .rodata*)
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
		*:Pcm8k_Cvsd.o(.data* .rodata* .bss*)
#endif
#if defined(HFP_1_6_ENABLE)
		*:table.o(.data* .rodata*)
		*:plc_16000.o(.data* .rodata*)
#endif
		/* Move algos from overlay to here */
#endif

		*:utils.o(.data* .rodata*)
		*:cqueue.o(.data* .rodata*)
		*:iir_process.o(.data* .rodata*)
#if defined(HFP_1_6_ENABLE)
		*:sbcplc.o(.data* .rodata*)
		*:sbc.o(.data* .rodata*)
		*:sbc_math.o(.data* .rodata*)
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
		*:nsx_core.o(.data* .rodata*)
#endif

#if defined(__BESNNDenoise__)
		*:denoise_10ms.o(.data* .rodata*)
		*:denoise_20ms.o(.data* .rodata*)
#endif

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
		*:hal_memsc.o(.rodata*)
		*:hal_trace.o(.rodata*)
		*:hal_trace_mod.o(.rodata*)
		*:mpu.o(.rodata*)
		*:multi_heap.o(.rodata*)
		*:pool_api.o(.rodata*)
		*:a2dp_decoder_cp.o(.rodata*)
		*libgcc.a:(.rodata*)

		/* for LIBC_ROM=0 */
		*libc_nano.a:(.rodata*)

		/* for SCO_CP_ACCEL=1 */
		*:cmsis_dsp_lib.o(.rodata*)
		*libm.a:(.rodata*)
#endif

#if defined(GSOUND_HOTWORD_EXTERNAL)
		*:voice_sbc.o(.data* .rodata*)
		*:gsound_custom_audio.o(.data* .rodata*)
		*:gsound_custom_hotword_external.o(.data* .rodata*)
		*:libspeech*.o(.data* .rodata*)
#endif

		*:norflash_api.o(.rodata*)
		*(.sram_data*)
		. = ALIGN(4);
	} > RAM

	__sram_text_data_end_flash__ = __sram_text_data_start_flash__ +
								SIZEOF(.sram_text) + SIZEOF(.sram_data);

	.sram_bss (.) (NOLOAD) : AT (ADDR(.sram_bss))
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

#ifdef RTOS_IN_RAM
		*:rtos_lib.o(.text*)
#endif

#if defined(GLOBAL_SRAM_KISS_FFT)
		*:fftwrap.o(.text*)
		*:kiss_fft.o(.text*)
		*:kiss_fftr.o(.text*)
#endif

#if defined(GLOBAL_SRAM_CMSIS_FFT) || defined(AI_CAPTURE_DATA_AEC)
		*:cmsis_fftwrap.o(.text*)
		*:arm_rfft_fast_f32.o(.text*)
		*:arm_cfft_f32.o(.text*)
#endif

#if defined(SCO_CP_ACCEL)
		*:frame_resize.o(.text*)
		*:buffer_manager.o(.text*)
		*:plc_8000.o(.text*)
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
		*:Pcm8k_Cvsd.o(.text*)
#endif
#if defined(HFP_1_6_ENABLE)
		*:dspfns.o(.text*)
		*:lpc_plc.o(.text*)
		*:plc_16000.o(.text*)
		*:sbc_filter.o(.text*)
#endif
/* Move algos from overlay to here */

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
#if defined(__SBC_FUNC_IN_ROM__) || (defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0))
		*:sbc.o(.text*)
#else
		*:sbc.o(.text*.sbc_is_stream_info_valid)
		*:sbc.o(.text*.sbc_frame_length_get)
		*:sbc.o(.text*.sbc_4bit_crc_calc)
		*:sbc.o(.text*.sbc_bits_for_mono_alloc)
		*:sbc.o(.text*.sbc_bits_for_stereo_alloc)
		*:sbc.o(.text*.sbc_header_process)
		*:sbc.o(.text*.sbc_factors_process)
		*:sbc.o(.text*.sbc_sample_extract)
		*:sbc.o(.text*.sbc_sbsamples_make)
		*:sbc.o(.text*.sbc_joint_process)
		*:sbc.o(.text*.sbc_4band_synth_filter)
		*:sbc.o(.text*.sbc_8band_synth_filter)
		*:sbc.o(.text*.sbc_decoder_state_reset)
		*:sbc.o(.text*.sbc_frame_mute)
		*:sbc.o(.text*.sbc_frames_decode)
		*:sbc.o(.text*.sbc_samples_quant)
		*:sbc.o(.text*.sbc_decoder_init)
		*:sbc.o(.text*.sbc_encoder_init)
		*:sbc.o(.text*.sbc_frames_encode)
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
		*:fft.o(.text*._Z3fftiPlPi)
		*:fft_rad2.o(.text*dit_fft*)
		*:dct.o(.text*getTables*)
#endif
#if defined(OPUS_IN_OVERLAY)
		*:voice_opus.o(.text*)
#elif defined(VOICE_DATAPATH) || defined(__ALEXA_WWE) || defined(__KWS_ALEXA__)
		*:opus_encoder.o(.text*)
#elif defined(__AI_VOICE__) && (RAM_SIZE >= 0x48000)
		*:voice_opus.o(.text*)
		/**:opus_encoder.o(.text*)*/
		*:opus_encoder.o(.text*.frame_size_select)
		*:opus_encoder.o(.text*.opus_encode_native)
		*:opus_encoder.o(.text*.user_bitrate_to_bitrate)
		*:opus_encoder.o(.text*.compute_equiv_rate)
		*:opus_encoder.o(.text*.decide_fec)
		*:opus_encoder.o(.text*.dc_reject)
		*:opus_encoder.o(.text*.gen_toc)
		*:opus_encoder.o(.text*.opus_encode)
		*:opus_encoder.o(.text*.opus_encoder_create)
		*:opus_encoder.o(.text*.opus_encoder_get_size)
		*:opus_encoder.o(.text*.opus_encoder_init)
		*:opus_encoder.o(.text*.opus_encoder_ctl)
		*:analysis.o(.text*)
		/**:celt_encoder.o(.text*)*/
		*:celt_encoder.o(.text*.opus_custom_encoder_ctl)
		*:celt_encoder.o(.text*.celt_encoder_get_size)
		*:celt_encoder.o(.text*.opus_custom_encoder_get_size)
		*:celt_encoder.o(.text*.celt_encoder_init)
		*:celt_encoder.o(.text*.opus_custom_encoder_init_arch)
		/**:repacketizer.o(.text*)*/
		*:repacketizer.o(.text*.opus_packet_pad)
		*:repacketizer.o(.text*.opus_repacketizer_init)
		*:repacketizer.o(.text*.opus_repacketizer_cat)
		*:repacketizer.o(.text*.opus_repacketizer_cat_impl)
		*:repacketizer.o(.text*.opus_repacketizer_out_range_impl)
		*:opus.o(.text*)
		*:enc_API.o(.text*)
		/**:entenc.o(.text*)*/
		*:entenc.o(.text*.ec_enc_init)
		*:entenc.o(.text*.ec_enc_icdf)
		*:entenc.o(.text*.ec_enc_normalize)
		*:entenc.o(.text*.ec_enc_carry_out)
		*:entenc.o(.text*.ec_write_byte)
		*:entenc.o(.text*.ec_enc_patch_initial_bits)
		*:entenc.o(.text*.ec_enc_done)
		*:control_audio_bandwidth.o(.text*)
		*:encode_pulses.o(.text*)
		*:LPC_analysis_filter.o(.text*)
		*:burg_modified_FIX.o(.text*.silk_burg_modified_c)
		*:vector_ops_FIX.o(.text*.silk_scale_copy_vector16)
		*:vector_ops_FIX.o(.text*.silk_inner_prod_aligned)
		*:vector_ops_FIX.o(.text*.silk_inner_prod16_aligned_64_c)
		*:NLSF_del_dec_quant.o(.text*.silk_NLSF_del_dec_quant)
		*:encode_frame_FIX.o(.text*.silk_encode_frame_FIX)
		*:encode_frame_FIX.o(.text*.silk_encode_do_VAD_FIX)
		*:encode_frame_FIX.o(.text*.silk_LBRR_encode_FIX)
		/**:pitch.o(.text*)*/
		*:pitch.o(.text*.celt_pitch_xcorr_c)
		*:resampler.o(.text*)
		*:VAD.o(.text*)
		*:encode_indices.o(.text*)
		/**:control_codec.o(.text*)*/
		*:control_codec.o(.text*.silk_control_encoder)
		*:control_codec.o(.text*.silk_setup_resamplers)
		*:control_codec.o(.text*.silk_setup_fs)
		*:control_codec.o(.text*.silk_setup_complexity)
		*:control_codec.o(.text*.silk_setup_LBRR)
		/**:celt_lpc.o(.text*)*/
		*:celt_lpc.o(.text*._celt_autocorr)
		/**:shell_coder.o(.text*)*/
		*:shell_coder.o(.text*.combine_pulses)
		*:shell_coder.o(.text*.silk_shell_encoder)
		*:shell_coder.o(.text*.encode_split)
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

#if defined(AI_CAPTURE_DATA_AEC)
		*:coherence.o(.text*)
		*:echo_canceller.o(.text*)
		*:echo_suppression.o(.text*)
		*:lcmmse.o(.text*)
		*:logmmse.o(.text*)
		*:mdf2.o(.text*)
		*:noise_generator.o(.text*)
		*:pink_noise_gen.o(.text*)
		*:white_noise_gen.o(.text*)
#endif

#if defined(__AUDIO_SPECTRUM__)
		*:spectrum_fix.o(.text*)
		*:audio_spectrum.o(.text*)
#endif
#if defined(A2DP_SCALABLE_ON) && defined(A2DP_AAC_ON)
		*:mdct.o(.text*)
#endif
		*:crossover_filter.o(.text*)
		*:data_compressor.o(.text*)
		*:drc.o(.text*)
		*:limiter.o(.text*)
		*:iir_process.o(.text*.iir_run .text*.*iir_run_per_*)

#if defined(KWS_BES)
		*:arm_*.o(.text*)
		*:mfcc.o(.text*)
		*:cnn.o(.text*)
#ifndef __KWS_TIANMAO__
		*:command_recognition.o(.text*)
#endif
		*:arm_radix8_butterfly_f32.o(.text*)
		*:arm_bitreversal_32.o(.text*)
		*:arm_bitreversal_16.o(.text*)
		*:dct.o(.text*dct_IV*)
		*:fft.o(.text*._Z3fftiPlPi)
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

		*:coherence.o(.text*)
		*:echo_canceller.o(.text*)
		*:echo_suppression.o(.text*)
		*:lcmmse.o(.text*)
		*:logmmse.o(.text*)
		*:mdf2.o(.text*)
		*:noise_generator.o(.text*)
		*:pink_noise_gen.o(.text*)
		*:white_noise_gen.o(.text*)

		*:ae_common.o(.text*)
		*:ae_rand.o(.text*)
		*:fastmath.o(.text*)
		*:integer_delay.o(.text*)
		*:iirfilt.o(.text*)
		*:ringbuf.o(.text*)
		*:scale_int16.o(.text*)
		*:speech_conv.o(.text*)
		*:speech_trans_buf.o(.text*)

		*:ns3.o(.text*)
		*:noise_suppression_x.o(.text*)
		*:nsx_core.o(.text*)
		*:nsx_core_c.o(.text*)
		*:copy_set_operations.o(.text*)
		*:cross_correlation.o(.text*)
		*:division_operations.o(.text*)
		*:dot_product_with_scale.o(.text*)
		*:downsample_fast.o(.text*)
		*:energy.o(.text*)
		*:get_scaling_square.o(.text*)
		*:min_max_operations.o(.text*)
		*:real_fft.o(.text*)
		*:resample_by_2.o(.text*)
		*:resample_by_2_internal.o(.text*)
		*:spl_init.o(.text*)
		*:spl_sqrt.o(.text*)
		*:spl_sqrt_floor.o(.text*)
		*:vector_scaling_operations.o(.text*)
#endif
#if !defined(SCO_OPTIMIZE_FOR_RAM)
		:audio_prompt_sbc.o(.text*)
#endif
		*(.fast_text_sram*)
		/*
		   If you want protect one section by armv8's mpu, the section should
		   be aligned to 32bytes
		 */
		. = ALIGN(32);
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
		*:cp_queue.o(.text*)
#if defined(GSOUND_HOTWORD_EXTERNAL)
		//*:libspeech*.o(.text*)
#endif
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
		*:cp_queue.o(.data* .rodata*)
#if defined(GSOUND_HOTWORD_EXTERNAL)
		//*:libspeech*.o(.data* .rodata*)
#endif
		. = ALIGN(4);
		__cp_data_sram_end = .;
	} > RAMCP

	__cp_sram_end_flash__ = __cp_data_sram_start_flash__ + SIZEOF(.cp_data_sram);

	.cp_bss_sram (.) (NOLOAD) : AT (ADDR(.cp_bss_sram))
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
			*:arm_biquad_cascade_df1_f32.o(.text*)
			*:integer_delay.o(.text*)
			*:lc_mmse_ns_float.o(.text*.speech_ns2float_process)
			*:coherence.o(.text*)
			*:echo_suppression.o(.text*.float_cmp)
			*:echo_suppression.o(.text*.es_process)
			*:lcmmse.o(.text*.lcmmse_noise_estimate)
			*:lcmmse.o(.text*.lcmmse_get_speech_prob)
			*:logmmse.o(.text*.filterbank_compute_bank_pflt)
			*:logmmse.o(.text*.filterbank_compute_psd_pflt)
			*:logmmse.o(.text*.logmmse_analysis)
			*:logmmse.o(.text*.logmmse_process)
#if defined(CHIP_BEST1402) || defined(CHIP_BEST1400)
			*:lpc_plc.o(.text*.WB_PLC_common)
			*:lpc_plc.o(.text*.WB_PLC_erasure)
			*:lpc_plc.o(.text*.apfilterQ1_Q0)
			*:lpc_plc.o(.text*.apfilterQ0_Q0)
			*:lpc_plc.o(.text*.azfilterQ0_Q1)
			*:lpc_plc.o(.text*.decim)
			*:lpc_plc.o(.text*.merit)
#endif
#else
			/* common */
#if !defined(AUDIO_ANC_FB_ADJ_MC)	
			*:ae_common.o(.text*)		
			*:ae_rand.o(.text*)
			*:fastmath.o(.text*)
			*:integer_delay.o(.text*)
			*:iirfilt.o(.text*)
			*:ringbuf.o(.text*)
#endif			
			*:scale_int16.o(.text*)
			*:speech_conv.o(.text*)
			*:speech_trans_buf.o(.text*)

#if !defined(SCO_CP_ACCEL)
			*:frame_resize.o(.text*)
			*:buffer_manager.o(.text*)
			*:plc_8000.o(.text*)
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
			*:Pcm8k_Cvsd.o(.text*)
#endif
#if defined(HFP_1_6_ENABLE)
			*:dspfns.o(.text*)
			*:lpc_plc.o(.text*)
			*:plc_16000.o(.text*)
			*:sbc_filter.o(.text*)
#endif
#endif
			*:preprocess.o(.text*)
			*:filterbank.o(.text*)
			*:mdf.o(.text*)
#if defined(SPEECH_TX_2MIC_NS)
			*:webrtc_vad.o(.text*.WebRtcVad_Process)
			*:webrtc_vad.o(.text*.WebRtcVad_ValidRateAndFrameLength)
			*:webrtc_vad.o(.text*.WebRtcVad_CalcVad16khz)
			*:webrtc_vad.o(.text*.WebRtcVad_Downsampling)
			*:webrtc_vad.o(.text*.WebRtcVad_CalcVad8khz)
			*:vad_filterbank.o(.text*)
			*:energy.o(.text*)
			*:get_scaling_square.o(.text*)
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
#if defined(SPEECH_TX_2MIC_NS4)
			*:sensormic_denoise.o(.text*)
#endif
#if defined(SPEECH_TX_2MIC_NS5)
            *:leftright_denoise.o(.text*)
#endif
#if defined(SPEECH_TX_NS2) || defined(SPEECH_RX_NS2)
			*:lc_mmse_ns.o(.text*)
#endif
#if defined(SPEECH_TX_NS2FLOAT) || defined(SPEECH_RX_NS2FLOAT)
			*:lc_mmse_ns_float.o(.text*)
#endif
#if defined(SPEECH_TX_NS3) || defined(SPEECH_RX_NS3)
			*:ns3.o(.text*)
			*:noise_suppression_x.o(.text*)
			*:nsx_core.o(.text*)
			*:nsx_core_c.o(.text*)
			*:copy_set_operations.o(.text*)
			*:cross_correlation.o(.text*)
			*:division_operations.o(.text*)
			*:dot_product_with_scale.o(.text*)
			*:downsample_fast.o(.text*)
			*:energy.o(.text*)
			*:get_scaling_square.o(.text*)
			*:min_max_operations.o(.text*)
			*:real_fft.o(.text*)
			*:resample_by_2.o(.text*)
			*:resample_by_2_internal.o(.text*)
			*:spl_init.o(.text*)
			*:spl_sqrt.o(.text*)
			*:spl_sqrt_floor.o(.text*)
			*:vector_scaling_operations.o(.text*)
#endif
#if defined(SPEECH_TX_AEC2FLOAT) && !defined(AUDIO_ANC_FB_ADJ_MC)
			*:coherence.o(.text*)
			*:echo_canceller.o(.text*)
			*:echo_suppression.o(.text*)
			*:lcmmse.o(.text*)
			*:logmmse.o(.text*)
			*:mdf2.o(.text*)
			*:noise_generator.o(.text*)
			*:pink_noise_gen.o(.text*)
			*:white_noise_gen.o(.text*)
#endif
#if defined(SPEECH_TX_AEC3)
			*:SubBandBasedAEC.o(.text*)
#endif
#if defined(SPEECH_TX_3MIC_NS)
			*:speech_3mic_ns.o(.text*)
#endif
#if defined(MSBC_8K_SAMPLE_RATE)
			*:iir_resample.o(.text*)
#endif
#if defined(SPEECH_TX_COMPEXP)
			*:compexp.o(.text*)
#endif
#if defined(SPEECH_TX_EQ) || defined(SPEECH_RX_EQ) || defined(MSBC_8K_SAMPLE_RATE)
			*:speech_eq.o(.text*)
			*:speech_arm_eq.o(.text*)
			*:arm_biquad_cascade_df1_f32.o(.text*)
#endif
#if defined(SPEECH_TX_POST_GAIN) || defined(SPEECH_RX_POST_GAIN)
			*:speech_gain.o(.text*)
#endif
#if defined(__SMARTVOICE__)
#if defined(SPEECH_TX_2MIC_NS2) || defined(SPEECH_TX_NS2)
			*:app_smartvoice.o(.text*._Z23mic_data_process_deinitv)
			*:app_smartvoice.o(.text*._Z21mic_data_process_initv)
			*:app_smartvoice.o(.text*._Z20mic_data_process_runPsm)
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
			*:sbc.o(.text*)
			*:sbc_math.o(.text*)
#endif
			*:fixedpoint.o(.text*)
			*:replaygain.o(.text*)
			*:dsp_core.o(.text*)
			*:dsp_misc.o(.text*)
			*:dsp_filter.o(.text*)
			*:dsp_sample_input.o(.text*)
			*:dsp_sample_output.o(.text*)
			*:eq.o(.text*)
			*:pga.o(.text*)
#if !defined(A2DP_AAC_ON) && !defined(A2DP_LHDC_ON)
			*:fir_process.o(.text*)
#endif
#endif
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
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
			*:block.o(.text*.*CBlock_GetEscape*)
			*:tpdec_lib.o(.text*.*transportDec_GetBitstream*)
			*:tpdec_lib.o(.text*.*transportDec_GetAuBitsRemaining*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetValue*)
			*:tpdec_latm.o(.text*.*CLatmDemux_Read*)
			*:tpdec_lib.o(.text*.*transportDec_FillData*)
			*:tpdec_lib.o(.text*.*transportDec_ReadAccessUnit*)
			*:tpdec_lib.o(.text*.*transportDec_readHeader*)
			*:block.o(.text*.*ApplyTools*)
			*:aacdec_pns.o(.text*.*CPns_Apply*)
			*:aacdec_tns.o(.text*.*CTns_Apply*)
			*:aacdec_tns.o(.text*.*CTns_Reset*)
			*:aacdec_pns.o(.text*.*CPns_ResetData*)
			*:pulsedata.o(.text*.*CPulseData_Read*)
#if !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:channel.o(.text*.*CChannelElement_Read*)
			*:tpdec_asc.o(.text*.*CProgramConfig_Reset*)
			*:tpdec_lib.o(.text*.*transportDec_CrcCheck*)
			*:block.o(.text*.*CBlock_FrequencyToTime*)
			*:block.o(.text*.*CBlock_ReadSectionData*)
			*:channel.o(.text*.*CChannelElement_Decode*)
			*:tpdec_lib.o(.text*.*transportDec_CrcEndReg*)
			*:aacdecoder.o(.text*.*CAacDecoder_DecodeFrame*)
			*:mdct.o(.text*)
#endif

#if !defined(__SBC_FUNC_IN_ROM_VBEST2000__) && !defined(AAC_TEXT_PARTIAL_IN_FLASH)
			*:block.o(.text*.*CBlock_ReadSpectralData*)
			*:block.o(.text*.*CBlock_ScaleSpectralData*)
			*:aacdec_tns.o(.text*.*CTns_ReadDataPresentFlag*)
			*:tpdec_lib.o(.text*.*transportDec_CrcStartReg*)
			*:block.o(.text*.*CBlock_ReadScaleFactorData*)
			*:tpdec_lib.o(.text*.*transportDec_EndAccessUnit*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetNrOfSubFrames*)
			*:tpdec_lib.o(.text*.*transportDec_GetAuBitsTotal*)
			*:tpdec_asc.o(.text*.*CProgramConfig_LookupElement*)
			*:tpdec_latm.o(.text*.*CLatmDemux_GetFrameLengthInBits*)
			*:tpdec_latm.o(.text*.*CLatmDemux_ReadPayloadLengthInfo*)
			*:block.o(.text*.*CBlock_InverseQuantizeSpectralData*)
			*:channelinfo.o(.text*.*IcsRead*)
			*:aacdec_tns.o(.text*.*CTns_Read*)
			*:tpdec_lib.o(.text*.*transportDec_AdjustEndOfAccessUnit*)
			*:pulsedata.o(.text*.*CPulseData_Apply*)
			*:tpdec_asc.o(.text*.*AudioSpecificConfig_Init*)
			*:tpdec_asc.o(.text*.*AudioSpecificConfig_Parse*)
			*:tpdec_latm.o(.text*.*CLatmDemux_ReadStreamMuxConfig*)
			*:tpdec_asc.o(.text*.*getSampleRate*)
			*:tpdec_asc.o(.text*.*getAOT*)
			*:aacdecoder.o(.text*.*CAacDecoder_Init*)
			*:tpdec_asc.o(.text*.*CProgramConfig_Init*)
			*:aacdecoder_lib.o(.text*.*aacDecoder_ConfigCallback*)
			*:aacdecoder.o(.text*.*CAacDecoder_ExtPayloadParse*)
			*:stereo.o(.text*.*CJointStereo_Read*)
#endif
#endif
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
			*:a2dp_decoder_aac_lc.o(.text*)
			*:fdkaac_codec.o(.text*)
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
			*:rc_decode.o(.text*)
			*:audio_int32.o(.text*)
			*:ssc_ifft.o(.text*)
			*:util.o(.text*)
			*:vector_quant.o(.text*)
			*:ssc_quant.o(.text*)
			*:ssc_pulsealloc.o(.text*)
			*:audio.o(.text*)
			*:set_sbm.o(.text*)
			*:rc_code.o(.text*)
			*:SBM_pitchBased.o(.text*)
			*:ssc_imdct.o(.text*)
			*:audio_decoder.o(.text*)
			*:SoundBooster_DRC_classic.o(.text*)
			*:ssc_cwrs.o(.text*)
			*:ssc_mode.o(.text*)
			*:ssc_math.o(.text*)
			*:ssc_lap.o(.text*)
			*:SeamlessBufferManagement.o(.text*)
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
			*:a2dp_decoder_scalable.o(.text*)
#endif
			*(.overlay_a2dp_ssc*)
#endif

#if defined(A2DP_LHDC_ON)
			*(.overlay_a2dp_lhdc*)
			*:a2dp_decoder_lhdc.o(.text*)
			*:testkey.o(.text*)
			*:lhdcUtil.o(.text*)
			*:lzh_dec.o(.text*)
			*:licdecrypt.o(.text*)
			*:lic_check.o(.text*)
			*:fast_upsampling_2x.o(.text*)
			*:encdec.o(.text*)
			*:dec.o(.text*)
			*:cirbuf.o(.text*)
			*:DecodeUps2xLPF.o(.text*)
			*:Arc.o(.text*)
#endif

			*(.overlay_text6)
			. = ALIGN(4);
		}
		.overlay_text7
		{
			LONG(0);
#if defined(A2DP_LDAC_ON)
			*:a2dp_decoder_ldac.o(.text*)
			*:bitalloc_ldac.o(.text*)
			*:bitalloc_sub_ldac.o(.text*)
			*:encode_ldac.o(.text*)
			*:ldacBT.o(.text*)
			*:ldaclib.o(.text*)
			*:pack_ldac.o(.text*)
			*:quant_ldac.o(.text*)
			*:mdct_ldac.o(.text*)
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
#if defined(OPUS_IN_OVERLAY)
			/**:voice_opus.o(.text*)*/
			*:opus_encoder.o(.text*)
			*:analysis.o(.text*)
			/**:celt_encoder.o(.text*)*/
			*:celt_encoder.o(.text*.opus_custom_encoder_ctl)
			*:celt_encoder.o(.text*.celt_encoder_get_size)
			*:celt_encoder.o(.text*.opus_custom_encoder_get_size)
			*:celt_encoder.o(.text*.celt_encoder_init)
			*:celt_encoder.o(.text*.opus_custom_encoder_init_arch)
			*:celt_encoder.o(.text*.transient_analysis)
			*:celt_encoder.o(.text*.patch_transient_decision)
			*:celt_encoder.o(.text*.celt_preemphasis)
			*:celt_encoder.o(.text*.l1_metric)
			*:celt_encoder.o(.text*.tf_analysis)
			*:celt_encoder.o(.text*.tf_encode)
			*:celt_encoder.o(.text*.alloc_trim_analysis)
			*:celt_encoder.o(.text*.stereo_analysis)
			*:celt_encoder.o(.text*.median_of_5)
			*:celt_encoder.o(.text*.median_of_3)
			*:celt_encoder.o(.text*.dynalloc_analysis)
			*:celt_encoder.o(.text*.run_prefilter)
			*:celt_encoder.o(.text*.compute_vbr)
			*:celt_encoder.o(.text*.celt_encode_with_ec)
			*:celt_encoder.o(.text*.opus_custom_encoder_ctl)
			*:repacketizer.o(.text*)
			*:opus.o(.text*)
			*:enc_API.o(.text*)
			*:entenc.o(.text*)
			*:control_audio_bandwidth.o(.text*)
			*:encode_pulses.o(.text*)
			*:LPC_analysis_filter.o(.text*)
			*:burg_modified_FIX.o(.text*)
			*:vector_ops_FIX.o(.text*)
			*:NLSF_del_dec_quant.o(.text*)
			*:encode_frame_FIX.o(.text*)
			*:pitch.o(.text*)
			*:resampler.o(.text*)
			*:VAD.o(.text*)
			*:encode_indices.o(.text*)
			*:control_codec.o(.text*)
			*:celt_lpc.o(.text*)
			*:shell_coder.o(.text*)
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
			*:NSQ.o(.text*) /*complexity 1*/
			*:NSQ_del_dec.o(.text*) /*complexity 2*/
#endif
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

	.dummy_overlay_text_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_text_end))
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
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
			*:bt_sco_chain.o(.data* .rodata*)
			*:bt_sco_chain_cp.o(.data* .rodata*)
#endif
#if !defined(SCO_OPTIMIZE_FOR_RAM)

#if !defined(GLOBAL_SRAM_KISS_FFT)
			*:fftwrap.o(.data* .rodata*)
			*:kiss_fft.o(.data* .rodata*)
			*:kiss_fftr.o(.data* .rodata*)
#endif

#if !defined(GLOBAL_SRAM_CMSIS_FFT)
			*:CommonTables.o(.rodata.armBitRevIndexTable256)
			*:CommonTables.o(.rodata.twiddleCoef_256)
			*:CommonTables.o(.rodata.twiddleCoef_rfft_512)
			*:CommonTables.o(.rodata.armBitRevIndexTable128)
			*:CommonTables.o(.rodata.twiddleCoef_128)
			*:CommonTables.o(.rodata.twiddleCoef_rfft_256)
#endif

			/* common */
			*:fastmath_const.o(.data* .rodata*)

			*:preprocess.o(.data* .rodata*)
			*:filterbank.o(.data* .rodata*)

#if !defined(SCO_CP_ACCEL)
			*:plc_8000.o(.data* .rodata*)
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
			*:Pcm8k_Cvsd.o(.data* .rodata* .bss*)
#endif
#if defined(HFP_1_6_ENABLE)
			*:table.o(.data* .rodata*)
			*:plc_16000.o(.data* .rodata*)
#endif
#endif
#if !defined(AUDIO_ANC_FB_ADJ_MC)
			*:mdf.o(.data* .rodata*)
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
#if defined(SPEECH_TX_2MIC_NS4)
			*:sensormic_denoise.o(.data* .rodata*)
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
#if defined(SPEECH_TX_AEC3)
			*:SubBandBasedAEC.o(.data* .rodata*)
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
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
		*:a2dp_decoder_sbc.o(.rodata*)
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
			*:fixpoint_math.o(.data* .rodata*)
			*:FDK_bitbuffer.o(.data* .rodata*)
#endif
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
			*:a2dp_decoder_aac_lc.o(.rodata*)
			*:fdkaac_codec.o(.rodata*)
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
			*:rc_decode.o(.data* .rodata*)
			*:audio_int32.o(.data* .rodata*)
			*:ssc_ifft.o(.data* .rodata*)
			*:util.o(.data* .rodata*)
			*:vector_quant.o(.data* .rodata*)
			*:ssc_quant.o(.data* .rodata*)
			*:ssc_pulsealloc.o(.data* .rodata*)
			*:audio.o(.data* .rodata*)
			*:set_sbm.o(.data* .rodata*)
			*:rc_code.o(.data* .rodata*)
			*:SBM_pitchBased.o(.data* .rodata*)
			*:ssc_imdct.o(.data* .rodata*)
			*:audio_decoder.o(.data* .rodata*)
			*:SoundBooster_DRC_classic.o(.data* .rodata*)
			*:ssc_cwrs.o(.data* .rodata*)
			*:ssc_mode.o(.data* .rodata*)
			*:ssc_math.o(.data* .rodata*)
			*:ssc_lap.o(.data* .rodata*)
			*:SeamlessBufferManagement.o(.data* .rodata*)
#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
			*:a2dp_decoder_scalable.o(.rodata*)
#endif			
#endif
#if defined(A2DP_LHDC_ON)
			*:a2dp_decoder_lhdc.o(.data* .rodata*)
			*:testkey.o(.data* .rodata*)
			*:lhdcUtil.o(.data* .rodata*)
			*:lzh_dec.o(.data* .rodata*)
			*:licdecrypt.o(.data* .rodata*)
			*:lic_check.o(.data* .rodata*)
			*:fast_upsampling_2x.o(.data* .rodata*)
			*:encdec.o(.data* .rodata*)
			*:dec.o(.data* .rodata*)
			*:cirbuf.o(.data* .rodata*)
			*:DecodeUps2xLPF.o(.data* .rodata*)
			*:Arc.o(.data* .rodata*)
#endif

			*(.overlay_data6 .overlay_rodata6)
			. = ALIGN(4);
		}
		.overlay_data7
		{
			LONG(0);
#if defined(A2DP_LDAC_ON)
			*:a2dp_decoder_ldac.o(.data* .rodata*)
			*:bitalloc_ldac.o(.data* .rodata*)
			*:bitalloc_sub_ldac.o(.data* .rodata*)
			*:encode_ldac.o(.data* .rodata*)
			*:ldacBT.o(.data* .rodata*)
			*:ldaclib.o(.data* .rodata*)
			*:pack_ldac.o(.data* .rodata*)
			*:quant_ldac.o(.data* .rodata*)
			*:mdct_ldac.o(.data* .rodata*)
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

#if defined(OPUS_IN_OVERLAY)
			*:voice_opus.o(.data* .rodata*)
			*:opus_encoder.o(.data* .rodata*)
			*:analysis.o(.data* .rodata*)
			*:celt_encoder.o(.data* .rodata*)
			*:repacketizer.o(.data* .rodata*)
			*:opus.o(.data* .rodata*)
			*:enc_API.o(data* .rodata)
			*:entenc.o(.data* .rodata*)
			*:control_audio_bandwidth.o(.data* .rodata*)
			*:encode_pulses.o(.data* .rodata*)
			*:LPC_analysis_filter.o(.data* .rodata*)
			*:burg_modified_FIX.o(.data* .rodata*)
			*:vector_ops_FIX.o(.data* .rodata*)
			*:NLSF_del_dec_quant.o(.data* .rodata*)
			*:encode_frame_FIX.o(.data* .rodata*)
			*:pitch.o(.data* .rodata*)
			*:resampler.o(.data* .rodata*)
			*:VAD.o(.data* .rodata*)
			*:encode_indices.o(.data* .rodata*)
			*:control_codec.o(.data* .rodata*)
			*:celt_lpc.o(.data* .rodata*)
			*:shell_coder.o(.data* .rodata*)
			*:init_encoder.o(.data* .rodata*)
			*:log2lin.o(.data* .rodata*)
			*:biquad_alt.o(.data* .rodata*)
			*:stereo_encode_pred.o(.data* .rodata*)
			*:NLSF_unpack.o(.data* .rodata*)
			*:code_signs.o(.data* .rodata*)
			*:lin2log.o(.data* .rodata*)
			*:HP_variable_cutoff.o(.data* .rodata*)
			*:sum_sqr_shift.o(.data* .rodata*)
			*:inner_prod_aligned.o(.data* .rodata*)
			*:stereo_quant_pred.o(.data* .rodata*)
			*:entcode.o(.data* .rodata*)
			*:celt.o(.data* .rodata*)
			*:laplace.o(.data* .rodata*)
			*:NSQ.o(.data* .rodata*) /*complexity 1*/
			*:NSQ_del_dec.o(.data* .rodata*) /*complexity 2*/
#endif
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

	.dummy_overlay_data_end (.) (NOLOAD) : AT (ADDR(.dummy_overlay_data_end))
	{
		__overlay_data_end__ = .;
	} > REGION_OVERLAY_DATA

	ASSERT(__overlay_data_end__ - __overlay_data_start__ <= OVERLAY_DATA_SECTION_SIZE, "overlay data sections too large")

#if defined(CHIP_HAS_CP) && (RAMCP_SIZE > 0)
	.cp_ramx_last_dummy (NOLOAD) : AT (ADDR(.cp_ramx_last_dummy))
	{
		__cp_ramx_last_dummy_start = .;
	} > RAMCPX

	.cp_ram_last_dummy (NOLOAD) : AT (ADDR(.cp_ram_last_dummy))
	{
		. = ALIGN(8);
		__cp_ram_last_dummy_start = .;
	} > RAMCP

	__cp_stack_top = ORIGIN(RAMCP) + LENGTH(RAMCP);
	__cp_stack_limit = __cp_stack_top - CP_STACK_SECTION_SIZE;

	/* LD v2.24 (GCC 4.9) cannot handle address conversion correctly if involving immediate value.
	   E.g., <absolute_addr> + <immediate_val> + <relative_addr> */
	__free_ramcpx = ORIGIN(RAMCPX) + LENGTH(RAMCPX) - ABSOLUTE(__cp_ramx_last_dummy_start);
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
#ifdef TRACE_STR_SECTION
	.rodata_str (.):
	{
	    __rodata_str_start__ = .;
		*(.rodata.__func__.*)
		*(.rodata.*__func__)
		*(.rodata.__FUNCTION__.*)
		*(.rodata.*__FUNCTION__)
		__rodata_str_end__ = .;
	} > FLASH

	RODATA_ADDRESS = .;
	. = 0xFFFC0000;
	.trc_str (.):
	{
		*(.rodata.__func__.*)
		*(.rodata.*__func__)
		*(.rodata.__FUNCTION__.*)
		*(.rodata.*__FUNCTION__)
		*(.trc_str*)
	}
	. = RODATA_ADDRESS;
#endif
	.rodata (.) :
	{
		*(.rodata*)
		*(.flash_rodata*)

		. = ALIGN(4);
		__cust_cmd_hldr_tbl_start = .;
		KEEP(*(.cust_cmd_hldr_tbl))
		__cust_cmd_hldr_tbl_end = .;
		. = ALIGN(4);
		__ai_gatt_server_table_start = .;
		KEEP(*(.ai_gatt_server_table))
		__ai_gatt_server_table_end = .;
		. = ALIGN(4);
		__ai_ble_handler_table_start = .;
		KEEP(*(.ai_ble_handler_table))
		__ai_ble_handler_table_end = .;
		. = ALIGN(4);
		__ai_spp_register_table_start = .;
		KEEP(*(.ai_spp_register_table))
		__ai_spp_register_table_end = .;
		. = ALIGN(4);
		__tota_handler_table_start = .;
		KEEP(*(.tota_handler_table))
		__tota_handler_table_end = .;
		. = ALIGN(4);
		__ai_handler_function_table_start = .;
		KEEP(*(.ai_handler_function_table))
		__ai_handler_function_table_end = .;
		. = ALIGN(4);
		__custom_handler_table_start = .;
		KEEP(*(.custom_handler_table))
		__custom_handler_table_end = .;
		. = ALIGN(4);
		__mixprompt_property_table_start = .;
		KEEP(*(.mixprompt_property_table))
		__mixprompt_property_table_end = .;
	 	KEEP(*(.eh_frame*))
		*(.note.gnu.build-id)
		*(.constdata)
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

	.bss (.) (NOLOAD) : AT (ADDR(.bss))
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.reboot_custom_param (.) (NOLOAD) :
	{
		KEEP(*(.reboot_custom_param*))
		. = ALIGN(4);
		__reboot_custom_param_end__ = .;
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
#ifdef TRACE_STR_SECTION
		LONG(ABSOLUTE(__rodata_str_start__));
#else
		LONG(0);
#endif
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;
	
	.hotword_model (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE - 
		OTA_UPGRADE_LOG_SIZE - CORE_DUMP_SECTION_SIZE -	HOTWORD_SECTION_SIZE) (NOLOAD):
	{
		__hotword_model_start = .;
		. = HOTWORD_SECTION_SIZE;
		__hotword_model_end = .;
	} > FLASH_NC

	.coredump_section (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE - 
		OTA_UPGRADE_LOG_SIZE - CORE_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__coredump_section_start = .;
		. = CORE_DUMP_SECTION_SIZE;
		__coredump_section_end = .;
	} > FLASH_NC

	.ota_upgrade_log (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE -
		OTA_UPGRADE_LOG_SIZE) (NOLOAD) :
	{
		__ota_upgrade_log_start = .;
		. = OTA_UPGRADE_LOG_SIZE;
		__ota_upgrade_log_end = .;
	} > FLASH_NC

	.log_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE - LOG_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__log_dump_start = .;
		. = LOG_DUMP_SECTION_SIZE;
		__log_dump_end = .;
	} > FLASH_NC

	.crash_dump (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE -  USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE - CRASH_DUMP_SECTION_SIZE) (NOLOAD) :
	{
		__crash_dump_start = .;
		. = CRASH_DUMP_SECTION_SIZE;
		__crash_dump_end = .;
	} > FLASH_NC

	.custom_parameter (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - CUSTOM_PARAMETER_SECTION_SIZE -
		LHDC_LICENSE_SECTION_SIZE) (NOLOAD) :
	{
		__custom_parameter_start = .;
		. = CUSTOM_PARAMETER_SECTION_SIZE;
		__custom_parameter_end = .;
	} > FLASH_NC
	
	.lhdc_license (ORIGIN(FLASH_NC) + LENGTH(FLASH_NC) - FACTORY_SECTION_SIZE  - RESERVED_SECTION_SIZE -
		AUD_SECTION_SIZE - USERDATA_SECTION_SIZE*2 - LHDC_LICENSE_SECTION_SIZE) (NOLOAD) :
	{
		__lhdc_license_start = .;
		. = LHDC_LICENSE_SECTION_SIZE;
		__lhdc_license_end = .;
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
	__tail_section_start = __hotword_model_start;
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

