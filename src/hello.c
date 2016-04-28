
#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE img, EFI_SYSTEM_TABLE *sys) {
        SIMPLE_TEXT_OUTPUT_INTERFACE *ConOut = sys->ConOut;
	ConOut->OutputString(ConOut, L"Hello, EFI World!\r\n");
        return EFI_SUCCESS;
}
