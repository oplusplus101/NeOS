
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
} __attribute__((packed)) sColour;

typedef QWORD (*ISR)(QWORD);
typedef QWORD (*ESR)(QWORD, BYTE);

#define _RGB(r, g, b) ((sColour) { (r), (g), (b) })

// Extra context
#define NEOS_ERROR_KERNEL         0x00010000
#define NEOS_ERROR_LIBRARY        0x00020000
#define NEOS_ERROR_EXECUTABLE     0x00030000
#define NEOS_ERROR_DISK           0x00040000
#define NEOS_ERROR_DRIVER         0x00050000
#define NEOS_ERROR_MEMORY         0x00060000
#define NEOS_OUT_OF_BOUNDS        0x00060001

// Status
#define NEOS_SUCCESS              0x00000000
#define NEOS_FAILURE              0x0000FFFF

// Read operations
#define NEOS_OPEN_FILE_FAILURE    0x00001000
#define NEOS_READ_FILE_FAILURE    0x00001001
#define NEOS_LOAD_FAILURE         0x00001002

// Write operations
#define NEOS_WRITE_FILE_FAILURE   0x00002000

// Other
#define NEOS_CLOSE_FILE_FAILURE   0x00003000

// Memory
#define NEOS_ALLOCATION_FAILURE   0x00004000
#define NEOS_FREEING_FAILURE      0x00004001
#define NEOS_OUT_OF_HEAP_SPACE    0x00004002
#define NEOS_CREATE_HEAP_FAILURE  0x00004003
#define NEOS_DESTROY_HEAP_FAILURE 0x00004004


#define _SUCCESSFUL(x) (((x) & 0x0000FFFF) == 0)
#define NEOS_BACKGROUND_COLOUR       _RGB(0, 0, 0)
#define NEOS_FOREGROUND_COLOUR       _RGB(168, 168, 168)
#define NEOS_ERROR_COLOUR            _RGB(255, 0, 0)
#define NEOS_MINIMUM_RAM_SIZE        (1024 * 1024 * 1024 * 1) // 1 GiB minimum ram
#define NEOS_HEAP_SIZE               (1024 * 1024 * 10)       // 10 MiB heap
#define NEOS_STACK_SIZE              (1024 * 32)              // 32 KiB heap
#define NEOS_SYSCALL_IRQ             0x81                     // 0x81 so it doesn't conflict with the POSIX interrupts
#define NEOS_KERNEL_PHYSICAL_ADDRESS 0x000000

// Memory map
#define MM_USER_SPACE_START  0x0000000000000000
#define MM_KERNEL_HEAP_START 0xFFFFF70000000000
#define MM_KERNEL_START      0xFFFFF80000000000
#define MM_ARCH_START        0xFFFFF81000000000
#define MM_DRIVERS_START     0xFFFFF82000000000

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
    void (*SetFGColor)(sColour c);
    void (*SetBGColor)(sColour c);
    void (*ClearScreen)();
    INT  (*GetScreenWidth)();
    INT  (*GetScreenHeight)();
    void (*RegisterException)(BYTE n, ESR pESR);
    void (*RegisterInterrupt)(BYTE n, ISR pISR);

} sNEOSKernelHeader;

#endif // __NEOS_H
