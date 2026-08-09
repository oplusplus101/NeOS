
#include <memory/pmm.h>
#include <memory/mm.h>
#include <memory/heap.h>
#include <runtime/objects.h>
#include <common/math.h>
#include <archimports.h>

PBYTE g_arrPageBitmap;
QWORD g_qwBitmapLength;
#define PAGE_SIZE 4096


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

void InitialisePhysicalMemoryManager(PBYTE pPageBitmap, QWORD qwBitmapLength)
{
    g_arrPageBitmap    = pPageBitmap;
    g_qwBitmapLength = qwBitmapLength;
}


PHYSICAL_ADDRESS SYSV PMMAllocatePage()
{
    static PHYSICAL_ADDRESS Offset = 0;
    // ArchPrintFormat(L"Offset %p\n", Offset);
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
        return PMMAllocatePage();
    }

    Offset++;
    
    _ReservePage(Address);
    return Address;
}

PHYSICAL_ADDRESS PMMAllocateContinuousPages(QWORD qwNumPages)
{
    for (QWORD i = 0; i < g_qwBitmapLength * 8; )
    {
        BOOL bAllFree = true;
        QWORD j = 0;
        for (; j < qwNumPages; j++)
            if (_IsReserved((i + j) * PAGE_SIZE))
            {
                bAllFree = false;
                break;
            }
        
        if (!bAllFree)
        {
            i += _MAX(1, j);
            continue;
        }
        
        _ReservePages(i * PAGE_SIZE, qwNumPages);
        return i * PAGE_SIZE;
    }
    
    return 0;
}

void PMMFreePage(PHYSICAL_ADDRESS pPage)
{
    _ReturnPage(pPage);
}


PHYSICAL_ADDRESS PMMAllocateHugePage()
{
    
}

PHYSICAL_ADDRESS PMMAllocateHugeContinuousPages(QWORD qwNumPages)
{

}

