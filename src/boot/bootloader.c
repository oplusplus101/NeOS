
#include <uefi.h>
#include <common/bootheaders.h>
#include <common/exeheaders.h>

#define KERNEL_FILENAME ".\\NEOSKRNL.SYS"

#define printerr(...) printf(__VA_ARGS__); while (1);

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
        printerr("Unable to get GOP.\n");
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
        printerr("Unable to get current video mode.\n");
    }

    GopData gopData;
    gopData.pFramebuffer       = (void *) pGop->Mode->FrameBufferBase;
    gopData.nBufferSize        = pGop->Mode->FrameBufferSize;
    gopData.nWidth             = pGop->Mode->Information->HorizontalResolution;
    gopData.nHeight            = pGop->Mode->Information->VerticalResolution;
    gopData.nPixelsPerScanline = pGop->Mode->Information->PixelsPerScanLine;

    // File loading
    FILE *pFile = fopen(KERNEL_FILENAME, "r");
    if (pFile == NULL)
    {
        printerr("Kernel '" KERNEL_FILENAME "' not found.");
    }
    
    // Get file size
    fseek(pFile, 0, SEEK_END);
    size_t nFilesize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    sMZHeader mzhdr;
    fread((void *) &mzhdr, sizeof(sMZHeader), 1, pFile);
    if (mzhdr.nMagic != 0x5A4D)
    {
        printerr("Invalid MZ header in kernel. %d", mzhdr.nMagic);
    }

    // Skip the DOS stub
    fseek(pFile, 128, SEEK_SET);

    sPE32Header pehdr;
    fread((void *) &pehdr, sizeof(sPE32Header), 1, pFile);
    if (pehdr.nMagic != 0x4550)
    {
        printerr("Invalid PE32 header in kernel.");
    }

    if (pehdr.nSizeOfOptionalHeader != 0)
    {
        sPE32OptionalHeader peohdr;
        fread((void *) &peohdr, sizeof(sPE32OptionalHeader), 1, pFile);
        if (peohdr.nMagic != 0x020B)
        {
            printerr("Invalid PE32 optional header in kernel.");
        }

        printf("Magic: %d\n", peohdr.nMagic);
        printf("MajorLinkerVersion: %d\n", peohdr.nMajorLinkerVersion);
        printf("MinorLinkerVersion: %d\n", peohdr.nMinorLinkerVersion);
        printf("SizeOfCode: %d\n", peohdr.nSizeOfCode);
        printf("SizeOfInitializedData: %d\n", peohdr.nSizeOfInitializedData);
        printf("SizeOfUninitializedData: %d\n", peohdr.nSizeOfUninitializedData);
        printf("AddressOfEntryPoint: %d\n", peohdr.nAddressOfEntryPoint);
        printf("BaseOfCode: %d\n", peohdr.nBaseOfCode);
        printf("BaseOfData: %d\n", peohdr.nBaseOfData);
        printf("ImageBase: %d\n", peohdr.nImageBase);
        printf("SectionAlignment: %d\n", peohdr.nSectionAlignment);
        printf("FileAlignment: %d\n", peohdr.nFileAlignment);
        printf("MajorOperatingSystemVersion: %d\n", peohdr.nMajorOperatingSystemVersion);
        printf("MinorOperatingSystemVersion: %d\n", peohdr.nMinorOperatingSystemVersion);
        printf("MajorImageVersion: %d\n", peohdr.nMajorImageVersion);
        printf("MinorImageVersion: %d\n", peohdr.nMinorImageVersion);
        printf("MajorSubsystemVersion: %d\n", peohdr.nMajorSubsystemVersion);
        printf("MinorSubsystemVersion: %d\n", peohdr.nMinorSubsystemVersion);
        printf("Win32VersionValue: %d\n", peohdr.nWin32VersionValue);
        printf("SizeOfImage: %d\n", peohdr.nSizeOfImage);
        printf("SizeOfHeaders: %d\n", peohdr.nSizeOfHeaders);
        printf("CheckSum: %d\n", peohdr.nCheckSum);
        printf("Subsystem: %d\n", peohdr.nSubsystem);
        printf("DllCharacteristics: %d\n", peohdr.nDllCharacteristics);
        printf("SizeOfStackReserve: %d\n", peohdr.nSizeOfStackReserve);
        printf("SizeOfStackCommit: %d\n", peohdr.nSizeOfStackCommit);
        printf("SizeOfHeapReserve: %d\n", peohdr.nSizeOfHeapReserve);
        printf("SizeOfHeapCommit: %d\n", peohdr.nSizeOfHeapCommit);
        printf("LoaderFlags: %d\n", peohdr.nLoaderFlags);
        printf("NumberOfRvaAndSizes: %d\n", peohdr.nNumberOfRvaAndSizes);
    }

    printf("%d", ftell(pFile));

    fclose(pFile);
    

    exit_bs();

    // TODO: Call the kernel

    while (1); // Shouldn't ever be reached; In case the kernel ever returns.
    return 0;
}
