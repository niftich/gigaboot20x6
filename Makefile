
ARCH		:= x86_64

EFI_TOOLCHAIN	:=
EFI_CC		:= $(EFI_TOOLCHAIN)gcc
EFI_LD		:= $(EFI_TOOLCHAIN)ld
EFI_OBJCOPY	:= $(EFI_TOOLCHAIN)objcopy
EFI_AR		:= $(EFI_TOOLCHAIN)ar

EFI_PATH	:= external/gnu-efi
EFI_LIB_PATHS	:= $(EFI_PATH)/$(ARCH)/lib $(EFI_PATH)/$(ARCH)/gnuefi out
EFI_INC_PATHS	:= $(EFI_PATH)/inc $(EFI_PATH)/inc/$(ARCH) $(EFI_PATH)/inc/protocol

EFI_CRT0	:= $(EFI_PATH)/$(ARCH)/gnuefi/crt0-efi-$(ARCH).o
EFI_LINKSCRIPT	:= $(EFI_PATH)/gnuefi/elf_$(ARCH)_efi.lds

EFI_CFLAGS	:= -fpic -fshort-wchar -fno-stack-protector -mno-red-zone
EFI_CFLAGS	+= -Wall
EFI_CFLAGS	+= -std=c99
EFI_CFLAGS	+= -ffreestanding -nostdinc -Iinclude -Isrc -Iexternal/edk2
EFI_CFLAGS	+= $(patsubst %,-I%,$(EFI_INC_PATHS))
EFI_CFLAGS	+= -DHAVE_USE_MS_ABI=1
EFI_CFLAGS	+= -ggdb

EFI_LDFLAGS	:= -nostdlib -znocombreloc -T $(EFI_LINKSCRIPT)
EFI_LDFLAGS	+= -shared -Bsymbolic
EFI_LDFLAGS	+= $(patsubst %,-L%,$(EFI_LIB_PATHS))

EFI_LIBS	:= -lstuff -lefi -lgnuefi

what_to_build::	all

# build rules and macros
include build/build.mk

# declare applications here
$(call efi_app, hello, hello.c)
$(call efi_app, showmem, showmem.c)
$(call efi_app, fileio, fileio.c)
$(call efi_app, osboot, osboot.c netboot.c netifc.c inet6.c)
$(call efi_app, usbtest, usbtest.c)

LIB_SRCS := lib/goodies.c lib/loadfile.c lib/console-printf.c lib/printf.c lib/string.c

LIB_OBJS := $(patsubst %.c,out/%.o,$(LIB_SRCS))
DEPS += $(patsubst %.c,out/%.d,$(LIB_SRCS))

out/libstuff.a: $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@echo archiving: $@
	$(QUIET)rm -f $@
	$(QUIET)ar rc $@ $^

# generate a small IDE disk image for qemu
out/disk.img: $(APPS)
	@mkdir -p $(dir $@)
	$(QUIET)./build/mkdiskimg.sh $@
	@echo copying: $(APPS) README.txt to disk.img
	$(QUIET)mcopy -o -i out/disk.img@@1024K $(APPS) README.txt ::

ALL += out/disk.img

-include $(DEPS)

# ensure gnu-efi gets built
$(EFI_CRT0):
	@echo building: gnu-efi
	$(QUIET)$(MAKE) -C $(EFI_PATH)

QEMU_OPTS := -cpu qemu64
QEMU_OPTS += -bios external/ovmf/OVMF.fd
QEMU_OPTS += -drive file=out/disk.img,format=raw,if=ide

qemu:: all
	qemu-system-x86_64 $(QEMU_OPTS)

out/nbserver: src/nbserver.c
	@mkdir -p out
	@echo building nbserver
	$(QUIET)gcc -o out/nbserver -Isrc -Wall src/nbserver.c

all: $(ALL) out/nbserver

clean::
	rm -rf out
