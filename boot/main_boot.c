#include "uefi/boot_services.h"
#include "uefi/entry.h"
#include "uefi/protocols/console.h"
#include "uefi/protocols/load_image.h"
#include "uefi/protocols/media.h"
#include "uefi/types.h"

static EFI_GUID EfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID EfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID EfiFileInfoId = EFI_FILE_INFO_ID;
static EFI_GUID EfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

typedef struct {
  VOID *BaseAddress;
  UINTN BufferSize;
  UINT32 Width;
  UINT32 Height;
  UINT32 PixelsPerScanLine;
  EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
} BootVideoInfo;

typedef struct {
  EFI_MEMORY_DESCRIPTOR *MemMapPrt;
  UINTN DescriptorSize;
  UINTN MemoryMapSize;
} BootMemoryInfo;

typedef struct {
  BootVideoInfo VideoInfo;
  BootMemoryInfo MemoryInfo;
} BootLoaderInfo;

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status = 0;
  EFI_BOOT_SERVICES *BS = SystemTable->BootServices;

  // Load Image Protocol
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  Status = BS->LocateProtocol(&EfiLoadedImageProtocolGuid, NULL, (void **)&LoadedImage);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot find which loader came from");
    return Status;
  }

  // Open Filesystem Protocol
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  Status = BS->LocateProtocol(&EfiSimpleFileSystemProtocolGuid, NULL, (void **)&FileSystem);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot find filesystem");
    return Status;
  }

  // Find GOP (Graphics Output Protocol)
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Graphics;
  Status = BS->LocateProtocol(&EfiGraphicsOutputProtocolGuid, NULL, (void **)&Graphics);

  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot find GOP (Graphics Output Protocol)");
    return Status;
  }

  // Write data GOP
  BootVideoInfo VideoInfo;

  VideoInfo.BaseAddress = (void *)Graphics->Mode->FrameBufferBase;
  VideoInfo.BufferSize = Graphics->Mode->FrameBufferSize;

  VideoInfo.Width = Graphics->Mode->Info->HorizontalResolution;
  VideoInfo.Height = Graphics->Mode->Info->VerticalResolution;

  VideoInfo.PixelsPerScanLine = Graphics->Mode->Info->PixelsPerScanLine;
  VideoInfo.PixelFormat = Graphics->Mode->Info->PixelFormat;

  // Write data BootInfo
  BootLoaderInfo BootInfo;
  BootInfo.VideoInfo = VideoInfo;

  // Open root directory on this disk
  EFI_FILE_PROTOCOL *Root;
  Status = FileSystem->OpenVolume(FileSystem, &Root);
  if (Status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot open root directory");
    return Status;
  }

  // Open kernel file
  EFI_FILE_PROTOCOL *KernelFile;
  Status = Root->Open(Root, &KernelFile, L"kernel", EFI_FILE_MODE_READ, 0);
  if (Status != EFI_SUCCESS) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot open kernel file");
    return Status;
  }

  // Get info from this file to read
  UINTN InfoBufferSize = 0;
  EFI_FILE_INFO *FileInfo = NULL;
  Status = KernelFile->GetInfo(KernelFile, &EfiFileInfoId, &InfoBufferSize, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Status = BS->AllocatePool(EfiLoaderData, InfoBufferSize, (void **)&FileInfo);
    if (Status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                        L"Error: cannot get info from this file");
      return Status;
    }

    Status = KernelFile->GetInfo(KernelFile, &EfiFileInfoId, &InfoBufferSize, FileInfo);
    if (Status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                        L"Error: cannot get info from this file");
      return Status;
    }

  } else {
    if (Status != EFI_SUCCESS) {
      SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: unknown");
      return Status;
    }
  }

  UINTN KernelSize = FileInfo->FileSize;
  BS->FreePool(FileInfo);

  // Allocate kernel
  EFI_PHYSICAL_ADDRESS KernelBuffer = 0;
  UINTN PagesCount = (KernelSize + 4095) / 4096;

  Status = BS->AllocatePages(AllocateAnyPages, EfiLoaderCode, PagesCount, &KernelBuffer);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot allocate");
    return Status;
  }

  // Read kernel File
  Status = KernelFile->Read(KernelFile, &KernelSize, (void *)KernelBuffer);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot read");
    return Status;
  }

  KernelFile->Close(KernelFile);
  Root->Close(Root);

  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello boot!\r\n");
  SystemTable->BootServices->Stall(2000000);
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello NovaOS!\r\n");
  SystemTable->BootServices->Stall(2000000);

  // Check were free space
  UINTN MapKey = 0, MemMapSize = 0, DescriptorSize = 0;
  UINT32 DescriptorVersion = 0;
  EFI_MEMORY_DESCRIPTOR *MemMap = NULL;

  BS->GetMemoryMap(&MemMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);
  MemMapSize += 2 * DescriptorSize;

  Status = BS->AllocatePool(EfiLoaderData, MemMapSize, (void **)&MemMap);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot allocate pool");
    return Status;
  }

  Status = BS->GetMemoryMap(&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot get memory");
    return Status;
  }

  Status = BS->ExitBootServices(ImageHandle, MapKey);

  // Enter to OS
  if (Status == EFI_SUCCESS) {
    // Rules SYS V ABI for variable KernelEntry
    typedef void(__attribute__((sysv_abi)) * KernelEntry)(BootLoaderInfo * BootInfo);
    KernelEntry RunKernel = (KernelEntry)KernelBuffer;
    RunKernel(&BootInfo);
  }

  // If on any reasons failed to load the OS
  while (TRUE)
    ;
}
