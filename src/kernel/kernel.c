
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <common/screen.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/ports.h>

uint64_t Test(uint64_t rsp)
{
    PrintString("Interrupt\n");
    PrintDec(rsp);
    return rsp;
}

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

    RegisterISR(0x20, Test);
    
    EnableInterrupts();
    while (1); // Make sure the kernel doesn't exit
}
