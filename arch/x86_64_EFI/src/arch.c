
#define ARCH_EXE
#include <hardware/idt.h>
#include <memory/gdt.h>
#include <memory/paging.h>
#include <common/memory.h>
#include <io/log.h>
#include <hardware/apic.h>
#include <io/screen.h>
#include <arch.h>
#include <neos.h>
#include <ramfile.h>

QWORD SYSV PageFault(QWORD qwRSP, WORD wErrorCode)
{
    QWORD qwFaultAddress = 0xFFFFFFFFFFFFFFFF;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(qwFaultAddress));
    
    ArchLog(LOG_GOODBYE, L"PAGE FAULT! Fault address: %p", qwFaultAddress);
    ArchPrintFormat(L"- Access type: ");
    if (wErrorCode & 0b0001)  ArchPrintFormat(L"PRESENT ");
    if (wErrorCode & 0b0010)  ArchPrintFormat(L"WRITE ");
    else                      ArchPrintFormat(L"READ ");
    if (wErrorCode & 0b0100)  ArchPrintFormat(L"USER ");
    else                      ArchPrintFormat(L"KERNEL ");
    if (wErrorCode & 0b10000) ArchPrintFormat(L"FETCH ");
    ArchPrintChar('\n');

    for (;;) __asm__ volatile ("cli\nhlt");
}

void SYSV ArchMain(sARCHHeader sHeader)
{
    *((PDWORD) sHeader.sScreen.pBaseAddress) = 0xFFFFFFFF;
    SetRoot(sHeader.pRoot);

    _InitialiseScreen(sHeader.sScreen, GetRAMFile(L"robotolightboot.bdf"));
    ArchClearScreen();

    _InitialiseGDT(&sHeader);
    WriteGDT();
    FlushTSS();

    _InitialiseAPIC(&sHeader);
    _InitialiseIDT();

    ArchRegisterException(0x0E, PageFault);
    
    _InitialisePaging(&sHeader);

    // Load the kernel
    ((void SYSV (*)(sRAMFile *)) (sHeader.qwKernelEntryRVA + MM_KERNEL_START))(sHeader.pRoot);
}
