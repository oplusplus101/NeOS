
#define ARCH_EXE
#include <memory/paging.h>
#include <msr.h>
#include <common/memory.h>
#include <io/screen.h>
#include <common/math.h>


typedef QWORD PTE;
typedef PTE *PageTable;

PageTable g_pPML4;

PBYTE g_arrBitmap;
QWORD g_qwBitmapSize;

#pragma region Utilities

inline static void _EnableWriteProtect()
{
    QWORD cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 1 << 16; // Write protect bit
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

inline static void _DisableWriteProtect()
{
    QWORD cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 16); // Write protect bit
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

inline static void _ReservePage(PVOID pPage)
{
    QWORD qwIndex = ((QWORD) pPage / PAGE_SIZE) / 8;
    if (qwIndex >= g_qwBitmapSize) return;
    g_arrBitmap[qwIndex] |= 1 << (((QWORD) pPage / PAGE_SIZE) % 8);
}

inline static void _ReturnPage(PVOID pPage)
{
    QWORD qwIndex = ((QWORD) pPage / PAGE_SIZE) / 8;
    if (qwIndex >= g_qwBitmapSize) return;
    g_arrBitmap[qwIndex] &= ~(1 << (((QWORD) pPage / PAGE_SIZE) % 8));
}

inline static BOOL _IsReserved(PVOID pPage)
{
    return (g_arrBitmap[(QWORD) pPage / PAGE_SIZE / 8] & (1 << (((QWORD) pPage / PAGE_SIZE) % 8))) != 0;
}

inline static void _ReservePages(PVOID pPage, QWORD qwNumberOfPages)
{
    for (QWORD i = 0; i < qwNumberOfPages; i++)
        _ReservePage((PVOID) ((QWORD) pPage + i * PAGE_SIZE));
}

inline static void _ReturnPages(PVOID pPage, QWORD qwNumberOfPages)
{
    for (QWORD i = 0; i < qwNumberOfPages; i++)
        _ReturnPage((PVOID) ((QWORD) pPage + i * PAGE_SIZE));
}

#pragma endregion

void SYSV ArchInvalidatePage(PVOID pPage)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(pPage) : "memory");
}

void SYSV ArchLoadPageTable(PVOID pPageTablePhysical)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r" (pPageTablePhysical) : "memory");
    g_pPML4 = pPageTablePhysical;
}

void SYSV ArchRefreshPages()
{
    __asm__ volatile ("mov %%cr3, %%rax\nmov %%rax, %%cr3" : : : "rax");
}

void SYSV ArchMapHugePage(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, BYTE bFlags, BOOL bRefreshPage)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;

    _DisableWriteProtect();
        
    pPhysicalAddress = (PVOID) _ALIGN_TO_HUGE_PAGE(pPhysicalAddress);
    pVirtualAddress  = (PVOID) _ALIGN_TO_HUGE_PAGE(pVirtualAddress);

    PageTable arrPML4 = pPageTable;
    // Page Directory Page Table
    PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    PageTable arrPDPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPDPT = ArchAllocatePage();
        ZeroMemory(arrPDPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPDPT) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    PageTable arrPD;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPD = ArchAllocatePage();
        ZeroMemory(arrPD, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPD) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (*pEntry & PF_PRESENT && !(*pEntry & PF_PAGETABLEATTRIBUTE))
        ArchFreePage((PVOID) (*pEntry & 0x000FFFFFFFFFF000));

    WORD wFlags = 0;
    if (bFlags & PAGING_WRITEABLE) wFlags |= PF_WRITEABLE;
    if (bFlags & PAGING_UNCACHED)  wFlags |= PF_CACHEDISABLE;
    if (bFlags & PAGING_USERMODE)  wFlags |= PF_USER;
    if (bFlags & PAGING_GLOBAL)    wFlags |= PF_GLOBAL;
    
    *pEntry = _ALIGN_TO_HUGE_PAGE(pPhysicalAddress) | PF_PRESENT | PF_PAGETABLEATTRIBUTE | (wFlags & 0x0FFF) | (bFlags & PAGING_NOEXECUTE ? PF_NOEXECUTE : 0);

    _EnableWriteProtect();

    if (bRefreshPage)
        ArchInvalidatePage(pVirtualAddress);
}

void _MapPageInternal(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, WORD wFlagsNative, BOOL bNoExecute)
{
    _DisableWriteProtect();
    PageTable arrPML4 = pPageTable;

    // Page Directory Page Table
    PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    PageTable arrPDPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPDPT = ArchAllocatePage();
        ZeroMemory(arrPDPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPDPT) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    PageTable arrPD;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPD = ArchAllocatePage();
        ZeroMemory(arrPD, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPD) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    PageTable arrPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPT = ArchAllocatePage();
        ZeroMemory(arrPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPT) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }
    else
        arrPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    pEntry = &arrPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    *pEntry = _ALIGN_TO_PAGE(pPhysicalAddress) | PF_PRESENT | wFlagsNative | ((bNoExecute & PAGING_NOEXECUTE) ? PF_NOEXECUTE : 0);

    _EnableWriteProtect();
}

void SYSV ArchMapPage(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, BYTE bFlags, BOOL bRefreshPage)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    PageTable arrPML4 = pPageTable;
    pPhysicalAddress = (PVOID) _ALIGN_TO_PAGE(pPhysicalAddress);
    pVirtualAddress  = (PVOID) _ALIGN_TO_PAGE(pVirtualAddress);

    // Page Directory Pointer
    PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto SkipPageFixup;
    PageTable arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto SkipPageFixup;
    PageTable arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);
    
    // Page Table
    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PAGETABLEATTRIBUTE)) goto SkipPageFixup;

    if (*pEntry & PF_PAGETABLEATTRIBUTE) // For huge pages
    {
        PVOID pPreviousPhysicalAddress = (PVOID) (*pEntry & 0x000FFFFFFFFFF000);
        WORD  wPreviousFlags           = *pEntry & 0x0FFF;
        BOOL bNoExecute                = (*pEntry & PF_NOEXECUTE) != 0;
        wPreviousFlags &= ~PF_PAGETABLEATTRIBUTE;

        PageTable arrPT = ArchAllocatePage();
        ZeroMemory(arrPT, PAGE_SIZE);

        for (QWORD i = 0; i < HUGE_PAGE_SIZE / PAGE_SIZE; i++)
        {
            if (_ALIGN_TO_PAGE(pVirtualAddress) == _ALIGN_TO_HUGE_PAGE(pVirtualAddress) + i * PAGE_SIZE) continue; // Skip this page
            arrPT[i] = _ALIGN_TO_PAGE(_ALIGN_TO_HUGE_PAGE(pPreviousPhysicalAddress) + i * PAGE_SIZE) | PF_PRESENT | wPreviousFlags | (bNoExecute ? PF_NOEXECUTE : 0);
        }

        *pEntry = _ALIGN_TO_PAGE(arrPT) | PF_PRESENT | PF_ACCESSED | PF_DIRTY | PF_WRITEABLE;
        arrPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)] = *pEntry;
    }

SkipPageFixup:
    WORD wFlags = 0;
    if (bFlags & PAGING_WRITEABLE) wFlags |= PF_WRITEABLE;
    if (bFlags & PAGING_UNCACHED)  wFlags |= PF_CACHEDISABLE;
    if (bFlags & PAGING_USERMODE)  wFlags |= PF_USER;
    if (bFlags & PAGING_GLOBAL)    wFlags |= PF_GLOBAL;

    _MapPageInternal(pPageTable, pVirtualAddress, pPhysicalAddress, wFlags, bFlags & PAGING_NOEXECUTE);

    if (bRefreshPage)
        ArchInvalidatePage(pVirtualAddress);
}

void SYSV ArchMapPages(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwNumberOfPages, BYTE bFlags, BYTE bRefreshPage)
{
    for (QWORD i = 0; i < qwNumberOfPages; i++)
        ArchMapPage(pPageTable, (PBYTE) pVirtualAddress + i * PAGE_SIZE, (PBYTE) pPhysicalAddress + i * PAGE_SIZE, bFlags, false);

    if (bRefreshPage)
        ArchRefreshPages();
}

void SYSV ArchUnMapPage(PVOID pPageTable, PVOID pVirtualAddress, BOOL bRefreshPage)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;

    _DisableWriteProtect();

    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto ArchUnMapPageExit;
    PageTable pPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto ArchUnMapPageExit;
    PageTable pPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto ArchUnMapPageExit;

    PageTable pPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Entry
    pEntry = &pPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    // Get rid of the entry
    *pEntry = 0;

    if (bRefreshPage)
        ArchInvalidatePage(pVirtualAddress);

ArchUnMapPageExit:
    _EnableWriteProtect();
}


void SYSV ArchUnMapHugePage(PVOID pPageTable, PVOID pVirtualAddress, BOOL bRefreshPage)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    _DisableWriteProtect();
    
    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPDPT = (PageTable) _ALIGN_TO_PAGE(*pEntry);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPD = (PageTable) _ALIGN_TO_PAGE(*pEntry);

    // Page Entry
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];

    // Get rid of the entry
    *pEntry = 0;

    _EnableWriteProtect();

    if (bRefreshPage)
        ArchInvalidatePage(pVirtualAddress);
}

void SYSV ArchUnMapPages(PVOID pPageTable, PVOID pVirtualAddress, QWORD qwNumberOfPages, BOOL bRefreshPages)
{
     for (QWORD i = 0; i < qwNumberOfPages; i++)
        ArchUnMapPage(pPageTable, (PBYTE) pVirtualAddress + i * PAGE_SIZE, false);

    if (bRefreshPages)
        ArchRefreshPages();
}

void SYSV ArchUnMapHugePages(PVOID pPageTable, PVOID pVirtualAddress, QWORD qwNumberOfPages, BOOL bRefreshPages)
{
     for (QWORD i = 0; i < qwNumberOfPages; i++)
        ArchUnMapHugePage(pPageTable, (PBYTE) pVirtualAddress + i * HUGE_PAGE_SIZE, false);

    if (bRefreshPages)
        ArchRefreshPages();
}

PVOID ArchGetPhysicalAddress(PVOID pPageTable, PVOID pVirtualAddress)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;
    PageTable pPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;
    PageTable pPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return NULL;

    if (*pEntry & PF_PAGETABLEATTRIBUTE) // For huge pages
        return (PVOID) ((*pEntry & 0x000FFFFFFFFFF000) + ((QWORD) pVirtualAddress % HUGE_PAGE_SIZE));

    PageTable pPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Entry
    pEntry = &pPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    return (*pEntry & PF_PRESENT) ? (PVOID) (_ALIGN_TO_PAGE(*pEntry) + ((QWORD) pVirtualAddress % PAGE_SIZE)) : NULL;
}


PVOID SYSV ArchAllocatePage()
{
    static QWORD qwOffset = 0;
    PVOID pAddress = NULL;
    for (; qwOffset < g_qwBitmapSize * 8; qwOffset++)
        if (!_IsReserved((PVOID) (qwOffset * PAGE_SIZE)))
        {
            pAddress = (PVOID) (qwOffset * PAGE_SIZE);
            break;
        }

    if (pAddress == NULL)
    {
        // Try again from the start
        qwOffset = 0;
        return ArchAllocatePage();
    }

    _ReservePage(pAddress);
    return pAddress;
}

void SYSV ArchFreePage(PVOID pPage)
{
    // ArchUnMapPage(PML4_VIRT, pPage);
}

PVOID SYSV ArchAllocatePagesContinuous(QWORD qwNumberOfPages)
{
    for (QWORD i = 0; i < g_qwBitmapSize * 8; )
    {
        BOOL bAllFree = true;
        QWORD j = 0;
        for (; j < qwNumberOfPages; j++)
            if (_IsReserved((PVOID) ((i + j) * PAGE_SIZE)))
            {
                bAllFree = false;
                break;
            }
        
        if (!bAllFree)
        {
            i += _MAX(1, j);
            continue;
        }
        
        _ReservePages((PVOID) _PAGE_TO_ADDRESS(i), qwNumberOfPages);
        return (PVOID) _PAGE_TO_ADDRESS(i);
    }
    
    return NULL;
}


void SYSV ArchFreePagesContinuous(PVOID pPages, QWORD qwNumberOfPages)
{
}

void _InitialisePaging(sARCHHeader *pHeader)
{
    // Enable the NX (No Execute) bit
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | EFER_NXE);
    _EnableWriteProtect();

    g_arrBitmap    = pHeader->pPageBitmap;
    g_qwBitmapSize = pHeader->qwPageBitmapSize;
    g_pPML4        = pHeader->pPML4;

    _ReservePages(NULL, 512); // Reserve the first 2MiB, otherwise strange things will happen

    for (sEFIMemoryDescriptor *pEntry = pHeader->arrMemoryMap; (QWORD) pEntry < (QWORD) pHeader->arrMemoryMap + pHeader->qwMemoryMapSize; pEntry = (sEFIMemoryDescriptor *) ((PBYTE) pEntry + pHeader->qwMemoryDescriptorSize))
    {
        if (pEntry->dwType != 7) // not EfiConventionalMemory
            _ReservePages((PVOID) pEntry->qwPhysicalStart, pEntry->qwNumberOfPages);
    }

    // Make page zero trigger a page fault
    ArchMapPage(NULL, NULL, NULL, 0, true);
    ArchUnMapPage(NULL, NULL, true);

    ArchMapPages(NULL, g_arrBitmap, g_arrBitmap, _BYTES_TO_PAGES(g_qwBitmapSize), PAGING_GLOBAL | PAGING_WRITEABLE | PAGING_NOEXECUTE, true);

    // Protect the RAM file system
    ArchMapPages(NULL, pHeader->pRoot, pHeader->pRoot, (pHeader->qwRootSize + PAGE_SIZE - 1) / PAGE_SIZE, PAGING_GLOBAL | PAGING_NOEXECUTE, true);

    
    // Should already be reserved.
    _ReservePages(pHeader->pRoot, (pHeader->qwRootSize + PAGE_SIZE - 1) / PAGE_SIZE);

    // for (QWORD i = 0; i < 511; i++)
    // {
    //     PTE *pEntry = &((PageTable) PML4_VIRT)[i];
    //     if (!(*pEntry & PF_PRESENT)) continue;;
    //     PageTable pPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    //     for (QWORD j = 0; j < 512; j++)
    //     {
    //         PTE *pEntry = &pPDPT[j];
    //         if (!(*pEntry & PF_PRESENT)) continue;
    //         PageTable pPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    //         for (QWORD k = 0; k < 512; k++)
    //         {
    //             PTE *pEntry = &pPD[k];
    //             if (!(*pEntry & PF_PRESENT)) continue;
    //             if (*pEntry & PF_PAGETABLEATTRIBUTE) continue;
    //             PageTable pPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    //             for (QWORD l = 0; l < 512; l++)
    //             {
    //                 PTE *pEntry = &pPT[l];
    //                 ArchPrintFormat(L"VIRT %p PTE %p\n", l << 12 | k << 21 | j << 30 | i << 39, *pEntry);
    //             }
    //         }
    //     }
    // }
    
    ArchRefreshPages();
}
