#pragma once

typedef struct {
  void *base_address;
  unsigned long long buffer_size;
  int width;
  int height;
  int pixels_per_scan_line;
} boot_video_info_t;
