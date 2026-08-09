
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
#include <syscalls.h>

extern PBYTE g_arrPageBitmap;
extern QWORD g_qwBitmapLength;

static void (SYSV *g_pPageFaultHandler)(VIRTUAL_ADDRESS FaultAddress, WORD wFlags, BOOL bInstructionFetch) = NULL;
static void (SYSV *g_pExceptionHandler)(WORD wException, WORD wErrorCode, PVOID pCPUState) = NULL;


typedef struct
{
    QWORD qwRAX;
    QWORD qwRBX;
    QWORD qwRCX;
    QWORD qwRDX;

    QWORD qwR8;
    QWORD qwR9;
    QWORD qwR10;
    QWORD qwR11;
    QWORD qwR12;
    QWORD qwR13;
    QWORD qwR14;
    QWORD qwR15;

    QWORD qwRSI;
    QWORD qwRDI;
    QWORD qwRBP;

    QWORD qwError;

    QWORD qwRIP;
    QWORD qwCS;
    QWORD qwFlags;
    QWORD qwRSP;
    QWORD qwSS;
} __attribute__((packed)) sX86_64CPUContext;


QWORD SYSV InvalidOpCode(QWORD qwRSP, WORD wErrorCode)
{
    ArchLog(LOG_ERROR, L"INVALID OPCODE!");
    sX86_64CPUContext *pState = (sX86_64CPUContext *) qwRSP;
    ArchPrintFormat(L"RSP %p", pState->qwRSP);
    for (;;);
    
    return qwRSP;
}

QWORD SYSV PageFault(QWORD qwRSP, WORD wErrorCode)
{
    QWORD qwFaultAddress = 0xFFFFFFFFFFFFFFFF;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(qwFaultAddress));
    

    if (g_pPageFaultHandler == NULL)
    {
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

    g_pPageFaultHandler(qwFaultAddress, _ToAbstractFlags(wErrorCode), (wErrorCode & 0b10000) != 0);
    return qwRSP;
}

QWORD SYSV GeneralProtectionFault(QWORD qwRSP, WORD wErrorCode)
{
    ArchLog(LOG_GOODBYE, L"GENERAL PROTECTION FAULT! Error Code: %04X", wErrorCode);

    for (;;) __asm__ volatile ("cli\nhlt");
}

void SYSV ArchRegisterExceptionHandler(void (SYSV *pHandler)(WORD wException, WORD wErrorCode, PVOID pCPUState))
{
    g_pExceptionHandler = pHandler;
}

void SYSV ArchRegisterPageFaultHandler(void (SYSV *pHandler)(VIRTUAL_ADDRESS FaultAddress, WORD wFlags, BOOL bInstructionFetch))
{
    g_pPageFaultHandler = pHandler;
}

void SYSV ArchMain(sARCHHeader sHeader)
{
    g_pPageFaultHandler = NULL; // For some reason globals are broken
    SetRoot(sHeader.pRoot);

    _InitialiseScreen(sHeader.sScreen);
    ArchClearScreen();

    _InitialiseGDT(&sHeader);
    WriteGDT();
    FlushTSS();

    _InitialiseAPIC(&sHeader);
    _InitialiseIDT();

    ArchRegisterException(0x06, InvalidOpCode);
    ArchRegisterException(0x0E, PageFault);
    ArchRegisterException(0x0D, GeneralProtectionFault);
    
    _InitialisePaging(&sHeader);

    _InitialiseSyscalls();
    
    // _LoadFont(GetRAMFile(L"robotolightboot.pcf"));

    // Load the kernel
    ((void SYSV (*)(sRAMFile *, PBYTE, QWORD)) (sHeader.qwKernelEntryRVA + MM_KERNEL_START))(sHeader.pRoot, g_arrPageBitmap, g_qwBitmapLength);

    for (;;) __asm__ volatile ("hlt");
}
