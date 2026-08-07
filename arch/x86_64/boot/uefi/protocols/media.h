#pragma once
#include "../runtime_services.h"
#include "../types.h"

//
// defines
//

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID                                                       \
  {0x0964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
#define EFI_FILE_INFO_ID                                                                           \
  {0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION 0x00010000

//******************************************************
// Open Modes
//******************************************************
#define EFI_FILE_MODE_READ 0x0000000000000001
#define EFI_FILE_MODE_WRITE 0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000
//******************************************************
// File Attributes
//******************************************************
#define EFI_FILE_READ_ONLY 0x0000000000000001
#define EFI_FILE_HIDDEN 0x0000000000000002
#define EFI_FILE_SYSTEM 0x0000000000000004
#define EFI_FILE_RESERVED 0x0000000000000008
#define EFI_FILE_DIRECTORY 0x0000000000000010
#define EFI_FILE_ARCHIVE 0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

//
// functions
//
typedef EFI_STATUS(EFIAPI *EFI_FILE_OPEN)(IN EFI_FILE_PROTOCOL *This,
                                          OUT EFI_FILE_PROTOCOL **NewHandle, IN CHAR16 *FileName,
                                          IN UINT64 OpenMode, IN UINT64 Attributes);
typedef EFI_STATUS(EFIAPI *EFI_FILE_CLOSE)(IN EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS(EFIAPI *EFI_FILE_DELETE)(IN EFI_FILE_PROTOCOL *This);
typedef EFI_STATUS(EFIAPI *EFI_FILE_READ)(IN EFI_FILE_PROTOCOL *This, IN OUT UINTN *BufferSize,
                                          OUT VOID *Buffer);
typedef EFI_STATUS(EFIAPI *EFI_FILE_WRITE)(IN EFI_FILE_PROTOCOL *This, IN OUT UINTN *BufferSize,
                                           IN VOID *Buffer);
typedef EFI_STATUS(EFIAPI *EFI_FILE_SET_POSITION)(IN EFI_FILE_PROTOCOL *This, IN UINT64 Position);
typedef EFI_STATUS(EFIAPI *EFI_FILE_GET_POSITION)(IN EFI_FILE_PROTOCOL *This, OUT UINT64 *Position);
typedef EFI_STATUS(EFIAPI *EFI_FILE_GET_INFO)(IN EFI_FILE_PROTOCOL *This,
                                              IN EFI_GUID *InformationType,
                                              IN OUT UINTN *BufferSize, OUT VOID *Buffer);
typedef EFI_STATUS(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This, OUT EFI_FILE_PROTOCOL **Root);

struct EFI_FILE_PROTOCOL {
  UINT64 Revision;
  EFI_FILE_OPEN Open;
  EFI_FILE_CLOSE Close;
  EFI_FILE_DELETE Delete;
  EFI_FILE_READ Read;
  EFI_FILE_WRITE Write;
  EFI_FILE_SET_POSITION SetPosition;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_GET_INFO GetInfo;

  VOID *SetInfo;
  VOID *Flush;

  VOID *OpenEx;
  VOID *ReadEx;
  VOID *WriteEx;
  VOID *FlushEx;
};

struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64 Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
};

typedef struct {
  UINT64 Size;
  UINT64 FileSize;
  UINT64 PhysicalSize;
  EFI_TIME CreateTime;
  EFI_TIME LastAccessTime;
  EFI_TIME ModificationTime;
  UINT64 Attribute;
  CHAR16 FileName[];
} EFI_FILE_INFO;
