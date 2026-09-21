//! Rust library for OpenPineBuds.
//!
//! Bare metal, no OS: this crate is `#![no_std]` and provides its own
//! `#[global_allocator]` (backed by the C heap's `malloc`/`free`, already
//! wired up for the rest of the firmware) and `#[panic_handler]`. Both must
//! be defined exactly once across the whole link, so as more C modules get
//! rewritten in Rust they should become new `#[cxx::bridge]` modules *in
//! this crate*, not new crates.
#![no_std]
#![deny(unused_import_braces, unused_qualifications, unused_extern_crates)]

use core::alloc::{GlobalAlloc, Layout};
use core::panic::PanicInfo;

extern "C" {
    fn malloc(size: usize) -> *mut u8;
    fn free(ptr: *mut u8);
}

struct COverlayAllocator;

unsafe impl GlobalAlloc for COverlayAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        malloc(layout.size())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        free(ptr)
    }
}

#[global_allocator]
static ALLOCATOR: COverlayAllocator = COverlayAllocator;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[cxx::bridge]
mod ffi {
    extern "Rust" {
        /// Placeholder bridge function proving the cxx-bridge/Corrosion
        /// pipeline builds and links for the real thumbv7em-none-eabihf
        /// target. Remove once a real module rewrite replaces it.
        fn opb_rust_ping() -> i32;
    }
}

fn opb_rust_ping() -> i32 {
    1
}
