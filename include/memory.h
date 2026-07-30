#pragma once

typedef struct {
  int type;
  unsigned long long physical_start;
  unsigned long long virtual_start;
  unsigned long long number_of_pages;
  unsigned long long attribute;
} efi_memory_descriptor_t;

typedef struct {
  efi_memory_descriptor_t *mem_map_ptr;
  unsigned long long descriptor_size;
  unsigned long long memory_map_size;
} boot_memory_info_t;
