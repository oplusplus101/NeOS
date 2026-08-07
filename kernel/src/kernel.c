
#include <neos.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/types.h>
#include <common/panic.h>
#include <common/math.h>
#include <common/ini.h>
#include <memory/heap.h>
#include <io/io.h>
#include <runtime/exceptions.h>
#include <runtime/process.h>
#include <runtime/objects.h>
#include <runtime/driver.h>
#include <runtime/exe.h>
#include <archimports.h>
#include <file.h>

#ifdef FUNCTION_INSTRUMENTATIONS

static BOOL g_bDebugReady = false;
static DWORD g_dwDepth = 0;

__attribute__((no_instrument_function))
void __cyg_profile_func_enter(PVOID func, PVOID caller)
{
    if (g_bDebugReady != 1)
        return;
    
    PrintFormat(L"+%u %p\n", g_dwDepth, func);

    g_dwDepth++;
}

__attribute__((no_instrument_function))
void __cyg_profile_func_exit(PVOID func, PVOID caller)
{
    if (g_bDebugReady != 1)
        return;
    g_dwDepth--;
    PrintFormat(L"-%u %p\n", g_dwDepth, func);
}

#endif

// Wraps a function inside an interrupt disable and enable instruction.
// This macro doesn't return anything
#define _ATOMIC(func, ...) { __asm__ volatile ("cli"); \
                           func(__VA_ARGS__); \
                           __asm__ volatile ("sti"); }

void Test()
{
    __asm__ volatile ("mov $0x71771E2, %r9");
    for (;;) __asm__ volatile ("hlt");
}

__attribute__((aligned(4096)))
void TestUserThread()
{
    __asm__ volatile ("mov $0xB00B1E2, %r9");
    for (;;) __asm__ volatile ("hlt");
        asm volatile (
        ".rept 4096\n\t"
        "nop\n\t"
        ".endr\n\t"
    );
}

DWORD lcg_parkmiller(DWORD *state)
{
	QWORD product = (QWORD)*state * 48271;
	DWORD x = (product & 0x7fffffff) + (product >> 31);

	x = (x & 0x7fffffff) + (x >> 31);
	return *state = x;
}

void SYSV KernelMain(sRAMFile *pRoot)
{
#ifdef FUNCTION_INSTRUMENTATIONS
    g_bDebugReady = true;
    g_dwDepth = 0;
#endif

    ArchLog(LOG_LOG, L"Kernel loaded!");
    
    SetKernelHeap(&sKernelHeap);

    ArchLog(LOG_LOG, L"Heap initialised!");

    STATUS s = InitObjectManager();
    _ASSERT(_SUCCESSFUL(s), L"Failed to initialise the object manager");

    // TODO: Move these into their own init function
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Devices"), NULL, NULL)), L"Could not create object directory \"Devices\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Processes"), NULL, NULL)), L"Could not create object directory \"Processes\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Threads"), NULL, NULL)), L"Could not create object directory \"Threads\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Cache"), NULL, NULL)), L"Could not create object directory \"Cache\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Cache\\Libraries"), NULL, NULL)), L"Could not create object directory \"Cache\\Libraries\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Cache\\Libraries\\Kernel"), NULL, NULL)), L"Could not create object directory \"Cache\\Libraries\\Kernel\"");
    _ASSERT(_SUCCESSFUL(CreateObjectDirectory(MakeString(L"Cache\\Libraries\\User"), NULL, NULL)), L"Could not create object directory \"Cache\\Libraries\\User\"");

    ArchLog(LOG_LOG, L"Object manager initialised");
    
    InitialiseBootFiles(pRoot);
    // 10th of a millisecond per cycle
    InitProcessScheduler(1000);
    ArchLog(LOG_LOG, L"Process scheduler initialised");

    InitDriverManager();
    ArchLog(LOG_LOG, L"Driver manager initialised");

    InitIOManager();
    ArchLog(LOG_LOG, L"I/O manager initialised");

    InitialiseSyscalls();
    ArchLog(LOG_LOG, L"User-mode system calls initialised");
    
    InitExecutableLoader();

    ArchLog(LOG_LOG, L"Loading drivers...");
    sRAMFile *pDriversFile = GetRAMFile(L"NeOS\\Drivers.cfg");

    // The config file must be copied, as the file data isn't null terminated.
    PCHAR szDriversFileText = KHeapAlloc(pDriversFile->qwFileSize + 1);
    memcpy(szDriversFileText, pDriversFile->pData, pDriversFile->qwFileSize);
    szDriversFileText[pDriversFile->qwFileSize] = 0;

    sList lstDrivers = ParseINIFile(szDriversFileText);
    // sList lstConfig  = LoadCFG(L"NeOS\\NeOS.cfg", &hdr);
    
    // Load the drivers
    for (QWORD i = 0; i < lstDrivers.qwLength; i++)
    {
        sINIEntry *pEntry = GetListElement(&lstDrivers, i);
        if (!strcmp(pEntry->szName, "Enabled") && pEntry->szValue[0] == '1')
        {
            ArchLog(LOG_LOG, L"Loading driver: %s", pEntry->szLabel);
            WCHAR wszDriverName[256];
            ZeroMemory(wszDriverName, sizeof(wszDriverName));
            for (int j = 0; pEntry->szLabel[j]; j++)
                wszDriverName[j] = pEntry->szLabel[j];

            INT iStatus = LoadDriver(wszDriverName);

            if (_SUCCESSFUL(iStatus))
                ArchLog(LOG_LOG, L"Driver '%S' loaded!", wszDriverName);
            else
                ArchLog(LOG_ERROR, L"Failed to load driver '%S' [%08X]", wszDriverName, iStatus);
        }
    }

    StartThread(L"Test thread", Test, 4096 * 2, false, 0, PROC_RUNNING);
    StartThread(L"User thread", TestUserThread, 4096 * 2, true, 0, PROC_RUNNING);

    ArchLog(LOG_LOG, L"Drivers loaded!");
    ArchLog(LOG_LOG, L"Enabling interrupts...");

    // TODO: Move all platform specific calls to the ARCH layer
    // ArchEnableInterrupts();
    __asm__ volatile ("sti");

    for (;;) __asm__ volatile ("hlt"); // Halt so less power is wasted
}
