
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/screen.h>

#include <hardware/memory/paging.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/pci.h>

void LoaderMain(sBootData data)
{
    DisableInterrupts();
    
    InitGDT();
    LoadGDT();

    InitScreen(data.gop.nWidth, data.gop.nHeight, data.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(_RGB(0, 0, 0));
    SetFGColor(_RGB(168, 168, 168));

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
    MapPageRange(data.gop.pFramebuffer, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    LoadPML4();

    EnableInterrupts();
    PrintString("Interrupts enabled!\n");

    // ScanPCIDevices();
    
    while (1);
}
