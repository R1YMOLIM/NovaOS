#include "../include/uefi/entry.h"

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  SystemTable->ConOut->OutputString(SystemTable->ConOut,
                                    L"Hello world! Hello NovaOS!");
  while (1) {
  }
  return EFI_SUCCESS;
}
