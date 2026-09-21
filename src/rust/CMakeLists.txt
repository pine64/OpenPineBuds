# See cmake/rust.cmake for the Corrosion FetchContent + Rust_CARGO_TARGET
# setup, and src/rust/src/lib.rs for why this is one crate rather than one
# per rewritten module.
corrosion_import_crate(MANIFEST_PATH Cargo.toml)

# cc-rs (used by cxx-build in build.rs) needs to know which compiler to
# invoke for the thumbv7em-none-eabihf target; it otherwise falls back to
# guessing a host-style cross-compiler name that doesn't exist in this
# toolchain (arm-none-eabi-gcc has no such alias).
corrosion_set_env_vars(openpinebuds_rs
    "CC_thumbv7em_none_eabihf=${CMAKE_C_COMPILER}"
    "CXX_thumbv7em_none_eabihf=${CMAKE_CXX_COMPILER}"
    "AR_thumbv7em_none_eabihf=${CMAKE_AR}"
)

corrosion_add_cxxbridge(openpinebuds_rs_bridge CRATE openpinebuds_rs FILES lib.rs)
