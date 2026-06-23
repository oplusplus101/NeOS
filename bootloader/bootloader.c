
#include <uefi.h>
#include <common/types.h>
#include <common/math.h>
#include <common/exestructs.h>
#include "exe.h"
#include "fs.h"
#include "../arch/x86_64_EFI/include/arch.h"
#include <neos.h>


#define _PANIC(...) { printf(__VA_ARGS__); while (1); }
#define _ASSERT(c, ...) if (!(c)) { _PANIC(__VA_ARGS__); }

typedef QWORD PTE;
typedef PTE *PageTable;

#define PAGE_TABLE_SIZE 512

#define _ADDRESS_TO_PML4_INDEX(a) (((QWORD) (a) >> 39) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PDPT_INDEX(a)  (((QWORD) (a) >> 30) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PD_INDEX(a)   (((QWORD) (a) >> 21) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PT_INDEX(a)   (((QWORD) (a) >> 12) & (PAGE_TABLE_SIZE - 1))
#define _ALIGN_TO_PAGE(a)         ((QWORD) (a) & ~0x0FFFULL)
#define _ALIGN_TO_HUGE_PAGE(a)    ((QWORD) (a) & ~0x1FFFFFULL)

PBYTE g_arrBitmap;
QWORD g_qwBitmapSize;

PVOID g_arrKernelStackStart;
PBYTE g_pBootFileSystemPhysicalOffset, g_pBootFileSystemPhysical;
PageTable g_arrEFICR3;

void LoadGOP(sARCHHeader *pParams)
{
    // Get the GOP
    efi_guid_t gopGUID = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_gop_t *pGOP = NULL;
    efi_gop_mode_info_t *pGOPInfo = NULL;
    uintn_t nISize = sizeof(efi_gop_mode_info_t);
    efi_status_t uStatus = BS->LocateProtocol(&gopGUID, NULL, (void **) &pGOP);

    _ASSERT(!EFI_ERROR(uStatus) && pGOP != NULL, "Unable to get the GOP.");

    uStatus = pGOP->QueryMode(pGOP, pGOP->Mode ? pGOP->Mode->Mode : 0, &nISize, &pGOPInfo);
    if (uStatus == EFI_NOT_STARTED || !pGOP->Mode)
    {
        uStatus = pGOP->SetMode(pGOP, 0);
        ST->ConOut->Reset(ST->ConOut, 0);
        ST->StdErr->Reset(ST->StdErr, 0);
    }

    _ASSERT(!EFI_ERROR(uStatus), "Unable to get the current video mode.");

    
    // Check the PixelFormat enum to determine BPP
    if (pGOPInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor || pGOPInfo->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
        pParams->sScreen.bBitsPerPixel = 32; // 8 bits each for R, G, B, and Reserved padding
    else if (pGOPInfo->PixelFormat == PixelBitMask)
    {
        // Compute BPP by checking custom bitmask allocations
        DWORD dwMask = pGOPInfo->PixelInformation.RedMask | pGOPInfo->PixelInformation.GreenMask | pGOPInfo->PixelInformation.BlueMask | pGOPInfo->PixelInformation.ReservedMask;
        pParams->sScreen.bBitsPerPixel = __builtin_popcount(dwMask);
    }
    else
        _PANIC("Oh no");

    pParams->sScreen.qwWidth  = pGOPInfo->HorizontalResolution;
    pParams->sScreen.qwHeight = pGOPInfo->VerticalResolution;
    pParams->sScreen.qwPitch  = pGOPInfo->PixelsPerScanLine;

    pParams->sScreen.pBaseAddress                    = (PVOID) pGOP->Mode->FrameBufferBase;


    // Removed temporarily, since PixelInformation seems to not work
    // pParams->sScreen.sPixelFormat.bRedStart          = __builtin_ctz(pGOPInfo->PixelInformation.RedMask);
    // pParams->sScreen.sPixelFormat.bGreenStart        = __builtin_ctz(pGOPInfo->PixelInformation.GreenMask);
    // pParams->sScreen.sPixelFormat.bBlueStart         = __builtin_ctz(pGOPInfo->PixelInformation.BlueMask);
    // pParams->sScreen.sPixelFormat.bRedLengthInBits   = __builtin_clz(pGOPInfo->PixelInformation.RedMask)   + pParams->sScreen.sPixelFormat.bRedStart   - 30;
    // pParams->sScreen.sPixelFormat.bGreenLengthInBits = __builtin_clz(pGOPInfo->PixelInformation.GreenMask) + pParams->sScreen.sPixelFormat.bGreenStart - 30;
    // pParams->sScreen.sPixelFormat.bBlueLengthInBits  = __builtin_clz(pGOPInfo->PixelInformation.BlueMask)  + pParams->sScreen.sPixelFormat.bBlueStart  - 30;

    pParams->sScreen.sPixelFormat.bRedStart          = 16;
    pParams->sScreen.sPixelFormat.bGreenStart        = 8;
    pParams->sScreen.sPixelFormat.bBlueStart         = 0;
    pParams->sScreen.sPixelFormat.bRedLengthInBits   = 8;
    pParams->sScreen.sPixelFormat.bGreenLengthInBits = 8;
    pParams->sScreen.sPixelFormat.bBlueLengthInBits  = 8;
    
}

void LoadMemoryMap(sARCHHeader *pParams)
{
    uintn_t uMemoryMapSize = 0, uMapKey = 0, uDescriptorSize = 0;
    DWORD dwDescriptorVersion = 0;
    efi_status_t st = BS->GetMemoryMap(&uMemoryMapSize, NULL, &uMapKey, &uDescriptorSize, &dwDescriptorVersion);
    _ASSERT(st == EFI_BUFFER_TOO_SMALL && uMemoryMapSize != 0, "Unable to get the memory map size %u", uMemoryMapSize);
    uMemoryMapSize += 4 * uDescriptorSize;
    
    st = BS->AllocatePool(EfiLoaderData, uMemoryMapSize, (PVOID) &pParams->arrMemoryMap);
    _ASSERT(!EFI_ERROR(st), "Unable to allocate memory.");

    st = BS->GetMemoryMap(&uMemoryMapSize, (efi_memory_descriptor_t *) pParams->arrMemoryMap, &uMapKey, &uDescriptorSize, NULL);
    _ASSERT(!EFI_ERROR(st), "Unable to get the memory map.\nEC: 0x%08X", st);

    pParams->qwMemoryDescriptorSize = uDescriptorSize;
    pParams->qwMemoryMapSize  = uMemoryMapSize;

    pParams->qwMemorySize = 0;
    for (sEFIMemoryDescriptor *pEntry = pParams->arrMemoryMap; (QWORD) pEntry < (QWORD) pParams->arrMemoryMap + pParams->qwMemoryMapSize; pEntry = (sEFIMemoryDescriptor *) ((PBYTE) pEntry + pParams->qwMemoryDescriptorSize))
    {
        if (pEntry->dwType != EfiConventionalMemory &&
            pEntry->dwType != EfiLoaderCode &&
            pEntry->dwType != EfiLoaderData &&
            pEntry->dwType != EfiBootServicesCode &&
            pEntry->dwType != EfiBootServicesData)
            continue;
        pParams->qwMemorySize = max(pParams->qwMemorySize, pEntry->qwPhysicalStart + pEntry->qwNumberOfPages * EFI_PAGE_SIZE);
    }
}

void _ReservePages(PVOID pStart, QWORD qwNumberOfPages)
{
    if ((((QWORD) pStart + qwNumberOfPages) / EFI_PAGE_SIZE + qwNumberOfPages) / 8 >= g_qwBitmapSize)
        return;
    
    for (QWORD i = 0; i < qwNumberOfPages; i++)
        g_arrBitmap[((QWORD) pStart / EFI_PAGE_SIZE + i) / 8] |= 1 << (((QWORD) pStart / EFI_PAGE_SIZE + i) % 8);
}

PVOID GetPage()
{
    efi_physical_address_t max = g_qwBitmapSize * 8 * EFI_PAGE_SIZE; // < 2 GiB
    STATUS s = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, 1, &max);
    _ASSERT(!EFI_ERROR(s), "Could not allocate page");

    memset((PVOID) max, 0, 4096);
    return (PVOID) max;
}

void MapPages(PageTable arrPML4, PVOID pPhysicalStart, PVOID pVirtualStart, QWORD qwLengthInPages, WORD wFlags)
{
    for (QWORD i = 0; i < qwLengthInPages; i++)
    {
        PVOID pVirtualAddress = (PBYTE) pVirtualStart + i * 0x1000;
        PVOID pPhysicalAddress = (PBYTE) pPhysicalStart + i * 0x1000;
        
        // Page Directory Page Table
        PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
        PageTable arrPDPT;
        if (!(*pEntry & 1))
        {
            arrPDPT = GetPage();
            *pEntry = _ALIGN_TO_PAGE(arrPDPT) | 0x103;
            arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)] = *pEntry;
        }
        else
            arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

        // Page Directory
        pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
        PageTable arrPD;
        if (!(*pEntry & 1))
        {
            arrPD = GetPage();
            *pEntry = _ALIGN_TO_PAGE(arrPD) | 0x103;
            arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)] = *pEntry;
        }
        else
            arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

        // Page Table
        pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
        PageTable arrPT;
        if (!(*pEntry & 1))
        {
            arrPT = GetPage();
            *pEntry = _ALIGN_TO_PAGE(arrPT) | 0x103;
            arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
        }
        else
            arrPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

        arrPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)] = _ALIGN_TO_PAGE(pPhysicalAddress) | wFlags;
    }
}

void MapHugePages(PageTable arrPML4, PVOID pPhysicalStart, PVOID pVirtualStart, QWORD qwLengthInPages, WORD wFlags)
{
    _ASSERT(qwLengthInPages != 0, "qwLengthInPages must be greater than 0");
    pPhysicalStart = (PVOID) _ALIGN_TO_HUGE_PAGE(pPhysicalStart);
    pVirtualStart  = (PVOID) _ALIGN_TO_HUGE_PAGE(pVirtualStart);

    for (QWORD i = 0; i < qwLengthInPages; i++)
    {
        PVOID pVirtualAddress = (PBYTE) pVirtualStart + i * 0x200000;
        PVOID pPhysicalAddress = (PBYTE) pPhysicalStart + i * 0x200000;
        
        // Page Directory Page Table
        PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
        PageTable arrPDPT;
        if (!(*pEntry & 1))
        {
            arrPDPT = GetPage();
            *pEntry = _ALIGN_TO_PAGE(arrPDPT) | 0x103;
            arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)] = *pEntry;
        }
        else
            arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

        // Page Directory
        pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
        PageTable arrPD;
        if (!(*pEntry & 1))
        {
            arrPD = GetPage();
            *pEntry = _ALIGN_TO_PAGE(arrPD) | 0x103;
            arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)] = *pEntry;
        }
        else
            arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

        pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
        *pEntry = _ALIGN_TO_HUGE_PAGE(pPhysicalAddress) | wFlags;
    }
}

PVOID InitialisePaging(sARCHHeader *pParams)
{
    efi_physical_address_t qwBitmapAddress;
    g_qwBitmapSize = pParams->qwMemorySize / EFI_PAGE_SIZE / 8;
    efi_status_t s = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, (g_qwBitmapSize + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE, &qwBitmapAddress);
    _ASSERT(!EFI_ERROR(s), "Could not allocate the page bitmap");

    g_arrBitmap               = (PBYTE) qwBitmapAddress;
    pParams->pPageBitmap      = g_arrBitmap;
    pParams->qwPageBitmapSize = g_qwBitmapSize;

    memset(pParams->pPageBitmap, 0, g_qwBitmapSize);
    _ReservePages(g_arrBitmap, g_qwBitmapSize / EFI_PAGE_SIZE);
    
    PageTable arrPML4 = GetPage();
    arrPML4[511] = (PTE) arrPML4 | 0x103;

    return arrPML4;
}

void LoadTables(sARCHHeader *pParams)
{
    efi_configuration_table_t *arrConfigTable = ST->ConfigurationTable;
    efi_guid_t sACPI20GUID = ACPI_20_TABLE_GUID;
    efi_guid_t sACPI10GUID = ACPI_TABLE_GUID;

    sRSDP *pRSDP = NULL;

    for (QWORD i = 0; i < ST->NumberOfTableEntries; i++)
        if (!memcmp(&arrConfigTable[i].VendorGuid, &sACPI20GUID, sizeof(sACPI20GUID)))
        {
            pRSDP = (sRSDP *) arrConfigTable[i].VendorTable;
            break;
        }

    if (pRSDP == NULL)
        for (QWORD i = 0; i < ST->NumberOfTableEntries; i++)
            if (!memcmp(&arrConfigTable[i].VendorGuid, &sACPI10GUID, sizeof(sACPI10GUID)))
            {
                pRSDP = (sRSDP *) arrConfigTable[i].VendorTable;
                break;
            }


    _ASSERT(pRSDP != NULL, "RSDP not found");

    sMADT *pMADT = NULL;
    sSDTHeader *pXSDT = (sSDTHeader *) pRSDP->qwXSDTAddress;
    _ASSERT(!strncmp(pXSDT->sSignature, "XSDT", 4), "Invalid XSDT signature");

    QWORD narrEntries = (pXSDT->dwLength - sizeof(sSDTHeader)) / 8;
    PQWORD arrTablePointers = (PQWORD) (pXSDT + 1);

    for (QWORD i = 0; i < narrEntries; i++)
    {
        sSDTHeader *pHeader = (sSDTHeader *) arrTablePointers[i];

        if (pHeader->sSignature[0] == 'A' && pHeader->sSignature[1] == 'P' && pHeader->sSignature[2] == 'I' && pHeader->sSignature[3] == 'C')
        {
            // Found MADT
            pMADT = (sMADT *) pHeader;
            break;
        }
    }

    _ASSERT(pMADT != NULL, "MADT not found");

    pParams->pRSDP = pRSDP;
    pParams->pXSDT = pXSDT;
    pParams->pMADT = pMADT;
}

typedef struct
{
    QWORD qwARCHEntryRVA;
    QWORD qwKernelEntryRVA;
    sRAMFile *pRoot;
    sARCHHeader sHeader;
} __attribute__((aligned(16))) sAfterStackSwitchParams;

void SYSV AfterStackSwitch(sAfterStackSwitchParams *sParams)
{
    // First start the ARCH layer
    ((void SYSV (*)(sARCHHeader)) (sParams->qwARCHEntryRVA + MM_ARCH_START))(sParams->sHeader);

    // In case the kernel ever returns.
    for (;;) __asm__ volatile("cli\nhlt");
}

PVOID AllocateBootFS(QWORD qwSize)
{
    g_pBootFileSystemPhysicalOffset += qwSize;
    if ((QWORD) g_pBootFileSystemPhysicalOffset >= (QWORD) g_pBootFileSystemPhysical + GetBootFileSystemSize())
        _PANIC("Out of space for boot file system");
    return g_pBootFileSystemPhysicalOffset - qwSize;
}

void CopyFiles(sRAMFile *pParent)
{
    if (pParent->qwFileSize == 0) return;
    
    sRAMFile **pDestSubFiles = AllocateBootFS(pParent->qwFileSize * sizeof(sRAMFile *));
        
    for (QWORD i = 0; i < pParent->qwFileSize; i++)
    {
        sRAMFile *pSubFile = ((sRAMFile **) pParent->pData)[i];

        pDestSubFiles[i] = AllocateBootFS(sizeof(sRAMFile));
        memcpy(pDestSubFiles[i], pSubFile, sizeof(sRAMFile));
        
        if (pSubFile->bFolder)
            CopyFiles(pDestSubFiles[i]);
        else
        {
            PVOID pData = AllocateBootFS(pSubFile->qwFileSize);
            memcpy(pData, pSubFile->pData, pSubFile->qwFileSize);
            
            pDestSubFiles[i]->pData = pData;
            BS->FreePool(pSubFile->pData);
        }
        pDestSubFiles[i]->pParent = pParent;

        BS->FreePool(pSubFile);
    }
    
    BS->FreePool(pParent->pData);
    pParent->pData = pDestSubFiles;
}

int main()
{
    sARCHHeader sHeader;
    sPE32Headers sARCHHeaders, sKernelHeaders;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(g_arrEFICR3));
    
    InitialiseFileSystem();
    printf("File system initialised!\n");
    LoadMemoryMap(&sHeader);
    printf("Memory map loaded!\n");
    LoadGOP(&sHeader);
    printf("GOP loaded!\n");
    LoadTables(&sHeader);
    printf("Tables loaded!\n");

    sRAMFile *pRoot = GetBootFileSystem();

    g_pBootFileSystemPhysical = (PVOID) sHeader.qwMemorySize;
    efi_status_t s = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, (GetBootFileSystemSize() + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE, (efi_physical_address_t *) &g_pBootFileSystemPhysical);
    _ASSERT(!EFI_ERROR(s), "[  ERROR  ] Could not allocate pages for the boot file system %u", s);
    memset(g_pBootFileSystemPhysical, 0, (GetBootFileSystemSize() + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE);
    g_pBootFileSystemPhysicalOffset = g_pBootFileSystemPhysical;

    sRAMFile *pRootClone = AllocateBootFS(sizeof(sRAMFile));
    memcpy(pRootClone, pRoot, sizeof(sRAMFile));
    CopyFiles(pRootClone);
    sHeader.pRoot      = pRootClone;
    sHeader.qwRootSize = GetBootFileSystemSize();

    STATUS st = InitialiseExecutableLoader(pRootClone);
    _ASSERT(st == 0, "[  ERROR  ] Failed to load initialise the executable loader! [%08X]\n", st);        

    sRAMFile *pARCHFile = GetFile(L"NeOS\\ARCH.sys", pRootClone);
    _ASSERT(pARCHFile != NULL, "Couldn't find NeOS\\ARCH.sys");

    sRAMFile *pKernelFile = GetFile(L"NeOS\\NeOS.sys", pRootClone);
    _ASSERT(pKernelFile != NULL, "Couldn't find NeOS\\NeOS.sys");

    sHeader.pARCHPhysical   = (PVOID) 0x200000;
    sHeader.pKernelPhysical = (PVOID) 0x400000;

    printf("Loading ARCH.sys\n");
    LoadExecutableTo(pARCHFile,   MM_ARCH_START,   (QWORD) sHeader.pARCHPhysical, &sARCHHeaders);
    printf("Loading NEOS.sys\n");
    LoadExecutableTo(pKernelFile, MM_KERNEL_START, (QWORD) sHeader.pKernelPhysical, &sKernelHeaders);
    
    sHeader.qwARCHSize   = _ALIGN_TO_HUGE_PAGE(CalculatePESize(&sARCHHeaders)   + 0x1FFFFF);
    sHeader.qwKernelSize = _ALIGN_TO_HUGE_PAGE(CalculatePESize(&sKernelHeaders) + 0x1FFFFF);
    sHeader.qwKernelEntryRVA  = sKernelHeaders.sPEOptional.dwAddressOfEntrypoint;
    
    printf("Executables loaded!\n");
    PageTable arrPML4 = InitialisePaging(&sHeader);
    sHeader.pPML4 = arrPML4;

    QWORD qwScreeenSizeBytes = sHeader.sScreen.qwPitch * sHeader.sScreen.qwHeight * sHeader.sScreen.bBitsPerPixel / 8;

    MapHugePages(arrPML4, NULL, NULL, (sHeader.qwMemorySize + 0x1FFFFF) / 0x200000, 0x183);
    MapHugePages(arrPML4, (PVOID) (QWORD) sHeader.pMADT->dwLocalAPICAddress, (PVOID) (QWORD) sHeader.pMADT->dwLocalAPICAddress, 1, 0x18B);
    MapHugePages(arrPML4, (PVOID) sHeader.sScreen.pBaseAddress, (PVOID) sHeader.sScreen.pBaseAddress, (qwScreeenSizeBytes + 0x1FFFFF) / 0x200000, 0x18B);
    _ReservePages((PVOID) (QWORD) sHeader.pMADT->dwLocalAPICAddress, 512);
    _ReservePages((PVOID) sHeader.sScreen.pBaseAddress, _ALIGN_TO_HUGE_PAGE(qwScreeenSizeBytes + 0x1FFFFF) / EFI_PAGE_SIZE);

    MapHugePages(arrPML4, sHeader.pARCHPhysical, (PVOID) MM_ARCH_START, (sHeader.qwARCHSize + 0x1FFFFF) / 0x200000, 0x183);
    MapHugePages(arrPML4, sHeader.pKernelPhysical, (PVOID) MM_KERNEL_START, (sHeader.qwKernelSize + 0x1FFFFF) / 0x200000, 0x183);
    _ReservePages(sHeader.pARCHPhysical, sHeader.qwARCHSize / EFI_PAGE_SIZE);
    _ReservePages(sHeader.pKernelPhysical, sHeader.qwKernelSize / EFI_PAGE_SIZE);

    _ReservePages(g_pBootFileSystemPhysical, (GetBootFileSystemSize() + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE);
    
    // Allocate the stacks
    g_arrKernelStackStart = (PVOID) sHeader.qwMemorySize;
    BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, 512, (efi_physical_address_t *) &g_arrKernelStackStart);
    memset(g_arrKernelStackStart, 0, EFI_PAGE_SIZE * 512);
    _ReservePages(g_arrKernelStackStart, 512);
    MapHugePages(arrPML4, g_arrKernelStackStart, (PVOID) MM_KERNEL_MAIN_STACK, 1, 0x183);

    sHeader.qwKernelStackSize = EFI_PAGE_SIZE * 512;

    printf("Paging initialised!\nExiting bootservices...\n");
    
    exit_bs();

    __asm__ volatile ("cli");
    // Disable globals
    __asm__ volatile ("mov %%cr4, %%rax\nand $~0x80, %%rax\nmov %%rax, %%cr4" : : : "rax");
    __asm__ volatile ("mov %0, %%cr3" : : "r"(arrPML4) : "memory");

    PBYTE pNewStackTop = (PBYTE) MM_KERNEL_MAIN_STACK + 512 * EFI_PAGE_SIZE - 16;

    sAfterStackSwitchParams sParams;
    sParams.qwARCHEntryRVA   = sARCHHeaders.sPEOptional.dwAddressOfEntrypoint;
    sParams.qwKernelEntryRVA = sKernelHeaders.sPEOptional.dwAddressOfEntrypoint;
    sParams.sHeader          = sHeader;
    sParams.pRoot            = pRootClone;

    __asm__ volatile (
        "mov %0, %%rsp\nmov %%rsp, %%rbp\nmov %2, %%rdi\njmp *%1\n"
        : : "r"(pNewStackTop), "r"(AfterStackSwitch), "r"(&sParams)
        : "memory"
    );

    return 0;
}
