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

