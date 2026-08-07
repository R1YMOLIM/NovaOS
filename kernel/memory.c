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

// --- Internal helpers ----------------------------------------------------

// Walk up from `index` to the root, recomputing each ancestor's state.
// FREE         -> both children FREE
// ALLOCATED    -> both children ALLOCATED
// SPLIT        -> mixed
static void update_parents(size_t index) {
  size_t curr = index;
  while (curr > 0) {
    size_t parent = (curr - 1) / 2;
    size_t left = (2 * parent) + 1;
    size_t right = (2 * parent) + 2;

    node_state_t l = g_buddy.tree[left];
    node_state_t r = g_buddy.tree[right];

    if (l == NODE_FREE && r == NODE_FREE) {
      g_buddy.tree[parent] = NODE_FREE;
    } else if (l == NODE_ALLOCATED && r == NODE_ALLOCATED) {
      g_buddy.tree[parent] = NODE_ALLOCATED;
    } else {
      g_buddy.tree[parent] = NODE_SPLIT;
    }

    curr = parent;
  }
}

// Translate a tree node index into the physical address of its block.
static void *index_to_addr(size_t index) {
  // Find the level (depth) of `index` in the tree.
  size_t level_start = 0;
  size_t nodes_on_level = 1;

  while (level_start + nodes_on_level <= index) {
    level_start += nodes_on_level;
    nodes_on_level <<= 1;
  }

  size_t local_offset = index - level_start;
  size_t pages_per_block = g_buddy.total_pages / nodes_on_level;
  uintptr_t byte_offset = (uintptr_t)local_offset * pages_per_block * PAGE_SIZE;

  return (void *)(g_buddy.phys_start + byte_offset);
}

// Translate a physical address back into the tree node index.
static size_t addr_to_index(void *phys_addr) {
  uintptr_t offset = (uintptr_t)phys_addr - g_buddy.phys_start;

  size_t index = 0;
  uintptr_t current_size = (uintptr_t)g_buddy.total_pages * PAGE_SIZE;

  while (index < g_buddy.tree_size) {
    node_state_t state = g_buddy.tree[index];

    if (state == NODE_ALLOCATED) {
      return index;
    }
    if (state == NODE_FREE) {
      return (size_t)-1; // Should not happen for a valid address.
    }

    // NODE_SPLIT — descend.
    current_size /= 2;

    if (offset < current_size) {
      index = (2 * index) + 1; // left
    } else {
      index = (2 * index) + 2; // right
      offset -= current_size;
    }
  }

  return (size_t)-1;
}

static void buddy_init(void *memory_buffer, size_t page_count, uintptr_t phys_start) {
  g_buddy.total_pages = page_count;
  g_buddy.tree_size = (2 * page_count) - 1;
  g_buddy.tree = (node_state_t *)memory_buffer;
  g_buddy.phys_start = phys_start;

  for (size_t i = 0; i < g_buddy.tree_size; i++) {
    g_buddy.tree[i] = NODE_FREE;
  }
}

// --- Public API ----------------------------------------------------------
void *kalloc(size_t req_pages) {
  if (req_pages == 0 || req_pages > g_buddy.total_pages) {
    return NULL;
  }

  // Round up to the next power of two.
  size_t target_pages = 1;
  while (target_pages < req_pages) {
    target_pages <<= 1;
  }

  size_t current_i = 0;
  size_t current_pages = g_buddy.total_pages;

  // If even the root is allocated, no memory is free.
  if (g_buddy.tree[current_i] == NODE_ALLOCATED) {
    return NULL;
  }

  // Descend until we reach a block of the requested size.
  while (current_pages > target_pages) {
    size_t left = (2 * current_i) + 1;
    size_t right = (2 * current_i) + 2;
    size_t child_size = current_pages / 2;

    // At the target size, the child must be FREE to take it.
    // Above the target size, the child must not be fully ALLOCATED —
    // FREE or SPLIT both mean there's room somewhere below.
    int left_ok = (child_size == target_pages) ? (g_buddy.tree[left] == NODE_FREE)
                                               : (g_buddy.tree[left] != NODE_ALLOCATED);

    int right_ok = (child_size == target_pages) ? (g_buddy.tree[right] == NODE_FREE)
                                                : (g_buddy.tree[right] != NODE_ALLOCATED);

    if (left_ok) {
      // Mark the parent as split (we are carving into its left half).
      current_i = left;
    } else if (right_ok) {
      current_i = right;
    } else {
      return NULL;
    }

    current_pages /= 2;
  }

  // The leaf must be free.
  if (g_buddy.tree[current_i] != NODE_FREE) {
    return NULL;
  }

  g_buddy.tree[current_i] = NODE_ALLOCATED;
  update_parents(current_i);

  return index_to_addr(current_i);
}

void kfree(void *ptr) {
  if (ptr == NULL) {
    return;
  }

  size_t index = addr_to_index(ptr);
  if (index == (size_t)-1 || index >= g_buddy.tree_size) {
    return;
  }
  if (g_buddy.tree[index] != NODE_ALLOCATED) {
    return;
  }

  g_buddy.tree[index] = NODE_FREE;

  // Coalesce: while the buddy is also FREE, merge up.
  while (index > 0) {
    size_t buddy = (index % 2 != 0) ? (index + 1) : (index - 1);
    size_t parent = (index - 1) / 2;

    if (g_buddy.tree[buddy] != NODE_FREE) {
      break;
    }

    g_buddy.tree[parent] = NODE_FREE;
    index = parent;
  }

  // Refresh ancestor states
  update_parents(index);
}

void init_memory(const boot_memory_info_t *memory) {
  g_memory = memory;
  if (!memory || !memory->mem_map_ptr || memory->descriptor_size == 0) {
    return;
  }

  const uint8_t *desc_ptr = (const uint8_t *)memory->mem_map_ptr;
  size_t desc_count = memory->memory_map_size / memory->descriptor_size;

  uintptr_t start = 0;
  uint64_t max_pages = 0;

  // Find the largest conventional (type 7) region.
  for (size_t i = 0; i < desc_count; i++) {
    const efi_memory_descriptor_t *desc =
      (const efi_memory_descriptor_t *)(desc_ptr + (i * memory->descriptor_size));

    if (desc->memory_type == 7) {
      if (desc->number_of_pages > max_pages) {
        max_pages = desc->number_of_pages;
        start = desc->physical_start;
      }
    }
  }

  if (max_pages == 0) {
    return;
  }

  // Round the (power-of-two) tree size up; the allocator needs a full
  // binary tree of depth log2(rounded).
  size_t pow2_pages = 1;
  while (pow2_pages < max_pages) {
    pow2_pages <<= 1;
  }
  size_t tree_nodes = (2 * pow2_pages) - 1;
  size_t tree_bytes = tree_nodes * sizeof(node_state_t);
  size_t tree_pages = (tree_bytes + 4095) / PAGE_SIZE;

  if (max_pages <= tree_pages) {
    return; // Region too small to host both tree and any usable pages.
  }

  // Tree metadata lives at the very start of the region; managed memory
  // starts after it.
  void *tree_buffer = (void *)start;
  uintptr_t usable_ram_start = start + (uintptr_t)tree_pages * PAGE_SIZE;
  uint64_t usable_page_count = max_pages - tree_pages;

  // Round down to the largest power of two that fits.
  size_t final_pow2_pages = 1;
  while ((final_pow2_pages << 1) <= usable_page_count) {
    final_pow2_pages <<= 1;
  }

  if (final_pow2_pages == 0) {
    return;
  }

  buddy_init(tree_buffer, final_pow2_pages, usable_ram_start);
}
