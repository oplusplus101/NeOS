
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <memory/paging.h>
#include <hardware/idt.h>
#include <common/memory.h>
#include <common/panic.h>

sPageTable *g_pCurrentPML4, *g_pKernelPML4;
sBitmap g_sPageBitmap;
QWORD g_qwMemorySize = 0, g_qwFreeMemory = 0;
QWORD g_qwPageBitmapIndex = 0;

sPagingData ExportPagingData()
{
    sPagingData sData =
    {
        .pPML4             = g_pCurrentPML4,
        .sPageBitmap       = g_sPageBitmap,
        .qwMemorySize      = g_qwMemorySize,
        .qwFreeMemory      = g_qwFreeMemory,
        .qwPageBitmapIndex = g_qwPageBitmapIndex
    };

    return sData;
}

void ImportPagingData(sPagingData sData)
{
    g_pCurrentPML4 = sData.pPML4;
    g_pKernelPML4 = sData.pPML4;
    g_sPageBitmap = sData.sPageBitmap;
    g_qwMemorySize = sData.qwMemorySize;
    g_qwFreeMemory = sData.qwFreeMemory;
    g_qwPageBitmapIndex = sData.qwPageBitmapIndex;
}

// Returns a page of memory filled with zeroes
PVOID AllocatePage()
{
    for (; g_qwPageBitmapIndex < g_sPageBitmap.qwLength * 8; g_qwPageBitmapIndex++)
    {
        if (GetBitmap(&g_sPageBitmap, g_qwPageBitmapIndex)) continue;

        ReservePage((PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex));
        // Clear the page
        ZeroMemory((PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex), PAGE_SIZE);

        return (PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex);
    }
 
    // _KERNEL_PANIC(L"Out of pages");
    
    return NULL;
}

PVOID AllocateContinousPages(QWORD qwPages)
{
    for (; g_qwPageBitmapIndex < g_sPageBitmap.qwLength * 8; g_qwPageBitmapIndex++)
    {
        BOOL bAllFree = true;
        for (QWORD i = 0; i < qwPages; i++)
            if (GetBitmap(&g_sPageBitmap, g_qwPageBitmapIndex + i))
            {
                g_qwPageBitmapIndex += i;
                bAllFree = false;
                break;
            }
        
        if (!bAllFree) continue;
        

        ReservePages((PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex), qwPages);
        // Clear the page
        ZeroMemory((PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex), PAGE_SIZE * qwPages);

        return (PVOID) _PAGE_TO_ADDRESS(g_qwPageBitmapIndex);
    }

    
    // _KERNEL_PANIC(L"Out of pages");
    return NULL;
}

void FreeContinousPages(PVOID pAddress, QWORD qwPages)
{
    if (pAddress == NULL) return;
    MapPageRangeToIdentity(NULL, pAddress, qwPages, PF_WRITEABLE);
    ReturnPages(pAddress, qwPages);
    g_qwPageBitmapIndex = _ADDRESS_TO_PAGE(pAddress);
}

void FreePage(PVOID pAddress)
{
    if (pAddress == NULL) return;
    MapPageToIdentity(NULL, pAddress, PF_WRITEABLE);
    ReturnPage(pAddress);
    g_qwPageBitmapIndex = _ADDRESS_TO_PAGE(pAddress);
}

void ReservePage(PVOID pAddress)
{
    QWORD qwIndex = (QWORD) pAddress / PAGE_SIZE;
    
    if (GetBitmap(&g_sPageBitmap, qwIndex)) return;

    if (SetBitmap(&g_sPageBitmap, qwIndex, true))
        g_qwFreeMemory -= PAGE_SIZE;
}

void ReturnPage(PVOID pAddress)
{
    QWORD qwIndex = (QWORD) pAddress / PAGE_SIZE;
    if (!GetBitmap(&g_sPageBitmap, qwIndex)) return;
    if (SetBitmap(&g_sPageBitmap, qwIndex, false))
    {
        g_qwFreeMemory += PAGE_SIZE;
        if (g_qwPageBitmapIndex > qwIndex) g_qwPageBitmapIndex = qwIndex;
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

PVOID GetPhysicalAddress(sPageTable *pPML4, PVOID pVirtualAddress)
{
    if (pPML4 == NULL)
        pPML4 = GetCurrentPML4();
    
    QWORD qwOffset = (QWORD) pVirtualAddress % PAGE_SIZE;
    
    // Page Directory Pointer
    PTE *pEntry = &pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectoryPointer = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);

    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;
    sPageTable *pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);

    // Page Entry
    pEntry = &pPageTable->arrEntries[_ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    return (*pEntry & PF_PRESENT) ? (PVOID) (_PAGE_TO_ADDRESS(*pEntry >> 12) + qwOffset) : NULL;
}

// If pPML4 is null, the current PML4 will be used
void MapPage(sPageTable *pPML4, PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD wFlags)
{
    if (pPML4 == NULL) pPML4 = g_pCurrentPML4;
    // Page Directory Pointer
    PTE *pEntry = &pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    sPageTable *pPageDirectoryPointer;
    if (!(*pEntry & PF_PRESENT))
    {
        pPageDirectoryPointer = AllocatePage();
        *pEntry = (_ADDRESS_TO_PAGE(pPageDirectoryPointer) << 12) | PF_PRESENT | PF_ACCESSED | PF_WRITEABLE;
        pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);


    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];

    sPageTable *pPageDirectory;
    if (!(*pEntry & PF_PRESENT))
    {
        pPageDirectory = AllocatePage();
        *pEntry = (_ADDRESS_TO_PAGE(pPageDirectory) << 12) | PF_PRESENT | PF_ACCESSED | PF_WRITEABLE;
        pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];

    sPageTable *pPageTable;
    if (!(*pEntry & PF_PRESENT))
    {
        pPageTable = AllocatePage();
        *pEntry = (_ADDRESS_TO_PAGE(pPageTable) << 12) | PF_PRESENT | PF_WRITEABLE;
        pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(*pEntry >> 12);

    // Page Entry
    pEntry = &pPageTable->arrEntries[_ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    *pEntry = (_ADDRESS_TO_PAGE(pPhysicalAddress) << 12) | PF_PRESENT | (wFlags & 0x0FFF);
    
    pPageTable->arrEntries[_ADDRESS_TO_PE_INDEX(pVirtualAddress)] = *pEntry;
}

void MapPageRange(sPageTable *pPML4, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwPages, WORD wFlags)
{
    for (QWORD i = 0; i < qwPages * PAGE_SIZE; i += PAGE_SIZE)
        MapPage(pPML4, (PVOID) ((QWORD) pVirtualAddress + i), (PVOID) ((QWORD) pPhysicalAddress + i), wFlags);
}

void MapPageToIdentity(sPageTable *pPML4, PVOID pPage, WORD wFlags)
{
    MapPage(pPML4, pPage, pPage, wFlags);
}

void MapPageRangeToIdentity(sPageTable *pPML4, PVOID pPages, QWORD qwPages, WORD wFlags)
{
    MapPageRange(pPML4, pPages, pPages, qwPages, wFlags);
}

void EnableWriteProtect()
{
    QWORD cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 1 << 16; // Write protect bit
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void DisableWriteProtect()
{
    QWORD cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 16); // Write protect bit
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

void PopulatePageMap(sPageTable *pPageTable)
{

}

void InitPaging(sEFIMemoryDescriptor *pMemoryDescriptor,
                QWORD nMemoryMapSize, QWORD nMemoryDescriptorSize,
                QWORD nLoaderStart, QWORD nLoaderEnd)
{
    PVOID pLargestSegment = NULL;
    QWORD nLargestSegmentSize = 0, qwMemorySize = 0;

    for (sEFIMemoryDescriptor *pEntry = pMemoryDescriptor;
         (BYTE *) pEntry < (BYTE *) pMemoryDescriptor + nMemoryMapSize;
         pEntry = (sEFIMemoryDescriptor *) ((BYTE *) pEntry + nMemoryDescriptorSize))
    {
        qwMemorySize += pEntry->nNumberOfPages * PAGE_SIZE;
        if (pEntry->nType == 7 && pEntry->nNumberOfPages * PAGE_SIZE > nLargestSegmentSize)
        {
            pLargestSegment     = (PVOID) pEntry->nPhysicalStart;
            nLargestSegmentSize = pEntry->nNumberOfPages * PAGE_SIZE;
        }
    }

    g_qwMemorySize         = qwMemorySize;
    g_qwFreeMemory         = qwMemorySize;
    
    g_sPageBitmap.pData    = pLargestSegment;
    g_sPageBitmap.qwLength = qwMemorySize / PAGE_SIZE / 8 + 1;
    ZeroMemory(g_sPageBitmap.pData, g_sPageBitmap.qwLength);
    
    ReservePages(0, g_qwMemorySize / PAGE_SIZE + 1);
    
    for (QWORD i = 0; i < nMemoryMapSize; i += nMemoryDescriptorSize)
    {
        sEFIMemoryDescriptor *pDesc = (sEFIMemoryDescriptor *) ((QWORD) pMemoryDescriptor + i);
        if (pDesc->nType == 7) ReturnPages((PVOID) pDesc->nPhysicalStart, pDesc->nNumberOfPages);
    }
    
    ReservePages(0, 256); // Reserve the first 1MiB.
    ReservePages(g_sPageBitmap.pData, g_sPageBitmap.qwLength / PAGE_SIZE + 1); // Reserve the bitmap's pages.
    
    // Reserve the bootloader's pages.
    ReservePages((PVOID) nLoaderStart, (nLoaderEnd - nLoaderStart) / PAGE_SIZE + 1);
    
    g_pKernelPML4 = AllocatePage();
    g_pCurrentPML4 = g_pKernelPML4;

    // Identity map the whole memory.
    MapPageRangeToIdentity(NULL, NULL, qwMemorySize / PAGE_SIZE + 1, PF_WRITEABLE);
    MapPageToIdentity(NULL, g_pCurrentPML4, PF_ACCESSED | PF_WRITEABLE);
    LoadPML4(g_pCurrentPML4);
    // EnableWriteProtect();
}

sPageTable *GetKernelPML4()
{
    return g_pKernelPML4;
}

sPageTable *GetCurrentPML4()
{
    return g_pCurrentPML4;
}

sPageTable *CreateEmptyPML4(WORD wFlags)
{
    sPageTable *pTable = AllocatePage();
    MapPageRangeToIdentity(pTable, NULL, g_qwMemorySize / PAGE_SIZE + 1, wFlags);
    return pTable;
}

void FreePML4(sPageTable *pTable)
{
    // PML4
    for (WORD i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        if (!(pTable->arrEntries[i] & PF_PRESENT)) continue;

        sPageTable *pPageDirectory = (sPageTable *) _ADDRESS_TO_PAGE(pTable->arrEntries[i] >> 12);
        
        // Page Directory
        for (WORD j = 0; j < PAGE_TABLE_SIZE; j++)
        {
            if (!(pPageDirectory->arrEntries[j] & PF_PRESENT)) continue;

            sPageTable *pPageTable = (sPageTable *) _ADDRESS_TO_PAGE(pPageDirectory->arrEntries[i] >> 12);
            
            // Page Table
            for (WORD k = 0; k < PAGE_TABLE_SIZE; k++)
            {
                if (!(pPageTable->arrEntries[k] & PF_PRESENT)) continue;

                sPageTable *pPageEntry = (sPageTable *) _ADDRESS_TO_PAGE(pPageTable->arrEntries[i] >> 12);
    
                FreePage(pPageEntry);
            }

            FreePage(pPageTable);
        }

        FreePage(pPageDirectory);
    }
    
    FreePage(pTable);
}

void LoadPML4(sPageTable *pTable)
{
    g_pCurrentPML4 = pTable;
    __asm__ volatile ("mov %0, %%cr3" : : "r" (g_pCurrentPML4));
}
