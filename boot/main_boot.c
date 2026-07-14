#include "uefi/boot_services.h"
#include "uefi/entry.h"
#include "uefi/protocols/load_image.h"
#include "uefi/protocols/media.h"
#include "uefi/types.h"

static EFI_GUID EfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID EfiSimpleFileSystemProtocolGuid =
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID EfiFileInfoId = EFI_FILE_INFO_ID;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  EFI_BOOT_SERVICES *BS = SystemTable->BootServices;

  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  status = BS->OpenProtocol(ImageHandle, &EfiLoadedImageProtocolGuid,
                            (void **)&LoadedImage, ImageHandle, NULL,
                            EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(
        SystemTable->ConOut, L"Error: cannot find which loader came from");
    return status;
  }

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  status =
      BS->OpenProtocol(LoadedImage->DeviceHandle,
                       &EfiSimpleFileSystemProtocolGuid, (void **)&FileSystem,
                       ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot open filesystem");
    return status;
  }

  EFI_FILE_PROTOCOL *Root;
  status = FileSystem->OpenVolume(FileSystem, &Root);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot open root directory");
    return status;
  }

  EFI_FILE_PROTOCOL *KernelFile;
  status = Root->Open(Root, &KernelFile, L"kernel", EFI_FILE_MODE_READ, 0);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot open kernel file");
    return status;
  }

  UINTN InfoBufferSize = 0;
  EFI_FILE_INFO *FileInfo = NULL;
  status =
      KernelFile->GetInfo(KernelFile, &EfiFileInfoId, &InfoBufferSize, NULL);
  if (status == EFI_BUFFER_TOO_SMALL) {
    status =
        BS->AllocatePool(EfiLoaderData, InfoBufferSize, (void **)&FileInfo);
    if (status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(
          SystemTable->ConOut, L"Error: cannot get info from this file");
      return status;
    }

    status = KernelFile->GetInfo(KernelFile, &EfiFileInfoId, &InfoBufferSize,
                                 FileInfo);
    if (status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(
          SystemTable->ConOut, L"Error: cannot get info from this file");
      return status;
    }

  } else {
    if (status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: unknown");
      return status;
    }
  }

  UINTN kernel_size = FileInfo->FileSize;
  BS->FreePool(FileInfo);

  EFI_PHYSICAL_ADDRESS kernel_buffer = 0x100000; // 1 MB
  UINTN pages_count = (kernel_size / 4096) + 1;

  status = BS->AllocatePages(AllocateAddress, EfiLoaderCode, pages_count,
                             &kernel_buffer);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot allocate");
    return status;
  }

  status = KernelFile->Read(KernelFile, &kernel_size, (void *)kernel_buffer);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot read");
    return status;
  }

  KernelFile->Close(KernelFile);
  Root->Close(Root);

  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello boota!\r\n");
  SystemTable->BootServices->Stall(6000000);
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello NovaOS!\r\n");

  UINTN map_key = 0, mem_map_size = 0, descriptor_size = 0;
  UINT32 descriptor_version = 0;
  EFI_MEMORY_DESCRIPTOR *mem_map = NULL;

  BS->GetMemoryMap(&mem_map_size, NULL, &map_key, &descriptor_size,
                   &descriptor_version);
  mem_map_size += 2 * descriptor_size;

  status = BS->AllocatePool(EfiLoaderData, mem_map_size, (void **)&mem_map);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot allocate pool");
    return status;
  }

  status = BS->GetMemoryMap(&mem_map_size, mem_map, &map_key, &descriptor_size,
                            &descriptor_version);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot get memory");
    return status;
  }

  status = BS->ExitBootServices(ImageHandle, map_key);

  if (status == EFI_SUCCESS) {
    typedef void(__attribute__((sysv_abi)) * kernel_entry_t)(void *fb_base,
                                                             UINTN fb_size);
    kernel_entry_t run_kernel = (kernel_entry_t)kernel_buffer;

    run_kernel(NULL, 0);
  }

  return status;
}
