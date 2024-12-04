
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <hardware/memory/paging.h>
#include <hardware/idt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/bitmap.h>
#include <common/panic.h>

sPageTable *g_pPML4;
sBitmap g_pageBitmap;
size_t g_nMemorySize = 0, g_nFreeMemory = 0;
size_t g_nPageBitmapIndex = 0;

void *AllocatePage()
{
    for (; g_nPageBitmapIndex < g_pageBitmap.nLength * 8; g_nPageBitmapIndex++)
    {
        if (GetBitmap(&g_pageBitmap, g_nPageBitmapIndex)) continue;
        ReservePage((void *) (g_nPageBitmapIndex * PAGE_SIZE));
        // Clear the page
        memzero((void *) (g_nPageBitmapIndex * PAGE_SIZE), PAGE_SIZE);
        return (void *) (g_nPageBitmapIndex * PAGE_SIZE);
    }

    return NULL;
}

void FreePage(void *pAddress)
{
    if (pAddress == NULL) return;
    
    size_t nIndex = (size_t) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_pageBitmap, nIndex)) return;

    ReturnPage(pAddress);
}

void ReservePage(void *pAddress)
{
    size_t nIndex = (size_t) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_pageBitmap, nIndex)) return;

    if (SetBitmap(&g_pageBitmap, nIndex, true))
        g_nFreeMemory -= PAGE_SIZE;
}

void ReturnPage(void *pAddress)
{
    uint64_t nIndex = (size_t) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_pageBitmap, nIndex)) return;
    if (SetBitmap(&g_pageBitmap, nIndex, false))
    {
        g_nFreeMemory += PAGE_SIZE;
        if (g_nPageBitmapIndex > nIndex) g_nPageBitmapIndex = nIndex;
    }
}

void ReservePages(void *pAddress, size_t nPages)
{
    for (size_t i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReservePage(pAddress + i);
}

void ReturnPages(void *pAddress, size_t nPages)
{
    for (size_t i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReturnPage(pAddress + i);
}

void MapPage(void *pVirtualMemory, void *pPhysicalMemory)
{
    // Page Directory Pointer
    sPageDirectoryEntry *pEntry = &g_pPML4->arrEntries[(((size_t) pVirtualMemory) >> 39) & (PAGE_TABLE_SIZE - 1)];
    sPageTable *pPageDirectoryPointer;
    if (!pEntry->bPresent)
    {
        pPageDirectoryPointer = AllocatePage();
        memzero(pPageDirectoryPointer, PAGE_SIZE);
        pEntry->nAddress   = (size_t) pPageDirectoryPointer >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        g_pPML4->arrEntries[(((size_t) pVirtualMemory) >> 39)] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) ((size_t) pEntry->nAddress << 12);

    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[(((size_t) pVirtualMemory) >> 30) & (PAGE_TABLE_SIZE - 1)];

    sPageTable *pPageDirectory;
    if (!pEntry->bPresent)
    {
        pPageDirectory = AllocatePage();
        memzero(pPageDirectory, PAGE_SIZE);
        pEntry->nAddress   = (size_t) pPageDirectory >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectoryPointer->arrEntries[(((size_t) pVirtualMemory) >> 30)] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) ((size_t) pEntry->nAddress << 12);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[(((size_t) pVirtualMemory) >> 21) & (PAGE_TABLE_SIZE - 1)];

    sPageTable *pPageTable;
    if (!pEntry->bPresent)
    {
        pPageTable = AllocatePage();
        memzero(pPageTable, PAGE_SIZE);
        pEntry->nAddress   = (size_t) pPageTable >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectory->arrEntries[(((size_t) pVirtualMemory) >> 21)] = *pEntry;
    }
    else
        pPageTable = (sPageTable *) ((size_t) pEntry->nAddress << 12);

    pEntry = &pPageTable->arrEntries[(((size_t) pVirtualMemory) >> 12) & (PAGE_TABLE_SIZE - 1)];
    pEntry->nAddress   = (size_t) pPageTable >> 12;
    pEntry->bPresent   = true;
    pEntry->bWriteable = true;
    pPageDirectoryPointer->arrEntries[(((size_t) pVirtualMemory) >> 30)] = *pEntry;
}

void MapPageRange(void *pVirtualMemory, void *pPhysicalMemory, size_t nPages)
{
    for (size_t i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        MapPage(pVirtualMemory + i, pPhysicalMemory + i);
}

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                size_t nMemoryMapSize, size_t nMemoryDescriptorSize,
                size_t nLoaderStart, size_t nLoaderEnd)
{

    void *pLargestSegment = NULL;
    size_t nLargestSegmentSize = 0, nMemorySize = 0;

    for (sEFIMemoryDescriptor *pEntry = pMemoryDescriptor;
         (uint8_t *) pEntry < (uint8_t *) pMemoryDescriptor + nMemoryMapSize;
         pEntry = (sEFIMemoryDescriptor *) ((uint8_t *) pEntry + nMemoryDescriptorSize))
    {
        nMemorySize += pEntry->nNumberOfPages * PAGE_SIZE;
        if (pEntry->nType == 7 && pEntry->nNumberOfPages * PAGE_SIZE > nLargestSegmentSize)
        {
            pLargestSegment     = (void *) pEntry->nPhysicalStart;
            nLargestSegmentSize = pEntry->nNumberOfPages * PAGE_SIZE;
        }
    }

    
    g_nMemorySize = nMemorySize;
    g_nFreeMemory = nMemorySize;

    g_pageBitmap.pData   = pLargestSegment;
    g_pageBitmap.nLength = nMemorySize / PAGE_SIZE / 8 + 1;
    memzero(g_pageBitmap.pData, g_pageBitmap.nLength);

    ReservePages(0, g_nMemorySize / PAGE_SIZE + 1);
    for (int i = 0; i < nMemoryMapSize; i++)
    {
        sEFIMemoryDescriptor *pDesc = (sEFIMemoryDescriptor *) ((uint64_t) pMemoryDescriptor + (i * nMemoryDescriptorSize));
        if (pDesc->nType == 7) ReturnPages((void *) pDesc->nPhysicalStart, pDesc->nNumberOfPages);
    }

    ReservePages(0, 256); // Reserve the first 1MiB.
    ReservePages(g_pageBitmap.pData, g_pageBitmap.nLength / PAGE_SIZE + 1); // Reserve the bitmap's pages.

    // Reserve the bootloader's pages.
    ReservePages((void *) nLoaderStart, (nLoaderEnd - nLoaderStart) / PAGE_SIZE + 1);

    g_pPML4 = AllocatePage();
    memzero(g_pPML4, PAGE_SIZE);

    // Identity map the whole memory.
    MapPageRange(NULL, NULL, nMemorySize / PAGE_SIZE + 1);

}

void LoadPML4()
{
    __asm__ volatile ("mov %0, %%cr3" :: "r" (g_pPML4));
}
