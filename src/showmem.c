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

#include <printf.h>

static const char *MemTypeName(UINT32 type, char *buf) {
	switch (type) {
	case EfiReservedMemoryType:	return "Reserved";
	case EfiLoaderCode:		return "LoaderCode";
	case EfiLoaderData:		return "LoaderData";
	case EfiBootServicesCode:	return "BootSvcsCode";
	case EfiBootServicesData:	return "BootSvcsData";
	case EfiRuntimeServicesCode:	return "RunTimeCode";
	case EfiRuntimeServicesData:	return "RunTimeData";
	case EfiConventionalMemory:	return "Conventional";
	case EfiUnusableMemory:		return "Unusable";
	case EfiACPIReclaimMemory:	return "ACPIReclaim";
	case EfiACPIMemoryNVS:		return "ACPINonVolMem";
	case EfiMemoryMappedIO:		return "MemMappedIO";
	case EfiMemoryMappedIOPortSpace: return "MemMappedPort";
	case EfiPalCode:		return "PalCode";
	default:
		sprintf(buf, "0x%08x", type);
		return buf;
	}
}

static unsigned char scratch[4096];

static void dump_memmap(EFI_SYSTEM_TABLE *systab) {
	EFI_STATUS r;
	UINTN msize, off;
	EFI_MEMORY_DESCRIPTOR *mmap;
	UINTN mkey, dsize;
	UINT32 dversion;
	char tmp[32];

	msize = sizeof(scratch);
	mmap = (EFI_MEMORY_DESCRIPTOR*) scratch;
	mkey = dsize = dversion;
	r = systab->BootServices->GetMemoryMap(&msize, mmap, &mkey, &dsize, &dversion);
	printf("r=%lx msz=%lx key=%lx dsz=%lx dvn=%x\n",
		r, msize, mkey, dsize, dversion);	
	if (r != EFI_SUCCESS) {
		return;
	}
	for (off = 0; off < msize; off += dsize) {
		mmap = (EFI_MEMORY_DESCRIPTOR*) (scratch + off);
		printf("%016lx %016lx %08lx %c %04lx %s\n",
			mmap->PhysicalStart, mmap->VirtualStart,
			mmap->NumberOfPages, 
			mmap->Attribute & EFI_MEMORY_RUNTIME ? 'R' : '-',
			mmap->Attribute & 0xFFFF,
			MemTypeName(mmap->Type, tmp));
	}
}

#include <goodies.h>

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *sys) {
        InitializeLib(img, sys);
	InitGoodies(img, sys);
	dump_memmap(sys);
	WaitAnyKey();
        return EFI_SUCCESS;
}
