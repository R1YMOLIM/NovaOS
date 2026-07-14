#pragma once

typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

typedef long long INT64;
typedef int INT32;
typedef short INT16;
typedef char INT8;

#if defined(__x86_64__) || defined(_M_X64)
typedef unsigned long long UINTN;
typedef __int128 INT128;
typedef unsigned __int128 UINT128;
#elif defined(__i386__) || defined(_M_IX86)
typedef unsigned int UINTN;
#endif

typedef void VOID;
typedef _Bool BOOLEAN;

typedef VOID *EFI_HANDLE;
typedef UINT64 EFI_STATUS;
typedef VOID *EFI_EVENT;

typedef UINT16 CHAR16;

#define TRUE 1
#define FALSE 0

#define IN
#define OUT
#define OPTIONAL
#define CONST const
#define EFIAPI __attribute__((ms_abi)) // Explicit marker for safety

#define NULL ((void *)0)

// Add this header struct
typedef struct {
  UINT64 Signature;
  UINT32 Revision;
  UINT32 HeaderSize;
  UINT32 CRC32;
  UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct {
  UINT32 Data1;
  UINT16 Data2;
  UINT16 Data3;
  UINT8 Data4[8];
} EFI_GUID;

// EFI STATUS CODES
#define EFI_SUCCESS 0
#define EFI_ERROR_MASK      (1ULL << 63)
#define EFI_LOAD_ERROR         (EFI_ERROR_MASK | 1)
#define EFI_INVALID_PARAMETER  (EFI_ERROR_MASK | 2)
#define EFI_UNSUPPORTED        (EFI_ERROR_MASK | 3)
#define EFI_BAD_BUFFER_SIZE    (EFI_ERROR_MASK | 4)
#define EFI_BUFFER_TOO_SMALL   (EFI_ERROR_MASK | 5)
#define EFI_NOT_READY          (EFI_ERROR_MASK | 6)
#define EFI_DEVICE_ERROR       (EFI_ERROR_MASK | 7)
#define EFI_WRITE_PROTECTED    (EFI_ERROR_MASK | 8)
#define EFI_OUT_OF_RESOURCES   (EFI_ERROR_MASK | 9)
#define EFI_ERROR(status)      (((INTN)(status)) < 0)

