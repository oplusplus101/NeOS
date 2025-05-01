
#include <neos.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/screen.h>
#include <common/string.h>
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
#include <exe.h>

sList g_lstDrivers, g_lstModules;

sList LoadCFG(PWCHAR wszPath, sNEOSKernelHeader *pHeader)
{
    PVOID pFile = pHeader->GetFile(wszPath);
    _ASSERT(pFile, "File \"C:\\%s\" not found", wszPath);
    QWORD qwFileSize = pHeader->GetFileSize(pFile);
    PVOID szText = HeapAlloc(qwFileSize + 1);
    pHeader->ReadFile(pFile, szText);
    ((PBYTE) szText)[qwFileSize] = 0;
    sList lst = ParseINIFile(szText);
    HeapFree(pFile);
    HeapFree(szText);
    return lst;
}

void StartDriver()
{

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

// RBX is the x position
// RCX is the y position
void Syscall_KNeoGetCursorPosition(sCPUState *pCPUState)
{
    pCPUState->qwRBX = GetCursorX();
    pCPUState->qwRCX = GetCursorX();
}

// RBX is the x position
// RCX is the y position
void Syscall_KNeoSetCursorPosition(sCPUState *pCPUState)
{
    SetCursor(pCPUState->qwRBX, pCPUState->qwRCX);
}

// 0x0010 - 0x001F Driver and Module functions

// RBX is the address of the driver name
// RCX will then be the memory address of the allocated driver
void Syscall_KNeoGetDriver(sCPUState *pCPUState)
{
    
}

// RBX is the address of the module name
// RCX will then be the memory address of the allocated module
void Syscall_KNeoGetModule(sCPUState *pCPUState)
{
    
}

// RBX is the pointer to the driver
// RCX is the pointer to the function's name
// RDX will then contain the function
void Syscall_KNeoGetDriverFunction(sCPUState *pCPUState)
{
    
}

// RBX is the pointer to the module
// RCX is the pointer to the function's name
// RDX will then contain the function
void Syscall_KNeoGetModuleFunction(sCPUState *pCPUState)
{
    
}

// 0x0020 - 0x002F Memory functions

// RBX is the address of the pages
// RCX is the number of pages
// RDX contains the flags
void Syscall_KNeoMapPagesToIdentity(sCPUState *pCPUState)
{
    MapPageRangeToIdentity((PVOID) pCPUState->qwRBX, pCPUState->qwRCX, pCPUState->qwRDX);
}

// EBX is the virtual address
// RCX is the number of pages
// RDX is the physical address
// ??? contains the flags
void Syscall_KNeoMapPages(sCPUState *pCPUState)
{
    MapPageRangeToIdentity((PVOID) pCPUState->qwRBX, pCPUState->qwRCX, pCPUState->qwRDX);
}


// RCX contains the number of pages to allocate
// RBX will then be the address of the first page
void Syscall_KNeoAllocatePages(sCPUState *pCPUState)
{
    pCPUState->qwRBX = (QWORD) AllocateContinousPages(pCPUState->qwRCX);
}

// RBX is the address of the first page
// RCX contains the number of pages to free
void Syscall_KNeoFreePages(sCPUState *pCPUState)
{
    FreeContinousPages((PVOID) pCPUState->qwRBX, pCPUState->qwRCX);
}

// RCX is the size in bytes
// RBX will then contain the memory address
void Syscall_KNeoAllocateHeap(sCPUState *pCPUState)
{
    pCPUState->qwRBX = (QWORD) HeapAlloc(pCPUState->qwRBX);
}

// RBX is the old memory address
// RCX is the new size in bytes
// RDX will then contain the re-allocated memory address
void Syscall_KNeoReAllocateHeap(sCPUState *pCPUState)
{
    pCPUState->qwRDX = (QWORD) HeapReAlloc((PVOID) pCPUState->qwRBX, pCPUState->qwRCX);
}

// RBX contains the memory address
void Syscall_KNeoFreeHeap(sCPUState *pCPUState)
{
    HeapFree((PVOID) pCPUState->qwRBX);
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
    PrintString("Kernel loaded!\n");

    ImportPagingData(hdr.sPaging);
    PrintFormat("Paging initialised (%u bytes free)\n", hdr.sPaging.qwFreeMemory);
    
    InitHeap(hdr.qwHeapStart, hdr.qwHeapStart + NEOS_HEAP_SIZE);
    
    InitSyscalls();
    PrintFormat("Syscalls initialised to interrput 0x%02X\n", NEOS_SYSCALL_IRQ);
    RegisterSyscall(0x0000, 0, Syscall_KNeoPrintString);
    RegisterSyscall(0x0001, 0, Syscall_KNeoKernelPanic);
    RegisterSyscall(0x0002, 0, Syscall_KNeoGetCursorPosition);
    RegisterSyscall(0x0003, 0, Syscall_KNeoSetCursorPosition);
    
    RegisterSyscall(0x0010, 0, Syscall_KNeoGetDriver);
    RegisterSyscall(0x0011, 0, Syscall_KNeoGetModule);
    RegisterSyscall(0x0012, 0, Syscall_KNeoGetDriverFunction);
    RegisterSyscall(0x0013, 0, Syscall_KNeoGetModuleFunction);
    
    RegisterSyscall(0x0020, 0, Syscall_KNeoMapPagesToIdentity);
    RegisterSyscall(0x0021, 0, Syscall_KNeoMapPages);
    RegisterSyscall(0x0022, 0, Syscall_KNeoAllocatePages);
    RegisterSyscall(0x0023, 0, Syscall_KNeoFreePages);
    RegisterSyscall(0x0024, 0, Syscall_KNeoAllocateHeap);
    RegisterSyscall(0x0025, 0, Syscall_KNeoReAllocateHeap);
    RegisterSyscall(0x0026, 0, Syscall_KNeoFreeHeap);

    
    InitProcessScheduler();
    RegisterInterrupt(32, ScheduleProcesses);
    PrintFormat("Process scheduler initialised\n");
    
    // sList lstModules = LoadCFG("NeOS\\Modules.cfg", &hdr);
    // sList lstDrivers = LoadCFG("NeOS\\Drivers.cfg", &hdr);
    // sList lstConfig  = LoadCFG("NeOS\\NeOS.cfg", &hdr);

    PrintFormat("Loading...\n");

    // for (QWORD i = 0; i < lstDrivers.qwLength; i++)
    // {
    //     sINIEntry *pEntry = GetListElement(&lstDrivers, i);
    //     if (!strcmp(pEntry->szName, "Enabled") && pEntry->szValue[0] == '1')
    //     {
    //         CHAR szPath[256];
    //         strcpy(szPath, "NeOS\\Drivers\\");
    //         strcat(szPath, pEntry->szLabel);
    //         strcat(szPath, ".drv");
    //         PrintFormat("AL: %s\n", szPath);
    //         PVOID pFile = hdr.GetFile(szPath);
    //         _ASSERT(pFile != NULL, "Driver %s not found on filesystem", szPath);

    //         QWORD qwFileSize = hdr.GetFileSize(pFile);
    //         PVOID pData = HeapAlloc(qwFileSize);
    //         hdr.ReadFile(pFile, pData);
    //         /* sExecutable sEXE =  */ParsePE32(pData, qwFileSize);
    //         HeapFree(pFile);
    //     }
    // }
    

    EnableInterrupts();
    for (;;);
}
