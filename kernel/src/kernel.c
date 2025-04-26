
#include <neos.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/screen.h>
#include <common/types.h>
#include <common/panic.h>
#include <common/math.h>
#include <memory/heap.h>
#include <common/ini.h>
#include <hardware/ports.h>
#include <hardware/idt.h>
#include <hardware/gdt.h>
#include <syscalls.h>
#include <process.h>
#include <timer.h>

sList g_lstDrivers, g_lstModules;

sList LoadINI(PCHAR szPath, sNEOSKernelHeader *pHeader)
{
    PVOID pFile = pHeader->GetFile(szPath);
    _ASSERT(pConfigFilePointer, "File \"C:\\NeOS\\NeOS.cfg\" not found");
    QWORD qwFileSize = pHeader->GetFileSize(pFile);
    PVOID szText = HeapAlloc(qwFileSize + 1);
    pHeader->ReadFile(pFile, szText);
    ((PBYTE) szText)[qwFileSize] = 0;
    sList lst = ParseINIFile(szText);
    HeapFree(pFile);
    HeapFree(szText);
    return lst;
}

// 0x0000 - 0x000F Print functions

// RBX is the address of a zero-terminated string
void Syscall_KNeoPrintString(sCPUState *pCPUState)
{
    PrintString((PCHAR) pCPUState->qwRBX);
}

// RBX is the address of a zero-terminated string
void Syscall_KNeoKernelPanic(sCPUState *pCPUState)
{
    _KERNEL_PANIC((PCHAR) pCPUState->qwRBX);
}

// 0x0010 - 0x001F Driver and Module functions

// RBX is the address of the driver name
// RCX will then be the memory address of the allocated driver
void Syscall_KNeoLoadDriver(sCPUState *pCPUState)
{
    
}

// RBX is the address of the module name
// RCX will then be the memory address of the allocated module
void Syscall_KNeoLoadModule(sCPUState *pCPUState)
{
    
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
    RegisterSyscall(0x0000, 0, Syscall_KNeoPrintString);
    RegisterSyscall(0x0001, 0, Syscall_KNeoKernelPanic);
    RegisterSyscall(0x0010, 0, Syscall_KNeoLoadDriver);
    RegisterSyscall(0x0011, 0, Syscall_KNeoLoadModule);
    
    InitProcessScheduler();
    RegisterInterrupt(32, ScheduleProcesses);
    PrintFormat("Process scheduler initialised\n");
    
    PVOID pModuleFilePointer = hdr.GetFile("NeOS/Modules.cfg");
    PVOID pDriverFilePointer = hdr.GetFile("NeOS/Drivers.cfg");
    PVOID pConfigFilePointer = hdr.GetFile("NeOS/NeOS.cfg");
    _ASSERT(pModuleFilePointer, "File \"C:\\NeOS\\Modules.cfg\" not found");
    _ASSERT(pDriverFilePointer, "File \"C:\\NeOS\\Drivers.cfg\" not found");
    _ASSERT(pConfigFilePointer, "File \"C:\\NeOS\\NeOS.cfg\" not found");
    PVOID pModuleFileText = HeapAlloc(hdr.GetFile(pModuleFilePointer) + 1);
    PVOID pDriverFileText = HeapAlloc(hdr.GetFile(pDriverFilePointer) + 1);
    PVOID pConfigFileText = HeapAlloc(hdr.GetFile(pDriverFilePointer) + 1);
    hdr.ReadFile(pModuleFilePointer, pModuleFileText);
    hdr.ReadFile(pDriverFilePointer, pDriverFileText);
    sList lstModules = ParseINIFile(pModuleFileText);
    sList lstDrivers = ParseINIFile(pDriverFileText);

    EnableInterrupts();
    for (;;);
}
