use crate::draw_rectangle_global;

#[repr(C)]
#[derive(Debug, Clone, Copy,)]
pub struct EfiMemoryDescriptor {
  memory_type: u32,
  physical_start: u64,
  virtual_start: u64,
  number_of_pages: u64,
  attribute: u64,
}

#[repr(C)]
pub struct BootMemoryInfo {
  mem_map_ptr: *const EfiMemoryDescriptor,
  descriptor_size: usize,
  memory_map_size: usize,
}

pub unsafe fn parse_memory_map(boot_memory_info: &BootMemoryInfo,) {
  let desc_ptr = boot_memory_info.mem_map_ptr as *const u8;
  let desc_size = boot_memory_info.descriptor_size;
  let mem_map_size = boot_memory_info.memory_map_size;

  if desc_size == 0
  {
    return;
  }

  let desc_count = mem_map_size / desc_size;
  let mut memory_free = false;

  for i in 0..desc_count
  {
    unsafe {
      let current_desc_ptr = desc_ptr.byte_add(i * desc_size,) as *const EfiMemoryDescriptor;

      let desc = core::ptr::read_unaligned(current_desc_ptr,);

      // 7 - Mean free
      if desc.memory_type == 7
      {
        let _phys_start = desc.physical_start;
        let _page_count = desc.number_of_pages;

        memory_free = true;
        break;
      }
    }
  }

  if memory_free
  {
    draw_rectangle_global(
      500,
      500,
      50,
      50,
      crate::video::Color {
        r: 0, g: 255, b: 0,
      },
    );
  }
}
