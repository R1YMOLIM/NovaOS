#![no_std]
#![no_main]

use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn _start(fb_base: *mut u32, fb_pixels_count: usize) -> ! {
    if !fb_base.is_null() {
        unsafe {
            for i in 0..fb_pixels_count {
                fb_base.add(i).write_volatile(0x0000FF00);
            }
        }
    }

    loop {
        core::hint::spin_loop();
    }
}
