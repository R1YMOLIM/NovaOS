#pragma once
#include "types.h"

//**************************************
// Bit Definitions for EFI_TIME.Daylight
//**************************************
#define EFI_TIME_ADJUST_DAYLIGHT 0x01
#define EFI_TIME_IN_DAYLIGHT 0x02
//***************************************
// Value Definition for EFI_TIME.TimeZone
//***************************************
#define EFI_UNSPECIFIED_TIMEZONE 0x07FF

//******************************************************
// EFI_RESET_TYPE
//******************************************************
typedef enum {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown,
  EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef struct {
  UINT16 Year;  // 1900 - 9999
  UINT8 Month;  // 1 - 12
  UINT8 Day;    // 1 - 31
  UINT8 Hour;   // 0 - 23
  UINT8 Minute; // 0 - 59
  UINT8 Second; // 0 - 59
  UINT8 Pad1;
  UINT32 Nanosecond; // 0 - 999,999,999
  INT16 TimeZone;    // —1440 to 1440 or 2047
  UINT8 Daylight;
  UINT8 Pad2;
} EFI_TIME;

typedef struct {
  UINT32 Resolution;
  UINT32 Accuracy;
  BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;

//
// Functions
//
typedef EFI_STATUS(EFIAPI *EFI_GET_TIME)(OUT EFI_TIME *Time,
                                         OUT EFI_TIME_CAPABILITIES *Capabilities OPTIONAL);
typedef VOID(EFIAPI *EFI_RESET_SYSTEM)(IN EFI_RESET_TYPE ResetType, IN EFI_STATUS ResetStatus,
                                       IN UINTN DataSize, IN VOID *ResetData OPTIONAL);

typedef struct {
  EFI_TABLE_HEADER Hdr;
  //
  // Time Services
  //
  EFI_GET_TIME GetTime;
  VOID *SetTime;
  VOID *GetWakeupTime;
  VOID *SetWakeupTime;
  //
  // Virtual Memory Services
  //
  VOID *SetVirtualAddressMap;
  VOID *ConvertPointer;
  //
  // Variable Services
  //
  VOID *GetVariable;
  VOID *GetNextVariableName;
  VOID *SetVariable;
  //
  // Miscellaneous Services
  //
  VOID *GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM ResetSystem;
  //
  // UEFI 2.0 Capsule Services
  //
  VOID *UpdateCapsule;
  VOID *QueryCapsuleCapabilities;
  //
  // Miscellaneous UEFI 2.0 Service
  //
  VOID *QueryVariableInfo;
} EFI_RUNTIME_SERVICES;
