#![no_std]
#![no_main]

use core::{arch::asm, panic::PanicInfo};

#[macro_use]
mod macros;

mod memory;
mod video;

use memory::{parse_memory_map, BootMemoryInfo};
use video::{
  draw_line_global, draw_pixel_global, draw_rectangle_global, fill_screen_global, BootVideoInfo,
  Color, LineType, FRAMEBUFFER,
};

#[repr(C)]
pub struct BootLoaderInfo {
  boot_video_info: BootVideoInfo,
  boot_memory_info: BootMemoryInfo,
}

#[unsafe(no_mangle)]
#[unsafe(link_section = ".text.main")]
pub unsafe extern "sysv64" fn _start(boot_info: *const BootLoaderInfo,) -> ! {
  if let Some(info,) = unsafe { boot_info.as_ref() }
  {
    // Video init
    *FRAMEBUFFER.lock() = Some(info.boot_video_info,);

    // draw
    fill_screen_global(Color {
      r: 255, g: 0, b: 0,
    },);
    draw_pixel_global(
      100,
      100,
      Color {
        r: 0, g: 255, b: 0,
      },
    );
    draw_line_global(
      110,
      110,
      100,
      Color {
        r: 0, g: 0, b: 255,
      },
      LineType::VerticalLine,
    );
    draw_line_global(
      110,
      110,
      100,
      Color {
        r: 0, g: 255, b: 0,
      },
      LineType::HorizontalLine,
    );
    draw_rectangle_global(
      300,
      300,
      50,
      50,
      Color {
        r: 255,
        g: 255,
        b: 255,
      },
    );

    // Parse memory
    unsafe {
      parse_memory_map(&info.boot_memory_info,);
    }
  }

  loop
  {
    unsafe {
      asm!("hlt");
    }
  }
}

#[panic_handler]
fn panic(_info: &PanicInfo,) -> ! {
  loop
  {}
}
