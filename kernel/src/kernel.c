
#include <neos.h>
#include <common/types.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/math.h>
#include <common/screen.h>
#include <memory/heap.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/ports.h>
#include <syscalls.h>
#include <process.h>
#include <timer.h>

QWORD Scheduler(QWORD qwRSP)
{
    return (QWORD) ScheduleProcesses((sCPUState *) qwRSP);
}

void TestPrint(sCPUState *sState)
{
    PrintString((PCHAR) sState->qwRBX);
}

void KernelMain(sNEOSKernelHeader hdr)
{
    DisableInterrupts();

    // Init the GDT
    InitGDT(false, NULL);
    WriteGDT();
    
    InitIDT();
    RegisterExceptions();
    
    InitScreen(hdr.sGOP.nWidth, hdr.sGOP.nHeight, hdr.sGOP.pFramebuffer);
    SetFGColor(NEOS_FOREGROUND_COLOR);
    SetBGColor(NEOS_BACKGROUND_COLOR);
    ClearScreen();
    PrintFormat("Kernel loaded!\n");
    
    ImportPagingData(hdr.sPaging);
    PrintFormat("Paging initialised (%u bytes free)\n", hdr.sPaging.qwFreeMemory);
    
    InitHeap(NEOS_HEAP_START, NEOS_HEAP_SIZE);
    
    InitSyscalls();
    PrintFormat("Syscalls initialized to interrput 0x%02X\n", NEOS_SYSCALL_IRQ);
    
    InitProcessScheduler();
    RegisterInterrupt(32, Scheduler);
    PrintFormat("Process scheduler initialised\n");
    
    EnableInterrupts();
    RegisterSyscall(0, 0, TestPrint);

    __asm__ volatile ("int $0x81" :: "a"(0), "b"("This is a syscall string"));
    
    for (;;);
}

