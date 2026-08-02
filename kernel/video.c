#include "microkernel.h"

#include <video.h>

static const boot_video_info_t *g_video = NULL;

typedef enum {
  PIXEL_RED_GREEN_BLUE_RESERVED_8BIT_PER_COLOR,
  PIXEL_BLUE_GREEN_RED_RESERVED_8BIT_PER_COLOR,
  PIXEL_BIT_MASK,
  PIXEL_BLT_ONLY,
  PIXEL_FORMAT_MAX
} efi_graphics_pixel_format_t;

// --- Functions ---
// It is forward declaration used for init only in main function
void init_video(const boot_video_info_t *video) {
  g_video = video;
}

void draw_pixel(size_t x, size_t y, uint32_t color) {
  if (!g_video)
    return;

  if (x < g_video->width && y < g_video->height) {
    g_video->base_address[y * g_video->pixels_per_scan_line + x] = color;
  }
}

void fill_screen(uint32_t color) {
  if (!g_video)
    return;

  size_t total_pixels = (size_t)g_video->pixels_per_scan_line * g_video->height;
  for (size_t i = 0; i < total_pixels; i++) {
    g_video->base_address[i] = color;
  }
}

void draw_line(size_t x, size_t y, size_t length, uint32_t color, line_type_t type_line) {
  if (!g_video)
    return;

  if (type_line == LINE_HORIZONTAL) {
    size_t max_x = (x + length < g_video->width) ? x + length : g_video->width;
    for (size_t cx = x; cx < max_x; cx++) {
      draw_pixel(cx, y, color);
    }
  } else if (type_line == LINE_VERTICAL) {
    size_t max_y = (y + length < g_video->height) ? y + length : g_video->height;
    for (size_t cy = y; cy < max_y; cy++) {
      draw_pixel(x, cy, color);
    }
  }
}

void draw_rectangle(size_t x, size_t y, size_t width, size_t height, uint32_t color) {
  if (!g_video)
    return;

  size_t max_x = (x + width < g_video->width) ? x + width : g_video->width;
  size_t max_y = (y + height < g_video->height) ? y + height : g_video->height;

  for (size_t cy = y; cy < max_y; cy++) {
    for (size_t cx = x; cx < max_x; cx++) {
      draw_pixel(cx, cy, color);
    }
  }
}
