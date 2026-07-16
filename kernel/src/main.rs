#![no_std]
#![no_main]

use core::panic::PanicInfo;

// Attribute says Rust arrange fields same like do C
#[repr(C)]
pub struct BootVideoInfo {
    pub base_address: *mut u32, 
    pub buffer_size: usize,     
    pub width: u32,             
    pub height: u32,            
    pub pixels_per_scanline: u32, 
}

// Ain't change function name while compilation
#[unsafe(no_mangle)] 
pub extern "sysv64" fn _start(boot_info: *const BootVideoInfo) -> ! {
    unsafe {
        if let Some(info) = boot_info.as_ref() {
            let fb = info.base_address;
            let pitch = info.pixels_per_scanline as usize;
            let width = info.width as usize;
            let height = info.height as usize;

            for y in 0..height {
                for x in 0..width {
                    let offset = y * pitch + x;
                    
                    *fb.add(offset) = 0x00FF0000;
                }
            }
        }
    }

    loop {}
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
