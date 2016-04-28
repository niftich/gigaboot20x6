// Copyright 2016 Google Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

void InitGoodies(EFI_HANDLE img, EFI_SYSTEM_TABLE *sys);

void WaitAnyKey(void);
void Fatal(const char *msg, EFI_STATUS status);
CHAR16 *HandleToString(EFI_HANDLE handle);

// Convenience wrappers for Open/Close protocol for use by
// UEFI app code that's not a driver model participant
EFI_STATUS OpenProtocol(EFI_HANDLE h, EFI_GUID *guid, void **ifc);
EFI_STATUS CloseProtocol(EFI_HANDLE h, EFI_GUID *guid);

void *LoadFile(CHAR16 *filename, UINTN *size_out);

// GUIDs
extern EFI_GUID SimpleFileSystemProtocol;
extern EFI_GUID FileInfoGUID;

// Global Context
extern EFI_HANDLE gImg;
extern EFI_SYSTEM_TABLE *gSys;
extern EFI_BOOT_SERVICES *gBS;
extern SIMPLE_TEXT_OUTPUT_INTERFACE *gConOut;
