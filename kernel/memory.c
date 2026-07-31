#include <memory.h>

static const boot_memory_info_t *g_memory;

void init_memory(const boot_memory_info_t *memory) {
  g_memory = memory;
}
