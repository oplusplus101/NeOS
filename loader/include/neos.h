
#ifndef __NEOS_H
#define __NEOS_H

#include <common/types.h>
#include <common/bootstructs.h>
#include <memory/paging.h>
#include <memory/bitmap.h>
#include <memory/heap.h>

typedef struct
{
    BYTE r, g, b;
} __attribute__((packed)) color_t;

#define _RGB(r, g, b) ((color_t) { (r), (g), (b) })

#define NEOS_SUCCESS 0
#define NEOS_FAILURE 1

#define _NEOS_SUCCESS(x) ((x) == 0)

#define NEOS_BACKGROUND_COLOR _RGB(0, 0, 0)
#define NEOS_FOREGROUND_COLOR _RGB(168, 168, 168)
#define NEOS_ERROR_COLOR      _RGB(255, 0, 0)
#define NEOS_MINIMUM_RAM_SIZE (1024 * 1024 * 1024 * 1) // 1 GiB minimum ram
#define NEOS_HEAP_SIZE        (1024 * 1024 * 10)       // 10 MiB heap
#define NEOS_STACK_SIZE       (1024 * 32)              // 32 KiB heap
#define NEOS_KERNEL_LOC_LOW   0x200000
#define NEOS_KERNEL_LOC_HIGH  0x300000
#define NEOS_SYSCALL_IRQ      0x81                   // 0x81 so it doesn't conflict with the POSIX interrupts

typedef struct
{
    sGOPData sGOP;
    sPagingData sPaging;
    BYTE nDriveNum;
    sHeap *pKernelHeap;

    // Returns a handle to the file, or NULL if not found
    PVOID (*LoaderOpenFile)(PWCHAR wszFileName);
    // Returns the file size in bytes
    QWORD (*LoaderGetFileSize)(PVOID pFile);
    // Will read no more bytes than specified by qwSize, will return the total number of bytes read .
    QWORD (*LoaderReadFile)(PVOID pFile, PVOID pBuffer, QWORD qwSize);
    // Frees the file handle
    void  (*LoaderCloseFile)(PVOID pFile);
    void  (*LoaderSeekFile)(PVOID, QWORD);
    QWORD (*LoaderTellFile)(PVOID);

    void (*PrintFormat)(const PWCHAR wszFormat, ...);
    void (*PrintString)(PCHAR szString);
    void (*PrintBytes)(PVOID pBuffer, QWORD qwLength, WORD wBytesPerLine, BOOL bASCII);
    void (*PrintChar)(CHAR c);
    void (*Log)(INT iType, const PWCHAR wszFormat, ...);
    INT  (*GetCursorX)();
    INT  (*GetCursorY)();
    void (*SetCursor)(INT x, INT y);
    void (*SetFGColor)(color_t c);
    void (*SetBGColor)(color_t c);
    void (*ClearScreen)();
    INT  (*GetScreenWidth)();
    INT  (*GetScreenHeight)();
} sNEOSKernelHeader;

#endif // __NEOS_H
