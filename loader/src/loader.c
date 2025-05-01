
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

QWORD GetFileSize(sFAT32DirectoryEntry *pFile)
{
    return pFile->dwFileSize;
}

PVOID GetFile(PWCHAR wszPath)
{
    PVOID pEntry = HeapAlloc(sizeof(sFAT32DirectoryEntry));
    if (!GetEntryFromPath(wszPath, pEntry)) return 0;
    return pEntry;
}

void LoaderMain(sBootData data)
{
    DisableInterrupts();

    InitGDT();
    WriteGDT();

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
    MapPageRangeToIdentity(data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    PrintString("Paging initialised!\n");

    PVOID pHeapBuffer = AllocateContinousPages(NEOS_HEAP_SIZE / PAGE_SIZE);
    MapPageRangeToIdentity(pHeapBuffer, NEOS_HEAP_SIZE / PAGE_SIZE, PF_WRITEABLE);
    InitHeap((QWORD) pHeapBuffer, _ALIGN_TO_PAGE((QWORD) pHeapBuffer + NEOS_HEAP_SIZE));
    PrintString("Heap initialised!\n");

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

    PBYTE pKernelData = HeapAlloc(sKernelEntry.dwFileSize);
    ReadDirectoryEntry(&sKernelEntry, pKernelData);
    
    PrintFormat("Loading PE headers...\n");
    // Parse the file (PE32+)
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
        PrintFormat("Loading section: %s at 0x%p\n", hdr->szName, qwAddress);
        
        memcpy((PVOID) qwAddress, (PBYTE) pKernelData + hdr->dwPointerToRawData, hdr->dwVirtualSize);
    }
    
    HeapFree(pKernelData);

    // Assume the kernel will be between 0x200000 - 0x300000
    ReservePages((PVOID) 0x200000, 0x100000 / PAGE_SIZE);
    MapPageRangeToIdentity((PVOID) 0x200000, 0x100000 / PAGE_SIZE, PF_WRITEABLE);
    
    sNEOSKernelHeader sHeader;
    sHeader.qwHeapStart = (QWORD) pHeapBuffer;
    sHeader.qwHeapSize  = NEOS_HEAP_SIZE; // TODO: Make the heap size change based on the total RAM size
    sHeader.sGOP        = data.gop;
    sHeader.sPaging     = ExportPagingData();
    sHeader.GetFileSize = (QWORD (*)(PVOID)) GetFileSize;
    sHeader.GetFile     = (PVOID (*)(PWCHAR)) GetFile;
    sHeader.ReadFile    = (void (*)(PVOID, PVOID)) ReadDirectoryEntry;
    
    PrintFormat("Executing C:\\NeOS\\NeOS.sys at 0x%p\n", pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint);
    ((void (*)(sNEOSKernelHeader)) (pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint))(sHeader);

    // Somthing went very wrong
    _KERNEL_PANIC("The kernel has crashed :/");
}
