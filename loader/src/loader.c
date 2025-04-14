
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
#include <neos.h>

void LoaderMain(sBootData data)
{
    DisableInterrupts();

    InitGDT();
    LoadGDT();

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
    MapPageRange(data.gop.pFramebuffer, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    LoadPML4();
    PrintString("Paging initialised!\n");

    for (QWORD i = 0; i < NEOS_HEAP_SIZE / PAGE_SIZE; i++)
    {
        PVOID pPage = AllocatePage();
        MapPage((PVOID) (NEOS_HEAP_START + i * PAGE_SIZE), pPage, PF_WRITEABLE);
    }

    InitHeap(NEOS_HEAP_START, _ALIGN_TO_PAGE(NEOS_HEAP_START + NEOS_HEAP_SIZE));
    PrintString("Heap initialised!\n");

    EnableInterrupts();
    PrintString("Interrupts enabled!\n");

    PrintString("Scanning for PCI devices...\n");
    ScanPCIDevices();
    PrintString("PCI scan done\nLoading GPT partitions...\n");

    InitDrives();

    LoadGPT(DRIVE_TYPE_AHCI_SATA);
    PrintString("GPT Loaded\n");
    
    sGPTPartitionEntry sKernelPartition = GetKernelPartition();
    LoadFAT32(DRIVE_TYPE_AHCI_SATA, sKernelPartition);


    sFAT32DirectoryEntry sKernelEntry;
    _ASSERT(GetEntryFromPath("NEOS/NEOS    SYS", &sKernelEntry), "Kernel not found at C:\\NeOS\\NeOS.sys");
    
    PBYTE pKernelData = HeapAlloc(sKernelEntry.dwFileSize);
    ReadDirectoryEntry(&sKernelEntry, pKernelData);
    
    // Parse the file (PE32+)
    sMZHeader *pMZHeader = (sMZHeader *) pKernelData;
    _ASSERT(pMZHeader->wMagic == 0x5A4D, "Invalid DOS stub.");

    sPE32Header *pPEHeader = (sPE32Header *) (pKernelData + 128); // FIXME: Replate the 128 with the proper size of the MZ header
    _ASSERT(pPEHeader->dwMagic == 0x4550, "Invalid PE32 header.");
    _ASSERT(pPEHeader->wMachine == 0x8664, "Kernel executable must be 64-bit.");
    _ASSERT(pPEHeader->wSizeOfOptionalHeader != 0, "Missing PE32 optional header.");
    
    sPE32OptionalHeader *pPEOHeader = (sPE32OptionalHeader *) (pPEHeader + 1);
    _ASSERT(pPEOHeader->wMagic == 0x020B, "Invalid PE32 optional header.");

    // TODO: Add saftey checks to ensure that the entrypoint inside the file matches the expected value (NEOS_KERNEL_LOCATION).
    DisableInterrupts();
    for (QWORD i = 0; i < pPEHeader->wNumberOfSections; i++)
    {
        sPE32SectionHeader *hdr = (sPE32SectionHeader *) ((PBYTE) pPEOHeader + pPEHeader->wSizeOfOptionalHeader + sizeof(sPE32SectionHeader) * i);

        QWORD qwAddress = hdr->dwVirtualAddress + pPEOHeader->qwImageBase;
        // QWORD qwPages   = (hdr->dwVirtualSize + 0x1000 - 1) / 0x1000;

        PrintFormat("Loading kernel segment: %s at 0x%p\n", hdr->szName, qwAddress);

        // for (QWORD i = 0; i < qwPages; i++)
        //     MapPage((PVOID) (qwAddress + i * PAGE_SIZE), AllocatePage(), PF_PRESENT | PF_WRITEABLE);
        
        memcpy((PVOID) qwAddress, (PBYTE) pKernelData + hdr->dwPointerToRawData, hdr->dwVirtualSize);
    }

    EnableInterrupts();
    HeapFree(pKernelData);
    
    sNEOSKernelHeader sHeader;
    sHeader.sGOP = data.gop;
    sHeader.sPaging = ExportPagingData();
    
    ClearScreen();
    ((void (*)(sNEOSKernelHeader)) (pPEOHeader->dwAddressOfEntrypoint + pPEOHeader->qwImageBase))(sHeader);
    
    DisableInterrupts();
    __asm__ volatile ("hlt");
    for (;;);
}
