
#include <uefi.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <common/exestructs.h>

#define KERNEL_FILENAME ".\\NEOSLDR.SYS"

#define _PRINTERR(...) { printf(__VA_ARGS__); while (1); }
#define _ASSERT(c, ...) if (!(c)) { _PRINTERR(__VA_ARGS__); }

int main()
{
    efi_status_t nStatus;

    // Get the memory map
    efi_memory_descriptor_t *pMemoryMap = NULL;
    uintn_t nMemoryMapSize = 0, nMapKey = 0, nDescSize = 0;

    nStatus = BS->GetMemoryMap(&nMemoryMapSize, NULL, &nMapKey, &nDescSize, NULL);
    _ASSERT(nStatus == EFI_BUFFER_TOO_SMALL && nMemoryMapSize != 0, "Unable to get the memory map.\nEC: 0x%02X", (uint8_t) nStatus);
    nMemoryMapSize += 4 * nDescSize;

    pMemoryMap = malloc(nMemoryMapSize);
    _ASSERT(pMemoryMap != NULL, "Unable to allocate memory.");

    nStatus = BS->GetMemoryMap(&nMemoryMapSize, pMemoryMap, &nMapKey, &nDescSize, NULL);
    _ASSERT(!EFI_ERROR(nStatus), "Unable to get the memory map.\nEC: 0x%02X", (uint8_t) nStatus);

    efi_guid_t gopGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_gop_t *pGop = NULL;
    efi_gop_mode_info_t *pGopInfo = NULL;
    uintn_t nISize = sizeof(efi_gop_mode_info_t);
    nStatus = BS->LocateProtocol(&gopGUID, NULL, (void **) &pGop);

    _ASSERT(!EFI_ERROR(nStatus) && pGop != NULL, "Unable to get the GOP.");

    nStatus = pGop->QueryMode(pGop, pGop->Mode ? pGop->Mode->Mode : 0, &nISize, &pGopInfo);
    if (nStatus == EFI_NOT_STARTED || !pGop->Mode)
    {
        nStatus = pGop->SetMode(pGop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
    }

    _ASSERT(!EFI_ERROR(nStatus), "Unable to get the current video mode.");

    sGOPData gopData;
    gopData.pFramebuffer       = (void *) pGop->Mode->FrameBufferBase;
    gopData.nBufferSize        = pGop->Mode->FrameBufferSize;
    gopData.nWidth             = pGop->Mode->Information->HorizontalResolution;
    gopData.nHeight            = pGop->Mode->Information->VerticalResolution;
    gopData.nPixelsPerScanline = pGop->Mode->Information->PixelsPerScanLine;

    sBootData bootData;
    bootData.gop = gopData;

    bootData.pMemoryDescriptor = (sEFIMemoryDescriptor *) pMemoryMap;
    bootData.nMemoryMapSize = nMemoryMapSize;
    bootData.nMemoryDescriptorSize = nDescSize;

    // File loading
    FILE *pFile = fopen(KERNEL_FILENAME, "r");
    _ASSERT(pFile != NULL, "Kernel '%s' not found.", KERNEL_FILENAME);

    fseek(pFile, 0, SEEK_END);
    size_t nFilesize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    sMZHeader mzhdr;
    fread((void *) &mzhdr, sizeof(sMZHeader), 1, pFile);
    _ASSERT(mzhdr.wMagic == 0x5A4D, "Invalid DOS stub.");

    // Skip the DOS stub
    // FIXME: Magic number
    fseek(pFile, 128, SEEK_SET);

    sPE32Header pehdr;
    fread((void *) &pehdr, sizeof(sPE32Header), 1, pFile);
    _ASSERT(pehdr.dwMagic == 0x4550, "Invalid PE32 header.");
    _ASSERT(pehdr.wMachine == 0x8664, "Kernel executable must be 64-bit.");
    _ASSERT(pehdr.wSizeOfOptionalHeader != 0, "Missing PE32 optional header.");
    
    sPE32OptionalHeader peohdr;
    fread((void *) &peohdr, sizeof(sPE32OptionalHeader), 1, pFile);
    _ASSERT(peohdr.wMagic == 0x020B, "Invalid PE32 optional header.");

    bootData.nLoaderStart = 0xFFFFFFFFFFFFFFFF;
    bootData.nLoaderEnd   = 0;

    fseek(pFile, pehdr.wSizeOfOptionalHeader - sizeof(sPE32OptionalHeader), SEEK_CUR);

    // The .text section is for code, .rdata section is for readonly data.
    for (size_t i = 0; i < pehdr.wNumberOfSections; i++)
    {
        sPE32SectionHeader hdr;
        fread((void *) &hdr, sizeof(sPE32SectionHeader), 1, pFile);

        if (!strncmp((char *) hdr.szName, ".text", 8) ||
            !strncmp((char *) hdr.szName, ".data", 8) ||
            !strncmp((char *) hdr.szName, ".rdata", 8) ||
            !strncmp((char *) hdr.szName, ".rodata", 8) ||
            !strncmp((char *) hdr.szName, ".bss", 8))
        {
            size_t nPrevPos = ftell(pFile);
            fseek(pFile, hdr.dwPointerToRawData, SEEK_SET);
            size_t nAddress = hdr.dwVirtualAddress + peohdr.qwImageBase;
            size_t nPages   = (hdr.dwVirtualSize + 0x1000 - 1) / 0x1000;

            bootData.nLoaderStart = _MIN(bootData.nLoaderStart, nAddress);
            bootData.nLoaderEnd   = _MAX(bootData.nLoaderEnd, nAddress + nPages * 0x1000);
            
            for (size_t j = 0; j < nPages; j++)
            {
                size_t nAddressOffset = nAddress + j * 0x1000;
                // FIXME: Error handling
                efi_status_t s = BS->AllocatePages(AllocateAddress, EfiLoaderData, 1, &nAddressOffset);
            }

            printf("Address: %16X, Pages: %d, Name: %s\n", nAddress, nPages, hdr.szName);
            //_ASSERT(!EFI_ERROR(s), "Unable to allocate pages for the %s section.", hdr.szName);
            fread((void *) nAddress, hdr.dwVirtualSize, 1, pFile);
            fseek(pFile, nPrevPos, SEEK_SET);
        }
    }

    fclose(pFile);
    exit_bs();

    ((void (*)(sBootData)) (peohdr.dwAddressOfEntrypoint + peohdr.qwImageBase))(bootData);

    // In case the kernel ever returns.
    __asm__ volatile("cli\nhlt");
    while (1);
    return 0;
}
