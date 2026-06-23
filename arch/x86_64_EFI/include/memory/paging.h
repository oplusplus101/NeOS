
#ifndef __ARCH__MEMORY__PAGING_H
#define __ARCH__MEMORY__PAGING_H

#include <common/types.h>

#define PAGE_SIZE 4096
#define HUGE_PAGE_SIZE (1024 * 1024 * 2) // 2 MiB
#define PAGE_TABLE_SIZE 512

#define PAGING_WRITEABLE 1
#define PAGING_UNCACHED  2
#define PAGING_USERMODE  4
#define PAGING_GLOBAL    8
#define PAGING_NOEXECUTE 16

#ifdef ARCH_EXE

#include <arch.h>


#define IA32_EFER 0xC0000080ULL
#define EFER_NXE  (1 << 11)

#define RECURSIVE_SLOT 511ULL
#define PML4_VIRT (PageTable) (0xFFFFULL << 48 | RECURSIVE_SLOT << 39 | RECURSIVE_SLOT << 30 | RECURSIVE_SLOT << 21 | RECURSIVE_SLOT << 12)

#define PF_PRESENT            1
#define PF_WRITEABLE          2
#define PF_USER               4
#define PF_WRITETHROUGH       8
#define PF_CACHEDISABLE       16
#define PF_ACCESSED           32
#define PF_DIRTY              64
#define PF_PAGETABLEATTRIBUTE 128
#define PF_GLOBAL             256
#define PF_NOEXECUTE          0x8000000000000000

#define _ADDRESS_TO_PML4_INDEX(a) (((QWORD) (a) >> 39) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PDPT_INDEX(a) (((QWORD) (a) >> 30) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PD_INDEX(a)   (((QWORD) (a) >> 21) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PT_INDEX(a)   (((QWORD) (a) >> 12) & (PAGE_TABLE_SIZE - 1))
#define _BYTES_TO_PAGES(n)        (((QWORD) (n) + PAGE_SIZE - 1) / PAGE_SIZE)
#define _PAGE_TO_ADDRESS(p)       ((QWORD) (p) << 12)
#define _ADDRESS_TO_PAGE(a)       ((QWORD) (a) >> 12)
#define _ALIGN_TO_PAGE(n)         ((QWORD) (n) & ~((QWORD) PAGE_SIZE - 1))
#define _ALIGN_TO_HUGE_PAGE(a)    ((QWORD) (a) & ~((QWORD) HUGE_PAGE_SIZE - 1))


void _InitialisePaging(sARCHHeader *pHeader);


#endif // ARCH_EXE

void SYSV ArchMapPage(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, BYTE bFlags, BOOL bRefreshPage);

/// @brief 
/// @param pPageTable 
/// @param pVirtualAddress 
/// @param pPhysicalAddress 
/// @param qwNumberOfPages 
/// @param bFlags 
/// @param bRefreshPage 
/// @warning bRefresh page doesn't apply to global pages
/// @return 
void SYSV ArchMapPages(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwNumberOfPages, BYTE bFlags, BOOL bRefreshPage);

void SYSV ArchUnMapPage(PVOID pPageTable, PVOID pVirtualAddress, BOOL bRefreshPage);
void SYSV ArchUnMapPages(PVOID pPageTable, PVOID pVirtualAddress, QWORD qwNumberOfPages, BOOL bRefreshPage);

void SYSV ArchMapHugePage(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, BYTE bFlags, BOOL bRefreshPage);
void SYSV ArchMapHugePages(PVOID pPageTable, PVOID pVirtualAddress, PVOID pPhysicalAddress, QWORD qwNumberOfPages, BYTE bFlags, BOOL bRefreshPage);

void SYSV ArchUnMapHugePage(PVOID pPageTable, PVOID pVirtualAddress, BOOL bRefreshPage);
void SYSV ArchUnMapHugePages(PVOID pPageTable, PVOID pVirtualAddress, QWORD qwNumberOfPages, BOOL bRefreshPage);


/// @brief Returns the physical address, which pVirtualAddress is mapped to
/// @return The physical address upon success, NULL otherwise
PVOID ArchGetPhysicalAddress(PVOID pPageTable, PVOID pVirtualAddress);

/// @brief Tries to allocate one page
/// @param pStart The virtual start of the page. Set this to NULL for an identity mapping
/// @return The physical page addresss upon success, NULL otherwise
/// @warning This function does not guarantee that the returned page is mapped
PVOID SYSV ArchAllocatePage();

/// @brief Tries to allocate a continuous region of pages
/// @return The start of the region upon success, NULL otherwise
/// @warning This function does not guarantee that the returned pages are mapped
PVOID SYSV ArchAllocatePagesContinuous(QWORD qwNumberOfPages);

/// @brief Frees a single page
void SYSV ArchFreePage(PVOID pPage);

/// @brief Frees a Continuous region of pages and maps them to identity
void SYSV ArchFreePagesContinuous(PVOID pPages, QWORD qwNumberOfPages);

#endif // __ARCH__MEMORY__PAGING_H
