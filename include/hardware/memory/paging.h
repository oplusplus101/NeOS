
#ifndef __PAGING_H
#define __PAGING_H

#include <common/types.h>
#include <common/bootstructs.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 512

#define ADDRESS_TO_PDP_INDEX(a) (((SIZE_T) (a) >> 39) % PAGE_TABLE_SIZE)
#define ADDRESS_TO_PD_INDEX(a)  (((SIZE_T) (a) >> 30) % PAGE_TABLE_SIZE)
#define ADDRESS_TO_PT_INDEX(a)  (((SIZE_T) (a) >> 21) % PAGE_TABLE_SIZE)
#define ADDRESS_TO_PE_INDEX(a)  (((SIZE_T) (a) >> 12) % PAGE_TABLE_SIZE)
#define PAGE_TO_ADDRESS(p)      ((SIZE_T) (p) << 12)
#define ADDRESS_TO_PAGE(a)      ((SIZE_T) (a) >> 12)

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
    SIZE_T nAddress          : 52;
} __attribute__((packed)) sPageTableEntry;

typedef struct
{
    sPageTableEntry arrEntries[PAGE_TABLE_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) sPageTable;

void ReservePage(PVOID pAddress);
void ReservePages(PVOID pAddress, QWORD nPages);
void ReturnPage(PVOID pAddress);
void ReturnPages(PVOID pAddress, QWORD nPages);
void MapPage(PVOID pVirtualAddress, PVOID pPhysicalAddress);
void MapPageRange(PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD nPages);
PVOID AllocatePage();
void FreePage(PVOID pAddress);
PVOID GetPhysicalAddress(PVOID pVirtualAddress);
void LoadPML4();

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                SIZE_T nMemoryMapSize, SIZE_T nMemoryDescriptorSize,
                SIZE_T nLoaderStart, SIZE_T nLoaderEnd);
#endif // __PAGING_H
