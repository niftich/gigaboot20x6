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
