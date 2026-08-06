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

  size_t buffer = kalloc(4);
  size_t another_buffer = kalloc(20);
  size_t a_another_buffer = kalloc(40);

  if (buffer != 0 && another_buffer != 0 && a_another_buffer != 0) {
    draw_rectangle(400, 400, 50, 50, 0x000000FF);
  }

  if (kfree(buffer) == NODE_FREE && kfree(another_buffer) == NODE_FREE &&
      kfree(a_another_buffer) == NODE_FREE) {
    draw_rectangle(300, 300, 50, 50, 0x000000FF);
  }

  while (1) {
    __asm__ volatile("hlt");
  }
}
