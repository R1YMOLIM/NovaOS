#include <bootloader_info.h>

extern void init_memory(const boot_memory_info_t *memory);
extern void init_video(const boot_video_info_t *video);

int __attribute__((sysv_abi)) _start(boot_loader_info_t *boot_info) {
  if (boot_info == ((void *)0)) {
    while (1) {
      __asm__ volatile("hlt");
    }
  }

  // Init modules in kernel
  init_video(&boot_info->video_info);
  init_memory(&boot_info->memory_info);

  while (1) {
    __asm__ volatile("hlt");
  }
  return 0;
}
