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
