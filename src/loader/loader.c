
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/screen.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/memory/paging.h>


void LoaderMain(sBootData data)
{
    InitGDT();
    LoadGDT();

    InitScreen(data.gop.nWidth, data.gop.nHeight, data.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(RGB(0, 0, 0));
    SetFGColor(RGB(168, 168, 168));

    PrintString("Screen initialized!\n");

    InitIDT();

    EnableInterrupts();
    PrintString("Interrupts enabled!\n");

    void *pLargestSegment = NULL;
    size_t nLargestSegmentSize = 0, nMemorySize = 0;

    for (sEFIMemoryDescriptor *pEntry = data.pMemoryDescriptor;
         (uint8_t *) pEntry < (uint8_t *) data.pMemoryDescriptor + data.nMemoryMapSize;
         pEntry = (sEFIMemoryDescriptor *) ((uint8_t *) pEntry + data.nMemoryDescriptorSize))
    {
        nMemorySize += pEntry->nNumberOfPages * PAGE_SIZE;
        if (pEntry->nType == 7 && pEntry->nNumberOfPages * PAGE_SIZE > nLargestSegmentSize)
        {
            pLargestSegment     = (void *) pEntry->nPhysicalStart;
            nLargestSegmentSize = pEntry->nNumberOfPages * PAGE_SIZE;
        }
    }


    // Paging
    InitPaging(nMemorySize, pLargestSegment, nLargestSegmentSize);

    // Lock the GOP screen memory.
    LockPages(data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    MapPageRange(data.gop.pFramebuffer, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    LoadPML4();
    
    while (1);
}
