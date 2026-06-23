
#ifndef __ARCHIMPORTS_H
#define __ARCHIMPORTS_H

#include <common/types.h>

// TODO: Replace this with an includ from ARCH

#define PAGE_SIZE 4096
// Calculates the number of pages needed for n bytes
#define _BYTES_TO_PAGES(n) (((n) + PAGE_SIZE - 1) / PAGE_SIZE)

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3



#define PAGING_WRITEABLE 1
#define PAGING_UNCACHED 2
#define PAGING_USERMODE 4
#define PAGING_NOEXECUTE 6
#define PAGING_GLOBAL 8

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

PVOID __declspec(dllimport) SYSV ArchGetKernelPageTable();
void  __declspec(dllimport) SYSV ArchRefreshPages();
void  __declspec(dllimport) SYSV ArchLoadPageTable(PVOID pPageTable);
void  __declspec(dllimport) SYSV ArchMapPage(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, BYTE bFlags, BOOL bRefreshPage);
void  __declspec(dllimport) SYSV ArchMapPages(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwNumberOfPages, BYTE bFlags, BOOL bRefreshPage);
void  __declspec(dllimport) SYSV ArchUnMapPage(PVOID pPageTable, PVOID pVirtualAddress, BOOL bRefreshPage);
void  __declspec(dllimport) SYSV ArchUnMapPages(PVOID pPageTable, PVOID pVirtualAddress, QWORD qwNumberOfPages, BOOL bRefreshPage);
PVOID __declspec(dllimport) SYSV ArchGetPhysicalAddress(PVOID pPageTable, PVOID pVirtualAddress);
PVOID __declspec(dllimport) SYSV ArchAllocatePage();
PVOID __declspec(dllimport) SYSV ArchAllocatePagesContinuous(QWORD qwNumberOfPages);
void  __declspec(dllimport) SYSV ArchFreePage(PVOID pPage);
void  __declspec(dllimport) SYSV ArchFreePagesContinuous(PVOID pPages, QWORD qwNumberOfPages);

void  __declspec(dllimport) MS ArchPrintFormat(const PWCHAR wszFormat, ...);
void  __declspec(dllimport) SYSV ArchPrintFormatVariadic(const PWCHAR wszFormat, __builtin_va_list lst);
void  __declspec(dllimport) MS ArchLog(INT iType, const PWCHAR wszFormat, ...);
void  __declspec(dllimport) SYSV ArchLogVariadic(INT iType, const PWCHAR wszFormat, __builtin_va_list lst);

void  __declspec(dllimport) SYSV ArchDrawLine(INT x0, INT y0, INT x1, INT y1, sColour c);
void  __declspec(dllimport) SYSV ArchPutPixel(INT x, INT y, sColour c);


#endif // __ARCHIMPORTS_H
