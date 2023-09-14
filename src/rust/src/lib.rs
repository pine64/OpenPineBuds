//! Rust library for OpenPineBuds
#![deny(
    warnings,
    missing_copy_implementations,
    missing_debug_implementations,
    missing_docs,
    clippy::all,
    clippy::cargo,
    trivial_casts,
    trivial_numeric_casts,
    unused_import_braces,
    unused_qualifications,
    unused_extern_crates,
    variant_size_differences
)]

#[cxx::bridge]
mod ffi {
//! Rust FFI module for OpenPineBuds

}
