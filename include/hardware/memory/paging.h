
#ifndef __PAGING_H
#define __PAGING_H

#include <common/types.h>
#include <common/bootstructs.h>

#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 512

typedef struct
{
   bool bPresent          : 1;
   bool bWriteable        : 1;
   bool bUser             : 1;
   bool bWriteThrough     : 1;
   bool bCacheDisabled    : 1;
   bool bAccessed         : 1;
   bool bDirty            : 1;
   bool bPageAttribute    : 1;
   bool bGlobal           : 1;
   uint8_t nAvailable     : 2;
   uint64_t nAddress      : 40;
   uint8_t nReserved      : 4;
   uint8_t nAvailable2    : 4;
   uint8_t nProtectionKey : 4;
   bool nExecuteDisable   : 1;
} __attribute__((packed)) sPageDirectoryEntry;

typedef struct
{
   sPageDirectoryEntry arrEntries[512];
} __attribute__((packed)) sPageTable;

void ReservePage(void *pAddress);
void ReservePages(void *pAddress, size_t nPages);
void ReturnPage(void *pAddress);
void ReturnPages(void *pAddress, size_t nPages);
void MapPage(void *pVirtualMemory, void *pPhysicalMemory);
void MapPageRange(void *pVirtualMemory, void *pPhysicalMemory, size_t nPages);
void *AllocatePage();
void LoadPML4();

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                size_t nMemoryMapSize, size_t nMemoryDescriptorSize,
                size_t nLoaderStart, size_t nLoaderEnd);
#endif // __PAGING_H
