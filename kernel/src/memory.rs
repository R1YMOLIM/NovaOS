use crate::BootLoaderInfo;

#[repr(C)]
#[derive(Debug, Clone, Copy,)]
pub struct EfiMemoryDescriptor {
  pub memory_type: u32,
  pub physical_start: u64,
  pub virtual_start: u64,
  pub number_of_pages: u64,
  pub attribute: u64,
}

#[repr(C)]
pub struct BootMemoryInfo {
  pub mem_map_ptr: *const EfiMemoryDescriptor,
  pub descriptor_size: usize,
  pub memory_map_size: usize,
}

pub unsafe fn parse_memory_map(boot_info: &BootLoaderInfo,) {
  let memory_info = &boot_info.boot_memory_info;
  let desc_ptr = memory_info.mem_map_ptr as *const u8;
  let desc_size = memory_info.descriptor_size;
  let mem_map_size = memory_info.memory_map_size;

  if desc_size == 0
  {
    return;
  }

  let desc_count = mem_map_size / desc_size;

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
      }
    }
  }
}
