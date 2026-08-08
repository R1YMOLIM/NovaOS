#pragma once

typedef struct {
  void *mem_map_ptr;
  unsigned long long descriptor_size;
  unsigned long long memory_map_size;
} boot_memory_info_t;
