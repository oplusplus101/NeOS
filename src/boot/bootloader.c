/*
NeOS: A simple 64-bit operating system
Copyright (C) 2024 Joel Marti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <uefi.h>
#include <common/bootheaders.h>

#define KERNEL_FILENAME "NEOSKRNL.SYS"

int main()
{
    efi_status_t nStatus;
    efi_guid_t gopGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_gop_t *pGop = NULL;
    efi_gop_mode_info_t *pGopInfo = NULL;
    uintn_t nISize = sizeof(efi_gop_mode_info_t);
    nStatus = BS->LocateProtocol(&gopGUID, NULL, (void **) &pGop);

    if (EFI_ERROR(nStatus) && pGop == NULL)
    {
        fprintf(stderr, "Unable to get GOP.\n");
        while (1);
    }

    nStatus = pGop->QueryMode(pGop, pGop->Mode ? pGop->Mode->Mode : 0, &nISize, &pGopInfo);
    if (nStatus == EFI_NOT_STARTED || !pGop->Mode)
    {
        nStatus = pGop->SetMode(pGop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
    }

    if (EFI_ERROR(nStatus))
    {
        fprintf(stderr, "Unable to get current video mode.\n");
        while (1);
    }

    GopData gopData;
    gopData.pFramebuffer       = (void *) pGop->Mode->FrameBufferBase;
    gopData.nBufferSize        = pGop->Mode->FrameBufferSize;
    gopData.nWidth             = pGop->Mode->Information->HorizontalResolution;
    gopData.nHeight            = pGop->Mode->Information->VerticalResolution;
    gopData.nPixelsPerScanline = pGop->Mode->Information->PixelsPerScanLine;


    return 0;
}
