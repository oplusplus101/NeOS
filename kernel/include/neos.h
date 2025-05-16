
#ifndef __NEOS_H
#define __NEOS_H

#include <common/types.h>
#include <common/bootstructs.h>
#include <memory/paging.h>
#include <memory/bitmap.h>
#include <memory/list.h>
#include <memory/heap.h>

#define NEOS_BACKGROUND_COLOR _RGB(0, 0, 0)
#define NEOS_FOREGROUND_COLOR _RGB(168, 168, 168)
#define NEOS_ERROR_COLOR      _RGB(255, 0, 0)
#define NEOS_MINIMUM_RAM_SIZE (1024 * 1024 * 1024 * 1) // 1 GiB minimum ram
#define NEOS_HEAP_SIZE        (1024 * 1024 * 10)       // 10 MiB heap
#define NEOS_KERNEL_LOC_LOW   0x200000
#define NEOS_KERNEL_LOC_HIGH  0x300000
#define NEOS_SYSCALL_IRQ      0x81                   // 0x81 so it doesn't conflict with the POSIX interrupts

typedef struct
{
    sGOPData sGOP;
    sPagingData sPaging;
    BYTE nDriveNum;
    sHeap *pKernelHeap;
    // These functions need to exist in order to avoid a chicken and egg type scenario, i.e. you are trying to load the filesystem module with the filesystem module
    PVOID (*GetFile)(PWCHAR wszFilename);
    QWORD (*GetFileSize)(PVOID pFile);
    void  (*ReadFile)(PVOID pFile, PVOID pBuffer);
} sNEOSKernelHeader;

#endif // __NEOS_H
