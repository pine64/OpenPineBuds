file (GLOB headers "*.h" "${CHIP}/*.h")
file (GLOB sources "*.c*" "${CHIP}/*.c")

add_library (platform_drivers_ana STATIC ${sources} ${headers})
target_link_libraries(platform_drivers_ana platform_hal)
target_include_directories(platform_drivers_ana PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${CHIP})

# TODO(Phase 3): full ANA_CFG_FLAGS/PMU_CFG_FLAGS/USBPHY_CFG_FLAGS parity
# with platform/drivers/ana/Makefile + platform/drivers/ana/${CHIP}/Makefile
# depends on many more feature flags than are modeled yet (AUDIO_OUTPUT_DIFF,
# CODEC_HIGH_QUALITY, MTEST_ENABLED, ...). Only the subset needed to compile
# for the open_source target's defaults is ported here so far.
if(CHIP STREQUAL "best2300p")
    # best2300p/Makefile: VCODEC_VOLT ?= 1.7V/1.95V, but target.mk exports
    # VCODEC_VOLT=1.8V first and `?=` doesn't override an exported value.
    set_property(SOURCE ${CHIP}/analog_${CHIP}.c APPEND PROPERTY COMPILE_DEFINITIONS
        VCODEC_VOLT=1.8
        ANC_VMIC_CFG=AUD_VMIC_MAP_VMIC2 # config/open_source/tgt_hardware.h
        ANC_APP
        ANC_FF_ENABLED
        ANC_FB_ENABLED
        AUDIO_OUTPUT_DC_CALIB_ANA
        LOW_POWER_ADC
    )
    set_property(SOURCE ${CHIP}/pmu_${CHIP}.c APPEND PROPERTY COMPILE_DEFINITIONS
        VCODEC_VOLT=1.8
        VHPPA_VOLT=1.8 # best2300p/Makefile default
        VANA_1P3V     # best2300p/Makefile default (LBRT=0 -> VANA_VOLT=1.3V)
        VCODEC_OFF
        HIGH_VCORE    # best2300p/Makefile default
        OSC_26M_X4_AUD2BB # open_source/target.mk
        VCRYSTAL_OFF  # best2300p/Makefile default (CHIP != best1000)
        ANC_APP
    )
endif()
