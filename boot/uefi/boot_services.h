#pragma once
#include "types.h"

typedef struct EFI_MEMORY_DESCRIPTOR EFI_MEMORY_DESCRIPTOR;

typedef struct EFI_BOOT_SERVICES {

  EFI_STATUS (*GetMemoryMap)(UINT64 *MemoryMapSize,
                             EFI_MEMORY_DESCRIPTOR *MemoryMap, UINT64 *MapKey,
                             UINT64 *DescriptorSize, UINT32 *DescriptorVersion);

  EFI_STATUS (*AllocatePages)(int Type, UINT64 MemoryType, UINT64 Pages,
                              UINT64 *Memory);

  EFI_STATUS (*FreePages)(UINT64 Memory, UINT64 Pages);

  EFI_STATUS (*ExitBootServices)(EFI_HANDLE ImageHandle, UINT64 MapKey);

} EFI_BOOT_SERVICES;
