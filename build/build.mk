# Copyright 2016 The Fuchsia Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ALL	:=
APPS	:=
DEPS	:=

QUIET	?= @

EFI_SECTIONS	:= .text .sdata .data .dynamic .dynsym
EFI_SECTIONS	+= .rel .rela .reloc .eh_frame
EFI_SECTIONS	:= $(patsubst %,-j %,$(EFI_SECTIONS))

DBG_SECTIONS	:= .debug_info .debug_abbrev .debug_loc .debug_aranges
DBG_SECTIONS	+= .debug_line .debug_macinfo .debug_str
DBG_SECTIONS	:= $(EFI_SECTIONS) $(patsubst %,-j %,$(DBG_SECTIONS))

ifneq ($(VERBOSE),)
$(info CFLAGS   := $(EFI_CFLAGS))
$(info LDFLAGS  := $(EFI_LDFLAGS))
$(info SECTIONS := $(EFI_SECTIONS))
endif

out/%.o: %.c
	@mkdir -p $(dir $@)
	@echo compiling: $@
	$(QUIET)$(EFI_CC) -MMD -MP -o $@ -c $(EFI_CFLAGS) $<

out/%.efi: out/%.so
	@mkdir -p $(dir $@)
	@echo building: $@
	$(QUIET)$(EFI_OBJCOPY) --target=efi-app-$(ARCH) $(EFI_SECTIONS) $< $@
	$(QUIET)if [ "`nm $< | grep ' U '`" != "" ]; then echo "error: $<: undefined symbols"; nm $< | grep ' U '; rm $<; exit 1; fi

out/%.dbg: out/%.so
	@mkdir -p $(dir $@)
	@echo building: $@
	$(QUIET)$(EFI_OBJCOPY) --target=efi-app-$(ARCH) $(DBG_SECTIONS) $< $@


# _efi_app <basename> <obj-files> <dep-files>
define _efi_app
ALL	+= out/$1.efi out/$1.dbg
APPS	+= out/$1.efi
DEPS	+= $3
out/$1.so: $2 $(EFI_CRT0) out/libstuff.a
	@mkdir -p $$(dir $$@)
	@echo linking: $$@
	$(QUIET)$(EFI_LD) -o $$@ $(EFI_LDFLAGS) $(EFI_CRT0) $2 $(EFI_LIBS)
endef

efi_app = $(eval $(call _efi_app,$(strip $1),\
$(patsubst %.c,out/src/%.o,$2),\
$(patsubst %.c,out/src/%.d,$2)))


