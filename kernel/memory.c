#include "microkernel.h"

#include <memory.h>

static const boot_memory_info_t *g_memory = NULL;

// --- Functions ---
// It is forward declaration used for init only in main function

void init_memory(const boot_memory_info_t *memory) {
  g_memory = memory;
  if (!memory || !memory->mem_map_ptr || memory->descriptor_size == 0) {
    return;
  }

  const uint8_t *desc_ptr = (const uint8_t *)memory->mem_map_ptr;
  size_t desc_count = memory->memory_map_size / memory->descriptor_size;

  for (size_t i = 0; i < desc_count; i++) {
    const efi_memory_descriptor_t *desc =
      (const efi_memory_descriptor_t *)(desc_ptr + (i * memory->descriptor_size));

    // 7 - Memory free
    if (desc->memory_type == 7) {
      uint64_t _phys_start = desc->physical_start;
      uint64_t _page_count = desc->number_of_pages;

      (void)_phys_start;
      (void)_page_count;
    }
  }
}
