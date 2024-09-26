
#include <uefi.h>
#include <common/bootstructs.h>
#include <common/exestructs.h>

#define KERNEL_FILENAME ".\\NEOSKRNL.SYS"

#define printerr(...) { printf(__VA_ARGS__); while (1); }
#define assert(c, ...) if (!(c)) { printerr(__VA_ARGS__); }

int main()
{
    efi_status_t nStatus;
    efi_guid_t gopGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_gop_t *pGop = NULL;
    efi_gop_mode_info_t *pGopInfo = NULL;
    uintn_t nISize = sizeof(efi_gop_mode_info_t);
    nStatus = BS->LocateProtocol(&gopGUID, NULL, (void **) &pGop);

    assert(!EFI_ERROR(nStatus) && pGop != NULL, "Unable to get GOP.");

    nStatus = pGop->QueryMode(pGop, pGop->Mode ? pGop->Mode->Mode : 0, &nISize, &pGopInfo);
    if (nStatus == EFI_NOT_STARTED || !pGop->Mode)
    {
        nStatus = pGop->SetMode(pGop, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
    }

    assert(!EFI_ERROR(nStatus), "Unable to get current video mode.");

    GopData gopData;
    gopData.pFramebuffer       = (void *) pGop->Mode->FrameBufferBase;
    gopData.nBufferSize        = pGop->Mode->FrameBufferSize;
    gopData.nWidth             = pGop->Mode->Information->HorizontalResolution;
    gopData.nHeight            = pGop->Mode->Information->VerticalResolution;
    gopData.nPixelsPerScanline = pGop->Mode->Information->PixelsPerScanLine;

    BootData bootData;
    bootData.gop = gopData;

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
    
    // FIXME: Magic number
    fseek(pFile, 128, SEEK_CUR);

    // The .text section is for code, .rdata section is for readonly data.
    for (size_t i = 0; i < pehdr.nNumberOfSections; i++)
    {
        sPE32SectionHeader hdr;
        fread((void *) &hdr, sizeof(sPE32SectionHeader), 1, pFile);
        if (!strncmp((char *) hdr.sName, ".text", 8) ||
            !strncmp((char *) hdr.sName, ".rdata", 8))
        {
            size_t nPrevPos = ftell(pFile);
            fseek(pFile, hdr.nPointerToRawData, SEEK_SET);
            size_t nAddress = hdr.nVirtualAddress + peohdr.nImageBase;
            size_t nPages   = hdr.nVirtualSize / 0x1000 + 1;
            efi_status_t s  = BS->AllocatePages(AllocateAddress, EfiLoaderData, nPages, &nAddress);
            assert(!EFI_ERROR(s), "Unable to allocate pages for the %s section.", hdr.sName);
            fread((void *) nAddress, hdr.nVirtualSize, 1, pFile);
            fseek(pFile, nPrevPos, SEEK_SET);
        }
    }

    fclose(pFile);
    exit_bs();
    void (*KernelMain)(BootData) = (__attribute__((sysv_abi)) void (*)(BootData)) (peohdr.nAddressOfEntrypoint + peohdr.nImageBase);
    KernelMain(bootData);
    while (1); // In case the kernel ever returns.
    return 0;
}
