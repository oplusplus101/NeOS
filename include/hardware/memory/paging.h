
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

#define PF_PRESENT 1
#define PF_WRITEABLE 2
#define PF_USER 4
#define PF_WRITETHROUGH 8
#define PF_CACHEDISABLE 16
#define PF_ACCESSED 32
#define PF_DIRTY 64
#define PF_PAGETABLEATTRIBUTE 128
#define PF_GLOBAL 256


typedef struct
{
    WORD nFlags     : 12;
    SIZE_T nAddress : 52;
} __attribute__((packed)) sPageTableEntry;

typedef struct
{
    sPageTableEntry arrEntries[PAGE_TABLE_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) sPageTable;

void ReservePage(PVOID pAddress);
void ReservePages(PVOID pAddress, QWORD nPages);
void ReturnPage(PVOID pAddress);
void ReturnPages(PVOID pAddress, QWORD nPages);
void MapPage(PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD nFlags);
void MapPageRange(PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD nPages, WORD nFlags);
PVOID AllocatePage();
void FreePage(PVOID pAddress);
PVOID GetPhysicalAddress(PVOID pVirtualAddress);
void LoadPML4();

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                SIZE_T nMemoryMapSize, SIZE_T nMemoryDescriptorSize,
                SIZE_T nLoaderStart, SIZE_T nLoaderEnd);
#endif // __PAGING_H
