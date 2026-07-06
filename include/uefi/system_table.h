#pragma once
#include "types.h"

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_OUTPUT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, CHAR16 *String);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void *Reset;
  EFI_OUTPUT_STRING OutputString;
};

typedef struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;                    // Offset 0x00
  CHAR16 *FirmwareVendor;                  // Offset 0x18
  UINT32 FirmwareRevision;                 // Offset 0x20
  UINT32 Padding;                          // 32-bit alignment padding
  EFI_HANDLE ConsoleInHandle;              // Offset 0x28
  void *ConIn;                             // Offset 0x30
  EFI_HANDLE ConsoleOutHandle;             // Offset 0x38
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut; // Offset 0x40 (Crucial!)

} EFI_SYSTEM_TABLE;
