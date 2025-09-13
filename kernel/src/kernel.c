
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
#include <common/log.h>
#include <memory/heap.h>
#include <hardware/ports.h>
#include <hardware/idt.h>
#include <hardware/gdt.h>
#include <io/syscallimpl.h>
#include <io/syscalls.h>
#include <io/timer.h>
#include <io/io.h>
#include <runtime/exceptions.h>
#include <runtime/process.h>
#include <runtime/objects.h>
#include <runtime/driver.h>
#include <runtime/exe.h>


void __stack_chk_fail(void)
{
    _KERNEL_PANIC(L"Stack smashing detected");
}

sList LoadCFG(PWCHAR wszPath, sNEOSKernelHeader *pHeader)
{
    PVOID pFile = pHeader->GetFile(wszPath);
    _ASSERT(pFile, L"File \"C:\\%w\" not found", wszPath);
    QWORD qwFileSize = pHeader->GetFileSize(pFile);
    PVOID szText = KHeapAlloc(qwFileSize + 1);
    _ASSERT(szText, L"Could not allocate memory for CFG file \"C:\\%w\"", wszPath);
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
    Log(LOG_LOG, L"Kernel loaded!");

    ImportPagingData(hdr.sPaging);
    Log(LOG_LOG, L"Paging initialised (%u bytes free)", hdr.sPaging.qwFreeMemory);
    SetKernelHeap(hdr.pKernelHeap);
    
    InitSyscalls();
    Log(LOG_LOG, L"Syscalls initialised to interrupt 0x%02X", NEOS_SYSCALL_IRQ);
    RegisterSyscalls();

    // Override the previous exception handlers so that when a process crashes it doesn't kill the whole kernel.
    RegisterKernelExceptions();
    // 10th of a millisecond per cycle 
    InitProcessScheduler(100);
    Log(LOG_LOG, L"Process scheduler initialised");
    
    InitDriverManager();
    Log(LOG_LOG, L"Driver manager initialised");
    InitObjectManager();
    CreateObjectDirectory(L"Devices");
    Log(LOG_LOG, L"Object manager initialised");

    InitIOManager();
    Log(LOG_LOG, L"I/O manager initialised");

    
    Log(LOG_LOG, L"Loading drivers...");
    sList lstDrivers = LoadCFG(L"NeOS\\Drivers.cfg", &hdr);
    // sList lstConfig  = LoadCFG(L"NeOS\\NeOS.cfg", &hdr);

    // Load the drivers
    for (QWORD i = 0; i < lstDrivers.qwLength; i++)
    {
        sINIEntry *pEntry = GetListElement(&lstDrivers, i);
        if (!strcmp(pEntry->szName, "Enabled") && pEntry->szValue[0] == '1')
        {
            Log(LOG_LOG, L"Loading driver: %s", pEntry->szLabel);
            WCHAR wszDriverName[256];
            ZeroMemory(wszDriverName, sizeof(wszDriverName));
            for (int j = 0; pEntry->szLabel[j]; j++)
                wszDriverName[j] = pEntry->szLabel[j];
            
            WCHAR wszPath[256];
            strcpyW(wszPath, L"NeOS\\Drivers\\");
            strcatW(wszPath, wszDriverName);
            strcatW(wszPath, L".drv");
            PVOID pFile = hdr.GetFile(wszPath);
            _ASSERT(pFile != NULL, L"Driver %w not found", wszDriverName);

            QWORD qwFileSize = hdr.GetFileSize(pFile);
            PVOID pData = KHeapAlloc(qwFileSize);
            hdr.ReadFile(pFile, pData);
            sExecutable sEXE = ParsePE32(pData);
            /* INT iStatus = */ LoadDriver(&sEXE, wszDriverName);
            KHeapFree(pFile);
        }
    }

    Log(LOG_LOG, L"Drivers loaded!");
    Log(LOG_LOG, L"Enabling interrupts...");

    EnableInterrupts(); 
    
    for (;;) __asm__ volatile ("hlt"); // Halt so less power is wasted
    __asm__ volatile ("cli\nhlt"); // Ensure the kernel does not exit at all
}
