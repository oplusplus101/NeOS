
    #include <common/types.h>
    #include <common/bootstructs.h>
    #include <common/exceptions.h>
    #include <common/screen.h>

    #include <hardware/storage/drive.h>
    #include <filesystem/gpt.h>
    #include <filesystem/ntfs.h>
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

    PrintString("Screen initialized!\n");

    // Interrupts
    InitIDT();
    RegisterExceptions();
    PrintString("Interrupts initialized!\n");

    // Paging
    InitPaging(data.pMemoryDescriptor,
            data.nMemoryMapSize, data.nMemoryDescriptorSize,
            data.nLoaderStart, data.nLoaderEnd);

    // Lock the GOP screen memory.
    ReservePages(data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    MapPageRange(data.gop.pFramebuffer, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    LoadPML4();
    PrintString("Paging initialized!\n");

    for (QWORD i = 0; i < NEOS_HEAP_SIZE / PAGE_SIZE; i++)
    {
        PVOID pPage = AllocatePage();
        MapPage((PVOID) (NEOS_HEAP_START + i * PAGE_SIZE), pPage, PF_WRITEABLE);
    }

    InitHeap(NEOS_HEAP_START, _ALIGN_TO_PAGE(NEOS_HEAP_START + NEOS_HEAP_SIZE));
    PrintString("Heap initialized!\n");

    EnableInterrupts();
    PrintString("Interrupts enabled!\n");

    PrintString("Scanning for PCI devices...\n");
    ScanPCIDevices();
    PrintString("PCI scan done\nLoading GPT partitions...\n");

    InitDrives();

    LoadGPT(DRIVE_TYPE_AHCI_SATA);
    PrintString("GPT Loaded\n");

    sGPTPartitionEntry kernelPartition = GetKernelPartition();
    LoadNTFS(DRIVE_TYPE_AHCI_SATA, kernelPartition);
    PrintString("NTFS Loaded\n");

    DisableInterrupts();
    __asm__ volatile ("hlt");
}
