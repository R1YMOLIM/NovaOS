#pragma once
#include "types.h"

typedef enum { LINE_HORIZONTAL, LINE_VERTICAL } line_type_t;

// video
void draw_pixel(size_t x, size_t y, uint32_t color);
void fill_screen(uint32_t color);
void draw_line(size_t x, size_t y, size_t length, uint32_t color, line_type_t type_line);
void draw_rectangle(size_t x, size_t y, size_t width, size_t height, uint32_t color);

// memory
void *buddy_alloc(size_t req_pages);
