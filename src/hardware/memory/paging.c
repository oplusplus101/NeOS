
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <hardware/memory/paging.h>
#include <hardware/idt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/bitmap.h>

sPageTable *g_pPML4;
sBitmap g_pageBitmap;
size_t g_nMemorySize = 0, g_nFreeMemory = 0;
size_t g_nPageBitmapIndex = 0;
extern size_t g_nLoaderStart, g_nLoaderEnd;

uint64_t PageFaultHandler(uint64_t rsp, uint8_t nErrorCode)
{
    SetCursor(0, 0);
    ClearScreen();
    SetBGColor(RGB(0, 0, 0));
    SetFGColor(RGB(255, 0, 0));
    PrintFormat("A page fault has occured!\nError Code: %d", nErrorCode);
    __asm__ volatile ("cli\nhlt");
    return rsp;
}

void *RequestPage()
{
    for (; g_nPageBitmapIndex < g_pageBitmap.nLength * 8; g_nPageBitmapIndex++)
    {
        if (GetBitmap(&g_pageBitmap, g_nPageBitmapIndex)) continue;
        LockPage((void *) (g_nPageBitmapIndex * PAGE_SIZE));
        return (void *) (g_nPageBitmapIndex * PAGE_SIZE);
    }

    return NULL;
}

void LockPage(void *pAddress)
{
    size_t nIndex = (size_t) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_pageBitmap, nIndex)) return;

    if (SetBitmap(&g_pageBitmap, nIndex, true))
        g_nFreeMemory -= PAGE_SIZE;
}

void LockPages(void *pAddress, size_t nPages)
{
    for (size_t i = 0; i < nPages; i++)
        LockPage(pAddress + i * PAGE_SIZE);
}

void FreePage(void *pAddress)
{
}

void MapPage(void *pVirtualMemory, void *pPhysicalMemory)
{
    // Page Directory Pointer
    sPageDirectoryEntry *pEntry = &g_pPML4->arrEntries[(((size_t) pVirtualMemory) >> 39) & (PAGE_TABLE_SIZE - 1)];

    sPageTable *pPageDirectoryPointer;
    if (!pEntry->bPresent)
    {
        pPageDirectoryPointer = RequestPage();
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
        pPageDirectory = RequestPage();
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
        pPageTable = RequestPage();
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

void InitPaging(size_t nMemorySize, void *pLargestSegment, size_t nLargestSegmentSize)
{
    g_nMemorySize = nMemorySize;
    g_nFreeMemory = nMemorySize;

    g_pageBitmap.pData   = pLargestSegment;
    g_pageBitmap.nLength = nMemorySize / PAGE_SIZE / 8 + 1;

    LockPages((void *) g_nLoaderStart, (g_nLoaderEnd - g_nLoaderStart) / PAGE_SIZE);

    g_pPML4 = RequestPage();
    memzero(g_pPML4, PAGE_SIZE);

    MapPageRange(NULL, NULL, nMemorySize / PAGE_SIZE);

    RegisterException(14, PageFaultHandler);
}

void LoadPML4()
{
    __asm__ volatile ("mov %0, %%cr3" :: "r" (g_pPML4));
}