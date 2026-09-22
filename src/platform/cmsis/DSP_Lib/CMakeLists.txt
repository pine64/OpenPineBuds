# platform/cmsis/DSP_Lib/Makefile: DSP_LIB_ALL_ENABLE not set for
# open_source, so this explicit subset (not the whole CMSIS-DSP tree) is
# what's actually built.
set (sources
    CommonTables/CommonTables.c
    BasicMathFunctions/arm_dot_prod_q15.c
    FilteringFunctions/arm_biquad_cascade_df1_init_f32.c
    FilteringFunctions/arm_biquad_cascade_df1_f32.c
    FilteringFunctions/arm_fir_init_q15.c
    FilteringFunctions/arm_fir_fast_q15.c
    TransformFunctions/arm_bitreversal2.S
    TransformFunctions/arm_cfft_f32.c
    TransformFunctions/arm_cfft_q15.c
    TransformFunctions/arm_cfft_radix4_q15.c
    TransformFunctions/arm_cfft_radix8_f32.c
    TransformFunctions/arm_rfft_fast_init_f32.c
    TransformFunctions/arm_rfft_fast_f32.c
    TransformFunctions/arm_rfft_init_q15.c
    TransformFunctions/arm_rfft_q15.c
    SupportFunctions/arm_copy_q31.c
    SupportFunctions/arm_fill_q31.c
    SupportFunctions/arm_q15_to_float.c
    BasicMathFunctions/arm_dot_prod_q7.c
    BasicMathFunctions/arm_add_q15.c
    BasicMathFunctions/arm_shift_q15.c
    BasicMathFunctions/arm_negate_q15.c
    BasicMathFunctions/arm_offset_q15.c
    BasicMathFunctions/arm_mult_q15.c
    StatisticsFunctions/StatisticsFunctions.c
)

add_library (platform_cmsis_dsp STATIC ${sources})
target_include_directories(platform_cmsis_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_compile_definitions(platform_cmsis_dsp PRIVATE ARM_MATH_LOOPUNROLL)
set_property(SOURCE TransformFunctions/arm_bitreversal2.S APPEND PROPERTY COMPILE_OPTIONS "-x" "assembler-with-cpp")
