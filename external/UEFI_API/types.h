#pragma once

typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

typedef void *EFI_HANDLE;
typedef UINT64 EFI_STATUS;
typedef void *EFI_EVENT;

typedef UINT16 CHAR16;

#define EFI_SUCCESS 0

#define EFIAPI __attribute__((ms_abi)) // Explicit marker for safety

typedef struct {
  UINT64 Signature;
  UINT32 Revision;
  UINT32 HeaderSize;
  UINT32 CRC32;
  UINT32 Reserved;
} EFI_TABLE_HEADER;
