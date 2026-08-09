
#ifndef __ARCH__MEMORY__PAGING_H
#define __ARCH__MEMORY__PAGING_H

#include <common/types.h>

#define PAGE_SIZE 4096
#define HUGE_PAGE_SIZE (1024 * 1024 * 2) // 2 MiB
#define PAGE_TABLE_SIZE 512


#define PAGING_PRESENT            1
#define PAGING_WRITEABLE          2
#define PAGING_USERMODE           4
#define PAGING_WRITETHROUGH       8
#define PAGING_UNCACHED           16
#define PAGING_ACCESSED           32
#define PAGING_DIRTY              64
#define PAGING_HUGE               128
#define PAGING_GLOBAL             256
#define PAGING_NOEXECUTE          512


#ifdef ARCH_EXE


typedef QWORD PTE;
typedef PTE *PageTable;

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

#define _ADDRESS_TO_PML4_INDEX(a) (((SIZE) (a) >> 39) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PDPT_INDEX(a) (((SIZE) (a) >> 30) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PD_INDEX(a)   (((SIZE) (a) >> 21) & (PAGE_TABLE_SIZE - 1))
#define _ADDRESS_TO_PT_INDEX(a)   (((SIZE) (a) >> 12) & (PAGE_TABLE_SIZE - 1))
#define _BYTES_TO_PAGES(n)        (((SIZE) (n) + PAGE_SIZE - 1) / PAGE_SIZE)
#define _PAGE_TO_ADDRESS(p)       ((SIZE) (p) << 12)
#define _ADDRESS_TO_PAGE(a)       ((SIZE) (a) >> 12)
#define _ALIGN_TO_PAGE(n)         ((SIZE) (n) & ~((SIZE) PAGE_SIZE - 1))
#define _ALIGN_TO_HUGE_PAGE(a)    ((SIZE) (a) & ~((SIZE) HUGE_PAGE_SIZE - 1))


void _InitialisePaging(sARCHHeader *pHeader);
WORD _ToAbstractFlags(PTE pte);


#endif // ARCH_EXE

typedef PHYSICAL_ADDRESS (SYSV *PageAllocatorFunction)();

/// @brief Maps a single page from pPhysicalAddress to pVirtualAddress
/// @param pPageTable A page table object. Set this to NULL if the current page table is to be used. 
/// @param pAllocator A function that returns a valid 4KiB-aligned page
void SYSV ArchMapPage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PHYSICAL_ADDRESS PhysicalAddress, WORD wFlags, PageAllocatorFunction pAllocator);

/// @brief Clears a 4KiB page by setting its PTE to zero
/// @param pPageTable A page table object. Set this to NULL if the current page table is to be used. 
void SYSV ArchUnMapPage(PVOID pPageTable, PVOID pVirtualAddress);

/// @brief Maps a single huge (2MiB) page from pPhysicalAddress to pVirtualAddress
/// @param pPageTable A page table object. Set this to NULL if the current page table is to be used. 
/// @param pAllocator A function that returns a valid 4KiB-aligned page
void SYSV ArchMapHugePage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress, PHYSICAL_ADDRESS PhysicalAddress, BYTE bFlags, PageAllocatorFunction pAllocator);


/// @brief Clears a 2MiB page by setting its PTE to zero
/// @param pPageTable A page table object. Set this to NULL if the current page table is to be used. 
void SYSV ArchUnMapHugePage(PVOID pPageTable, VIRTUAL_ADDRESS VirtualAddress);

/// @brief Invalidates a single page
void SYSV ArchInvalidatePage(VIRTUAL_ADDRESS Page);

/// @brief Creates a new page table with the kernel regions (0xFFFFF70000000000+) pointing to the kernel's page table, save for the recursive slot
/// @param pAllocator A function that returns a valid 4KiB-aligned page
/// @return The cloned page table
PVOID SYSV ArchCloneKernelPageTable(PageAllocatorFunction pAllocator);


/// @brief Reads a page table entry and returns an abstraction of the flags along with its physical address
/// @param pPageTable The page table to be read
/// @param pVirtualAddress The virtual address, whose PTE is to be read.
/// @param pPhysicalAddress OPTIONAL: The resulting physical address. Set this to NULL in case it isn't needed.
/// @return The abstracted flags
WORD SYSV ArchReadPageTableEntry(PVOID pPageTable, PVOID pVirtualAddress, PHYSICAL_ADDRESS *pPhysicalAddress);


#endif // __ARCH__MEMORY__PAGING_H
