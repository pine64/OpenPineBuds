# Makefile → CMake Migration Plan

## Status (2026-09-21)
- **Phase 0 (fix broken WIP): done.** Top-level and `src/CMakeLists.txt`
  configure cleanly again; stray `rust_backend/` removed.
- **Phase 1 (toolchain + minimal link): done, verified in a real
  arm-none-eabi-gcc container.** `cmake/toolchain-arm-none-eabi.cmake` +
  `cmake/compile-flags.cmake` + `cmake/image.cmake` (linker-script
  preprocessing, build_info.c generation, objcopy) are in place. The full
  `platform_hal → platform_cmsis → platform_drivers_{ana,codec,norflash} →
  rtos → utils` chain compiles with correct per-file/per-chip flags, and
  `test/smoke/` links a real firmware image through the real
  `best1000.lds.S` down to exactly one missing symbol
  (`nv_record_flash_flush`, genuinely in unconverted `services/nvrecord`) —
  confirming the pipeline itself is correct. Several real pre-existing bugs
  were found and fixed along the way (see commit log): `assembler-with-cpp`
  leaking onto C sources in `cmsis`/`rtos`, a missing `platform_hal/best2300p/`
  chip-source glob, a missing `util_cqueue` macro define, missing RTX kernel
  defines, missing global include paths (`include_directories()` was called
  after the `add_subdirectory()`s it was meant to affect).
- **Phase 2 (options + chip/target selection): core deliverable done.**
  `cmake/options.cmake` provides `opb_option()`/`opb_option_value()` —
  ~30 KConfig-like feature toggles matching `open_source/target.mk`, each a
  real CMake `option()` that also becomes a global `-D<NAME>` when ON.
  `CMakePresets.json` has an `open_source` preset matching CI's
  `T=open_source DEBUG=1`. **Not yet done:** `AUDIO=<lang>` resource
  selection; only one chip/target pair is modeled (see TODOs in
  `cmake/target-defaults.cmake`).
- **Phase 3 (bulk-convert remaining Makefiles): substantial progress —
  the `test/smoke/opb_smoke_test` firmware image now links with ZERO
  undefined references**, producing a real `.elf`/`.bin`/`.hex` (verified in
  the same arm-none-eabi-gcc container as Phase 1). Converted since the
  status above: `platform/drivers/bt`, and ~20 `services/` directories
  (`nvrecord`, `nv_section/userdata_section`, `osif`, `audioflinger`,
  `audio_process`, `communication`, `audio_dump`, `hw_dsp`, `cp_accel`,
  `overlay`, `resources`, `bt_if_enhanced`, `bt_profiles_enhanced`,
  `ibrt_core`, `ibrt_ui`, plus `utils/libc`/`utils/sha256` enabled).
  Key discoveries that reduced scope: `ble_stack`/`ble_app`/`bridge`/
  `ble_profiles` are only in `core-y` when `BLE=1` (open_source has `BLE=0`
  — none of the RivieraWaves BLE stack is needed for this target, just a
  couple of shared headers); `bt_if_enhanced`/`bt_profiles_enhanced`/
  `ibrt_core`/`ibrt_ui` ship **zero source** for this config — BES's BT/IBRT
  stack is a closed-source prebuilt archive per build configuration, wired
  as CMake `IMPORTED` targets rather than compiled.
  **Update: `services/bt_app`, `services/multimedia`, `app_ibrt`, `app_tws`,
  and the remaining `apps/*` application layer are now converted and the
  FULL `open_source` target (293/293 ninja targets — every library under
  `src/`, not just `test/smoke`) compiles and links with ZERO errors** in
  the same arm-none-eabi-gcc container used for Phase 1 verification.
  `services/multimedia` (FDK-AAC, SBC, speech, ANC/DRC/resample/limiter
  filters) turned out to be, like the BT/IBRT stack, a closed-source
  prebuilt archive (`best2300p_libmultimedia_cp_anc.a`) for this build
  config — wired as an `IMPORTED` target rather than converted, with only
  the handful of directories that do ship real `.c` (`anc/cfg`,
  `filters/cfg`, `resample/coef`) built from source. `thirdparty/audio_codec_lib/ldac`
  is likewise an `IMPORTED` blob (`ldac_lib.a`). This eliminated the
  numerically-sensitive-DSP verification concern raised earlier — there is
  almost no real multimedia *source* to verify, just headers/link wiring.
  `services/bt_app`, `app_ibrt`, and `app_tws` have a genuine circular
  static-library dependency (also involving `apps/anc`), which CMake
  tolerates. One upstream-only source gap was found and excluded rather
  than patched: `app_fp_rfcomm.cpp` references RFCOMM constants absent from
  every shipped header (dead code anyway, since `GFPS_ENABLE=0`).
  `apps/factory` is now wired. Note: only `test/smoke` produces an actual
  linked `.elf`/`.bin` executable — wiring `apps_main` et al. into a real
  bootable `open_source` firmware image (matching the Makefile's final
  link step, real entry point, full linker script) is unstarted and would
  be a reasonable next milestone, distinct from "everything compiles."
- **Phase 4 (Rust/Corrosion): plumbing done and verified end-to-end.**
  `cmake/rust.cmake` fetches Corrosion behind a new `OPB_ENABLE_RUST`
  option (default ON) and pins `Rust_CARGO_TARGET=thumbv7em-none-eabihf`
  (Corrosion's own triple-guessing has no way to know we want the
  Cortex-M4F hard-float ABI). `src/rust/` is a single `openpinebuds_rs`
  crate — one crate for the whole firmware, not one per rewritten module,
  since `#[global_allocator]`/`#[panic_handler]` must be defined exactly
  once — using `cxx` for the bridge, matching the `rust-support` branch's
  chosen design but adapted to genuinely be `#![no_std]` (own allocator
  wrapping the C heap's `malloc`/`free`, own panic handler; `cxx` needs
  `default-features = false, features = ["alloc"]`). Verified in the same
  container as the rest of the build: the crate cross-compiles via
  `cargo rustc --target thumbv7em-none-eabihf`, produces correct
  `armv7e-m` object code, and the generated `opb_rust_ping` bridge symbol
  has the expected C++ mangling — Corrosion is correctly driving cargo
  with this toolchain's `CC`/`CXX`/`AR` (via `corrosion_set_env_vars`).
  Both `OPB_ENABLE_RUST=ON` and `=OFF` build the full `open_source`
  target cleanly. **Not yet done:** an actual C/C++ call site consuming
  the bridge output — this hit an unrelated pre-existing issue where
  `cmake/compile-flags.cmake`'s global `-U__INT32_TYPE__`/
  `-U__UINT32_TYPE__` (needed so newlib's `<stdint.h>` matches
  `plat_types.h`'s typedefs) breaks libstdc++'s real `<cstdint>`, which
  `rust/cxx.h` pulls in transitively and no existing file in the tree
  happened to touch before now. Needs its own investigation rather than
  a blind per-file flag override; left as a follow-up, same treatment as
  the `app_fp_rfcomm.cpp`/`anc_assist.c` upstream gaps.
- **Phase 5 (CI/Makefile removal): not started.**

## Original plan

## Context

OpenPineBuds builds BES2300P earbud firmware with a Linux-Kbuild-style
recursive Makefile system (`src/Makefile`, 121 sub-Makefiles, `T=<target>`
target selection, `config/<target>/target.mk` + `config/common.mk` feature
flags). We want to replace this with CMake:

- A **KConfig-like options layer**: turning firmware features on/off via
  `-D<FLAG>` compile definitions and `#ifdef`, the same way `target.mk` /
  `common.mk` do today, but expressed as CMake `option()`s instead of Make
  variables.
- **Provision for incremental Rust rewrites** of individual modules via
  [Corrosion](https://github.com/corrosion-rs/corrosion), so C code can be
  replaced piecemeal rather than in one rewrite.
- Eventual **removal of all Makefiles** once the CMake build reaches parity.

The `cmake` branch already has a partial, WIP conversion (44 of 121
`CMakeLists.txt` written) started by directory-globbing sources, but it does
**not** yet handle feature flags, chip/target selection, the linker-script
pipeline, image post-processing, or a working Rust integration. The current
working tree is actually non-buildable (see Findings below) — this plan
starts by fixing that, then fills in the missing pieces in dependency order
before doing the long tail of remaining directories.

## Findings that shape this plan

- **No `obj-$(CONFIG_X)` idiom is used anywhere.** Feature gating is done
  three ways, all of which need a CMake equivalent:
  1. Per-file defines: `CFLAGS_<file>.o += -DFOO` (107 occurrences) →
     `set_source_files_properties(file.c PROPERTIES COMPILE_DEFINITIONS FOO)`.
  2. Whole-directory defines: `ccflags-y`/`subdir-ccflags-y += -DFOO` →
     `target_compile_definitions(lib PRIVATE FOO)`.
  3. Whole-file/subdirectory inclusion: `obj-y += $(wildcard app_gfps/*.c)`
     gated by `ifeq ($(TEST_OVER_THE_AIR),1)` → conditional
     `add_subdirectory()` / `target_sources()`.
  - ~468 distinct flag variables exist repo-wide. Trying to model all of them
    as first-class CMake `option()`s up front is not worth it — see the
    tiered approach below.
- **`src/scripts/include.mk`** provides `cc-option` (→ CMake's
  `check_c_compiler_flag`), and `if_changed`/dep-file tracking (→ subsumed by
  CMake/Ninja's own incremental rebuild — no equivalent needed).
  **`submods_init.mk`** (per-directory recursive-make invocation) has no
  CMake equivalent and simply disappears once the tree is one CMake project.
- **`config/_default_cfg_src_/`** holds both generated/overridable C sources
  (`tgt_hardware.c`, `app_status_ind.c`, `slave_code.S`) and audio-prompt
  resources (`res/en`, `res/cn`, …). The "per-target override, else default"
  pattern (`config/Makefile`, `apps/main/Makefile`) is used in exactly those
  two spots — small enough to be one reusable CMake helper function.
- **No toolchain file, no chip table beyond one hardcoded chip, no final
  link/image step exist yet** in the WIP CMake. `common.mk`'s ~300-line CHIP
  table (per-chip `CHIP_HAS_*` capability flags), the `.lds.S` → `.lds`
  linker-script preprocessing step, and the ELF→BIN/HEX/build-info
  generation step are all unconverted.
- **CI only builds one configuration**: `T=open_source DEBUG=1` (see
  `.github/workflows/build-firmware.yml`, `src/build.sh`). That's the parity
  target for a first working CMake build. Toolchain is fixed:
  `arm-none-eabi-gcc` 9-2019-q4 (`Dockerfile`) — no `armclang` support needs
  porting.
- **The working tree's Rust scaffold is broken and duplicated.** Two
  untracked, never-committed scaffolds exist (`src/rust/` and top-level
  `rust_backend/`), both crate `opb_rust`, plain `corrosion_add_library`, no
  `#![no_std]`. Separately, `origin/rust-support` (one commit, `38edcb6`,
  never merged) has a more complete design: `corrosion_import_crate` +
  `corrosion_add_cxxbridge`, crate `openpinebuds_rs`, using `cxx` for typed
  FFI — but it's also not `no_std` and isn't wired into the root
  `CMakeLists.txt`. **These two designs are incompatible and must be
  reconciled**, and neither addresses that this is a bare-metal Cortex-M
  target (no `std`, needs a `thumbv7em-none-eabihf`/`thumbv8m.main-none-eabihf`
  Rust target and a panic strategy). Additionally, the current uncommitted
  diff to the root and `src/CMakeLists.txt` **deletes**
  `cmake_minimum_required`/`project()`/`add_subdirectory(src)` and
  `add_subdirectory(apps)` — i.e. the working tree doesn't configure at all
  right now.

## Approach

### Phase 0 — Fix the broken WIP (prerequisite for everything else)
- Restore `cmake_minimum_required`, `project(...)`, `add_subdirectory(src)`
  in the top-level `CMakeLists.txt`; restore `add_subdirectory(apps)` in
  `src/CMakeLists.txt`.
- Delete the stray, never-committed, empty `rust_backend/` directory (dead
  duplicate of `src/rust/`).
- Move the Corrosion `FetchContent` block out of the top-level
  `CMakeLists.txt` into `cmake/Rust.cmake`, included once, guarded by
  `option(OPB_ENABLE_RUST "Build the Rust subsystem via Corrosion" ON)` so
  building doesn't hard-require `cargo`.

### Phase 1 — Toolchain file + one working end-to-end link
- Add `cmake/toolchain-arm-none-eabi.cmake` (`CMAKE_SYSTEM_NAME Generic`,
  `CMAKE_C/CXX/ASM_COMPILER arm-none-eabi-{gcc,g++}`, `CMAKE_TRY_COMPILE_TARGET_TYPE
  STATIC_LIBRARY`, base flags lifted from `src/Makefile`'s `KBUILD_CFLAGS`).
- Port the linker-script pipeline: a CMake function that preprocesses
  `scripts/link/<name>.lds.S` → `.lds` with the cross-compiler's `-E -P -C -x c`
  (replacing `cmd_cpp_lds_S`), then a post-build step running `objcopy` to
  produce `.bin`/`.hex`/`.map`, and a small `configure_file`-generated
  `build_info.c` carrying `REVISION_INFO` (git describe, ported from
  `GIT_REVISION` logic in `src/Makefile`).
- Prove this pipeline on the minimal dependency chain that already has
  CMakeLists.txt (`rtos` → `platform/hal` → `platform/main`, once `main` is
  un-commented and converted) before touching the ~90 remaining directories.
  Goal: one linkable, flashable `.bin` for a stripped-down config.

### Phase 2 — KConfig-like options layer + chip/target selection
- `cmake/options.cmake`: a thin `opb_option(NAME DEFAULT DOC)` wrapper over
  CMake `option()`/`set(... CACHE)` that also registers the flag for
  `add_compile_definitions($<$<BOOL:${NAME}>:NAME>)`, so existing `#ifdef
  FOO` code needs no changes — only the ~30–50 *root-level* flags that
  `target.mk`/`common.mk` actually set for the real build targets go here
  first (`IBRT`, `BLE`, `OTA_ENABLE`, `ANC_APP`, `GFPS_ENABLE`, `TOTA`, …).
  The long tail of the other ~400 per-file/per-subdir flags stay local,
  handled directly in each leaf `CMakeLists.txt` with
  `target_compile_definitions`/conditional `target_sources`, mirroring
  whichever of the 3 patterns above that Makefile used — don't try to
  centralize those.
- `cmake/chips/<chip>.cmake`: port `common.mk`'s per-`CHIP` capability table
  (`CHIP_HAS_FPU`, `CHIP_HAS_USB`, …) — one file per chip, selected by a
  cache variable `OPB_CHIP` (default `best2300p`, the only chip this fork
  currently ships).
- `cmake/targets/<target>.cmake`: one file per existing `config/<target>/target.mk`
  (`open_source`, `best2300p_ibrt`, `best2300p_ibrt_anc`, `mic_alg`,
  `bak_open`) setting the `opb_option()` values for that target — replaces
  `T=<target>`. Expose via `CMakePresets.json` (`open_source` preset matches
  current CI).
- Add the `opb_target_override_source(NAME)` helper for the
  override-else-default pattern (`config/<T>/<f>` if present, else
  `config/_default_cfg_src_/<f>`), used by `config/CMakeLists.txt` and
  `apps/main/CMakeLists.txt`.
- Port `AUDIO=<lang>` resource selection (`config/_default_cfg_src_/res/<lang>`)
  as an `OPB_AUDIO_LANG` cache variable feeding the same resource-copy step
  `convert.sh`/`config/Makefile` currently does.

### Phase 3 — Bulk-convert the remaining ~90 Makefile directories
With the patterns proven in Phase 1–2, convert directory-by-directory,
services first (most apps/platform code depends on them), in this rough
order: `services/*` (30 dirs) → `thirdparty/*` (7 dirs, mostly plain globs,
low effort) → remaining `platform/drivers/*`, `platform/cmsis/*` → `rtos`
leaf dirs → `apps/factory` (currently has neither Makefile-glob-parity nor
CMakeLists.txt) → `utils/libc`, `utils/rom_utils`, `utils/sha256` (currently
commented out in `src/utils/CMakeLists.txt`). Each directory: replace
`file(GLOB)` with explicit source lists only where the Makefile already
conditionally includes/excludes files (glob is fine where the Makefile was
an unconditional glob, e.g. `thirdparty/audio_codec_lib`).

### Phase 4 — Rust/Corrosion: reconcile and harden
- Adopt `origin/rust-support`'s `cxx`-bridge approach
  (`corrosion_import_crate` + `corrosion_add_cxxbridge`, crate
  `openpinebuds_rs`) over the plain-staticlib `opb_rust` stub — typed FFI is
  a better fit for "incremental rewrite of individual C modules" than a
  hand-rolled `extern "C"` boundary. Bring `src/rust/{CMakeLists.txt,Cargo.toml,Cargo.lock,src/lib.rs}`
  from that branch into `cmake`, replacing the untracked stub and the dead
  `rust_backend/` directory (already deleted in Phase 0).
- Make the crate `#![no_std]` (it currently isn't) and add a `panic_handler`
  strategy appropriate for a bare-metal target; set up
  `rustup target add thumbv7em-none-eabihf` (Cortex-M4, matches `CPU := m4`
  for best2300p) and pass it to Corrosion via
  `set(Rust_CARGO_TARGET thumbv7em-none-eabihf)`.
- Land it as a real, linked static library once one real migration
  candidate exists — good first candidates are small, already-isolated,
  pure-algorithm leaf libraries with clean interfaces: `utils/crc32`,
  `utils/crc16`, `utils/sha256`, or `utils/encrypt`.

### Phase 5 — CI parity, then remove Makefiles
- Add a CMake+Ninja build job to `.github/workflows/build-firmware.yml`
  reproducing the `T=open_source DEBUG=1` config; run it alongside the
  existing Make-based job (not replacing it yet) until output parity
  (boots, same feature set) is confirmed.
- Once green and trusted: delete all 121 `Makefile`s, `src/scripts/{include.mk,submods_init.mk,clean.mk}`,
  `src/config/*/target.mk` + `common.mk` (superseded by `cmake/targets/*.cmake`
  and `cmake/chips/*.cmake`), retire the old CI job, and update
  `README.md`/`build.sh`/`start_dev.sh` to document the CMake workflow.

## Verification
- Phase 1: linked `.bin` produced for the minimal chain; flash-test via
  existing `download.sh` (or at minimum diff `.map`/section sizes against
  the equivalent Make build) to confirm the linker-script/objcopy pipeline
  is correct.
- Phase 3: after each directory batch, `cmake --build` the full
  `open_source` preset and confirm it still configures/links.
- Phase 5: byte-for-byte or at least boot-for-boot comparison between the
  Make-built and CMake-built `open_source.bin` before deleting any Makefile.
