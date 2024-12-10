
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <hardware/memory/paging.h>
#include <hardware/idt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/bitmap.h>
#include <common/panic.h>

sPageTable *g_pPML4;
sBitmap g_pageBitmap;
QWORD g_nMemorySize = 0, g_nFreeMemory = 0;
QWORD g_nPageBitmapIndex = 0;

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

    _KernelPanic("Out of memory");
    
    return NULL;
}

void FreePage(void *pAddress)
{
    if (pAddress == NULL) return;

    ReturnPage(pAddress);
}

void ReservePage(void *pAddress)
{
    QWORD nIndex = (QWORD) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_pageBitmap, nIndex)) return;

    if (SetBitmap(&g_pageBitmap, nIndex, true))
        g_nFreeMemory -= PAGE_SIZE;
}

void ReturnPage(void *pAddress)
{
    QWORD nIndex = (QWORD) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_pageBitmap, nIndex)) return;
    if (SetBitmap(&g_pageBitmap, nIndex, false))
    {
        g_nFreeMemory += PAGE_SIZE;
        if (g_nPageBitmapIndex > nIndex) g_nPageBitmapIndex = nIndex;
    }
}

void ReservePages(void *pAddress, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReservePage((void *) ((QWORD) pAddress + i));
}

void ReturnPages(void *pAddress, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReturnPage((void *) ((QWORD) pAddress + i));
}

void MapPage(void *pVirtualMemory, void *pPhysicalMemory)
{
    // Page Directory Pointer
    sPageDirectoryEntry *pEntry = &g_pPML4->arrEntries[((QWORD) pVirtualMemory >> 39) % PAGE_TABLE_SIZE];
    sPageTable *pPageDirectoryPointer;
    if (!pEntry->bPresent)
    {
        pPageDirectoryPointer = AllocatePage();
        memzero(pPageDirectoryPointer, PAGE_SIZE);
        pEntry->nAddress   = (QWORD) pPageDirectoryPointer >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        g_pPML4->arrEntries[((QWORD) pVirtualMemory >> 39) % PAGE_TABLE_SIZE] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) ((QWORD) pEntry->nAddress << 12);


    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[((QWORD) pVirtualMemory >> 30) % PAGE_TABLE_SIZE];

    sPageTable *pPageDirectory;
    if (!pEntry->bPresent)
    {
        pPageDirectory = AllocatePage();
        memzero(pPageDirectory, PAGE_SIZE);
        pEntry->nAddress   = (QWORD) pPageDirectory >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectoryPointer->arrEntries[((QWORD) pVirtualMemory >> 30) % PAGE_TABLE_SIZE] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) ((QWORD) pEntry->nAddress << 12);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[((QWORD) pVirtualMemory >> 21) % PAGE_TABLE_SIZE];

    sPageTable *pPageTable;
    if (!pEntry->bPresent)
    {
        pPageTable = AllocatePage();
        memzero(pPageTable, PAGE_SIZE);
        pEntry->nAddress   = (QWORD) pPageTable >> 12;
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectory->arrEntries[((QWORD) pVirtualMemory >> 21) % PAGE_TABLE_SIZE] = *pEntry;
    }
    else
        pPageTable = (sPageTable *) ((QWORD) pEntry->nAddress << 12);

    pEntry = &pPageTable->arrEntries[((QWORD) pVirtualMemory >> 12) % PAGE_TABLE_SIZE];
    pEntry->nAddress   = (QWORD) pPhysicalMemory >> 12;
    pEntry->bPresent   = true;
    pEntry->bWriteable = true;
    pPageTable->arrEntries[((QWORD) pVirtualMemory >> 12) % PAGE_TABLE_SIZE] = *pEntry;
}

void MapPageRange(void *pVirtualMemory, void *pPhysicalMemory, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        MapPage((void *) ((QWORD) pVirtualMemory + i), (void *) ((QWORD) pPhysicalMemory + i));
}

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                QWORD nMemoryMapSize, QWORD nMemoryDescriptorSize,
                QWORD nLoaderStart, QWORD nLoaderEnd)
{

    void *pLargestSegment = NULL;
    QWORD nLargestSegmentSize = 0, nMemorySize = 0;

    for (sEFIMemoryDescriptor *pEntry = pMemoryDescriptor;
         (BYTE *) pEntry < (BYTE *) pMemoryDescriptor + nMemoryMapSize;
         pEntry = (sEFIMemoryDescriptor *) ((BYTE *) pEntry + nMemoryDescriptorSize))
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

    for (QWORD i = 0; i < nMemoryMapSize; i += nMemoryDescriptorSize)
    {
        sEFIMemoryDescriptor *pDesc = (sEFIMemoryDescriptor *) ((QWORD) pMemoryDescriptor + i);
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
