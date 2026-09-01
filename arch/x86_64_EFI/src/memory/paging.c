
#define ARCH_EXE
#include <memory/paging.h>
#include <msr.h>
#include <common/memory.h>
#include <io/screen.h>
#include <io/log.h>
#include <common/math.h>
#include <common/panic.h>

PageTable g_pKernelPML4;
PageTable g_pPML4;

PBYTE g_arrPageBitmap;
QWORD g_qwBitmapLength;

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

WORD _ToAbstractFlags(PTE pte)
{
    return (pte & 0x0FFF) | (pte & PF_NOEXECUTE ? PAGING_NOEXECUTE : 0);
}

PTE _ToNativeFlags(WORD w)
{
    return (w & 0x0FFF) | (w & PAGING_NOEXECUTE ? PF_NOEXECUTE : 0);
}

inline static BOOL _IsReserved(PHYSICAL_ADDRESS Page)
{
    Page /= PAGE_SIZE;
    return (g_arrPageBitmap[Page / 8] & (1 << Page % 8)) != 0;
}

inline static void _ReservePage(PHYSICAL_ADDRESS Page)
{
    Page /= PAGE_SIZE;
    QWORD qwIndex = Page / 8;
    if (qwIndex >= g_qwBitmapLength) return;
    g_arrPageBitmap[qwIndex] |= 1 << (Page % 8);
}

inline static void _ReturnPage(PHYSICAL_ADDRESS Page)
{
    Page /= PAGE_SIZE;
    QWORD qwIndex = Page / 8;
    if (qwIndex >= g_qwBitmapLength) return;
    g_arrPageBitmap[qwIndex] &= ~(1 << (Page % 8));
}


inline static void _ReservePages(PHYSICAL_ADDRESS Page, QWORD qwNumberOfPages)
{
    for (QWORD i = 0; i < qwNumberOfPages; i++)
        _ReservePage(Page + i * PAGE_SIZE);
}


PHYSICAL_ADDRESS SYSV _AllocatePage()
{
    static PHYSICAL_ADDRESS Offset = 0;
    PHYSICAL_ADDRESS Address = 0;
    for (; Offset < g_qwBitmapLength * 8; Offset++)
        if (!_IsReserved(Offset * PAGE_SIZE))
        {
            Address = Offset * PAGE_SIZE;
            break;
        }

    if (Address == 0)
    {
        // Try again from the start
        Offset = 0;
        return _AllocatePage();
    }

    Offset++;

    _ReservePage(Address);
    return Address;
}

#pragma endregion

void SYSV ArchInvalidatePage(VIRTUAL_ADDRESS Page)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(Page) : "memory");
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

void SYSV ArchMapHugePage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PHYSICAL_ADDRESS PhysicalAddress, BYTE bFlags, PageAllocatorFunction pAllocator)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;

    _DisableWriteProtect();
        
    PhysicalAddress = _ALIGN_TO_HUGE_PAGE(PhysicalAddress);
    VirtualAddress  = _ALIGN_TO_HUGE_PAGE(VirtualAddress);

    PageTable arrPML4 = pPageTable;
    // Page Directory Page Table
    PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(VirtualAddress)];
    PageTable arrPDPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPDPT = (PVOID) pAllocator();
        _ASSERT(arrPDPT != 0, L"Could not allocate memory for arrPDPT");
        ZeroMemory(arrPDPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPDPT) | PF_PRESENT | PF_WRITEABLE;
        arrPML4[_ADDRESS_TO_PML4_INDEX(VirtualAddress)] = *pEntry;
    }
    else
        arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)];
    PageTable arrPD;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPD = (PVOID) pAllocator();
        _ASSERT(arrPD != 0, L"Could not allocate memory for arrPD");
        ZeroMemory(arrPD, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPD) | PF_PRESENT | PF_WRITEABLE;
        arrPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)] = *pEntry;
    }
    else
        arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)];
    if (*pEntry & PF_PRESENT && !(*pEntry & PF_PAGETABLEATTRIBUTE))
        _ReturnPage(*pEntry & 0x000FFFFFFFFFF000);

    WORD wFlags = 0;
    if (bFlags & PAGING_WRITEABLE) wFlags |= PF_WRITEABLE;
    if (bFlags & PAGING_UNCACHED)  wFlags |= PF_CACHEDISABLE;
    if (bFlags & PAGING_USERMODE)  wFlags |= PF_USER;
    if (bFlags & PAGING_GLOBAL)    wFlags |= PF_GLOBAL;
    
    *pEntry = _ALIGN_TO_HUGE_PAGE(PhysicalAddress) | PF_PRESENT | PF_PAGETABLEATTRIBUTE | (wFlags & 0x0FFF) | (bFlags & PAGING_NOEXECUTE ? PF_NOEXECUTE : 0);

    _EnableWriteProtect();
}

void _WritePTE4KiB(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PTE pte, PageAllocatorFunction pAllocator)
{
    _DisableWriteProtect();
    PageTable arrPML4 = pPageTable;

    // Page Directory Page Table
    PTE *pEntry = &arrPML4[_ADDRESS_TO_PML4_INDEX(VirtualAddress)];
    if (pte & PF_USER)
        *pEntry |= PF_USER;
    else
        *pEntry &= ~PF_USER;

    PageTable arrPDPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPDPT = (PageTable) pAllocator();
        _ASSERT(arrPDPT != 0, L"Could not allocate memory for arrPDPT");
        ZeroMemory(arrPDPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPDPT) | PF_PRESENT | PF_WRITEABLE | (pte & PF_USER);
        arrPML4[_ADDRESS_TO_PML4_INDEX(VirtualAddress)] = *pEntry;
    }
    else
        arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)];
    if (pte & PF_USER)
        *pEntry |= PF_USER;
    else
        *pEntry &= ~PF_USER;

    PageTable arrPD;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPD = (PageTable) pAllocator();
        _ASSERT(arrPD != 0, L"Could not allocate memory for arrPD");
        ZeroMemory(arrPD, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPD) | PF_PRESENT | PF_WRITEABLE | (pte & PF_USER);
        arrPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)] = *pEntry;
    }
    else
        arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)];
    if (pte & PF_USER)
        *pEntry |= PF_USER;
    else
        *pEntry &= ~PF_USER;

    PageTable arrPT;
    if (!(*pEntry & PF_PRESENT))
    {
        arrPT = (PageTable) pAllocator();
        _ASSERT(arrPT != 0, L"Could not allocate memory for arrPT");
        ZeroMemory(arrPT, PAGE_SIZE);
        *pEntry = _ALIGN_TO_PAGE(arrPT) | PF_PRESENT | PF_WRITEABLE | (pte & PF_USER);
        arrPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)] = *pEntry;
    }
    else
        arrPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    arrPT[_ADDRESS_TO_PT_INDEX(VirtualAddress)] = pte;

    _EnableWriteProtect();
}

void SYSV ArchMapPage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PHYSICAL_ADDRESS PhysicalAddress, WORD wFlags, PageAllocatorFunction pAllocator)
{
    if (!(wFlags & PAGING_PRESENT))
        _KERNEL_PANIC(L"PAGING_PRESENT not specified in ArchMapPage");
    
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    PhysicalAddress = _ALIGN_TO_PAGE(PhysicalAddress);
    VirtualAddress  = _ALIGN_TO_PAGE(VirtualAddress);

    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(VirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto SkipPageFixup;
    PageTable arrPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &arrPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) goto SkipPageFixup;
    PageTable arrPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);
    
    // Page Table
    pEntry = &arrPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)];
    if (!(*pEntry & PF_PAGETABLEATTRIBUTE)) goto SkipPageFixup;

    if (*pEntry & PF_PAGETABLEATTRIBUTE) // For huge pages
    {
        PVOID pPreviousPhysicalAddress = (PVOID) (*pEntry & 0x000FFFFFFFFFF000);
        WORD  wPreviousFlags           = *pEntry & 0x0FFF;
        BOOL bNoExecute                = (*pEntry & PF_NOEXECUTE) != 0;
        wPreviousFlags &= ~PF_PAGETABLEATTRIBUTE;

        PageTable arrPT = (PVOID) pAllocator();
        _ASSERT(arrPT != 0, L"Could not allocate memory for PageTable");
        ZeroMemory(arrPT, PAGE_SIZE);

        for (QWORD i = 0; i < HUGE_PAGE_SIZE / PAGE_SIZE; i++)
        {
            if (_ALIGN_TO_PAGE(VirtualAddress) == _ALIGN_TO_HUGE_PAGE(VirtualAddress) + i * PAGE_SIZE) continue; // Skip this page
            arrPT[i] = _ALIGN_TO_PAGE(_ALIGN_TO_HUGE_PAGE(pPreviousPhysicalAddress) + i * PAGE_SIZE) | wPreviousFlags | (bNoExecute ? PF_NOEXECUTE : 0);
        }

        *pEntry = _ALIGN_TO_PAGE(arrPT) | PF_PRESENT | PF_WRITEABLE;
        arrPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)] = *pEntry;
    }

SkipPageFixup:

    _WritePTE4KiB(pPageTable, VirtualAddress, _ToNativeFlags(wFlags) | (PhysicalAddress & ~(PAGE_SIZE - 1)), pAllocator);
}

void SYSV ArchUnMapPage(PVOID pPageTable, PVOID pVirtualAddress)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;

    _DisableWriteProtect();

    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;

    PageTable pPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Entry
    pEntry = &pPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];

    // Get rid of the entry
    *pEntry = 0;

    _EnableWriteProtect();
}


void SYSV ArchUnMapHugePage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    _DisableWriteProtect();
    
    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(VirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPDPT = (PageTable) _ALIGN_TO_PAGE(*pEntry);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(VirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return;
    PageTable pPD = (PageTable) _ALIGN_TO_PAGE(*pEntry);

    // Page Entry
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(VirtualAddress)];

    // Get rid of the entry
    *pEntry = 0;

    _EnableWriteProtect();
}

WORD SYSV ArchReadPageTableEntry(PVOID pPageTable, PVOID pVirtualAddress, PHYSICAL_ADDRESS *pPhysicalAddress)
{
    if (pPageTable == NULL)
        pPageTable = PML4_VIRT;
    
    // Page Directory Pointer
    PTE *pEntry = &((PageTable) pPageTable)[_ADDRESS_TO_PML4_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return 0;
    PageTable pPDPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Directory
    pEntry = &pPDPT[_ADDRESS_TO_PDPT_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return 0;
    PageTable pPD = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Table
    pEntry = &pPD[_ADDRESS_TO_PD_INDEX(pVirtualAddress)];
    if (!(*pEntry & PF_PRESENT)) return 0;

    if (*pEntry & PF_PAGETABLEATTRIBUTE) // For huge pages
    {
        if (pPhysicalAddress != NULL)
            *pPhysicalAddress = *pEntry & 0x000FFFFFFFFFF000;
        return _ToAbstractFlags(*pEntry);
    }

    PageTable pPT = (PageTable) (*pEntry & 0x000FFFFFFFFFF000);

    // Page Entry
    pEntry = &pPT[_ADDRESS_TO_PT_INDEX(pVirtualAddress)];
    
    if (pPhysicalAddress != NULL)
        *pPhysicalAddress = *pEntry & 0x000FFFFFFFFFF000;
    
    return _ToAbstractFlags(*pEntry);
}

PVOID SYSV ArchGetCurrentPageTable()
{
    return g_pPML4;
}

PVOID SYSV ArchGetKernelPageTable()
{
    return g_pKernelPML4;
}

PVOID SYSV ArchCloneKernelPageTable(PageAllocatorFunction pAllocator)
{
    PageTable pPML4 = (PageTable) pAllocator();
    _ASSERT(pPML4 != 0, L"Could not allocate memory for PageTable");
    if (pPML4 == NULL) return NULL;
    ArchMapPage(NULL, (VIRTUAL_ADDRESS) pPML4, (PHYSICAL_ADDRESS) pPML4, PAGING_WRITEABLE | PF_PRESENT, pAllocator);
    memcpy(pPML4, g_pKernelPML4, PAGE_SIZE);
    pPML4[511] = (PTE) pPML4 | 0x103;
    
    return pPML4;
}


void _InitialisePaging(sARCHHeader *pHeader)
{
    // Enable the NX (No Execute) bit
    wrmsr(IA32_EFER, rdmsr(IA32_EFER) | EFER_NXE);
    _EnableWriteProtect();

    g_arrPageBitmap  = pHeader->pPageBitmap;
    g_qwBitmapLength = pHeader->qwPageBitmapSize;
    g_pKernelPML4    = pHeader->pPML4;
    g_pPML4          = pHeader->pPML4;

    _ReservePages(0, 512); // Reserve the first 2MiB, otherwise strange things will happen

    for (sEFIMemoryDescriptor *pEntry = pHeader->arrMemoryMap; (QWORD) pEntry < (QWORD) pHeader->arrMemoryMap + pHeader->qwMemoryMapSize; pEntry = (sEFIMemoryDescriptor *) ((PBYTE) pEntry + pHeader->qwMemoryDescriptorSize))
    {
        if (pEntry->dwType != 7) // not EfiConventionalMemory
            _ReservePages(pEntry->qwPhysicalStart, pEntry->qwNumberOfPages);
    }

    // Make page zero trigger a page fault
    ArchMapPage(NULL, 0, 0, PAGING_PRESENT, _AllocatePage);
    ArchUnMapPage(NULL, 0);

    // Protect the RAM file system
    for (QWORD i = 0; i < pHeader->qwRootSize + PAGE_SIZE - 1; i += PAGE_SIZE)
    {
        ArchMapPage(NULL,
                    (VIRTUAL_ADDRESS) pHeader->pRoot + i,
                    (PHYSICAL_ADDRESS) pHeader->pRoot + i,
                    PAGING_GLOBAL | PAGING_NOEXECUTE | PAGING_PRESENT,
                    _AllocatePage);
        ArchInvalidatePage((VIRTUAL_ADDRESS) pHeader->pRoot + i);
    }

    // Should already be reserved.
    _ReservePages((PHYSICAL_ADDRESS) pHeader->pRoot, (pHeader->qwRootSize + PAGE_SIZE - 1) / PAGE_SIZE);
}
