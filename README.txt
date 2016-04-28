What is This?
-------------

This project contains some experiments in software that runs on UEFI
firmware for the purpose of exploring UEFI development and bootloader
development.


External Software Included
--------------------------

Local Path:   external/gnu-efi/...
Description:  headers and tooling to build UEFI binaries with gcc, etc
Project:      https://sourceforge.net/projects/gnu-efi/
Source:       git://git.code.sf.net/p/gnu-efi/code
Version:      6605c16fc8b1fd3b2085364902d1fa73aa7fad76 (post-3.0.4)
License:      BSD-ish, see gnu-efi/README.*

Local Path:   external/edk2/...
Description:  headers for UEFI from Tianocore EDK II
Project:      http://www.tianocore.org/edk2/
Source:       https://github.com/tianocore/edk2
License:      BSD-ish, see headers

Local Path:   external/ovmf/... 
Description:  UEFI Firmware Suitable for use in Qemu
Distribution: http://www.tianocore.org/ovmf/
Version:      OVMF-X64-r15214.zip
License:      BSD-ish, see ovmf/LICENSE


External Dependencies
---------------------

qemu-system-x86_64 is needed to test in emulation
gnu parted and mtools are needed to generate the disk.img for Qemu


Useful Resources & Documentation
--------------------------------

ACPI & UEFI Specifications 
http://www.uefi.org/specifications

Intel 64 and IA-32 Architecture Manuals
http://www.intel.com/content/www/us/en/processors/architectures-software-developer-manuals.html

Tianocore UEFI Open Source Community
(Source for OVMF, EDK II Dev Environment, etc)
http://www.tianocore.org/
https://github.com/tianocore

