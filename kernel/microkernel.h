#pragma once
#include "types.h"

// --- Enums ---
typedef enum { LINE_HORIZONTAL, LINE_VERTICAL } line_type_t;

typedef enum {
  PIXEL_RED_GREEN_BLUE_RESERVED_8BIT_PER_COLOR,
  PIXEL_BLUE_GREEN_RED_RESERVED_8BIT_PER_COLOR,
  PIXEL_BIT_MASK,
  PIXEL_BLT_ONLY,
  PIXEL_FORMAT_MAX
} efi_graphics_pixel_format_t;

// video
void draw_pixel(size_t x, size_t y, uint32_t color);
void fill_screen(uint32_t color);
void draw_line(size_t x, size_t y, size_t length, uint32_t color, line_type_t type_line);
void draw_rectangle(size_t x, size_t y, size_t width, size_t height, uint32_t color);

// memory
