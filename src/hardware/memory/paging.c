
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

PVOID AllocatePage()
{
    for (; g_nPageBitmapIndex < g_pageBitmap.qwLength * 8; g_nPageBitmapIndex++)
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
    QWORD qwIndex = (QWORD) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_pageBitmap, qwIndex)) return;

    if (SetBitmap(&g_pageBitmap, qwIndex, true))
        g_nFreeMemory -= PAGE_SIZE;
}

void ReturnPage(PVOID pAddress)
{
    QWORD qwIndex = (QWORD) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_pageBitmap, qwIndex)) return;
    if (SetBitmap(&g_pageBitmap, qwIndex, false))
    {
        g_nFreeMemory += PAGE_SIZE;
        if (g_nPageBitmapIndex > qwIndex) g_nPageBitmapIndex = qwIndex;
    }
}

void ReservePages(PVOID pAddress, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReservePage((PVOID) ((QWORD) pAddress + i));
}

void ReturnPages(PVOID pAddress, QWORD nPages)
{
    for (QWORD i = 0; i < nPages * PAGE_SIZE; i += PAGE_SIZE)
        ReturnPage((PVOID) ((QWORD) pAddress + i));
}

PVOID GetPhysicalAddress(PVOID pVirtualAddress)
{
    // Page Directory Pointer
    sPageTableEntry *pEntry = &g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    if (!(pEntry->nFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectoryPointer = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(pEntry->nFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectory = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    if (!(pEntry->nFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageTable = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Entry
    pEntry = &pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    return (pEntry->nFlags & PF_PRESENT) ? (PVOID) PAGE_TO_ADDRESS(pEntry->nAddress) : NULL;
}

void MapPage(PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD wFlags)
{
    // Page Directory Pointer
    sPageTableEntry *pEntry = &g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    sPageTable *pPageDirectoryPointer;
    if (!(pEntry->nFlags & PF_PRESENT))
    {
        pPageDirectoryPointer = AllocatePage();
        ZeroMemory(pPageDirectoryPointer, PAGE_SIZE);
        pEntry->nAddress = ADDRESS_TO_PAGE(pPageDirectoryPointer);
        pEntry->nFlags = PF_PRESENT | PF_WRITEABLE;
        g_pPML4->arrEntries[ADDRESS_TO_PDP_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);


    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)];

    sPageTable *pPageDirectory;
    if (!(pEntry->nFlags & PF_PRESENT))
    {
        pPageDirectory = AllocatePage();
        ZeroMemory(pPageDirectory, PAGE_SIZE);
        pEntry->nAddress   = ADDRESS_TO_PAGE(pPageDirectory);
        pEntry->nFlags = PF_PRESENT | PF_WRITEABLE;
        pPageDirectoryPointer->arrEntries[ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)];

    sPageTable *pPageTable;
    if (!(pEntry->nFlags & PF_PRESENT))
    {
        pPageTable = AllocatePage();
        ZeroMemory(pPageTable, PAGE_SIZE);
        pEntry->nAddress   = ADDRESS_TO_PAGE(pPageTable);
        pEntry->nFlags = PF_PRESENT | PF_WRITEABLE;
        pPageDirectory->arrEntries[ADDRESS_TO_PT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageTable = (sPageTable *) PAGE_TO_ADDRESS(pEntry->nAddress);

    // Page Entry
    pEntry = &pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    pEntry->nAddress = ADDRESS_TO_PAGE(pPhysicalAddress);
    pEntry->nFlags   = PF_PRESENT | (wFlags & 0x0FFF);
    
    pPageTable->arrEntries[ADDRESS_TO_PE_INDEX(pVirtualAddress)] = *pEntry;
}


void MapPageRange(PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwPages, WORD wFlags)
{
    for (QWORD i = 0; i < qwPages * PAGE_SIZE; i += PAGE_SIZE)
        MapPage((PVOID) ((QWORD) pVirtualAddress + i), (PVOID) ((QWORD) pPhysicalAddress + i), wFlags);
}

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                QWORD nMemoryMapSize, QWORD nMemoryDescriptorSize,
                QWORD nLoaderStart, QWORD nLoaderEnd)
{

    PVOID pLargestSegment = NULL;
    QWORD nLargestSegmentSize = 0, nMemorySize = 0;

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
    g_pageBitmap.qwLength = nMemorySize / PAGE_SIZE / 8 + 1;
    ZeroMemory(g_pageBitmap.pData, g_pageBitmap.qwLength);

    ReservePages(0, g_nMemorySize / PAGE_SIZE + 1);

    for (QWORD i = 0; i < nMemoryMapSize; i += nMemoryDescriptorSize)
    {
        sEFIMemoryDescriptor *pDesc = (sEFIMemoryDescriptor *) ((QWORD) pMemoryDescriptor + i);
        if (pDesc->nType == 7) ReturnPages((PVOID) pDesc->nPhysicalStart, pDesc->nNumberOfPages);
    }

    ReservePages(0, 256); // Reserve the first 1MiB.
    ReservePages(g_pageBitmap.pData, g_pageBitmap.qwLength / PAGE_SIZE + 1); // Reserve the bitmap's pages.

    // Reserve the bootloader's pages.
    ReservePages((PVOID) nLoaderStart, (nLoaderEnd - nLoaderStart) / PAGE_SIZE + 1);

    g_pPML4 = AllocatePage();
    ZeroMemory(g_pPML4, PAGE_SIZE);

    // Identity map the whole memory.
    MapPageRange(NULL, NULL, nMemorySize / PAGE_SIZE + 1, PF_WRITEABLE);
}

void LoadPML4()
{
    __asm__ volatile ("mov %0, %%cr3" :: "r" (g_pPML4));
}
