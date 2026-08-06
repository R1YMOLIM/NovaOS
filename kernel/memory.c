#include "microkernel.h"

#include <memory.h>

typedef struct {
  node_state_t *tree;
  size_t tree_size;
  size_t total_pages;
  uintptr_t phys_start;
} buddy_allocator_t;

static const boot_memory_info_t *g_memory = NULL;
static buddy_allocator_t g_buddy;

static void update_parents(size_t index) {
  size_t curr = index;
  while (curr > 0) {
    size_t parent = (curr - 1) / 2;
    size_t left = (2 * parent) + 1;
    size_t right = (2 * parent) + 2;

    if (g_buddy.tree[left] == NODE_FREE && g_buddy.tree[right] == NODE_FREE) {
      g_buddy.tree[parent] = NODE_FREE;
    } else if (g_buddy.tree[left] == NODE_ALLOCATED && g_buddy.tree[right] == NODE_ALLOCATED) {
      g_buddy.tree[parent] = NODE_ALLOCATED;
    } else {
      g_buddy.tree[parent] = NODE_SPLIT;
    }

    curr = parent;
  }
}

static void buddy_init(void *memory_buffer, size_t page_count, uint64_t phys_start) {
  g_buddy.total_pages = page_count;
  g_buddy.tree_size = (2 * page_count) - 1;
  g_buddy.tree = (node_state_t *)memory_buffer;
  g_buddy.phys_start = phys_start;

  for (size_t i = 0; i < g_buddy.tree_size; i++) {
    g_buddy.tree[i] = NODE_FREE;
  }
}

size_t kalloc(size_t req_pages) {
  if (req_pages == 0 || req_pages > g_buddy.total_pages) {
    return NODE_FAILED;
  }

  size_t target_pages = 1;
  while (target_pages < req_pages) {
    target_pages <<= 1;
  }

  size_t current_i = 0;
  size_t current_pages = g_buddy.total_pages;

  if (g_buddy.tree[current_i] == NODE_ALLOCATED) {
    return NODE_FAILED;
  }

  while (current_pages > target_pages) {
    size_t left = (2 * current_i) + 1;
    size_t right = (2 * current_i) + 2;
    size_t child_size = current_pages / 2;

    int left_ok = (child_size == target_pages) ? (g_buddy.tree[left] == NODE_FREE)
                                               : (g_buddy.tree[left] != NODE_ALLOCATED);

    int right_ok = (child_size == target_pages) ? (g_buddy.tree[right] == NODE_FREE)
                                                : (g_buddy.tree[right] != NODE_ALLOCATED);

    if (left_ok) {
      current_i = left;
    } else if (right_ok) {
      current_i = right;
    } else {
      return NODE_FAILED; // Not found free node
    }

    current_pages /= 2;
  }

  if (g_buddy.tree[current_i] != NODE_FREE) {
    return NODE_FAILED;
  }

  g_buddy.tree[current_i] = NODE_ALLOCATED;
  update_parents(current_i);

  return current_i;
}

node_state_t kfree(size_t index) {
  if (g_buddy.tree[index] != NODE_ALLOCATED) {
    return NODE_FAILED;
  }
  g_buddy.tree[index] = NODE_FREE;

  while (index > 0) {
    size_t buddy_index = 0;
    if (index % 2 != 0) {
      buddy_index = index + 1; // I'm left, buddy right
    } else {
      buddy_index = index - 1; // I'm right, buddy left
    }

    if (g_buddy.tree[buddy_index] != NODE_FREE) {
      break;
    }

    size_t parent_index = (index - 1) / 2;

    g_buddy.tree[parent_index] = NODE_FREE;
    index = parent_index;
  }

  return NODE_FREE;
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

  // Find free memory and write data in variables
  for (size_t i = 0; i < desc_count; i++) {
    const efi_memory_descriptor_t *desc =
      (const efi_memory_descriptor_t *)(desc_ptr + (i * memory->descriptor_size));

    // 7 - conventional memory (Free memory)
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

  /**
   * Calculate space required for the buddy allocator metadata tree.
   *
   * 1. Round up max_pages to the nearest power of 2 for a balanced binary tree.
   * 2. Compute total node count ((2 * N) - 1).
   * 3. Determine byte size needed to store node states (FREE, SPLIT, ALLOCATED).
   * 4. Convert byte size to 4KB page count (rounded up).
   */
  size_t pow2_pages = 1;
  while (pow2_pages < max_pages) {
    pow2_pages <<= 1;
  }
  size_t tree_nodes = (2 * pow2_pages) - 1;
  size_t tree_bytes = tree_nodes * sizeof(node_state_t);
  size_t tree_pages = (tree_bytes + 4095) / PAGE_SIZE;

  if (max_pages <= tree_pages) {
    return; // Too tiny memory
  }

  void *tree_buffer = (void *)(uintptr_t)start;

  uint64_t usable_ram_start = start + (tree_pages * PAGE_SIZE);
  uint64_t usable_page_count = max_pages - tree_pages;

  size_t final_pow2_pages = 1;
  while ((final_pow2_pages << 1) <= usable_page_count) {
    final_pow2_pages <<= 1;
  }

  buddy_init(tree_buffer, final_pow2_pages, usable_ram_start);
}
