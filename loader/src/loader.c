
#include <common/math.h>
#include <common/types.h>
#include <common/panic.h>
#include <common/string.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/exestructs.h>
#include <common/screen.h>

#include <hardware/storage/drive.h>
#include <filesystem/gpt.h>
#include <filesystem/fat32.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/pci.h>
#include <memory/paging.h>
#include <memory/heap.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/ini.h>
#include <neos.h>

QWORD __stack_chk_guard = 0x595e9fbd94fda766;

void __stack_chk_fail(void)
{
	_KERNEL_PANIC("Stack smashing detected");
}

// The functions below are for the kernel,
// so the it doesn't have to access the directory entry directly (for compatibility with other file systems)
QWORD GetFileSize(sFAT32DirectoryEntry *pFile)
{
    return pFile->dwFileSize;
}

PVOID GetFile(PWCHAR wszPath)
{
    sFAT32DirectoryEntry sEntry;
    if (!GetEntryFromPath(wszPath, &sEntry)) return NULL;
    PVOID pEntry = KHeapAlloc(sizeof(sFAT32DirectoryEntry));
    memcpy(pEntry, &sEntry, sizeof(sEntry));
    return pEntry;
}

void LoaderMain(sBootData data)
{
    DisableInterrupts();

    InitGDT();
    WriteGDT();

    // Screen
    InitScreen(data.gop.nWidth, data.gop.nHeight, data.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(NEOS_BACKGROUND_COLOR);
    SetFGColor(NEOS_FOREGROUND_COLOR);

    PrintString("Screen initialised!\n");

    // Interrupts
    InitIDT();
    RegisterExceptions();
    PrintString("Interrupts initialised!\n");

    // Paging
    InitPaging(data.pMemoryDescriptor,
               data.nMemoryMapSize, data.nMemoryDescriptorSize,
               data.nLoaderStart, data.nLoaderEnd);

    // Lock the GOP screen memory.
    ReservePages(data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    MapPageRangeToIdentity(NULL, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    PrintString("Paging initialised!\n");

    // Set up the heap
    sHeap sKernelHeap = CreateHeap(NEOS_HEAP_SIZE / PAGE_SIZE, false, true, NULL);
    SetKernelHeap(&sKernelHeap);
    PrintString("Heap initialised!\n");

    // Set up the drive
    PrintString("Scanning for PCI devices...\n");
    ScanPCIDevices();

    InitDrives();

    LoadGPT(DRIVE_TYPE_AHCI_SATA);
    PrintString("GPT Loaded\n");
    
    sGPTPartitionEntry sKernelPartition = GetKernelPartition();
    LoadFAT32(DRIVE_TYPE_AHCI_SATA, sKernelPartition);

    PrintFormat("Loading kernel...\n");

    sFAT32DirectoryEntry sKernelEntry;
    _ASSERT(GetEntryFromPath(L"NeOS\\NeOS.sys", &sKernelEntry), "Kernel not found at C:\\NeOS\\NeOS.sys");

    // Read the kernel
    PBYTE pKernelData = KHeapAlloc(sKernelEntry.dwFileSize);
    ReadDirectoryEntry(&sKernelEntry, pKernelData);
    
    // Parse the file (PE32+)
    PrintFormat("Loading PE headers...\n");
    sMZHeader *pMZHeader = (sMZHeader *) pKernelData;
    _ASSERT(pMZHeader->wMagic == 0x5A4D, "Invalid DOS stub.");

    sPE32Header *pPEHeader = (sPE32Header *) (pKernelData + 128); // FIXME: Replate the 128 with the proper size of the MZ header
    _ASSERT(pPEHeader->dwMagic == 0x4550, "Invalid PE32 header.");
    _ASSERT(pPEHeader->wMachine == 0x8664, "Kernel executable must be 64-bit.");
    _ASSERT(pPEHeader->wSizeOfOptionalHeader != 0, "Missing PE32 optional header.");
    
    sPE32OptionalHeader *pPEOHeader = (sPE32OptionalHeader *) (pPEHeader + 1);
    _ASSERT(pPEOHeader->wMagic == 0x020B, "Invalid PE32 optional header.");
    PrintFormat("Loading sections...\n");

    // TODO: Add saftey checks to ensure that the entrypoint inside the file matches the expected value (NEOS_KERNEL_LOCATION).
    DisableInterrupts(); // To ensure that everything is loaded in the right order
    for (QWORD i = 0; i < pPEHeader->wNumberOfSections; i++)
    {
        sPE32SectionHeader *hdr = (sPE32SectionHeader *) ((PBYTE) pPEOHeader + pPEHeader->wSizeOfOptionalHeader + sizeof(sPE32SectionHeader) * i);
        QWORD qwAddress = hdr->dwVirtualAddress + pPEOHeader->qwImageBase;
        if (hdr->dwCharacteristics & PE32_SCN_MEM_DISCARDABLE) continue;
        PrintFormat("Loading section: %s at 0x%p c: 0x%08X\n", hdr->szName, qwAddress, hdr->dwCharacteristics);
        memcpy((PVOID) qwAddress, (PBYTE) pKernelData + hdr->dwPointerToRawData, hdr->dwVirtualSize);
    }
    
    KHeapFree(pKernelData);

    // Assume the kernel will be between 0x200000 - 0x300000
    ReservePages((PVOID) NEOS_KERNEL_LOC_LOW, (NEOS_KERNEL_LOC_HIGH - NEOS_KERNEL_LOC_LOW) / PAGE_SIZE);
    MapPageRangeToIdentity(NULL, (PVOID) NEOS_KERNEL_LOC_LOW, (NEOS_KERNEL_LOC_HIGH - NEOS_KERNEL_LOC_LOW) / PAGE_SIZE, PF_WRITEABLE);
    
    sNEOSKernelHeader sHeader;
    sHeader.sGOP        = data.gop;
    sHeader.pKernelHeap = &sKernelHeap;
    sHeader.sPaging     = ExportPagingData();
    sHeader.GetFileSize = (QWORD (*)(PVOID)) GetFileSize;
    sHeader.GetFile     = (PVOID (*)(PWCHAR)) GetFile;
    sHeader.ReadFile    = (void (*)(PVOID, PVOID)) ReadDirectoryEntry;

    PrintFormat("Executing C:\\NeOS\\NeOS.sys at 0x%p\n", pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint);
    SetCursor(0, 0);
    ((void (*)(sNEOSKernelHeader)) (pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint))(sHeader);

    // Somthing went very wrong
    _KERNEL_PANIC("The kernel has crashed :/");
}
