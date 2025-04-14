
#include <neos.h>
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <common/screen.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/ports.h>

void KernelMain(sNEOSKernelHeader hdr)
{
    DisableInterrupts();
    InitScreen(hdr.sGOP.nWidth, hdr.sGOP.nHeight, hdr.sGOP.pFramebuffer);
    SetFGColor(NEOS_FOREGROUND_COLOR);
    SetBGColor(NEOS_BACKGROUND_COLOR);
    PrintFormat("Kernel loaded!\n");
    
    // Load Paging data
    ImportPagingData(hdr.sPaging);

    DisableInterrupts();
    __asm__ volatile ("hlt");
    for (;;);
}
