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
#include <stdio.h>
#include <goodies.h>

void *LoadFile(CHAR16 *filename, UINTN *_sz) {
	EFI_LOADED_IMAGE *loaded;
	EFI_STATUS r;
	void *data = NULL;

	r = OpenProtocol(gImg, &LoadedImageProtocol, (void**) &loaded);
	if (r) {
		printf("LoadFile: Cannot open LoadedImageProtocol (%ld)\n", r);
		goto exit0;
	}

#if 0
	printf("Img DeviceHandle='%s'\n", HandleToString(loaded->DeviceHandle));
	printf("Img FilePath='%s'\n", DevicePathToStr(loaded->FilePath));
	printf("Img Base=%lx Size=%lx\n", loaded->ImageBase, loaded->ImageSize);
#endif

	EFI_FILE_IO_INTERFACE *fioi;
	r = OpenProtocol(loaded->DeviceHandle, &SimpleFileSystemProtocol, (void **) &fioi);
	if (r) {
		printf("LoadFile: Cannot open SimpleFileSystemProtocol (%ld)\n", r);
		goto exit1;
	}

	EFI_FILE_HANDLE root;
	r = fioi->OpenVolume(fioi, &root);
	if (r) {
		printf("LoadFile: Cannot open root volume (%ld)\n", r);
		goto exit2;
	}

	EFI_FILE_HANDLE file;
	r = root->Open(root, &file, filename, EFI_FILE_MODE_READ, 0);
	if (r) {
		printf("LoadFile: Cannot open file (%ld)\n", r);
		goto exit3;
	}

	char buf[512];
	UINTN sz = sizeof(buf);
	EFI_FILE_INFO *finfo = (void*) buf;
	r = file->GetInfo(file, &FileInfoGUID, &sz, finfo);
	if (r) {
		printf("LoadFile: Cannot get FileInfo (%ld)\n", r);
		goto exit3;
	}

	r = gBS->AllocatePool(EfiLoaderData, finfo->FileSize, (void**) &data);
	if (r) {
		printf("LoadFile: Cannot allocate buffer (%ld)\n", r);
		data = NULL;
		goto exit4;
	}

	sz = finfo->FileSize;
	r = file->Read(file, &sz, data);
	if (r) {
		printf("LoadFile: Error reading file (%ld)\n", r);
		gBS->FreePool(data);
		data = NULL;
		goto exit4;
	}
	if (sz != finfo->FileSize) {
		printf("LoadFile: Short read\n");
		gBS->FreePool(data);
		data = NULL;
		goto exit4;
	}
	*_sz = finfo->FileSize;
exit4:
	file->Close(file);
exit3:
	root->Close(root);
exit2:
	CloseProtocol(loaded->DeviceHandle, &SimpleFileSystemProtocol);
exit1:
	CloseProtocol(gImg, &LoadedImageProtocol);
exit0:
	return data;
}

