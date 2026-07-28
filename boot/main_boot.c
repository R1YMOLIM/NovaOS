#include "uefi/boot_services.h"
#include "uefi/entry.h"
#include "uefi/protocols/console.h"
#include "uefi/protocols/load_image.h"
#include "uefi/protocols/media.h"
#include "uefi/types.h"

#define COM1 0x3F8

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

// UART
static inline VOID outb(UINT16 port, UINT8 value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
  UINT8 ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

VOID uart_init() {
  outb(COM1 + 1, 0x00); // turn off interupts
  outb(COM1 + 3, 0x80); // turn on DLAB
  outb(COM1 + 0, 0x03); // baud divisor low
  outb(COM1 + 1, 0x00); // baud divisor high
  outb(COM1 + 3, 0x03); // 8 bits no parity one stop bit
  outb(COM1 + 2, 0xC7); // FIFO enable
  outb(COM1 + 4, 0x0B); // RTS/DSR
}

int uart_ready() {
  return inb(COM1 + 5) & 0x20;
}

VOID uart_putchar(char c) {
  while (!uart_ready())
    ;

  outb(COM1, c);
}

VOID uart_print(const char *str) {
  while (*str) {
    uart_putchar(*str++);
  }
}

VOID uart_print_hex(UINT64 value) {
  const char *hex = "0123456789ABCDEF";

  uart_print("0x");

  for (int i = 15; i >= 0; i--) {
    uart_putchar(hex[(value >> (i * 4)) & 0x0F]);
  }
}

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status = 0;
  EFI_BOOT_SERVICES *BS = SystemTable->BootServices;
  uart_init();

  // Load Image Protocol
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  Status = BS->LocateProtocol(&EfiLoadedImageProtocolGuid, NULL, (VOID **)&LoadedImage);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot find which loader came from");
    return Status;
  }

  // Open Filesystem Protocol
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
  Status = BS->LocateProtocol(&EfiSimpleFileSystemProtocolGuid, NULL, (VOID **)&FileSystem);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot find filesystem");
    return Status;
  }

  // Find GOP (Graphics Output Protocol)
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Graphics;
  Status = BS->LocateProtocol(&EfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Graphics);

  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                      L"Error: cannot find GOP (Graphics Output Protocol)");
    return Status;
  }

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
    Status = BS->AllocatePool(EfiLoaderData, InfoBufferSize, (VOID **)&FileInfo);
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

  uart_print("KernelBuffer: ");
  uart_print_hex(KernelBuffer);

  // Read kernel File
  Status = KernelFile->Read(KernelFile, &KernelSize, (VOID *)KernelBuffer);
  if (EFI_ERROR(Status)) {
    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Error: cannot read");
    return Status;
  }

  KernelFile->Close(KernelFile);
  Root->Close(Root);

  uart_print("KernelSize: ");
  uart_print_hex(KernelSize);

  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello boot!\r\n");
  SystemTable->BootServices->Stall(2000000);
  SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello NovaOS!\r\n");
  SystemTable->BootServices->Stall(2000000);

  UINTN MapKey = 0, MemMapSize = 0, DescriptorSize = 0;
  UINT32 DescriptorVersion = 0;
  EFI_MEMORY_DESCRIPTOR *MemMap = NULL;

  BS->GetMemoryMap(&MemMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

  MemMapSize += 2 * DescriptorSize;

  Status = BS->AllocatePool(EfiLoaderData, MemMapSize, (VOID **)&MemMap);
  if (EFI_ERROR(Status)) {
    uart_print("Error: cannot allocate pool\n");
    return Status;
  }

  // Write data GOP
  BootVideoInfo VideoInfo;

  VideoInfo.BaseAddress = (VOID *)Graphics->Mode->FrameBufferBase;
  VideoInfo.BufferSize = Graphics->Mode->FrameBufferSize;

  VideoInfo.Width = Graphics->Mode->Info->HorizontalResolution;
  VideoInfo.Height = Graphics->Mode->Info->VerticalResolution;

  VideoInfo.PixelsPerScanLine = Graphics->Mode->Info->PixelsPerScanLine;

  // Write data MemoryInfo
  BootMemoryInfo MemoryInfo;
  MemoryInfo.DescriptorSize = DescriptorSize;
  MemoryInfo.MemoryMapSize = MemMapSize;
  MemoryInfo.MemMapPrt = MemMap;

  // Write data BootInfo
  BootLoaderInfo BootInfo;
  BootInfo.VideoInfo = VideoInfo;
  BootInfo.MemoryInfo = MemoryInfo;

  while (1) {
    Status = BS->GetMemoryMap(&MemMapSize, MemMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    if (EFI_ERROR(Status)) {
      break;
    }

    Status = BS->ExitBootServices(ImageHandle, MapKey);

    if (Status == EFI_SUCCESS) {
      break;
    }
  }

  if (Status == EFI_SUCCESS) {
    typedef VOID(__attribute__((sysv_abi)) * KernelEntry)(BootLoaderInfo * BootInfo);
    KernelEntry RunKernel = (KernelEntry)KernelBuffer;

    uart_print("Entry: ");
    uart_print_hex((UINT64)RunKernel);
    uart_print("\nCongratulations! You enter to OS!\n");

    RunKernel(&BootInfo);
  } else {
    uart_print("Failed to exit Boot Services!\n");
  }

  // If on any reasons failed to load the OS
  while (TRUE)
    ;
}
