#![no_std]
#![no_main]

use core::panic::PanicInfo;

// Attribute says Rust arrange fields same like do C
// From UEFI Specification
#[repr(C)]
pub struct EfiMemoryDescriptor {
  pub memory_type: u32,
  pub physical_start: u64,
  pub virtual_start: u64,
  pub number_of_pages: u64,
  pub attribute: u64,
}

// From our structs
#[repr(C)]
pub(crate) struct BootVideoInfo {
  pub(crate) base_address: *mut u32,
  pub(crate) buffer_size: usize,
  pub(crate) width: u32,
  pub(crate) height: u32,
  pub(crate) pixels_per_scanline: u32,
}

#[repr(C)]
pub(crate) struct BootMemoryInfo {
  pub(crate) mem_map_ptr: *const EfiMemoryDescriptor,
  pub(crate) descriptor_size: usize,
  pub(crate) memory_map_size: usize,
}

#[repr(C)]
pub struct BootLoaderInfo {
  pub(crate) boot_video_info: BootVideoInfo,
  pub(crate) boot_memory_info: BootMemoryInfo,
}

// Ain't change function name while compilation
#[unsafe(no_mangle)]
pub unsafe extern "sysv64" fn _start(boot_info: *const BootLoaderInfo,) -> ! {
  unsafe {
    if let Some(info,) = boot_info.as_ref()
    {
      memory_map(info,);
      draw_on_screen(info,);
    }
  }
  loop
  {}
}

pub unsafe fn draw_on_screen(boot_info: &BootLoaderInfo,) {
  let video_info = &boot_info.boot_video_info;
  let fb = video_info.base_address;
  let pitch = video_info.pixels_per_scanline as usize;
  let width = video_info.width as usize;
  let height = video_info.height as usize;

  for y in 0..height
  {
    for x in 0..width
    {
      let offset = y * pitch + x;

      unsafe {
        *fb.add(offset,) = 0x00FFAA55;
      }
    }
  }
}

pub unsafe fn memory_map(boot_info: &BootLoaderInfo,) {
  let memory_info = &boot_info.boot_memory_info;
  let desc_ptr = memory_info.mem_map_ptr as *const u8;
  let desc_size = memory_info.descriptor_size as usize;
  let total_entries = memory_info.memory_map_size as usize;

  for i in 0..total_entries
  {
    unsafe {
      let current_desc_ptr = desc_ptr.add(i * desc_size,) as *const EfiMemoryDescriptor;
      let desc = &*current_desc_ptr;

      if desc.memory_type == 7
      {
        let phys_start = desc.physical_start;
        let page_count = desc.number_of_pages;
        let size_in_bytes = page_count * 4096;
      }
    }
  }
}

#[panic_handler]
fn panic(_info: &PanicInfo,) -> ! {
  loop
  {}
}
