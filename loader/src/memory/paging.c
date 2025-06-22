
// Code inspired by: https://github.com/Absurdponcho/PonchoOS

#include <memory/paging.h>
#include <hardware/idt.h>
#include <common/screen.h>
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
        .pPML4        = g_pCurrentPML4,
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

    _KERNEL_PANIC("Out of pages");
    
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

    
    _KERNEL_PANIC("Out of pages");
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
    // Page Directory Pointer
    sPageTableEntry *pEntry = &pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    if (!(pEntry->wFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectoryPointer = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);

    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(pEntry->wFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    if (!(pEntry->wFlags & PF_PRESENT)) return NULL;
    sPageTable *pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);

    // Page Entry
    pEntry = &pPageTable->arrEntries[_ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    return (pEntry->wFlags & PF_PRESENT) ? (PVOID) _PAGE_TO_ADDRESS(pEntry->qwAddress) : NULL;
}

// If pPML4 is null, the current PML4 will be used
void MapPage(sPageTable *pPML4, PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD wFlags)
{
    if (pPML4 == NULL) pPML4 = g_pCurrentPML4;
    // Page Directory Pointer
    sPageTableEntry *pEntry = &pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)];
    sPageTable *pPageDirectoryPointer;
    if (!(pEntry->wFlags & PF_PRESENT))
    {
        pPageDirectoryPointer = AllocatePage();
        pEntry->qwAddress = _ADDRESS_TO_PAGE(pPageDirectoryPointer);
        pEntry->wFlags    = PF_PRESENT | PF_WRITEABLE;
        pPML4->arrEntries[_ADDRESS_TO_PDP_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectoryPointer = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);


    // Page Directory
    pEntry = &pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];

    sPageTable *pPageDirectory;
    if (!(pEntry->wFlags & PF_PRESENT))
    {
        pPageDirectory = AllocatePage();
        pEntry->qwAddress = _ADDRESS_TO_PAGE(pPageDirectory);
        pEntry->wFlags    = PF_PRESENT | PF_WRITEABLE;
        pPageDirectoryPointer->arrEntries[_ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);

    // Page Table
    pEntry = &pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];

    sPageTable *pPageTable;
    if (!(pEntry->wFlags & PF_PRESENT))
    {
        pPageTable = AllocatePage();
        pEntry->qwAddress = _ADDRESS_TO_PAGE(pPageTable);
        pEntry->wFlags    = PF_PRESENT | PF_WRITEABLE;
        pPageDirectory->arrEntries[_ADDRESS_TO_PT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
    {
        pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(pEntry->qwAddress);
    }

    // Page Entry
    pEntry            = &pPageTable->arrEntries[_ADDRESS_TO_PE_INDEX(pVirtualAddress)];
    pEntry->qwAddress = _ADDRESS_TO_PAGE(pPhysicalAddress);
    pEntry->wFlags    = PF_PRESENT | (wFlags & 0x0FFF);
    
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

    LoadPML4(g_pCurrentPML4);
}

sPageTable *GetKernelPML4()
{
    return g_pKernelPML4;
}

sPageTable *GetCurrentPML4()
{
    return g_pCurrentPML4;
}

sPageTable *CreateEmptyPML4()
{
    sPageTable *pTable = AllocatePage();
    MapPageRangeToIdentity(pTable, NULL, g_qwMemorySize / PAGE_SIZE + 1, PF_WRITEABLE);
    return pTable;
}

sPageTable *ClonePML4(sPageTable *pPML4)
{
    sPageTable *pPML4Clone = AllocatePage();
    
    // PML4
    for (WORD i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        if (!(pPML4->arrEntries[i].wFlags & PF_PRESENT)) continue;

        sPageTable *pPageDirectory = (sPageTable *) _PAGE_TO_ADDRESS(pPML4->arrEntries[i].qwAddress);
        sPageTable *pPageDirectoryClone = AllocatePage();
        pPML4Clone->arrEntries[i].qwAddress = _ADDRESS_TO_PAGE(pPageDirectoryClone);
        pPML4Clone->arrEntries[i].wFlags    = PF_PRESENT | PF_WRITEABLE;

        // Page Directory
        for (WORD j = 0; j < PAGE_TABLE_SIZE; j++)
        {
            if (!(pPageDirectory->arrEntries[j].wFlags & PF_PRESENT)) continue;

            sPageTable *pPageTable = (sPageTable *) _PAGE_TO_ADDRESS(pPageDirectory->arrEntries[j].qwAddress);
            sPageTable *pPageTableClone = AllocatePage();
            pPageDirectoryClone->arrEntries[j].qwAddress = _ADDRESS_TO_PAGE(pPageTableClone);
            pPageDirectoryClone->arrEntries[j].wFlags    = PF_PRESENT | PF_WRITEABLE;

            // Page Table
            for (WORD k = 0; k < PAGE_TABLE_SIZE; k++)
            {
                if (!(pPageTable->arrEntries[k].wFlags & PF_PRESENT)) continue;

                sPageTable *pPageEntry = (sPageTable *) _PAGE_TO_ADDRESS(pPageTable->arrEntries[k].qwAddress);
                sPageTable *pPageEntryClone = AllocatePage();
                pPageTableClone->arrEntries[k].qwAddress = _ADDRESS_TO_PAGE(pPageEntryClone);
                pPageTableClone->arrEntries[k].wFlags    = PF_PRESENT | PF_WRITEABLE;

                // FIXME: remove this code once the cause of the error is found (i.e. the addresses randomly blowing up to ridiculously high amounts)
                if ((QWORD) pPageEntry > 0xFFFFFFFF)
                    continue;

                // Page Entry
                for (WORD l = 0; l < PAGE_TABLE_SIZE; l++)
                {
                    if (!(pPageEntry->arrEntries[l].wFlags & PF_PRESENT)) continue;

                    pPageEntryClone->arrEntries[l].qwAddress = pPageEntry->arrEntries[l].qwAddress;
                    pPageEntryClone->arrEntries[l].wFlags    = pPageEntry->arrEntries[l].wFlags;
                }
            }
        }

    }

    return pPML4Clone;
}

void FreePML4(sPageTable *pTable)
{
    // PML4
    for (WORD i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        if (!(pTable->arrEntries[i].wFlags & PF_PRESENT)) continue;

        sPageTable *pPageDirectory = (sPageTable *) _ADDRESS_TO_PAGE(pTable->arrEntries[i].qwAddress);
        
        // Page Directory
        for (WORD j = 0; j < PAGE_TABLE_SIZE; j++)
        {
            if (!(pPageDirectory->arrEntries[j].wFlags & PF_PRESENT)) continue;

            sPageTable *pPageTable = (sPageTable *) _ADDRESS_TO_PAGE(pPageDirectory->arrEntries[i].qwAddress);
            
            // Page Table
            for (WORD k = 0; k < PAGE_TABLE_SIZE; k++)
            {
                if (!(pPageTable->arrEntries[k].wFlags & PF_PRESENT)) continue;

                sPageTable *pPageEntry = (sPageTable *) _ADDRESS_TO_PAGE(pPageTable->arrEntries[i].qwAddress);
    
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
