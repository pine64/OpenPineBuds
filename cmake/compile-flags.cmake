# Project-wide compile flags ported from src/Makefile's KBUILD_CFLAGS /
# C_ONLY_FLAGS / CXX_ONLY_FLAGS and the always-on portions of
# src/config/common.mk (i.e. flags that don't depend on a feature/chip
# option). Chip- and target-specific defines live in cmake/chips/*.cmake and
# cmake/targets/*.cmake (Phase 2), not here.

# src/Makefile: KBUILD_CFLAGS
add_compile_options(
    -fno-common
    -fmessage-length=0
    -Wall
    -fno-exceptions
    -ffunction-sections
    -fdata-sections
    -fomit-frame-pointer
    -fsigned-char
    -fno-aggressive-loop-optimizations
    -fno-isolate-erroneous-paths-dereference
    -fsingle-precision-constant
    -Wdouble-promotion
    -Wfloat-conversion
    -g
)

# src/Makefile: C_ONLY_FLAGS / CXX_ONLY_FLAGS
add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)

# src/Makefile: CONFIG_STRICT_CFLAGS (default y)
add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-int>
    $<$<COMPILE_LANGUAGE:C>:-Werror-implicit-function-declaration>
    -Wno-trigraphs
    -fno-strict-aliasing
    -Wno-format-security
)

# src/config/common.mk: newlib's <stdint.h> derives uint32_t from the
# compiler-builtin __UINT32_TYPE__/__INT32_TYPE__ macros; undefining them
# makes newlib fall back to deriving uint32_t as `unsigned int`, matching
# the firmware's own typedef in platform/hal/plat_types.h exactly (avoids a
# "conflicting types for 'uint32_t'" redeclaration error). Guarded the same
# way common.mk guards it (skipped only when NO_CHK_TRC_FMT=1).
add_compile_options(
    -U__INT32_TYPE__
    -D__INT32_TYPE__=int
    -U__UINT32_TYPE__
)

add_compile_definitions(UNALIGNED_ACCESS)
