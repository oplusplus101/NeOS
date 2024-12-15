
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <hardware/memory/paging.h>
#include <hardware/idt.h>
#include <common/screen.h>
#include <common/memory.h>
#include <common/bitmap.h>
#include <common/panic.h>

sPageTable *g_pPML4;
sBitmap g_pageBitmap;
SIZE_T g_nMemorySize = 0, g_nFreeMemory = 0;
SIZE_T g_nPageBitmapIndex = 0;

PVOID AllocatePage()
{
    for (; g_nPageBitmapIndex < g_pageBitmap.nLength * 8; g_nPageBitmapIndex++)
    {
        if (GetBitmap(&g_pageBitmap, g_nPageBitmapIndex)) continue;

        ReservePage((PVOID) PAGE_TO_ADDRESS(g_nPageBitmapIndex));
        // Clear the page
        ZeroMemory((PVOID) PAGE_TO_ADDRESS(g_nPageBitmapIndex), PAGE_SIZE);

        return (PVOID) PAGE_TO_ADDRESS(g_nPageBitmapIndex);
    }

    _KernelPanic("Out of pages");
    
    return NULL;
}

void FreePage(PVOID pAddress)
{
    if (pAddress == NULL) return;

    ReturnPage(pAddress);
}

void ReservePage(PVOID pAddress)
{
    SIZE_T nIndex = (SIZE_T) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_pageBitmap, nIndex)) return;

    if (SetBitmap(&g_pageBitmap, nIndex, true))
        g_nFreeMemory -= PAGE_SIZE;
}

void ReturnPage(PVOID pAddress)
{
    SIZE_T nIndex = (SIZE_T) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_pageBitmap, nIndex)) return;
    if (SetBitmap(&g_pageBitmap, nIndex, false))
    {
        g_nFreeMemory += PAGE_SIZE;
        if (g_nPageBitmapIndex > nIndex) g_nPageBitmapIndex = nIndex;
    }
}

void ReservePages(PVOID pAddress, SIZE_T nPages)
{
    for (SIZE_T i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReservePage((PVOID) ((SIZE_T) pAddress + i));
}

void ReturnPages(PVOID pAddress, SIZE_T nPages)
{
    for (SIZE_T i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReturnPage((PVOID) ((SIZE_T) pAddress + i));
}

PVOID GetPhysicalAddress(PVOID pVirtualAddress)
{
    // Page Directory Pointer
    sPageTableEntry *pEntry = &g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    if (!pEntry->bPresent) return NULL;
    sPageTable *pPageDirectoryPointer = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!pEntry->bPresent) return NULL;
    sPageTable *pPageDirectory = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    if (!pEntry->bPresent) return NULL;
    sPageTable *pPageTable = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Entry
    pEntry = &pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    return pEntry->bPresent ? (PVOID) PAGE_TO_ADDRESS(pEntry->nAddress) : NULL;
}

void MapPage(PVOID pVirtualAddress, PVOID pPhysicalAddress)
{
    // Page Directory Pointer
    sPageTableEntry *pEntry = &g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    sPageTable *pPageDirectoryPointer;
    if (!pEntry->bPresent)
    {
        pPageDirectoryPointer = AllocatePage();
        ZeroMemory(pPageDirectoryPointer, PAGE_SIZE);
        pEntry->nAddress   = ADDRESS_TO_PAGE(pPageDirectoryPointer);
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);


    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)];

    sPageTable *pPageDirectory;
    if (!pEntry->bPresent)
    {
        pPageDirectory = AllocatePage();
        ZeroMemory(pPageDirectory, PAGE_SIZE);
        pEntry->nAddress   = ADDRESS_TO_PAGE(pPageDirectory);
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)];

    sPageTable *pPageTable;
    if (!pEntry->bPresent)
    {
        pPageTable = AllocatePage();
        ZeroMemory(pPageTable, PAGE_SIZE);
        pEntry->nAddress   = ADDRESS_TO_PAGE(pPageTable);
        pEntry->bPresent   = true;
        pEntry->bWriteable = true;
        pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageTable = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Entry
    pEntry = &pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    pEntry->nAddress   = ADDRESS_TO_PAGE(pPhysicalAddress);
    pEntry->bPresent   = true;
    pEntry->bWriteable = true;
    pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)] = *pEntry;
}


void MapPageRange(PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        MapPage((PVOID) ((QWORD) pVirtualAddress + i), (PVOID) ((QWORD) pPhysicalAddress + i));
}

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                SIZE_T nMemoryMapSize, SIZE_T nMemoryDescriptorSize,
                SIZE_T nLoaderStart, SIZE_T nLoaderEnd)
{

    PVOID pLargestSegment = NULL;
    SIZE_T nLargestSegmentSize = 0, nMemorySize = 0;

    for (sEFIMemoryDescriptor *pEntry = pMemoryDescriptor;
         (BYTE *) pEntry < (BYTE *) pMemoryDescriptor + nMemoryMapSize;
         pEntry = (sEFIMemoryDescriptor *) ((BYTE *) pEntry + nMemoryDescriptorSize))
    {
        nMemorySize += pEntry->nNumberOfPages * PAGE_SIZE;
        if (pEntry->nType == 7 && pEntry->nNumberOfPages * PAGE_SIZE > nLargestSegmentSize)
        {
            pLargestSegment     = (PVOID) pEntry->nPhysicalStart;
            nLargestSegmentSize = pEntry->nNumberOfPages * PAGE_SIZE;
        }
    }

    
    g_nMemorySize = nMemorySize;
    g_nFreeMemory = nMemorySize;

    g_pageBitmap.pData   = pLargestSegment;
    g_pageBitmap.nLength = nMemorySize / PAGE_SIZE / 8 + 1;
    ZeroMemory(g_pageBitmap.pData, g_pageBitmap.nLength);

    ReservePages(0, g_nMemorySize / PAGE_SIZE + 1);

    for (QWORD i = 0; i < nMemoryMapSize; i += nMemoryDescriptorSize)
    {
        sEFIMemoryDescriptor *pDesc = (sEFIMemoryDescriptor *) ((QWORD) pMemoryDescriptor + i);
        if (pDesc->nType == 7) ReturnPages((PVOID) pDesc->nPhysicalStart, pDesc->nNumberOfPages);
    }

    ReservePages(0, 256); // Reserve the first 1MiB.
    ReservePages(g_pageBitmap.pData, g_pageBitmap.nLength / PAGE_SIZE + 1); // Reserve the bitmap's pages.

    // Reserve the bootloader's pages.
    ReservePages((PVOID) nLoaderStart, (nLoaderEnd - nLoaderStart) / PAGE_SIZE + 1);

    g_pPML4 = AllocatePage();
    ZeroMemory(g_pPML4, PAGE_SIZE);

    // Identity map the whole memory.
    MapPageRange(NULL, NULL, nMemorySize / PAGE_SIZE + 1);
}

void LoadPML4()
{
    __asm__ volatile ("mov %0, %%cr3" :: "r" (g_pPML4));
}
