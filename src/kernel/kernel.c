
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <common/screen.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/ports.h>

void KernelMain(sBootData hdr)
{
    DisableInterrupts();
    InitGDT();
    LoadGDT();
    InitScreen(hdr.gop.nWidth, hdr.gop.nHeight, hdr.gop.pFramebuffer);
    SetFGColor(RGB(255, 255, 255));
    SetBGColor(RGB(0, 0, 0));
    PrintString("Kernel loaded!\n");
    InitIDT();

    EnableInterrupts();
    while (1); // Make sure the kernel doesn't exit
}
