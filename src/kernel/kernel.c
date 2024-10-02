
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <common/screen.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/ports.h>

uint64_t Test(uint64_t rsp, uint8_t nErrorCode)
{
    PrintString("Division by zero!\n");
    __asm__ volatile ("cli\nhlt");
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

    RegisterException(0x00, Test);

    __asm__ volatile ("div %0" :: "r"(0));

    EnableInterrupts();
    while (1); // Make sure the kernel doesn't exit
}
