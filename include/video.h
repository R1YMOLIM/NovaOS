#pragma once

typedef struct {
  unsigned int *base_address;
  unsigned long long buffer_size;
  unsigned int width;
  unsigned int height;
  unsigned int pixels_per_scan_line;
  unsigned int pixel_format;
} boot_video_info_t;
