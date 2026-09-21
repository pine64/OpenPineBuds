# KConfig-like feature options: each becomes a CMake option() (so it shows
# up in `ccmake`/`cmake-gui`/`cmake -L`, and can be toggled with
# `-D<NAME>=ON/OFF`) AND, when enabled, a global `-D<NAME>` compile
# definition — so existing `#ifdef <NAME>` code needs no changes.
#
# This intentionally does not attempt to model every one of the ~468 flags
# scattered across every leaf Makefile (see PLAN.md's Findings) — only the
# target-level feature toggles a developer would actually want to flip.
# Per-file/per-directory flags (the long tail) stay local to their own
# CMakeLists.txt, set directly with target_compile_definitions/
# set_property(SOURCE ...), the same way the Makefiles scoped them.
#
# Defaults below match src/config/open_source/target.mk — the only target
# this port currently has parity data for (see cmake/target-defaults.cmake).
# TODO(Phase 2 cont'd): once cmake/targets/<target>.cmake exists, move these
# defaults there and let opb_option() take its default from the selected
# target file instead of being hardcoded to open_source here.

function(opb_option name default doc)
    option(${name} "${doc}" ${default})
    if(${name})
        add_compile_definitions(${name})
    endif()
endfunction()

function(opb_option_value name default doc)
    set(${name} "${default}" CACHE STRING "${doc}")
    if(${name})
        add_compile_definitions(${name}=${${name}})
    endif()
endfunction()

# --- ANC (open_source/target.mk) ---
opb_option(ANC_APP ON "Active Noise Cancellation application")
opb_option(ANC_FF_ENABLED ON "ANC feed-forward (external mic) path")
opb_option(ANC_FB_ENABLED ON "ANC feed-back (internal mic) path")
opb_option(ANC_FB_CHECK ON "ANC feed-back mic check")
opb_option(ANC_WNR_ENABLED OFF "ANC wind-noise reduction")
opb_option(ANC_ASSIST_ENABLED OFF "ANC assist")

# --- TWS / Bluetooth (open_source/target.mk) ---
opb_option(IBRT ON "True Wireless Stereo (independent bidirectional real-time transmission)")
opb_option(IBRT_SEARCH_UI ON "IBRT reconnection search UI")
opb_option(BLE OFF "Bluetooth Low Energy")
opb_option(TOTA OFF "Test-Over-The-Air BLE diagnostics channel")
opb_option(GATT_OVER_BR_EDR OFF "GATT over BR/EDR transport")
opb_option(BT_ONE_BRING_TWO OFF "One phone paired to two earbuds simultaneously")

# --- OTA / update (open_source/target.mk) ---
opb_option(OTA_ENABLE OFF "Over-the-air firmware update support")

# --- Google services (open_source/target.mk) ---
opb_option(GOOGLE_SERVICE_ENABLE OFF "Google Fast Pair / BISTO umbrella switch")
opb_option(BISTO_ENABLE OFF "Google Assistant voice service (BISTO)")
opb_option(GFPS_ENABLE OFF "Google Fast Pair Service")
opb_option(GSOUND_HOTWORD_ENABLE OFF "GSound hotword detection")
opb_option(GSOUND_HOTWORD_EXTERNAL OFF "GSound hotword model stored externally")
opb_option(REDUCED_GUESTURE_ENABLE OFF "Reduced-gesture mode")

# --- Codecs (open_source/target.mk) ---
opb_option(A2DP_AAC_ON ON "A2DP AAC codec")
opb_option(A2DP_LHDC_ON OFF "A2DP LHDC codec")
opb_option(A2DP_LDAC_ON ON "A2DP LDAC codec")
opb_option(A2DP_SCALABLE_ON OFF "A2DP Samsung Scalable codec")
opb_option(HFP_1_6_ENABLE ON "HFP 1.6 (mSBC wideband speech)")

# --- Misc (open_source/target.mk) ---
opb_option(FACTORY_MODE OFF "Factory test mode")
opb_option(ENGINEER_MODE OFF "Engineering diagnostics mode")
opb_option(CORE_DUMP ON "Crash core-dump capture")
opb_option(TILE_DATAPATH_ENABLED OFF "Tile find-my-item datapath")
opb_option(INTERCONNECTION OFF "Huawei Interconnection service")
opb_option(INTERACTION OFF "Find-My-Device-style interaction service")
opb_option(WL_UI ON "\"Wear lock\" in-ear detection UI")

# POWER_MODE (open_source/target.mk default: DIG_DCDC) doesn't map to a
# `#ifdef POWER_MODE` directly — platform/drivers/ana/Makefile translates it
# to one of -DLDO_MODE/-DDIG_DCDC_MODE/-DANA_DCDC_MODE on pmu_$(CHIP).o, so
# it's a plain cache variable, not run through opb_option_value().
# TODO(Phase 3): wire this option into platform/drivers/ana/CMakeLists.txt's
# per-file defines instead of the hardcoded DIG_DCDC_MODE currently missing
# there (a known gap — see that file).
set(POWER_MODE "DIG_DCDC" CACHE STRING "Power-supply mode: LDO, DIG_DCDC, or ANA_DCDC")
