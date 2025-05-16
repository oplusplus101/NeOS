
#include <neos.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/screen.h>
#include <common/string.h>
#include <common/types.h>
#include <common/panic.h>
#include <common/math.h>
#include <common/ini.h>
#include <memory/heap.h>
#include <hardware/ports.h>
#include <hardware/idt.h>
#include <hardware/gdt.h>
#include <syscallimpl.h>
#include <syscalls.h>
#include <process.h>
#include <timer.h>
#include <exe.h>

sList g_lstDrivers, g_lstModules;

void __stack_chk_fail(void)
{
	_KERNEL_PANIC("Stack smashing detected");
}

sList LoadCFG(PWCHAR wszPath, sNEOSKernelHeader *pHeader)
{
    PVOID pFile = pHeader->GetFile(wszPath);
    _ASSERT(pFile, "File \"C:\\%s\" not found", wszPath);
    QWORD qwFileSize = pHeader->GetFileSize(pFile);
    PVOID szText = KHeapAlloc(qwFileSize + 1);
    pHeader->ReadFile(pFile, szText);
    ((PBYTE) szText)[qwFileSize] = 0;
    sList lst = ParseINIFile(szText);
    KHeapFree(pFile);
    KHeapFree(szText);
    return lst;
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
    SetKernelHeap(hdr.pKernelHeap);
    
    InitSyscalls();
    PrintFormat("Syscalls initialised to interrput 0x%02X\n", NEOS_SYSCALL_IRQ);
    RegisterSyscall(0x0000, 0, Syscall_KNeoPrintString);
    RegisterSyscall(0x0001, 0, Syscall_KNeoKernelPanic);
    RegisterSyscall(0x0002, 0, Syscall_KNeoGetCursorPosition);
    RegisterSyscall(0x0003, 0, Syscall_KNeoSetCursorPosition);
    RegisterSyscall(0x0004, 0, Syscall_KNeoClearScreen);
    
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

    RegisterSyscall(0x0030, 0, Syscall_KNeoRegisterSyscall);
    RegisterSyscall(0x0031, 0, Syscall_KNeoClearSyscall);

    InitProcessScheduler();
    RegisterInterrupt(32, ScheduleProcesses);
    PrintFormat("Process scheduler initialised\n");

    PVOID pFile = hdr.GetFile(L"Programs\\test.exe");
    PrintFormat("LOC: %p\n", pFile);
    PrintFormat("File loaded!\n");
    QWORD qwFileSize = hdr.GetFileSize(pFile);
    PVOID pData = KHeapAlloc(qwFileSize);
    _ASSERT(pData, "Could not allocate SZ: %d", qwFileSize);
    hdr.ReadFile(pFile, pData);
    PrintFormat("LOC: %p\n", pData);
    
    PrintFormat("File read!\n");
    sExecutable sEXE = ParsePE32(pData);
    PrintFormat("Starting executable..\n");
    StartProcessExecutable("TEST", 0, 0, &sEXE);
    
    PrintFormat("Loading...\n");
    // sList lstModules = LoadCFG("NeOS\\Modules.cfg", &hdr);
    // sList lstDrivers = LoadCFG(L"NeOS\\Drivers.cfg", &hdr);
    // sList lstConfig  = LoadCFG("NeOS\\NeOS.cfg", &hdr);

    
    // for (QWORD i = 0; i < lstDrivers.qwLength; i++)
    // {
    //     sINIEntry *pEntry = GetListElement(&lstDrivers, i);
    //     if (!strcmp(pEntry->szName, "Enabled") && pEntry->szValue[0] == '1')
    //     {
    //         PrintFormat("Found driver: %s\n", pEntry->szName);
    //         // WCHAR szPath[256];
    //         // strcpyW(szPath, L"NeOS\\Drivers\\");
    //         // strcatW(szPath, pEntry->szLabel);
    //         // strcatW(szPath, L".drv");
    //         // PrintFormat("AL: %s\n", szPath);
    //         // PVOID pFile = hdr.GetFile(szPath);
    //         // _ASSERT(pFile != NULL, "Driver %s not found on filesystem", szPath);

    //         // QWORD qwFileSize = hdr.GetFileSize(pFile);
    //         // PVOID pData = KHeapAlloc(qwFileSize);
    //         // hdr.ReadFile(pFile, pData);
    //         // // /* sExecutable sEXE =  */ParsePE32(pData, qwFileSize);
    //         // KHeapFree(pFile);
    //     }
    // }
    
    // for (;;);
    EnableInterrupts(); 

    for (;;);
    __asm__ volatile ("cli\nhlt"); // Ensure the kernel does not exit at all
}
