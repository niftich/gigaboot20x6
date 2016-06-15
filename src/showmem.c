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
