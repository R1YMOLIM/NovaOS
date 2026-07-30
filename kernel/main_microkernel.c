#include <bootloader_info.h>

int __attribute__((sysv_abi)) _start(boot_loader_info_t *boot_info) {
  __asm__ volatile("hlt");

  return 0;
}
