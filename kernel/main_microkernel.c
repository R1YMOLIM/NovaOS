#include "microkernel.h"

#include <bootloader_info.h>

extern void init_memory(const boot_memory_info_t *memory);
extern void init_video(const boot_video_info_t *video);

void __attribute__((sysv_abi, noreturn)) _start(boot_loader_info_t *boot_info) {
  if (boot_info == NULL) {
    while (1) {
      __asm__ volatile("hlt");
    }
  }

  // Init modules in kernel
  init_video(&boot_info->video_info);
  init_memory(&boot_info->memory_info);

  fill_screen(0xFFFFFFFF);
  draw_pixel(100, 100, 0xFF000000);
  draw_rectangle(500, 500, 50, 50, 0x00FF0000);
  draw_line(300, 300, 100, 0x0000FF00, LINE_VERTICAL);

  // Allocate three buffers and write a pattern into the first to prove
  // we received real, writable memory.
  uint32_t *buffer = (uint32_t *)kalloc(4);          // 4 pages = 16 KiB
  uint32_t *another_buffer = (uint32_t *)kalloc(20); // 20 pages
  uint32_t *a_another_buffer = (uint32_t *)kalloc(40);

  if (buffer && another_buffer && a_another_buffer) {
    buffer[0] = 0xCAFEBABE;
    buffer[1] = 0xDEADBEEF;
    draw_rectangle(400, 400, 50, 50, 0x000000FF);
  }

  kfree(buffer);
  kfree(another_buffer);
  kfree(a_another_buffer);

  draw_rectangle(300, 300, 50, 50, 0x000000FF);

  while (1) {
    __asm__ volatile("hlt");
  }
}
