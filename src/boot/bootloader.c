
#include <uefi.h>
#include <common/bootstructs.h>
#include <common/exestructs.h>

#define KERNEL_FILENAME ".\\NEOSLDR.SYS"

#define printerr(...) { printf(__VA_ARGS__); while (1); }
#define assert(c, ...) if (!(c)) { printerr(__VA_ARGS__); }

int main()
{
    efi_status_t nStatus;

    // Get the memory map
    efi_memory_descriptor_t *pMemoryMap = NULL;
    uintn_t nMapSize = 0, nMapKey = 0, nDescSize = 0;

    nStatus = BS->GetMemoryMap(&nMapSize, NULL, &nMapKey, &nDescSize, NULL);
    assert(nStatus == EFI_BUFFER_TOO_SMALL && nMapSize != 0, "Unable to get the memory map.\nEC: 0x%02X", (uint8_t) nStatus);
    nMapSize += 4 * nDescSize;

    pMemoryMap = malloc(nMapSize);
    assert(pMemoryMap != NULL, "Unable to allocate memory.");

    nStatus = BS->GetMemoryMap(&nMapSize, pMemoryMap, &nMapKey, &nDescSize, NULL);
    
    efi_guid_t gopGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_gop_t *pGop = NULL;
    efi_gop_mode_info_t *pGopInfo = NULL;
    uintn_t nISize = sizeof(efi_gop_mode_info_t);
    nStatus = BS->LocateProtocol(&gopGUID, NULL, (void **) &pGop);

    assert(!EFI_ERROR(nStatus) && pGop != NULL, "Unable to get the GOP.");

    nStatus = pGop->QueryMode(pGop, pGop->Mode ? pGop->Mode->Mode : 0, &nISize, &pGopInfo);
    if (nStatus == EFI_NOT_STARTED || !pGop->Mode)
    {
        nStatus = pGop->SetMode(pGop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
    }

    assert(!EFI_ERROR(nStatus), "Unable to get the current video mode.");

    sGopData gopData;
    gopData.pFramebuffer       = (void *) pGop->Mode->FrameBufferBase;
    gopData.nBufferSize        = pGop->Mode->FrameBufferSize;
    gopData.nWidth             = pGop->Mode->Information->HorizontalResolution;
    gopData.nHeight            = pGop->Mode->Information->VerticalResolution;
    gopData.nPixelsPerScanline = pGop->Mode->Information->PixelsPerScanLine;

    sBootData bootData;
    bootData.gop = gopData;

    bootData.memoryDescriptor.nType          = pMemoryMap->Type;
    bootData.memoryDescriptor.nPad           = pMemoryMap->Pad;
    bootData.memoryDescriptor.nPhysicalStart = pMemoryMap->PhysicalStart;
    bootData.memoryDescriptor.nVirtualStart  = pMemoryMap->VirtualStart;
    bootData.memoryDescriptor.nNumberOfPages = pMemoryMap->NumberOfPages;
    bootData.memoryDescriptor.nAttribute     = pMemoryMap->Attribute;

    // File loading
    FILE *pFile = fopen(KERNEL_FILENAME, "r");
    assert(pFile != NULL, "Kernel '%s' not found.", KERNEL_FILENAME);
    
    sMZHeader mzhdr;
    fread((void *) &mzhdr, sizeof(sMZHeader), 1, pFile);
    assert(mzhdr.nMagic == 0x5A4D, "Invalid DOS stub.");

    // Skip the DOS stub
    // FIXME: Magic number
    fseek(pFile, 128, SEEK_SET);

    sPE32Header pehdr;
    fread((void *) &pehdr, sizeof(sPE32Header), 1, pFile);
    assert(pehdr.nMagic == 0x4550, "Invalid PE32 header.");
    assert(pehdr.nMachine == 0x8664, "Kernel executable must be 64-bit.");
    assert(pehdr.nSizeOfOptionalHeader != 0, "Missing PE32 optional header.");
    
    sPE32OptionalHeader peohdr;
    fread((void *) &peohdr, sizeof(sPE32OptionalHeader), 1, pFile);
    assert(peohdr.nMagic == 0x020B, "Invalid PE32 optional header.");

    fseek(pFile, pehdr.nSizeOfOptionalHeader - sizeof(sPE32OptionalHeader), SEEK_CUR);

    // The .text section is for code, .rdata section is for readonly data.
    for (size_t i = 0; i < pehdr.nNumberOfSections; i++)
    {
        sPE32SectionHeader hdr;
        fread((void *) &hdr, sizeof(sPE32SectionHeader), 1, pFile);

        if (!strncmp((char *) hdr.sName, ".text", 8) ||
            !strncmp((char *) hdr.sName, ".data", 8) ||
            !strncmp((char *) hdr.sName, ".rdata", 8) ||
            !strncmp((char *) hdr.sName, ".rodata", 8) ||
            !strncmp((char *) hdr.sName, ".bss", 8))
        {
            size_t nPrevPos = ftell(pFile);
            fseek(pFile, hdr.nPointerToRawData, SEEK_SET);
            size_t nAddress = hdr.nVirtualAddress + peohdr.nImageBase;
            size_t nPages   = (hdr.nVirtualSize + 0x1000 - 1) / 0x1000;
            for (size_t j = 0; j < nPages; j++)
            {
                size_t nAddressOffset = nAddress + j * 0x1000;
                // FIXME: Error handling
                efi_status_t s = BS->AllocatePages(AllocateAddress, EfiLoaderData, 1, &nAddressOffset);
            }

            printf("Address: %16X, Pages: %d, Name: %s\n", nAddress, nPages, hdr.sName);
            //assert(!EFI_ERROR(s), "Unable to allocate pages for the %s section.", hdr.sName);
            fread((void *) nAddress, hdr.nVirtualSize, 1, pFile);
            fseek(pFile, nPrevPos, SEEK_SET);
        }
    }

    fclose(pFile);
    exit_bs();

    void (*KernelMain)(sBootData) = (void (*)(sBootData)) (peohdr.nAddressOfEntrypoint + peohdr.nImageBase);
    KernelMain(bootData);
    while (1); // In case the kernel ever returns.
    return 0;
}
