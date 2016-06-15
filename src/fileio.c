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

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *sys) {
	EFI_LOADED_IMAGE *loaded;
	EFI_STATUS r;

        InitializeLib(img, sys);
	InitGoodies(img, sys);

	Print(L"Hello, EFI World\n");

	r = OpenProtocol(img, &LoadedImageProtocol, (void**) &loaded);
	if (r) Fatal("LoadedImageProtocol", r);

	Print(L"Img DeviceHandle='%s'\n", HandleToString(loaded->DeviceHandle));
	Print(L"Img FilePath='%s'\n", DevicePathToStr(loaded->FilePath));
	Print(L"Img Base=%lx Size=%lx\n", loaded->ImageBase, loaded->ImageSize);

	EFI_FILE_IO_INTERFACE *fioi;
	r = OpenProtocol(loaded->DeviceHandle, &SimpleFileSystemProtocol, (void **) &fioi);
	if (r) Fatal("SimpleFileSystemProtocol", r);

	EFI_FILE_HANDLE root;
	r = fioi->OpenVolume(fioi, &root);
	if (r) Fatal("OpenVolume", r);

	EFI_FILE_HANDLE file;
	r = root->Open(root, &file, L"README.txt", EFI_FILE_MODE_READ, 0);

	if (r == EFI_SUCCESS) {
		char buf[512];
		UINTN sz = sizeof(buf);
		EFI_FILE_INFO *finfo = (void*) buf;
		r = file->GetInfo(file, &FileInfoGUID, &sz, finfo);
		if (r) Fatal("GetInfo", r);
		Print(L"FileSize %ld\n", finfo->FileSize);

		sz = sizeof(buf) - 1;
		r = file->Read(file, &sz, buf);
		if (r) Fatal("Read", r);

		char *x = buf;
		while(sz-- > 0) Print(L"%c", (CHAR16) *x++);

		file->Close(file);
	}

	root->Close(root);
	CloseProtocol(loaded->DeviceHandle, &SimpleFileSystemProtocol);
	CloseProtocol(img, &LoadedImageProtocol);

	WaitAnyKey();

        return EFI_SUCCESS;
}
