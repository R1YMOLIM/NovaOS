#include "microkernel.h"

#include <memory.h>

typedef uint8_t node_state_t;

enum { NODE_FREE = 0, NODE_SPLIT = 1, NODE_ALLOCATED = 2 };

typedef struct {
  node_state_t *tree;
  size_t tree_size;
  size_t total_pages;
  uintptr_t phys_start;
} buddy_allocator_t;

static const boot_memory_info_t *g_memory;
static buddy_allocator_t *g_buddy;

static void update_parents(buddy_allocator_t *alloc, size_t index) {
  size_t curr = index;
  while (curr > 0) {
    size_t parent = (curr - 1) / 2;
    size_t left = (2 * parent) + 1;
    size_t right = (2 * parent) + 2;

    if (alloc->tree[left] == NODE_ALLOCATED && alloc->tree[right] == NODE_ALLOCATED) {
      alloc->tree[parent] = NODE_ALLOCATED;
    } else {
      alloc->tree[parent] = NODE_SPLIT;
    }
    curr = parent;
  }
}

void buddy_init(void *memory_buffer, size_t raw_page_count, uint64_t phys_start) {
  size_t pages = 1;
  while (pages < raw_page_count) {
    pages <<= 1; // Round to nearest power by 2
  }

  g_buddy->total_pages = pages;
  g_buddy->tree_size = (2 * pages) - 1;
  g_buddy->tree = (node_state_t *)memory_buffer;
  g_buddy->phys_start = phys_start;

  for (size_t i = 0; i < g_buddy->tree_size; i++) {
    g_buddy->tree[i] = NODE_FREE;
  }
}

void *buddy_alloc(size_t req_pages) {
  if (req_pages == 0 || req_pages > g_buddy->total_pages) {
    return NULL;
  }

  size_t target_pages = 1;
  while (target_pages < req_pages) {
    target_pages <<= 1;
  }

  size_t current_i = 0;
  size_t current_pages = g_buddy->total_pages;

  if (g_buddy->tree[current_i] == NODE_ALLOCATED) {
    return NULL;
  }

  size_t page_offset = 0;

  while (current_pages > target_pages) {
    size_t left = (2 * current_i) + 1;
    size_t right = (2 * current_i) + 2;
    size_t child_size = current_pages / 2;

    int left_ok = (child_size == target_pages) ? (g_buddy->tree[left] == NODE_FREE)
                                               : (g_buddy->tree[left] != NODE_ALLOCATED);

    int right_ok = (child_size == target_pages) ? (g_buddy->tree[right] == NODE_FREE)
                                                : (g_buddy->tree[right] != NODE_ALLOCATED);

    if (left_ok) {
      current_i = left;
    } else if (right_ok) {
      current_i = right;
      page_offset += child_size;
    } else {
      return NULL; // Not found free node
    }

    current_pages /= 2;
  }

  if (g_buddy->tree[current_i] != NODE_FREE) {
    return NULL;
  }

  g_buddy->tree[current_i] = NODE_ALLOCATED;
  update_parents(g_buddy, current_i);

  uint64_t phys_addr = g_buddy->phys_start + (page_offset * 4096);
  return (void *)(uintptr_t)phys_addr;
}

void init_memory(const boot_memory_info_t *memory) {
  g_memory = memory;
  if (!memory || !memory->mem_map_ptr || memory->descriptor_size == 0) {
    return;
  }

  const uint8_t *desc_ptr = (const uint8_t *)memory->mem_map_ptr;
  size_t desc_count = memory->memory_map_size / memory->descriptor_size;

  uint64_t start = 0;
  uint64_t max_pages = 0;

  for (size_t i = 0; i < desc_count; i++) {
    const efi_memory_descriptor_t *desc =
      (const efi_memory_descriptor_t *)(desc_ptr + (i * memory->descriptor_size));

    // 7 - conventional memory
    if (desc->memory_type == 7) {
      if (desc->number_of_pages > max_pages) {
        max_pages = desc->number_of_pages;
        start = desc->physical_start;
      }
    }
  }

  if (max_pages == 0) {
    return; // Not found free memory
  }

  size_t pow2_pages = 1;
  while (pow2_pages < max_pages) {
    pow2_pages <<= 1;
  }
  size_t tree_nodes = (2 * pow2_pages) - 1;
  size_t tree_bytes = tree_nodes * sizeof(node_state_t);
  size_t tree_pages = (tree_bytes + 4095) / 4096;

  if (max_pages <= tree_pages) {
    return;
  }

  void *tree_buffer = (void *)(uintptr_t)start;

  uint64_t usable_ram_start = start + (tree_pages * 4096);
  uint64_t usable_page_count = max_pages - tree_pages;

  // Init global allocator
  buddy_init(tree_buffer, usable_page_count, usable_ram_start);
}
