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

#include <efi.h>
#include <efilib.h>

#include <goodies.h>
#include <printf.h>

// Useful GUID Constants Not Defined by -lefi
EFI_GUID SimpleFileSystemProtocol = SIMPLE_FILE_SYSTEM_PROTOCOL;
EFI_GUID FileInfoGUID = EFI_FILE_INFO_ID;

// -lefi has its own globals, but this may end up not
// depending on that, so let's not depend on those
EFI_SYSTEM_TABLE *gSys;
EFI_HANDLE gImg;
EFI_BOOT_SERVICES *gBS;
SIMPLE_TEXT_OUTPUT_INTERFACE *gConOut;

void InitGoodies(EFI_HANDLE img, EFI_SYSTEM_TABLE *sys) {
	gSys = sys;
	gImg = img;
	gBS = sys->BootServices;
	gConOut = sys->ConOut;
}

void WaitAnyKey(void) {
	SIMPLE_INPUT_INTERFACE *sii = gSys->ConIn;
	EFI_INPUT_KEY key;
	while (sii->ReadKeyStroke(sii, &key) != EFI_SUCCESS) ;
}

void Fatal(const char *msg, EFI_STATUS status) {
	printf("\nERROR: %s (%ld)\n", msg, status);
	WaitAnyKey();
	gBS->Exit(gImg, 1, 0, NULL);
}

CHAR16 *HandleToString(EFI_HANDLE h) {
	EFI_DEVICE_PATH *path = DevicePathFromHandle(h);
	if (path == NULL) return L"<NoPath>";
	CHAR16 *str = DevicePathToStr(path);
	if (str == NULL) return L"<NoString>";
	return str;
}


EFI_STATUS OpenProtocol(EFI_HANDLE h, EFI_GUID *guid, void **ifc) {
	return gBS->OpenProtocol(h, guid, ifc, gImg, NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
}

EFI_STATUS CloseProtocol(EFI_HANDLE h, EFI_GUID *guid) {
	return gBS->CloseProtocol(h, guid, gImg, NULL);
}
