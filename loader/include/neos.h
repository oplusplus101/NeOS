
#ifndef __NEOS_H
#define __NEOS_H

#include <common/types.h>
#include <common/bootstructs.h>
#include <memory/paging.h>
#include <memory/bitmap.h>

#define NEOS_BACKGROUND_COLOR _RGB(0, 0, 0)
#define NEOS_FOREGROUND_COLOR _RGB(168, 168, 168)
#define NEOS_ERROR_COLOR      _RGB(255, 0, 0)
#define NEOS_MINIMUM_RAM_SIZE 1024 * 1024 * 1024 * 1 // 1 GiB minimum ram
#define NEOS_HEAP_SIZE        1024 * 1024 * 10       // 10 MiB heap
#define NEOS_HEAP_START       0x100000000
#define NEOS_KERNEL_LOCATION  0x200000

typedef struct
{
    sGOPData sGOP;
    sPagingData sPaging;
    BYTE nDriveNum;
} __attribute__((packed)) sNEOSKernelHeader;

#endif // __NEOS_H
