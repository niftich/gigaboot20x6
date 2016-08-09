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
#include <string.h>

#include <goodies.h>
#include <netboot.h>

#define E820_IGNORE 0
#define E820_RAM 1
#define E820_RESERVED 2
#define E820_ACPI 3
#define E820_NVS 4
#define E820_UNUSABLE 5

const char* e820name[] = {
    "IGNORE",
    "RAM",
    "RESERVED",
    "ACPI",
    "NVS",
    "UNUSABLE",
};

struct e820entry {
    UINT64 addr;
    UINT64 size;
    UINT32 type;
} __attribute__((packed));

unsigned e820type(unsigned uefi_mem_type) {
    switch (uefi_mem_type) {
    case EfiReservedMemoryType:
    case EfiPalCode:
        return E820_RESERVED;
    case EfiRuntimeServicesCode:
    case EfiRuntimeServicesData:
#if WITH_RUNTIME_SERVICES
        return E820_RESERVED;
#else
        return E820_RAM;
#endif
    case EfiACPIReclaimMemory:
        return E820_ACPI;
    case EfiACPIMemoryNVS:
        return E820_NVS;
    case EfiLoaderCode:
    case EfiLoaderData:
    case EfiBootServicesCode:
    case EfiBootServicesData:
    case EfiConventionalMemory:
        return E820_RAM;
    case EfiMemoryMappedIO:
    case EfiMemoryMappedIOPortSpace:
        return E820_IGNORE;
    default:
        if (uefi_mem_type >= 0x80000000) {
            return E820_RAM;
        }
        return E820_UNUSABLE;
    }
}

static unsigned char scratch[32768];
static struct e820entry e820table[128];

int process_memory_map(EFI_SYSTEM_TABLE* sys, UINTN* _key, int silent) {
    EFI_MEMORY_DESCRIPTOR* mmap;
    struct e820entry* entry = e820table;
    UINTN msize, off;
    UINTN mkey, dsize;
    UINT32 dversion;
    unsigned n, type;
    EFI_STATUS r;

    msize = sizeof(scratch);
    mmap = (EFI_MEMORY_DESCRIPTOR*)scratch;
    mkey = dsize = dversion = 0;
    r = sys->BootServices->GetMemoryMap(&msize, mmap, &mkey, &dsize, &dversion);
    if (!silent)
        printf("r=%lx msz=%lx key=%lx dsz=%lx dvn=%x\n", r, msize, mkey, dsize, dversion);
    if (r != EFI_SUCCESS) {
        return -1;
    }
    if (msize > sizeof(scratch)) {
        if (!silent)
            printf("Memory Table Too Large (%ld entries)\n", (msize / dsize));
        return -1;
    }
    for (off = 0, n = 0; off < msize; off += dsize) {
        mmap = (EFI_MEMORY_DESCRIPTOR*)(scratch + off);
        type = e820type(mmap->Type);
        if (type == E820_IGNORE) {
            continue;
        }
        if ((n > 0) && (entry[n - 1].type == type)) {
            if ((entry[n - 1].addr + entry[n - 1].size) == mmap->PhysicalStart) {
                entry[n - 1].size += mmap->NumberOfPages * 4096UL;
                continue;
            }
        }
        entry[n].addr = mmap->PhysicalStart;
        entry[n].size = mmap->NumberOfPages * 4096UL;
        entry[n].type = type;
        n++;
        if (n == 128) {
            if (!silent)
                printf("E820 Table Too Large (%ld raw entries)\n", (msize / dsize));
            return -1;
        }
    }
    *_key = mkey;
    return n;
}

#define ZP_E820_COUNT 0x1E8   // byte
#define ZP_SETUP 0x1F1        // start of setup structure
#define ZP_SETUP_SECTS 0x1F1  // byte (setup_size/512-1)
#define ZP_JUMP 0x200         // jump instruction
#define ZP_HEADER 0x202       // word "HdrS"
#define ZP_VERSION 0x206      // half 0xHHLL
#define ZP_LOADER_TYPE 0x210  // byte
#define ZP_RAMDISK_BASE 0x218 // word (ptr or 0)
#define ZP_RAMDISK_SIZE 0x21C // word (bytes)
#define ZP_EXTRA_MAGIC 0x220  // word
#define ZP_CMDLINE 0x228      // word (ptr)
#define ZP_SYSSIZE 0x1F4      // word (size/16)
#define ZP_XLOADFLAGS 0x236   // half
#define ZP_E820_TABLE 0x2D0   // 128 entries

#define ZP_ACPI_RSD 0x080 // word phys ptr
#define ZP_FB_BASE 0x090
#define ZP_FB_WIDTH 0x094
#define ZP_FB_HEIGHT 0x098
#define ZP_FB_STRIDE 0x09C
#define ZP_FB_FORMAT 0x0A0
#define ZP_FB_REGBASE 0x0A4
#define ZP_FB_SIZE 0x0A8

#define ZP_MAGIC_VALUE 0xDBC64323

#define ZP8(p, off) (*((UINT8*)((p) + (off))))
#define ZP16(p, off) (*((UINT16*)((p) + (off))))
#define ZP32(p, off) (*((UINT32*)((p) + (off))))

typedef struct {
    UINT8* zeropage;
    UINT8* cmdline;
    void* image;
    UINT32 pages;
} kernel_t;

void install_memmap(kernel_t* k, struct e820entry* memmap, unsigned count) {
    memcpy(k->zeropage + ZP_E820_TABLE, memmap, sizeof(*memmap) * count);
    ZP8(k->zeropage, ZP_E820_COUNT) = count;
}

void start_kernel(kernel_t* k) {
    // 64bit entry is at offset 0x200
    UINT64 entry = (UINT64)(k->image + 0x200);

    // ebx = 0, ebp = 0, edi = 0, esi = zeropage
    __asm__ __volatile__(
        "movl $0, %%ebp \n"
        "cli \n"
        "jmp *%[entry] \n" ::[entry] "a"(entry),
        [zeropage] "S"(k->zeropage),
        "b"(0), "D"(0));
    for (;;)
        ;
}

int load_kernel(EFI_BOOT_SERVICES* bs, uint8_t* image, size_t sz, kernel_t* k) {
    UINT32 setup_sz;
    UINT32 image_sz;
    UINT32 setup_end;
    EFI_PHYSICAL_ADDRESS mem;

    k->zeropage = NULL;
    k->cmdline = NULL;
    k->image = NULL;
    k->pages = 0;

    if (sz < 1024) {
        // way too small to be a kernel
        goto fail;
    }

    if (ZP32(image, ZP_HEADER) != 0x53726448) {
        printf("kernel: invalid setup magic %08x\n", ZP32(image, ZP_HEADER));
        goto fail;
    }
    if (ZP16(image, ZP_VERSION) < 0x020B) {
        printf("kernel: unsupported setup version %04x\n", ZP16(image, ZP_VERSION));
        goto fail;
    }
    setup_sz = (ZP8(image, ZP_SETUP_SECTS) + 1) * 512;
    image_sz = (ZP32(image, ZP_SYSSIZE) * 16);
    setup_end = ZP_JUMP + ZP8(image, ZP_JUMP + 1);

    printf("setup %d image %d  hdr %04x-%04x\n", setup_sz, image_sz, ZP_SETUP, setup_end);
    // image size may be rounded up, thus +15
    if ((setup_sz < 1024) || ((setup_sz + image_sz) > (sz + 15))) {
        printf("kernel: invalid image size\n");
        goto fail;
    }

    mem = 0xFF000;
    if (bs->AllocatePages(AllocateMaxAddress, EfiLoaderData, 1, &mem)) {
        printf("kernel: cannot allocate 'zero page'\n");
        goto fail;
    }
    k->zeropage = (void*)mem;

    mem = 0xFF000;
    if (bs->AllocatePages(AllocateMaxAddress, EfiLoaderData, 1, &mem)) {
        printf("kernel: cannot allocate commandline\n");
        goto fail;
    }
    k->cmdline = (void*)mem;

    mem = 0x100000;
    k->pages = (image_sz + 4095) / 4096;
    if (bs->AllocatePages(AllocateAddress, EfiLoaderData, k->pages + 1, &mem)) {
        printf("kernel: cannot allocate kernel\n");
        goto fail;
    }
    k->image = (void*)mem;

    // setup zero page, copy setup header from kernel binary
    ZeroMem(k->zeropage, 4096);
    CopyMem(k->zeropage + ZP_SETUP, image + ZP_SETUP, setup_end - ZP_SETUP);

    CopyMem(k->image, image + setup_sz, image_sz);

    // empty commandline for now
    ZP32(k->zeropage, ZP_CMDLINE) = (uint64_t)k->cmdline;
    k->cmdline[0] = 0;

    // default to no ramdisk
    ZP32(k->zeropage, ZP_RAMDISK_BASE) = 0;
    ZP32(k->zeropage, ZP_RAMDISK_SIZE) = 0;

    // undefined bootloader
    ZP8(k->zeropage, ZP_LOADER_TYPE) = 0xFF;

    printf("kernel @%p, zeropage @%p, cmdline @%p\n",
           k->image, k->zeropage, k->cmdline);

    return 0;
fail:
    if (k->image) {
        bs->FreePages((EFI_PHYSICAL_ADDRESS)k->image, k->pages);
    }
    if (k->cmdline) {
        bs->FreePages((EFI_PHYSICAL_ADDRESS)k->cmdline, 1);
    }
    if (k->zeropage) {
        bs->FreePages((EFI_PHYSICAL_ADDRESS)k->zeropage, 1);
    }

    return -1;
}

static EFI_GUID GraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

void dump_graphics_modes(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop) {
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN sz;
    UINT32 num;
    for (num = 0; num < gop->Mode->MaxMode; num++) {
        if (gop->QueryMode(gop, num, &sz, &info)) {
            continue;
        }
        printf("Mode %d  %d x %d (stride %d) fmt %d\n",
               num, info->HorizontalResolution, info->VerticalResolution,
               info->PixelsPerScanLine, info->PixelFormat);
        if (info->PixelFormat == PixelBitMask) {
            printf("Mode %d R:%08x G:%08x B:%08x X:%08x\n", num,
                   info->PixelInformation.RedMask,
                   info->PixelInformation.GreenMask,
                   info->PixelInformation.BlueMask,
                   info->PixelInformation.ReservedMask);
        }
    }
}

static EFI_GUID AcpiTableGUID = ACPI_TABLE_GUID;
static EFI_GUID Acpi2TableGUID = ACPI_20_TABLE_GUID;

static UINT8 ACPI_RSD_PTR[8] = "RSD PTR ";

uint32_t find_acpi_root(EFI_HANDLE img, EFI_SYSTEM_TABLE* sys) {
    EFI_CONFIGURATION_TABLE* cfgtab = sys->ConfigurationTable;
    int i;

    for (i = 0; i < sys->NumberOfTableEntries; i++) {
        if (!CompareGuid(&cfgtab[i].VendorGuid, &AcpiTableGUID) &&
            !CompareGuid(&cfgtab[i].VendorGuid, &Acpi2TableGUID)) {
            // not an ACPI table
            continue;
        }
        if (CompareMem(cfgtab[i].VendorTable, ACPI_RSD_PTR, 8)) {
            // not the Root Description Pointer
            continue;
        }
        return (uint64_t)cfgtab[i].VendorTable;
    }
    return 0;
}

static EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;

int boot_kernel(EFI_HANDLE img, EFI_SYSTEM_TABLE* sys,
                void* image, size_t sz, void* ramdisk, size_t rsz,
                void* cmdline, size_t csz) {
    kernel_t kernel;
    EFI_STATUS r;
    UINTN key;
    int n, i;

    printf("boot_kernel() from %p (%ld bytes)\n", image, sz);
    if (ramdisk && rsz) {
        printf("ramdisk at %p (%ld bytes)\n", ramdisk, rsz);
    }

    if (load_kernel(sys->BootServices, image, sz, &kernel)) {
        printf("Failed to load kernel image\n");
        return -1;
    }

    ZP32(kernel.zeropage, ZP_EXTRA_MAGIC) = ZP_MAGIC_VALUE;
    ZP32(kernel.zeropage, ZP_ACPI_RSD) = find_acpi_root(img, sys);

    ZP32(kernel.zeropage, ZP_FB_BASE) = (UINT32)gop->Mode->FrameBufferBase;
    ZP32(kernel.zeropage, ZP_FB_WIDTH) = (UINT32)gop->Mode->Info->HorizontalResolution;
    ZP32(kernel.zeropage, ZP_FB_HEIGHT) = (UINT32)gop->Mode->Info->VerticalResolution;
    ZP32(kernel.zeropage, ZP_FB_STRIDE) = (UINT32)gop->Mode->Info->PixelsPerScanLine;
    ZP32(kernel.zeropage, ZP_FB_FORMAT) = 4; // XRGB32
    ZP32(kernel.zeropage, ZP_FB_REGBASE) = 0;
    ZP32(kernel.zeropage, ZP_FB_SIZE) = 256 * 1024 * 1024;

    if (cmdline) {
        // Truncate the cmdline to fit on a page
        if (csz >= 4095) {
            csz = 4095;
        }
        memcpy(kernel.cmdline, cmdline, csz);
        kernel.cmdline[csz] = '\0';
    }
    if (ramdisk && rsz) {
        ZP32(kernel.zeropage, ZP_RAMDISK_BASE) = (uint32_t) (uintptr_t) ramdisk;
        ZP32(kernel.zeropage, ZP_RAMDISK_SIZE) = rsz;
    }
    n = process_memory_map(sys, &key, 0);

    for (i = 0; i < n; i++) {
        struct e820entry* e = e820table + i;
        printf("%016lx %016lx %s\n", e->addr, e->size, e820name[e->type]);
    }

    r = sys->BootServices->ExitBootServices(img, key);
    if (r == EFI_INVALID_PARAMETER) {
        n = process_memory_map(sys, &key, 1);
        r = sys->BootServices->ExitBootServices(img, key);
        if (r) {
            printf("Cannot ExitBootServices! (2) %ld\n", r);
            return -1;
        }
    } else if (r) {
        printf("Cannot ExitBootServices! (1) %ld\n", r);
        return -1;
    }

    install_memmap(&kernel, e820table, n);
    start_kernel(&kernel);

    return 0;
}

#define KBUFSIZE (32*1024*1024)
#define RBUFSIZE (256*1024*1024)

static nbfile nbkernel;
static nbfile nbramdisk;
static nbfile nbcmdline;

nbfile* netboot_get_buffer(const char* name) {
    // we know these are in a buffer large enough
    // that this is safe (todo: implement strcmp)
    if (!memcmp(name, "kernel.bin", 11)) {
        return &nbkernel;
    }
    if (!memcmp(name, "ramdisk.bin", 11)) {
        return &nbramdisk;
    }
    if (!memcmp(name, "cmdline", 7)) {
        return &nbcmdline;
    }
    return NULL;
}

static char cmdline[4096];

int try_local_boot(EFI_HANDLE img, EFI_SYSTEM_TABLE* sys) {
    UINTN ksz, rsz, csz;
    void* kernel;
    void* ramdisk;
    void* cmdline;
    
    if ((kernel = LoadFile(L"magenta.bin", &ksz)) == NULL) {
        printf("Failed to load 'magenta.bin' from boot media\n\n");
        return 0;
    }
    
    ramdisk = LoadFile(L"ramdisk.bin", &rsz);
    cmdline = LoadFile(L"cmdline", &csz);

    boot_kernel(img, sys, kernel, ksz, ramdisk, rsz, cmdline, csz);
    return -1;
}

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE* sys) {
    EFI_BOOT_SERVICES* bs = sys->BootServices;
    EFI_PHYSICAL_ADDRESS mem;

    InitializeLib(img, sys);
    InitGoodies(img, sys);

    printf("\nOSBOOT v0.2\n\n");

    bs->LocateProtocol(&GraphicsOutputProtocol, NULL, (void**)&gop);
    printf("Framebuffer base is at %lx\n\n", gop->Mode->FrameBufferBase);

    if (try_local_boot(img, sys) < 0) {
        goto fail;
    }

    mem = 0xFFFFFFFF;
    if (bs->AllocatePages(AllocateMaxAddress, EfiLoaderData, KBUFSIZE / 4096, &mem)) {
        printf("Failed to allocate network io buffer\n");
        goto fail;
    }
    nbkernel.data = (void*) mem;
    nbkernel.size = KBUFSIZE;

    mem = 0xFFFFFFFF;
    if (bs->AllocatePages(AllocateMaxAddress, EfiLoaderData, RBUFSIZE / 4096, &mem)) {
        printf("Failed to allocate network io buffer\n");
        goto fail;
    }
    nbramdisk.data = (void*) mem;
    nbramdisk.size = RBUFSIZE;

    nbcmdline.data = (void*) cmdline;
    nbcmdline.size = sizeof(cmdline);
    cmdline[0] = 0;

    if (netboot_init()) {
        printf("Failed to initialize NetBoot\n");
        goto fail;
    }
    printf("\nNetBoot Server Started...\n\n");
    for (;;) {
        int n = netboot_poll();
        if (n < 1) {
            continue;
        }
        if (nbkernel.offset < 32768) {
            // too small to be a kernel
            continue;
        }
        uint8_t* x = nbkernel.data;
        if ((x[0] == 'M') && (x[1] == 'Z') && (x[0x80] == 'P') && (x[0x81] == 'E')) {
            UINTN exitdatasize;
            EFI_STATUS r;
            EFI_HANDLE h;
            printf("Attempting to run EFI binary...\n");
            r = bs->LoadImage(FALSE, img, NULL, (void*) nbkernel.data, nbkernel.offset, &h);
            if (r != EFI_SUCCESS) {
                printf("LoadImage Failed %ld\n", r);
                continue;
            }
            r = bs->StartImage(h, &exitdatasize, NULL);
            if (r != EFI_SUCCESS) {
                printf("StartImage Failed %ld\n", r);
                continue;
            }
            printf("\nNetBoot Server Resuming...\n");
            continue;
        }

        // make sure network traffic is not in flight, etc
        netboot_close();

        // maybe it's a kernel image?
        boot_kernel(img, sys, (void*) nbkernel.data, nbkernel.offset,
                    (void*) nbramdisk.data, nbramdisk.offset,
                    cmdline, sizeof(cmdline));
        goto fail;
    }

fail:
    printf("\nBoot Failure\n");
    WaitAnyKey();
    return EFI_SUCCESS;
}
