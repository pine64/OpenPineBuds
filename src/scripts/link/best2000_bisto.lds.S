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

#ifdef RB_CODEC
#define OVERLAY_DATA_SECTION_SIZE	0xA000
#else
#define OVERLAY_DATA_SECTION_SIZE	0x8000
#endif

#if (RAM_SIZE >= 0x48000)
#if defined(CHIP_BEST2000)
#if defined(BTUSB_AUDIO_MODE)
#define FAST_XRAM_SECTION_SIZE		0xC200
#else
#define FAST_XRAM_SECTION_SIZE		0xC200
#endif
#else
#if defined(BTUSB_AUDIO_MODE)
#define FAST_XRAM_SECTION_SIZE		0x1A000
#else
#define FAST_XRAM_SECTION_SIZE		0x16000
#endif
#endif
#else
#define FAST_XRAM_SECTION_SIZE		0x8000
#endif

#ifndef OTA_CODE_OFFSET
#define OTA_CODE_OFFSET				0
#endif
#define FLASH_REGION_BASE			(FLASH_BASE + OTA_CODE_OFFSET)
#ifndef FLASH_REGION_SIZE
#define FLASH_REGION_SIZE			(FLASH_SIZE - (FLASH_REGION_BASE - FLASH_BASE))
#endif
#define FLASH_NC_REGION_BASE		FLASH_C_TO_NC(FLASH_REGION_BASE)
#define FLASHX_REGION_BASE			FLASH_TO_FLASHX(FLASH_REGION_BASE)


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
		*:norflash_*.o(.text*)

		*(.boot_text_sram*)
		. = ALIGN(4);
		__boot_text_sram_end = .;
	} > RAMX

	.boot_data_sram (RAMX_TO_RAM(ADDR(.boot_text_sram) + SIZEOF(.boot_text_sram))) :
					AT (__boot_sram_start_flash__ + SIZEOF(.boot_text_sram))
	{
		*:hal_norflash*.o(.data* .rodata*)
		*:norflash_*.o(.data* .rodata*)
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
		*:norflash_*.o(.bss*)
		*:hal_psram.o(.bss*)
		*(.boot_bss*)
		. = ALIGN(4);
		__boot_bss_sram_end__ = .;
	} > RAM

	__sram_text_data_start_flash__ = __boot_sram_end_flash__;
	__sram_text_data_start__ = __boot_bss_sram_end__;
	.sram_text (RAM_TO_RAMX(__sram_text_data_start__)) :
				AT (__sram_text_data_start_flash__)
	{
		*(.sram_text*)
		. = ALIGN(4);
	} > RAMX

	.sram_data (RAMX_TO_RAM(ADDR(.sram_text) + SIZEOF(.sram_text))) :
				AT (__sram_text_data_start_flash__ + SIZEOF(.sram_text))
	{
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
		__fast_sram_text_data_start__ = .;

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
		*:sbc.o(.text.sbc_is_stream_info_valid)
		*:sbc.o(.text.sbc_frame_length_get)
		*:sbc.o(.text.sbc_4bit_crc_calc)
		*:sbc.o(.text.sbc_bits_for_mono_alloc)
		*:sbc.o(.text.sbc_bits_for_stereo_alloc)
		*:sbc.o(.text.sbc_header_process)
		*:sbc.o(.text.sbc_factors_process)
		*:sbc.o(.text.sbc_sample_extract)
		*:sbc.o(.text.sbc_sbsamples_make)
		*:sbc.o(.text.sbc_joint_process)
		*:sbc.o(.text.sbc_4band_synth_filter)
		*:sbc.o(.text.sbc_8band_synth_filter)
		*:sbc.o(.text.sbc_decoder_state_reset)
		*:sbc.o(.text.sbc_frame_mute)
		*:sbc.o(.text.sbc_frames_decode)
		*:sbc.o(.text.sbc_samples_quant)
		*:sbc.o(.text.sbc_decoder_init)
		*:sbc.o(.text.sbc_encoder_init)
		*:sbc.o(.text.sbc_frames_encode)
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
#endif
#endif

#if defined(__AUDIO_SPECTRUM__)
		*:kiss_fft.o(.text*)
		*:kiss_fftr.o(.text*)
		*:fftwrap.o(.text*)
		*:spectrum_fix.o(.text*)
		*:audio_spectrum.o(.text*)
#endif
		*:exp_func.o(.data* .rodata*)
		*(.fast_text_sram*)
		. = ALIGN(4);
		__fast_sram_text_data_end__ = .;
	} > FRAMX
	__fast_sram_text_data_end_flash__ = __fast_sram_text_data_start_flash__ + SIZEOF(.fast_text_sram);

	__overlay_text_start__ = RAMX_TO_RAM(__fast_sram_text_data_end__);
	__overlay_text_exec_start__ = RAM_TO_RAMX(__overlay_text_start__);
	OVERLAY (__overlay_text_exec_start__) : NOCROSSREFS AT (__fast_sram_text_data_end_flash__)
	{
#ifndef NO_OVERLAY
		.overlay_text0
		{
			LONG(0);
			*:preprocess.o(.text*)
			*:filterbank.o(.text*)
#if !defined(A2DP_SCALABLE_ON)
			*:kiss_fft.o(.text*)
#endif
			*:kiss_fftr.o(.text*)
			*:fftwrap.o(.text*)
			*:mdf.o(.text*)
			*:plc_8000.o(.text*)
#if defined(HFP_1_6_ENABLE)
			*:plc_16000.o(.text*)
#endif
#if defined(_CVSD_BYPASS_) || defined(CVSD_BYPASS)
			*:Pcm8k_Cvsd.o(.text*)
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
#if defined(SPEECH_TX_2MIC_NS4)
			*:sensormic_denoise.o(.text*)
#endif
#if defined(SPEECH_TX_NS2)
			*:lc_mmse_ns.o(.text*)
#endif
#if defined(SPEECH_TX_NS2FLOAT)
			*:lc_mmse_ns_float.o(.text*)
			*:cmsis_fftwrap.o(.text*)
			*:arm_rfft_fast_f32.o(.text*)
			*:arm_cfft_f32.o(.text*)
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
#if defined(SPEECH_TX_AEC2FLOAT)
			*:coherence.o(.text*)
			*:echo_canceller.o(.text*)
			*:echo_suppression(.text*)
			*:iirfilt.o(.text*)
			*:integer_delay.o(.text*)
			*:mdf2.o(.text*)
			*:ringbuf.o(.text*)
#endif
#if defined(SPEECH_TX_EQ)
			*:speech_arm_eq.o(.text*)
			*:arm_biquad_cascade_df1_f32.o(.text*)
#endif
#if defined(__SMARTVOICE__)
#if defined(SPEECH_TX_2MIC_NS2) || defined(SPEECH_TX_2MIC_NS4) || defined(SPEECH_TX_NS2)
            *:app_smartvoice.o(.text._Z23mic_data_process_deinitv)
            *:app_smartvoice.o(.text._Z21mic_data_process_initv)
            *:app_smartvoice.o(.text._Z20mic_data_process_runPsm)
#endif
#endif
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
			*:iir_process.o(.text.iir_run .text.*iir_run_per_*)
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
			*:aacdec.o(.text*)
			*:aactabs.o(.text*)
			*:bitstream.o(.text*)
			*:buffers.o(.text*)
			*:dct4.o(.text*)
			*:decelmnt.o(.text*)
			*:dequant.o(.text*)
			*:fft.o(.text*)
			*:filefmt.o(.text*)
			*:huffman.o(.text*)
			*:hufftabs.o(.text*)
			*:imdct.o(.text*)
			*:noiseless.o(.text*)
			*:pns.o(.text*)
			*:stproc.o(.text*)
			*:tns.o(.text*)
			*:trigtabs.o(.text*)
			*:trigtabs_fltgen.o(.text*)
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
		.overlay_text_end
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > FRAMX

	.dummy_overlay_text (.) (NOLOAD) :
	{
		__dummy_overlay_text_end__ = .;
	} > FRAMX

	ASSERT(__dummy_overlay_text_end__ - __fast_sram_text_data_start__ <= FAST_XRAM_SECTION_SIZE, "fast xram sections too large")
	__free_fram = FAST_XRAM_SECTION_SIZE - (__dummy_overlay_text_end__ - __fast_sram_text_data_start__);

	__overlay_data_start__ = __sram_bss_end__;
	OVERLAY (__overlay_data_start__) : NOCROSSREFS AT (__load_stop_overlay_text_end)
	{
#ifndef NO_OVERLAY
		.overlay_data0
		{
			/* Explicitly place 4 bytes at section start to avoid "section type changed to PROGBITS" warnings */
			LONG(0);
			/* common */
			*:fastmath_const.o(.data* .rodata*)
			*:speech_win.o(.data* .rodata*)

			*:preprocess.o(.data* .rodata*)
			*:filterbank.o(.data* .rodata*)
#if !defined(A2DP_SCALABLE_ON)
			*:kiss_fft.o(.data* .rodata*)
#endif
			*:kiss_fftr.o(.data* .rodata*)
			*:fftwrap.o(.data* .rodata*)
			*:mdf.o(.data* .rodata*)
			*:plc_8000.o(.data* .rodata*)
#if defined(HFP_1_6_ENABLE)
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
#if defined(SPEECH_TX_NS2) || defined(SPEECH_RX_NS2)
			*:lc_mmse_ns.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS3) || defined(SPEECH_RX_NS3)
			*:nsx_core.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS2FLOAT) || defined(SPEECH_RX_NS2FLOAT)
			*:lc_mmse_ns_float.o(.data* .rodata*)
#endif
#if defined(MSBC_8K_SAMPLE_RATE)
			*:iir_resample.o(.data* .rodata*)
#endif
#if defined(SPEECH_TX_NS2FLOAT) || defined(SPEECH_RX_NS2FLOAT) || defined(SPEECH_TX_AEC2FLOAT)
			*:arm_common_tables.o(.rodata.armBitRevIndexTable256)
			*:arm_common_tables.o(.rodata.twiddleCoef_256)
			*:arm_common_tables.o(.rodata.twiddleCoef_rfft_512)
			*:arm_common_tables.o(.rodata.armBitRevIndexTable128)
			*:arm_common_tables.o(.rodata.twiddleCoef_128)
			*:arm_common_tables.o(.rodata.twiddleCoef_rfft_256)
#endif
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
			*:mdct.o(.data* .rodata*)
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
			*:a2dpplay.o(.bss.*scalable_decoder_place)
			*:a2dpplay.o(.bss.*scalable_input_mid_buf)
#endif
#if defined(A2DP_LHDC_ON)
			*:lhdcUtil.o(.data* .rodata*)
    		        *:dec.o(.data* .rodata*)
			*:cirbuf.o(.data* .rodata*)
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
		.overlay_data_end
		{
			LONG(0);
			. = ALIGN(4);
		}
	} > RAM

	.dummy_overlay_data (.) (NOLOAD) :
	{
		__dummy_overlay_data_end__ = .;
	} > RAM

	ASSERT(__dummy_overlay_data_end__ - __overlay_data_start__ <= OVERLAY_DATA_SECTION_SIZE, "overlay sections too large")

	.text (FLASH_TO_FLASHX(__load_stop_overlay_data_end)) : AT (FLASHX_TO_FLASH(ADDR(.text)))
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

	__etext = .;

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(vtable)
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
		. = ALIGN(4);
		/* All data end */
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
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
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

	/* The following section be the last loaded section */
	.code_start_addr (.) :
	{
		LONG(BUILD_INFO_MAGIC);
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;


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

	ASSERT(FLASH_NC_TO_C(__crash_dump_start) >= __flash_end, "region FLASH overflowed")
	__free_flash = FLASH_NC_TO_C(__crash_dump_start) - __flash_end;
}

