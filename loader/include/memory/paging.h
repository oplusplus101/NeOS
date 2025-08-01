
#ifndef __MEMORY__PAGING_H
#define __MEMORY__PAGING_H

#include <common/types.h>
#include <common/bootstructs.h>
#include <memory/bitmap.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 512

#define _ADDRESS_TO_PDP_INDEX(a) (((QWORD) (a) >> 39) % PAGE_TABLE_SIZE)
#define _ADDRESS_TO_PD_INDEX(a)  (((QWORD) (a) >> 30) % PAGE_TABLE_SIZE)
#define _ADDRESS_TO_PT_INDEX(a)  (((QWORD) (a) >> 21) % PAGE_TABLE_SIZE)
#define _ADDRESS_TO_PE_INDEX(a)  (((QWORD) (a) >> 12) % PAGE_TABLE_SIZE)
#define _BYTES_TO_PAGES(n)       (((QWORD) (n) + PAGE_SIZE - 1) / PAGE_SIZE)
#define _PAGE_TO_ADDRESS(p)      ((QWORD) (p) << 12)
#define _ADDRESS_TO_PAGE(a)      ((QWORD) (a) >> 12)
#define _ALIGN_TO_PAGE(n)        ((n) / PAGE_SIZE * PAGE_SIZE)

#define PF_PRESENT            1
#define PF_WRITEABLE          2
#define PF_USER               4
#define PF_WRITETHROUGH       8
#define PF_CACHEDISABLE       16
#define PF_ACCESSED           32
#define PF_DIRTY              64
#define PF_PAGETABLEATTRIBUTE 128
#define PF_GLOBAL             256


typedef struct
{
    WORD  wFlags    : 12;
    QWORD qwAddress : 52;
} __attribute__((packed)) sPageTableEntry;

typedef struct
{
    sPageTableEntry arrEntries[PAGE_TABLE_SIZE];
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE))) sPageTable;


typedef struct
{
    sPageTable *pPML4;
    sBitmap sPageBitmap;
    QWORD qwMemorySize, qwFreeMemory;
    QWORD qwPageBitmapIndex;
} sPagingData;

void ReservePage(PVOID pAddress);
void ReservePages(PVOID pAddress, QWORD qwPages);
void ReturnPage(PVOID pAddress);
void ReturnPages(PVOID pAddress, QWORD qwPages);
void MapPage(sPageTable *pPML4, PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD wFlags);
void MapPageRange(sPageTable *pPML4, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwPages, WORD wFlags);
void MapPageToIdentity(sPageTable *pPML4, PVOID pPage, WORD wFlags);
void MapPageRangeToIdentity(sPageTable *pPML4, PVOID pPages, QWORD qwPages, WORD wFlags);
PVOID AllocatePage();
PVOID AllocateContinousPages(QWORD qwPages);
void FreeContinousPages(PVOID pAddress, QWORD qwPages);
void FreePage(PVOID pAddress);
PVOID GetPhysicalAddress(sPageTable *pPML4, PVOID pVirtualAddress);
sPageTable *ClonePML4(sPageTable *pTable);
sPageTable *GetKernelPML4();
sPageTable *GetCurrentPML4();
sPageTable *CreateEmptyPML4();
void LoadPML4(sPageTable *pTable);
sPagingData ExportPagingData();
void ImportPagingData(sPagingData sData);
void FreePML4(sPageTable *pTable);

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                QWORD qwMemoryMapSize, QWORD qwMemoryDescriptorSize,
                QWORD qwLoaderStart, QWORD qwLoaderEnd);

#endif // __MEMORY__PAGING_H
