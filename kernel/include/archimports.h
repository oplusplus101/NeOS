
#ifndef __ARCHIMPORTS_H
#define __ARCHIMPORTS_H

#include <common/types.h>

// TODO: Replace this with an includes from ARCH
typedef struct
{
    QWORD qwSyscallNumber;
    QWORD qwParam0;
    QWORD qwParam1;
    QWORD qwParam2;
    QWORD qwParam3;
    QWORD qwParam4;
    QWORD qwParam5;
    QWORD qwParam6;
} __attribute((packed)) sSyscallFrame;

typedef PHYSICAL_ADDRESS (SYSV *PageAllocatorFunction)();

#define PAGING_PRESENT            1
#define PAGING_WRITEABLE          2
#define PAGING_USERMODE           4
#define PAGING_WRITETHROUGH       8
#define PAGING_UNCACHED           16
#define PAGING_ACCESSED           32
#define PAGING_DIRTY              64
#define PAGING_HUGE 128
#define PAGING_GLOBAL             256
#define PAGING_NOEXECUTE          512

#define PAGE_SIZE 4096
// Calculates the number of pages needed for n bytes
#define _BYTES_TO_PAGES(n) (((n) + PAGE_SIZE - 1) / PAGE_SIZE)

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

void  __declspec(dllimport) SYSV ArchClearScreen();
void  __declspec(dllimport) SYSV ArchSetFGColour(sColour c);
void  __declspec(dllimport) SYSV ArchSetBGColour(sColour c);
void  __declspec(dllimport) SYSV ArchPrintChar(WCHAR c);
INT   __declspec(dllimport) SYSV ArchGetCursorX();
INT   __declspec(dllimport) SYSV ArchGetCursorY();
void  __declspec(dllimport) SYSV ArchSetCursor(INT x, INT y);
INT   __declspec(dllimport) SYSV ArchTextWidth();
INT   __declspec(dllimport) SYSV ArchTextHeight();
INT   __declspec(dllimport) SYSV ArchScreenWidth();
INT   __declspec(dllimport) SYSV ArchScreenHeight();
BOOL  __declspec(dllimport) SYSV ArchGraphicsSupported();
void  __declspec(dllimport) SYSV ArchPutPixel(INT x, INT y, sColour c);

PVOID __declspec(dllimport) SYSV ArchGetKernelPageTable();
void  __declspec(dllimport) SYSV ArchRefreshPages();
void  __declspec(dllimport) SYSV ArchLoadPageTable(PVOID pPageTable);
PVOID __declspec(dllimport) SYSV ArchCloneKernelPageTable(PageAllocatorFunction pAllocator);
void  __declspec(dllimport) SYSV ArchMapPage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PHYSICAL_ADDRESS PhysicalAddress, WORD wFlags, PageAllocatorFunction pAllocator);
WORD  __declspec(dllimport) SYSV ArchReadPageTableEntry(PVOID pPageTable, VIRTUAL_ADDRESS pVirtualAddress, PHYSICAL_ADDRESS *pPhysicalAddress);
void  __declspec(dllimport) SYSV ArchInvalidatePage(VIRTUAL_ADDRESS Page);


void  __declspec(dllimport) MS ArchPrintFormat(const PWCHAR wszFormat, ...);
void  __declspec(dllimport) SYSV ArchPrintFormatVariadic(const PWCHAR wszFormat, __builtin_va_list lst);
void  __declspec(dllimport) MS ArchLog(INT iType, const PWCHAR wszFormat, ...);
void  __declspec(dllimport) SYSV ArchLogVariadic(INT iType, const PWCHAR wszFormat, __builtin_va_list lst);

void  __declspec(dllimport) SYSV ArchDrawLine(INT x0, INT y0, INT x1, INT y1, sColour c);
void  __declspec(dllimport) SYSV ArchPutPixel(INT x, INT y, sColour c);


// Process stuff
void  __declspec(dllimport) SYSV ArchSetKernelStackTop(QWORD qwSP);
void  __declspec(dllimport) SYSV ArchCreateTaskContext(QWORD qwEntryPoint, BOOL bUser, PVOID pStackTop, PVOID pDestination);
QWORD __declspec(dllimport) SYSV ArchGetTaskContextSize();
void  __declspec(dllimport) SYSV ArchRegisterSchedulerFunction(PVOID (*pCallback)(PVOID));
void  __declspec(dllimport) SYSV ArchSetSchedulerFrequency(DWORD qwHZ);

void  __declspec(dllimport) SYSV ArchRegisterSyscallHandler(QWORD (SYSV *pHandler)(sSyscallFrame *pFrame));
void  __declspec(dllimport) SYSV ArchRegisterPageFaultHandler(void (SYSV *pHandler)(VIRTUAL_ADDRESS FaultAddress, WORD wFlags, BOOL bInstructionFetch));


#endif // __ARCHIMPORTS_H
