#pragma once
#include "types.h"

#define PAGE_SIZE 4096

typedef enum { LINE_HORIZONTAL, LINE_VERTICAL } line_type_t;
typedef uint8_t node_state_t;
enum { NODE_FREE = 0, NODE_SPLIT = 1, NODE_ALLOCATED = 2, NODE_FAILED = 3 };

// video
void draw_pixel(size_t x, size_t y, uint32_t color);
void fill_screen(uint32_t color);
void draw_line(size_t x, size_t y, size_t length, uint32_t color, line_type_t type_line);
void draw_rectangle(size_t x, size_t y, size_t width, size_t height, uint32_t color);

// memory
size_t kalloc(size_t req_pages);
node_state_t kfree(size_t index);
