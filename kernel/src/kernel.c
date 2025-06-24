
#include <neos.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/memory.h>
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
#include <events/syscallimpl.h>
#include <events/syscalls.h>
#include <events/timer.h>
#include <runtime/exceptions.h>
#include <runtime/process.h>
#include <runtime/driver.h>
#include <runtime/exe.h>


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

    InitGDT();
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
    RegisterSyscalls();

    // Override the previous exception handlers so that when a process crashes it doesn't kill the whole kernel.
    RegisterKernelExceptions();
    // 10th of a millisecond per cycle 
    InitProcessScheduler(100);
    PrintFormat("Process scheduler initialised\n");

    PrintFormat("Loading...\n");
    sList lstDrivers = LoadCFG(L"NeOS\\Drivers.cfg", &hdr);
    // sList lstConfig  = LoadCFG(L"NeOS\\NeOS.cfg", &hdr);

    // Load the drivers
    for (QWORD i = 0; i < lstDrivers.qwLength; i++)
    {
        sINIEntry *pEntry = GetListElement(&lstDrivers, i);
        if (!strcmp(pEntry->szName, "Enabled") && pEntry->szValue[0] == '1')
        {
            PrintFormat("Loading driver: %s\n", pEntry->szLabel);
            WCHAR wszDriverName[13];
            ZeroMemory(wszDriverName, sizeof(wszDriverName));
            for (int j = 0; pEntry->szLabel[j]; j++)
                wszDriverName[j] = pEntry->szLabel[j];
            
            WCHAR wszPath[256];
            strcpyW(wszPath, L"NeOS\\Drivers\\");
            strcatW(wszPath, wszDriverName);
            strcatW(wszPath, L".drv");
            PVOID pFile = hdr.GetFile(wszPath);
            _ASSERT(pFile != NULL, "Driver %w not found", wszDriverName);

            QWORD qwFileSize = hdr.GetFileSize(pFile);
            PVOID pData = KHeapAlloc(qwFileSize);
            hdr.ReadFile(pFile, pData);
            sExecutable sEXE = ParsePE32(pData);
            INT iPID = StartKernelProcess(pEntry->szLabel, &sEXE, PROC_PAUSED);
            sDriver sTask =
            {
                .iPID = iPID
            };
            strcpyW(sTask.wszName, wszDriverName);
            // AddListElement(&g_lstDrivers, &sTask);
            KHeapFree(pFile);
        }
    }
        
    // PrintFormat("Starting drivers...\n");
    // for (QWORD i = 0; i < g_lstDrivers.qwLength; i++)
    // {
    //     sKernelTask *pDriver = (sKernelTask *) GetListElement(&g_lstDrivers, i);
    //     SetProcessState(pDriver->iPID, PROC_RUNNING);
    // }
    
    EnableInterrupts(); 

    for (;;) __asm__ volatile ("hlt"); // Halt so less power is wasted
    __asm__ volatile ("cli\nhlt"); // Ensure the kernel does not exit at all
}
