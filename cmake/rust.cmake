# Phase 4: incremental Rust rewrites of individual C/C++ modules via
# Corrosion (https://github.com/corrosion-rs/corrosion) + cxx-bridge, per
# PLAN.md. This is plumbing only — src/rust/ is a single crate that grows
# one #[cxx::bridge] module per C file/subsystem as it gets rewritten,
# rather than one crate per module (see src/rust/CMakeLists.txt for why:
# the panic handler and global allocator must be defined exactly once in
# the whole link).
option(OPB_ENABLE_RUST "Enable incremental Rust module rewrites via Corrosion (needs cargo, cxxbridge-cmd, and the thumbv7em-none-eabihf rustup target)" ON)

if(OPB_ENABLE_RUST)
    find_program(OPB_CARGO cargo)
    if(NOT OPB_CARGO)
        message(FATAL_ERROR "OPB_ENABLE_RUST requires cargo; install via https://rustup.rs/, run "
            "`rustup target add thumbv7em-none-eabihf` and `cargo install cxxbridge-cmd`, "
            "or configure with -DOPB_ENABLE_RUST=OFF")
    endif()

    # cortex-m4 hard-float, matching OPB_CPU_FLAGS in the toolchain file.
    # Corrosion's own CMAKE_SYSTEM_PROCESSOR/NAME-based triple guessing has
    # no way to know we want the hard-float ABI variant, so set it explicitly.
    # TODO(Phase 2 follow-up): derive from OPB_CHIP once other chips/CPUs are
    # ported, same as the toolchain file's CPU_FLAGS.
    set(Rust_CARGO_TARGET thumbv7em-none-eabihf CACHE STRING
        "Rust target triple for the firmware's Rust code")

    include(FetchContent)
    FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.5.1
    )
    FetchContent_MakeAvailable(Corrosion)

    add_compile_definitions(OPB_ENABLE_RUST=1)
endif()
