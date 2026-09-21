cmake_minimum_required (VERSION 3.12)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project (OpenPineBuds C CXX ASM)

option (OPB_BLUE_CONN_LIGHT "Enable the blue connection light" OFF) # Use BLE to control this.

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
# src/Makefile: C++_ONLY_FLAGS := -std=gnu++98 -fno-rtti. This isn't just
# historical — the codebase's TRACE_*() macros pervasively paste a string
# literal directly against a parameter name with no space (e.g.
# `"[AUD][MGR]"str`, expecting str to substitute to another string literal
# for C-style adjacent-literal concatenation). C++11 and later lex
# `"literal"identifier` with no space as a single user-defined-literal
# token *before* macro substitution happens, so anything >= C++11 fails to
# compile these headers at all. Must stay gnu++98 (extensions ON, not the
# strict ISO c++98) to match exactly.
set (CMAKE_CXX_STANDARD 98)
set (CMAKE_CXX_EXTENSIONS ON)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(compile-flags)
include(target-defaults)
include(options)
include(image)

add_subdirectory (src)

option (OPB_BUILD_SMOKE_TEST "Build a minimal firmware image against the ported subset of the tree, to regression-test the toolchain/link/image pipeline independently of unported apps/services" ON)
if(OPB_BUILD_SMOKE_TEST)
    add_subdirectory (test/smoke)
endif()
