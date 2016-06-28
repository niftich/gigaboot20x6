// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

void InitGoodies(EFI_HANDLE img, EFI_SYSTEM_TABLE* sys);

void WaitAnyKey(void);
void Fatal(const char* msg, EFI_STATUS status);
CHAR16* HandleToString(EFI_HANDLE handle);

// Convenience wrappers for Open/Close protocol for use by
// UEFI app code that's not a driver model participant
EFI_STATUS OpenProtocol(EFI_HANDLE h, EFI_GUID* guid, void** ifc);
EFI_STATUS CloseProtocol(EFI_HANDLE h, EFI_GUID* guid);

void* LoadFile(CHAR16* filename, UINTN* size_out);

// GUIDs
extern EFI_GUID SimpleFileSystemProtocol;
extern EFI_GUID FileInfoGUID;

// Global Context
extern EFI_HANDLE gImg;
extern EFI_SYSTEM_TABLE* gSys;
extern EFI_BOOT_SERVICES* gBS;
extern SIMPLE_TEXT_OUTPUT_INTERFACE* gConOut;
