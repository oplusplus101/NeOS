
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

void TestA()
{
    for (;;) __asm__ volatile ("int $0x81" : : "a"(0), "b"("A"));
}

void TestB()
{
    for (;;) __asm__ volatile ("int $0x81" : : "a"(0), "b"("B"));
}

void TestC()
{
    for (;;) __asm__ volatile ("int $0x81" : : "a"(0), "b"("C"));
}

void Syscall_KNeoPrintString(sCPUState *pCPUState)
{
    PrintString((PCHAR) pCPUState->qwRBX);
}

void KernelMain(sNEOSKernelHeader hdr)
{
    DisableInterrupts();

    sTaskStateSegment sTSS = { 0 };
    
    InitGDT();
    SetTSS(&sTSS);
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
    
    InitHeap(hdr.qwHeapStart, hdr.qwHeapStart + NEOS_HEAP_SIZE);
    
    InitSyscalls();
    PrintFormat("Syscalls initialised to interrput 0x%02X\n", NEOS_SYSCALL_IRQ);
    RegisterSyscall(0, 0, Syscall_KNeoPrintString);
    
    InitProcessScheduler();
    RegisterInterrupt(32, ScheduleProcesses);
    PrintFormat("Process scheduler initialised\n");
    
    StartProcess("Test A", TestA, 1024 * 1024, 0, 0);
    StartProcess("Test B", TestB, 1024 * 1024, 0, 0);
    StartProcess("Test C", TestC, 1024 * 1024, 0, 0);

    EnableInterrupts();
    for (;;);
}
