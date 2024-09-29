
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/math.h>
#include <hardware/gdt.h>

void KernelMain(sBootData hdr)
{
    __asm__ volatile ("cli"); // Temporary: disable interrupts
    sGlobalDescriptorTable gdt;
    InitGDT(&gdt);
    LoadGDT(&gdt, 5);
    while (1); // Make sure the kernel doesn't exit
}
