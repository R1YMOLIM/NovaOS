#include "uefi/boot_services.h"
#include "uefi/entry.h"
#include "uefi/protocols/console.h"
#include "uefi/protocols/load_image.h"
#include "uefi/protocols/media.h"
#include "uefi/types.h"

static EFI_GUID EfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID EfiSimpleFileSystemProtocolGuid =
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID EfiFileInfoId = EFI_FILE_INFO_ID;
static EFI_GUID EfiGraphicsOutputProtocolGuid =
    EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

typedef struct {
  void *BaseAddress;
  UINTN BufferSize;
  UINT32 Width;
  UINT32 Height;
  UINT32 PixelsPerScanLine;
} BootVideoInfo;

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS status;
  EFI_BOOT_SERVICES *BS = SystemTable->BootServices;

  // Load Image Protocol
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  status = BS->LocateProtocol(&EfiLoadedImageProtocolGuid, NULL,
                              (void **)&LoadedImage);
  if (EFI_ERROR(status)) {
    SystemTable->ConOut->OutputString(
        SystemTable->ConOut, L"Error: cannot find which loader came from");
    return status;
  }

  // Open Filesystem Protocol
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  status = BS->LocateProtocol(&EfiSimpleFileSystemProtocolGuid, NULL,
                              (void **)&FileSystem);
  if (EFI_ERROR(status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot find filesystem");
    return status;
  }

  // Find GOP (Graphics Output Protocol)
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Graphics;
  status = BS->LocateProtocol(&EfiGraphicsOutputProtocolGuid, NULL,
                              (void **)&Graphics);

  if (EFI_ERROR(status)) {
    SystemTable->ConOut->OutputString(
        SystemTable->ConOut,
        L"Error: cannot find GOP (Graphics Output Protocol)");
    return status;
  }

  // Write data GOP
  BootVideoInfo VideoInfo;

  VideoInfo.BaseAddress = (void *)Graphics->Mode->FrameBufferBase;
  VideoInfo.BufferSize = Graphics->Mode->FrameBufferSize;

  VideoInfo.Width = Graphics->Mode->Info->HorizontalResolution;
  VideoInfo.Height = Graphics->Mode->Info->VerticalResolution;

  VideoInfo.PixelsPerScanLine = Graphics->Mode->Info->PixelsPerScanLine;

  // Open root directory on this disk
  EFI_FILE_PROTOCOL *Root;
  status = FileSystem->OpenVolume(FileSystem, &Root);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot open root directory");
    return status;
  }

  // Open kernel file
  EFI_FILE_PROTOCOL *KernelFile;
  status = Root->Open(Root, &KernelFile, L"kernel", EFI_FILE_MODE_READ, 0);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot open kernel file");
    return status;
  }

  // Get info from this file to read
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

  // Allocate kernel
  EFI_PHYSICAL_ADDRESS kernel_buffer = 0x100000; // 1 MB
  UINTN pages_count = (kernel_size / 4096) + 1;

  status = BS->AllocatePages(AllocateAddress, EfiLoaderCode, pages_count,
                             &kernel_buffer);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot allocate");
    return status;
  }

  // Read kernel file
  status = KernelFile->Read(KernelFile, &kernel_size, (void *)kernel_buffer);
  if (status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot read");
    return status;
  }

  KernelFile->Close(KernelFile);
  Root->Close(Root);

  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello boot!\r\n");
  SystemTable->BootServices->Stall(2000000);
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello NovaOS!\r\n");

  // Check were free space
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

  // Enter to OS
  if (status == EFI_SUCCESS) {
    // Rules SYS V ABI for variable kernel_entry_t
    typedef void(__attribute__((sysv_abi)) * kernel_entry_t)(BootVideoInfo *
                                                             BootVideoInfo);
    kernel_entry_t run_kernel = (kernel_entry_t)kernel_buffer;
    run_kernel(&VideoInfo);
  }

  // If on any reasons failed to load the OS
  while (TRUE)
    ;
}
