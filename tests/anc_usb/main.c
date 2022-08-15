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
#include "hal_bootmode.h"
#include "hal_cmu.h"
#include "hal_dma.h"
#include "hal_gpadc.h"
#include "hal_iomux.h"
#include "hal_key.h"
#include "hal_norflash.h"
#include "hal_sleep.h"
#include "hal_sysfreq.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "cmsis.h"
#include "main_entry.h"
#include "pmu.h"
#include "analog.h"
#include "string.h"
#include "hwtimer_list.h"
#include "audioflinger.h"
#include "anc_usb_app.h"
#include "usb_audio_app.h"
#include "dualadc_audio_app.h"
#include "adda_loop_app.h"
#include "usb_audio_frm_defs.h"
#include "tgt_hardware.h"
#include "audio_process.h"
#if defined(_VENDOR_MSG_SUPPT_)
#include "usb_vendor_msg.h"
#endif

#ifdef RTOS
#include "cmsis_os.h"
#endif
#ifdef __PC_CMD_UART__
#include "hal_cmd.h"
#endif

#ifdef USB_AUDIO_SPEECH
#define CODEC_BUFF_FRAME_NUM            (2 * 16)
#define USB_BUFF_FRAME_NUM              (CODEC_BUFF_FRAME_NUM * 2)
#else
#define CODEC_BUFF_FRAME_NUM            4
#define USB_BUFF_FRAME_NUM              10
#endif

#if (CODEC_BUFF_FRAME_NUM >= USB_BUFF_FRAME_NUM)
#error "Codec buffer frame num should be less than usb buffer frame num (on the requirement of conflict ctrl)"
#endif

#ifdef USB_AUDIO_DYN_CFG

#define USB_AUDIO_PLAYBACK_BUFF_SIZE    NON_EXP_ALIGN(MAX_FRAME_SIZE_PLAYBACK * CODEC_BUFF_FRAME_NUM, DAC_BUFF_ALIGN)
#define USB_AUDIO_CAPTURE_BUFF_SIZE     NON_EXP_ALIGN(MAX_FRAME_SIZE_CAPTURE * CODEC_BUFF_FRAME_NUM, ADC_BUFF_ALIGN)

#define USB_AUDIO_RECV_BUFF_SIZE        NON_EXP_ALIGN(MAX_FRAME_SIZE_RECV * USB_BUFF_FRAME_NUM, RECV_BUFF_ALIGN)
#define USB_AUDIO_SEND_BUFF_SIZE        NON_EXP_ALIGN(MAX_FRAME_SIZE_SEND * USB_BUFF_FRAME_NUM, SEND_BUFF_ALIGN)

#if defined(CHIP_BEST1000)
// FIR EQ is working on 16-bit
// FIR_EQ_buffer_size = max_playback_symbol_number_in_buffer * sizeof(int16_t)
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      USB_AUDIO_PLAYBACK_BUFF_SIZE
#elif defined(CHIP_BEST2000)
// FIR EQ is working on 32-bit
// FIR_EQ_buffer_size = max_playback_symbol_number_in_buffer * sizeof(int32_t)
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE*2)
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P)
// FIR EQ is working on 32-bit
// FIR_EQ_buffer_size = max_playback_symbol_number_in_buffer * sizeof(int32_t)
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE)
#define USB_AUDIO_IIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE)
#define USB_AUDIO_DSD_BUFF_SIZE         (USB_AUDIO_PLAYBACK_BUFF_SIZE*16)
#endif

#else // !USB_AUDIO_DYN_CFG

#define USB_AUDIO_PLAYBACK_BUFF_SIZE    NON_EXP_ALIGN(FRAME_SIZE_PLAYBACK * CODEC_BUFF_FRAME_NUM, DAC_BUFF_ALIGN)
#define USB_AUDIO_CAPTURE_BUFF_SIZE     NON_EXP_ALIGN(FRAME_SIZE_CAPTURE * CODEC_BUFF_FRAME_NUM, ADC_BUFF_ALIGN)

#define USB_AUDIO_RECV_BUFF_SIZE        NON_EXP_ALIGN(FRAME_SIZE_RECV * USB_BUFF_FRAME_NUM, RECV_BUFF_ALIGN)
#define USB_AUDIO_SEND_BUFF_SIZE        NON_EXP_ALIGN(FRAME_SIZE_SEND * USB_BUFF_FRAME_NUM, SEND_BUFF_ALIGN)

#if defined(CHIP_BEST1000)
// FIR EQ is working on 16-bit
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE * sizeof(int16_t) / SAMPLE_SIZE_PLAYBACK)
#elif defined(CHIP_BEST2000)
// FIR EQ is working on 32-bit
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE * sizeof(int32_t) / SAMPLE_SIZE_PLAYBACK)
#elif defined(CHIP_BEST2300) || defined(CHIP_BEST2300P)
// FIR EQ is working on 32-bit
#define USB_AUDIO_FIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE)
#define USB_AUDIO_IIR_EQ_BUFF_SIZE      (USB_AUDIO_PLAYBACK_BUFF_SIZE)
#define USB_AUDIO_DSD_BUFF_SIZE         (USB_AUDIO_PLAYBACK_BUFF_SIZE*16)
#endif

#endif // !USB_AUDIO_DYN_CFG

#if (defined(CHIP_BEST1000) && (defined(ANC_APP) || defined(_DUAL_AUX_MIC_))) && (CHAN_NUM_CAPTURE == CHAN_NUM_SEND)
// Resample input buffer size should be (half_of_max_sample_num * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE).
// half_of_max_sample_num = 48000 / 1000 * CODEC_BUFF_FRAME_NUM / 2 * 48 / 44
#define RESAMPLE_INPUT_BUFF_SIZE        ALIGN(48000 / 1000 * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE * CODEC_BUFF_FRAME_NUM / 2 * 48 / 44, 4)
#else
#define RESAMPLE_INPUT_BUFF_SIZE        0
#endif
// Resample history buffer size should be
// sizeof(struct RESAMPLE_CTRL_T) + ((SAMPLE_NUM + phase_coef_num) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)
#define RESAMPLE_HISTORY_BUFF_SIZE      (50 + (256 * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE))
#define USB_AUDIO_RESAMPLE_BUFF_SIZE    (RESAMPLE_INPUT_BUFF_SIZE + RESAMPLE_HISTORY_BUFF_SIZE)

#define ALIGNED4                        ALIGNED(4)

#if defined(USB_AUDIO_APP) || defined(DUALADC_AUDIO_TEST)

#ifdef AUDIO_ANC_FB_MC
static uint8_t ALIGNED4 playback_buff[USB_AUDIO_PLAYBACK_BUFF_SIZE * 9];//max 48->384 or 44.1->44.1*8;
#else
static uint8_t ALIGNED4 playback_buff[USB_AUDIO_PLAYBACK_BUFF_SIZE];
#endif

static uint8_t ALIGNED4 capture_buff[USB_AUDIO_CAPTURE_BUFF_SIZE];

#endif

#ifdef USB_AUDIO_APP

#if defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
static uint8_t ALIGNED4 eq_buff[USB_AUDIO_FIR_EQ_BUFF_SIZE+USB_AUDIO_IIR_EQ_BUFF_SIZE];
#elif defined(__HW_FIR_EQ_PROCESS__) && !defined(__HW_IIR_EQ_PROCESS__)
static uint8_t ALIGNED4 eq_buff[USB_AUDIO_FIR_EQ_BUFF_SIZE];
#elif !defined(__HW_FIR_EQ_PROCESS__) && defined(__HW_IIR_EQ_PROCESS__)
static uint8_t ALIGNED4 eq_buff[USB_AUDIO_IIR_EQ_BUFF_SIZE];
#else
static uint8_t ALIGNED4 eq_buff[0];
#endif

#ifdef SW_CAPTURE_RESAMPLE
static uint8_t ALIGNED4 resample_buff[USB_AUDIO_RESAMPLE_BUFF_SIZE];
#else
static uint8_t ALIGNED4 resample_buff[0];
#endif

static uint8_t ALIGNED4 recv_buff[USB_AUDIO_RECV_BUFF_SIZE];
static uint8_t ALIGNED4 send_buff[USB_AUDIO_SEND_BUFF_SIZE];

#ifdef USB_AUDIO_MULTIFUNC
static uint8_t ALIGNED4 recv2_buff[USB_AUDIO_RECV_BUFF_SIZE];
#endif

#endif

#ifdef CFG_HW_KEY_LED_PIN
const struct HAL_IOMUX_PIN_FUNCTION_MAP pinmux_key_led[1] = {
    {CFG_HW_KEY_LED_PIN, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_NOPULL},
};
#endif

#ifdef CFG_MIC_KEY
extern void mic_key_open (void);
#endif

static void uart_i2c_switch(void)
{
    static int flag = 0;

    flag ^= 1;

    if (flag) {
        hal_iomux_set_analog_i2c();
    } else {
        hal_iomux_set_uart0();
    }
}

static int key_event_process(uint32_t key_code, uint8_t key_event)
{
    TRACE(3,"%s: code=0x%X, event=%u", __FUNCTION__, key_code, key_event);

#ifdef CFG_HW_KEY_LED_PIN
    if (key_event == HAL_KEY_EVENT_DOWN) {
        hal_gpio_pin_set(CFG_HW_KEY_LED_PIN);
    } else if (key_event == HAL_KEY_EVENT_UP) {
        hal_gpio_pin_clr(CFG_HW_KEY_LED_PIN);
    }
#endif

#ifdef USB_AUDIO_APP
    if (usb_audio_app_key(key_code, key_event) == 0) {
        return 0;
    }
#endif

#ifdef ANC_APP
    if (anc_usb_app_key(key_code, key_event) == 0) {
        return 0;
    }
#endif

    if (key_event == HAL_KEY_EVENT_CLICK) {
        if (key_code == HAL_KEY_CODE_FN9) {
            uart_i2c_switch();
        }
    }

    return 0;
}

void anc_usb_open(void)
{
    TRACE(1,"%s", __FUNCTION__);

#ifdef __AUDIO_RESAMPLE__
    hal_cmu_audio_resample_enable();
#endif
#ifdef USB_AUDIO_APP
    struct USB_AUDIO_BUF_CFG cfg;

    memset(&cfg, 0, sizeof(cfg));
    cfg.play_buf = playback_buff;
#ifdef AUDIO_ANC_FB_MC
    cfg.play_size = sizeof(playback_buff) / 9;
#else
    cfg.play_size = sizeof(playback_buff);
#endif
    cfg.cap_buf = capture_buff;
    cfg.cap_size = sizeof(capture_buff);
    cfg.recv_buf = recv_buff;
    cfg.recv_size = sizeof(recv_buff);
    cfg.send_buf = send_buff;
    cfg.send_size = sizeof(send_buff);
    cfg.eq_buf = eq_buff;
    cfg.eq_size = sizeof(eq_buff);
    cfg.resample_buf = resample_buff;
    cfg.resample_size = sizeof(resample_buff);
#ifdef USB_AUDIO_MULTIFUNC
    cfg.recv2_buf = recv2_buff;
    cfg.recv2_size = sizeof(recv2_buff);
#endif

    usb_audio_app_init(&cfg);
    usb_audio_app(true);
#endif

#ifdef ANC_APP
    anc_usb_app_init(AUD_INPUT_PATH_MAINMIC, SAMPLE_RATE_PLAYBACK, SAMPLE_RATE_CAPTURE);
#endif

#ifdef DUALADC_AUDIO_TEST
    dualadc_audio_app_init(playback_buff, USB_AUDIO_PLAYBACK_BUFF_SIZE,
        capture_buff, USB_AUDIO_CAPTURE_BUFF_SIZE);
    dualadc_audio_app(true);
#endif

#ifdef ADDA_LOOP_APP
    adda_loop_app(true);
#endif

#if defined(_VENDOR_MSG_SUPPT_)
#ifndef USB_ANC_MC_EQ_TUNING
    vendor_info_init();
#endif
#endif

#if defined(CFG_MIC_KEY)
    mic_key_open();
#endif

    // Allow sleep
    hal_sysfreq_req(HAL_SYSFREQ_USER_INIT, HAL_CMU_FREQ_32K);

    while (1) {
#ifdef USB_AUDIO_APP
        usb_audio_app_loop();
#endif

#ifdef ANC_APP
        anc_usb_app_loop();
#endif

#ifdef ADDA_LOOP_APP
        adda_loop_app_loop();
#endif

#ifdef RTOS
        // Let the task sleep
        osDelay(20);
#else // !RTOS

#ifdef USB_EQ_TUNING
        audio_eq_usb_eq_update();
#endif

        hal_sleep_enter_sleep();

#endif // !RTOS
    }
}

#ifdef CFG_HW_GPADCKEY
void gpadc_key_handler(uint16_t irq_val, HAL_GPADC_MV_T volt)
{
    static uint16_t stable_cnt = 0;
    static uint16_t click_cnt = 0;
    static uint32_t click_time;
    uint32_t time;
    enum HAL_KEY_EVENT_T event;
    bool send_event = false;

    time = hal_sys_timer_get();

    if (volt < 100) {
        stable_cnt++;
        //TRACE(5,"adc_key down: volt=%u stable=%u click_cnt=%u click_time=%u time=%u", volt, stable_cnt, click_cnt, click_time, time);
    } else {
        if (stable_cnt > 1) {
            //TRACE(5,"adc_key up: volt=%u stable=%u click_cnt=%u click_time=%u time=%u", volt, stable_cnt, click_cnt, click_time, time);
            if (click_cnt == 0 || (time - click_time) < MS_TO_TICKS(500)) {
                click_time = time;
                click_cnt++;
                if (click_cnt >= 3) {
                    send_event = true;
                }
            } else {
                send_event = true;
            }
        }
        stable_cnt = 0;

        if (click_cnt > 0 && (time - click_time) >= MS_TO_TICKS(500)) {
            send_event = true;
        }

        if (send_event) {
            //TRACE(5,"adc_key click: volt=%u stable=%u click_cnt=%u click_time=%u time=%u", volt, stable_cnt, click_cnt, click_time, time);
            if (click_cnt == 1) {
                event = HAL_KEY_EVENT_CLICK;
            } else if (click_cnt == 2) {
                event = HAL_KEY_EVENT_DOUBLECLICK;
            } else {
                event = HAL_KEY_EVENT_TRIPLECLICK;
            }
            key_event_process(CFG_HW_GPADCKEY, event);
            click_cnt = 0;
        }
    }
}
#endif

#ifdef DEBUG_MODE_USB_DOWNLOAD
static void process_usb_download_mode(void)
{
    if (pmu_charger_get_status() == PMU_CHARGER_PLUGIN && hal_pwrkey_pressed()) {
        hal_sw_bootmode_set(HAL_SW_BOOTMODE_FORCE_USB_DLD);
        hal_cmu_sys_reboot();
    }
}
#endif

// GDB can set a breakpoint on the main function only if it is
// declared as below, when linking with STD libraries.
int MAIN_ENTRY(void)
{
#ifdef INTSRAM_RUN
//    hal_norflash_sleep(HAL_NORFLASH_ID_0);
#endif

#ifdef DEBUG_MODE_USB_DOWNLOAD
    process_usb_download_mode();
#endif

    hal_cmu_simu_init();
    hwtimer_init();
    hal_audma_open();
    hal_gpdma_open();

#if (DEBUG_PORT == 2)
    hal_iomux_set_analog_i2c();
    hal_iomux_set_uart1();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#else
    hal_iomux_set_uart0();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
#endif

#if !defined(SIMU) && !defined(FPGA)
    uint8_t flash_id[HAL_NORFLASH_DEVICE_ID_LEN];
    hal_norflash_get_id(HAL_NORFLASH_ID_0, flash_id, ARRAY_SIZE(flash_id));
    TRACE(3,"FLASH_ID: %02X-%02X-%02X", flash_id[0], flash_id[1], flash_id[2]);
#endif

    pmu_open();
    analog_open();
    af_open();
    hal_sleep_start_stats(10000, 10000);

#ifdef CHIP_BEST1000
    hal_cmu_force_bt_sleep();
#endif

    hal_key_open(0, key_event_process);
#ifdef CFG_HW_GPADCKEY
    hal_gpadc_open(HAL_GPADC_CHAN_3, HAL_GPADC_ATP_20MS, gpadc_key_handler);
#endif

#ifdef CFG_HW_KEY_LED_PIN
    hal_iomux_init(pinmux_key_led, ARRAY_SIZE(pinmux_key_led));
    hal_gpio_pin_set_dir(CFG_HW_KEY_LED_PIN, HAL_GPIO_DIR_OUT, 0);
#endif

#if defined(__SW_IIR_EQ_PROCESS__) || defined(__HW_FIR_EQ_PROCESS__)|| defined(__HW_DAC_IIR_EQ_PROCESS__)|| defined(__HW_IIR_EQ_PROCESS__)
    audio_process_init();
    // TODO: Get EQ store parameter
    // audio_eq_fir_update_cfg(int16_t *coef, int16_t num);
    // audio_eq_iir_update_cfg(int *gain, int num);
#endif

    anc_usb_open();

    TRACE(0,"byebye~~~\n");

    hal_sys_timer_delay(MS_TO_TICKS(200));

    pmu_shutdown();

    return 0;
}

//--------------------------------------------------------------------------------
// Start of Programmer Entry
//--------------------------------------------------------------------------------
#ifdef PROGRAMMER
#include "cmsis_nvic.h"
#include "hal_chipid.h"
#include "tool_msg.h"

extern uint32_t __StackTop[];
extern uint32_t __StackLimit[];
extern uint32_t __bss_start__[];
extern uint32_t __bss_end__[];

#define EXEC_STRUCT_LOC                 __attribute__((section(".exec_struct")))

bool task_yield(void)
{
    return true;
}

void anc_usb_ramrun_main(void)
{
    uint32_t *dst;

    for (dst = __bss_start__; dst < __bss_end__; dst++) {
        *dst = 0;
    }

    NVIC_InitVectors();
#ifdef UNALIGNED_ACCESS
    SystemInit();
#endif
    hal_cmu_setup();
    main();

#if !defined(PROGRAMMER_INFLASH)
    SAFE_PROGRAM_STOP();
#endif
}

void anc_usb_ramrun_start(void)
{
    GotBaseInit();
#ifdef __ARM_ARCH_8M_MAIN__
    __set_MSPLIM((uint32_t)__StackLimit);
#endif
    __set_MSP((uint32_t)__StackTop);
    anc_usb_ramrun_main();
}

const struct exec_struct_t EXEC_STRUCT_LOC ramrun_struct = {
    .entry = (uint32_t)anc_usb_ramrun_start,
    .param = 0,
    .sp = 0,
    .exec_addr = (uint32_t)&ramrun_struct,
};

void programmer_start(void) __attribute__((weak,alias("anc_usb_ramrun_main")));
#endif
//--------------------------------------------------------------------------------
// End of Programmer Entry
//--------------------------------------------------------------------------------

