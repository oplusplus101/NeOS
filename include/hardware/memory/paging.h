
#ifndef __PAGING_H
#define __PAGING_H

#include <common/types.h>
#include <common/bootstructs.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 512

typedef struct
{
    BOOL bPresent            : 1;
    BOOL bWriteable          : 1;
    BOOL bUser               : 1;
    BOOL bWriteThrough       : 1;
    BOOL bCacheDisable       : 1;
    BOOL bAccessed           : 1;
    BOOL bDirty              : 1;
    BOOL bPageTableAttribute : 1;
    BOOL bGlobal             : 1;
    BYTE nAvailable          : 3;
    QWORD nAddress           : 52;
} __attribute__((packed)) sPageDirectoryEntry;

typedef struct
{
    sPageDirectoryEntry arrEntries[PAGE_TABLE_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) sPageTable;

void ReservePage(void *pAddress);
void ReservePages(void *pAddress, QWORD nPages);
void ReturnPage(void *pAddress);
void ReturnPages(void *pAddress, QWORD nPages);
void MapPage(void *pVirtualMemory, void *pPhysicalMemory);
void MapPageRange(void *pVirtualMemory, void *pPhysicalMemory, QWORD nPages);
void *AllocatePage();
void FreePage(void *pAddress);
void LoadPML4();

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                QWORD nMemoryMapSize, QWORD nMemoryDescriptorSize,
                QWORD nLoaderStart, QWORD nLoaderEnd);
#endif // __PAGING_H
