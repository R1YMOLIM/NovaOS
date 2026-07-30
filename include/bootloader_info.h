#include "memory.h"
#include "video.h"

typedef struct {
  boot_video_info_t video_info;
  boot_memory_info_t memory_info;
} boot_loader_info_t;
