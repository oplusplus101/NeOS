
#include <memory/heap.h>
#include <memory/mm.h>
#include <memory/pmm.h>
#include <common/panic.h>
#include <common/memory.h>
#include <archimports.h>

// This needs importing as extern, as the memory addresses are different in the kernel and loader
#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

sHeap *g_pKernelHeap;

STATUS CreateHeap(SIZE szLength, BOOL bUser, BOOL bExecutable, BOOL bResizeable, DWORD dwAlignment, VIRTUAL_ADDRESS Start, sHeap *pHeap)
{
    if (((dwAlignment) & ((dwAlignment) - 1)) && dwAlignment >= 8)
        return NEOS_INVALID_ARGUMENT | 0x40000;

    pHeap->Start = Start;
    pHeap->bUser = bUser;

    for (QWORD i = 0; i < szLength; i += PAGE_SIZE)
    {
        PHYSICAL_ADDRESS Page = PMMAllocatePage();
        if (Page == 0)
        {
            // Free everything allocated until now
            for (QWORD j = 0; j <= i; j += PAGE_SIZE)
            {
                PHYSICAL_ADDRESS Address;
                WORD wFlags = ArchReadPageTableEntry(NULL, Start + j, &Address);
                if (!(wFlags & PAGING_PRESENT)) continue;
                PMMFreePage(Address);
            }

            _KERNEL_PANIC(L"Failed to create the kernels heap. Managed to allocate %u pages before failure", i / PAGE_SIZE);
        }
        
        ArchMapPage(NULL, Start + i, Page,
                          PAGING_WRITEABLE | PAGING_NOEXECUTE | PAGING_GLOBAL | PAGING_PRESENT, PMMAllocatePage);
        ArchInvalidatePage(Start + i);

        ZeroMemory((PBYTE) Start + i, PAGE_SIZE);
    }
    

    ArchRefreshPages();
    ZeroMemory((PVOID) Start, szLength);
    pHeap->dwAlignment             = dwAlignment;
    pHeap->bResizeable             = bResizeable;
    pHeap->szLength                = szLength;
    pHeap->qwFreeMemory            = szLength - sizeof(sHeapHeader) - sizeof(sHeapFooter) - _HEAP_PADDING_SIZE(pHeap->Start, dwAlignment);

    WriteChunk((PVOID) pHeap->Start, pHeap->qwFreeMemory, NULL, false, dwAlignment);
    pHeap->pFirstChunk = (sHeapHeader *) pHeap->Start;

    return NEOS_SUCCESS;
}


STATUS DestroyHeap(sHeap *pHeap)
{
    for (QWORD i = 0; i < pHeap->szLength; i++)
    {
        PHYSICAL_ADDRESS Address;
        WORD wFlags = ArchReadPageTableEntry(NULL, pHeap->Start + i * PAGE_SIZE, &Address);
        if (!(wFlags & PAGING_PRESENT)) continue;
        PMMFreePage(Address);
    }
    return NEOS_SUCCESS;
}

void SetKernelHeap(sHeap *pHeap)
{
    g_pKernelHeap = pHeap;
}

sHeap *GetKernelHeap()
{
    return g_pKernelHeap;
}

PVOID MS KHeapAlloc(QWORD szLength)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    return HeapAlloc(g_pKernelHeap, szLength);
}

void MS KHeapFree(PVOID pMemory)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    HeapFree(g_pKernelHeap, pMemory);
}

PVOID MS KHeapReAlloc(PVOID pMemory, QWORD szLength)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    return HeapReAlloc(g_pKernelHeap, pMemory, szLength);
}

STATUS ResizeHeap(sHeap *pHeap, QWORD qwNewSize)
{
    if (!pHeap->bResizeable || pHeap->szLength == qwNewSize) return NEOS_FAILURE;

    // The heap gets expanded
    if (qwNewSize > pHeap->szLength)
        for (QWORD i = 0; i < _BYTES_TO_PAGES(qwNewSize - pHeap->szLength); i++)
        {
            PHYSICAL_ADDRESS Page = PMMAllocatePage();
            // TODO: Free each page allocated within this loop upon failure
            if (Page == 0)
                return NEOS_ALLOCATION_FAILURE;
            ArchMapPage(NULL, pHeap->Start + pHeap->szLength + i * PAGE_SIZE, Page, PAGING_WRITEABLE | (pHeap->bUser ? PAGING_USERMODE : 0), false);
        }

    // The heap gets shrunk
    else
    {
        // First check if the size is too small
        QWORD Start = 0;
        for (sHeapHeader *pHeader = pHeap->pFirstChunk;
             pHeader != NULL;
             pHeader = pHeader->pNext,
             Start += sizeof(sHeapHeader) + pHeader->qwSize + sizeof(sHeapFooter))
        {
            if (Start + pHeader->qwSize + sizeof(sHeapFooter) >= qwNewSize && !pHeader->bAllocated)
                return NEOS_OUT_OF_BOUNDS;
        }

        for (QWORD i = 0; i < pHeap->szLength - qwNewSize; i++)
        {
            PHYSICAL_ADDRESS Address;
            WORD wFlags = ArchReadPageTableEntry(NULL, pHeap->Start + qwNewSize * PAGE_SIZE + i * PAGE_SIZE, &Address);
            if (!(wFlags & PAGING_PRESENT)) continue;
            PMMFreePage(Address);
        }
    }

    pHeap->szLength = qwNewSize;

    return NEOS_SUCCESS;
}

// Returns true if intact and false otherwise
inline static BOOL CheckChunkIntegrity(sHeap *pHeap, sHeapHeader *pHeader, BOOL bCheckAll)
{
    BOOL bStatus = true;
    // Check whether the chunk is within bounds
    if ((QWORD) pHeader < pHeap->Start || (QWORD) pHeader - sizeof(sHeapHeader) >= pHeap->Start + pHeap->szLength)
    {
        ArchLog(LOG_WARNING, L"Chunk 0x%p is out of bounds in heap 0x%p starting at 0x%p with size %u bytes", pHeader, pHeap, pHeap->Start, pHeap->szLength);
        bStatus = false;
        if (!bCheckAll) return false;
    }

    // Check whether the chunk is bigger than the heap
    QWORD qwPaddingSize = _HEAP_PADDING_SIZE((QWORD) pHeader, pHeap->dwAlignment);
    if (pHeader->qwSize > pHeap->szLength - sizeof(sHeapHeader) - sizeof(sHeapFooter) - qwPaddingSize)
    {
        ArchLog(LOG_WARNING, L"Chunk 0x%p is bigger than the heap 0x%p, size: %u bytes, heap size: %u bytes", pHeader, pHeap, pHeader->qwSize + sizeof(sHeapHeader) + sizeof(sHeapFooter) + qwPaddingSize, pHeap->szLength);
        bStatus = false;
        if (!bCheckAll) return false;
    }
    
    // Check the header vs the footer
    if (pHeader->qwSize != _HEAP_FOOTER(pHeader, pHeap->dwAlignment)->qwSize)
    {
        ArchLog(LOG_WARNING, L"Header size %u bytes and footer size %u bytes do not match in heap 0x%p", pHeader->qwSize, _HEAP_FOOTER(pHeader, pHeap->dwAlignment)->qwSize, pHeap);
        bStatus = false;
        if (!bCheckAll) return false;
    }
    
    // Check whether the payload's size matches the value listed in the header (and footer)
    QWORD qwPayloadSize = (QWORD) _HEAP_FOOTER(pHeader, pHeap->dwAlignment) - (QWORD) pHeader - sizeof(sHeapHeader) - qwPaddingSize;
    if (qwPayloadSize != pHeader->qwSize)
    {
        ArchLog(LOG_WARNING, L"Payload size %u bytes doesn't match the size listed in the header: %u bytes in heap 0x%p", qwPayloadSize, pHeader->qwSize, pHeap);
        bStatus = false;
        if (!bCheckAll) return false;
    }

    // The next field must be larger than the previous
    if ((QWORD) pHeader->pNext <= (QWORD) pHeader && (QWORD) pHeader->pNext != 0)
    {
        ArchLog(LOG_WARNING, L"The chunk's next field is lower than the current chunk: current chunk at 0x%p, next chunk 0x%p", pHeader, pHeader->pNext);
        bStatus = false;
        if (!bCheckAll) return false;
    }

    if (pHeader->bAllocated != false && pHeader->bAllocated != true)
    {
        ArchLog(LOG_LOG, L"Allocated boolean was not the expected 0 or 1, but %u", pHeader->bAllocated);
        bStatus = false;
        if (!bCheckAll) return false;
    }


    PBYTE pPadding = (PBYTE) pHeader + sizeof(sHeapHeader);
    for (QWORD i = 0; i < qwPaddingSize - sizeof(sHeapHeader *); i++)
        if (pPadding[i] != HEAP_PADDING_REDZONE_SEQUENCE)
        {
            ArchLog(LOG_WARNING, L"Invalid padding redzone sequence 0x%02X found at address 0x%p in chunk 0x%p", pPadding[i], &pPadding[i], pHeader);
            bStatus = false;
            if (!bCheckAll) return false;
        }
    
    sHeapFooter *pFooter = _HEAP_FOOTER(pHeader, pHeap->dwAlignment);
    for (QWORD i = 0; i < HEAP_FOOTER_REDZONE_SIZE; i++)
        if (pFooter->arrRedzone[i] != HEAP_FOOTER_REDZONE_SEQUENCE)
        {
            ArchLog(LOG_WARNING, L"Invalid footer redzone sequence 0x%02X found at address 0x%p in chunk 0x%p", pFooter->arrRedzone[i], &pFooter->arrRedzone[i], pHeader);
            bStatus = false;
            if (!bCheckAll) return false;
        }
    

    // Check whether the padding pointer matches the header address
    sHeapHeader *pHeaderPointer = *((sHeapHeader **) ((PBYTE) pHeader + sizeof(sHeapHeader) + qwPaddingSize - sizeof(PVOID)));
    if ((QWORD) pHeader != (QWORD) pHeaderPointer)
    {
        ArchLog(LOG_WARNING, L"Padding pointer 0x%p doesn't match actual header address 0x%p", pHeaderPointer, pHeader);
        bStatus = false;
        if (!bCheckAll) return false;
    }

    return bStatus;
}

BOOL CheckHeapIntegrity(sHeap *pHeap)
{
    for (sHeapHeader *pChunk = pHeap->pFirstChunk; pChunk != NULL; pChunk = pChunk->pNext)
        if (!CheckChunkIntegrity(pHeap, pChunk, true))
            return false;
    
    return true;
}

// Writes all the necessary metadata into a chunk without overwriting the payload.
void WriteChunk(PVOID pStart, QWORD qwPayloadSize, sHeapHeader *pNext, BOOL bAllocated, DWORD dwAlignment)
{
    // Header
    sHeapHeader *pHeader = pStart;
    pHeader->qwSize      = qwPayloadSize;
    pHeader->bAllocated  = bAllocated;
    pHeader->pNext       = pNext;

    // Padding
    QWORD qwPaddingSize = _HEAP_PADDING_SIZE((QWORD) pStart, dwAlignment);
    PBYTE pPadding = (PBYTE) pHeader + sizeof(sHeapHeader);

    memset(pPadding, HEAP_PADDING_REDZONE_SEQUENCE, qwPaddingSize - sizeof(sHeapHeader *));

    // Store a pointer back to the header for HeapFree
    *((sHeapHeader **) ((PBYTE) pPadding + qwPaddingSize - sizeof(PVOID))) = pHeader;

    // Footer
    sHeapFooter *pFooter = (sHeapFooter *) (pPadding + qwPaddingSize + qwPayloadSize);
    memset(pFooter->arrRedzone, HEAP_FOOTER_REDZONE_SEQUENCE, HEAP_FOOTER_REDZONE_SIZE);
    pFooter->qwSize = pHeader->qwSize;
}

inline static void MergeChunks(sHeapHeader *pFirstHeader, sHeapHeader *pSecondHeader, DWORD dwAlignment)
{
    if (pFirstHeader == NULL || pSecondHeader == NULL) return;
    pFirstHeader->pNext                             = pSecondHeader->pNext;
    pFirstHeader->qwSize                           += _HEAP_CHUNK_SIZE((QWORD) pSecondHeader, pSecondHeader->qwSize, dwAlignment);
    _HEAP_FOOTER(pFirstHeader, dwAlignment)->qwSize = pFirstHeader->qwSize;
    pSecondHeader->bAllocated                       = false;
}

// Returns the second chunk, if a split is possible, null otherwise
inline static sHeapHeader *SplitChunk(sHeapHeader *pFirstHeader, QWORD qwFirstSize, DWORD dwAlignment)
{
    _ASSERT(qwFirstSize < pFirstHeader->qwSize, L"Tried to split a chunk out of bounds, size: %d chunk size: %d", qwFirstSize, pFirstHeader->qwSize);
    QWORD qwFirstPadding = _HEAP_PADDING_SIZE((QWORD) pFirstHeader, dwAlignment);
    sHeapHeader *pSecondHeader = (sHeapHeader *) ((PBYTE) pFirstHeader + sizeof(sHeapHeader) + qwFirstPadding + qwFirstSize + sizeof(sHeapFooter));
    QWORD qwSecondPadding = _HEAP_PADDING_SIZE((QWORD) pSecondHeader, dwAlignment);
    
    // Make a backup of bAllocated
    BOOL bFirstAllocated = pFirstHeader->bAllocated;
    
    // Ensure that no negative-sized headers occur
    QWORD qwRemainder = qwFirstSize + qwSecondPadding + sizeof(sHeapHeader) + sizeof(sHeapFooter);
    if (qwRemainder >= pFirstHeader->qwSize) return NULL;
    
    QWORD qwSecondSize = pFirstHeader->qwSize - qwRemainder;

    // The second chunk
    WriteChunk(pSecondHeader, qwSecondSize, pFirstHeader->pNext, false, dwAlignment);
    
    // The first chunk
    WriteChunk(pFirstHeader, qwFirstSize, pSecondHeader, bFirstAllocated, dwAlignment);

    return pSecondHeader;
}

PVOID HeapAlloc(sHeap *pHeap, QWORD szLength)
{
                                                //  Minimum padding size
    if (pHeap->qwFreeMemory < sizeof(sHeapHeader) + sizeof(PVOID) + szLength + sizeof(sHeapFooter) || szLength == 0)
        return NULL;

    sHeapHeader *pResult = NULL;

    for (sHeapHeader *pChunk = pHeap->pFirstChunk; pChunk != NULL && pResult == NULL; pChunk = pChunk->pNext)
    {
        if (pChunk->qwSize >= szLength && !pChunk->bAllocated)
        {
            // TODO: Actually fix the chunk (perhaps with by adding it to a list of corrupted chunks or directly in the HeapAlloc routine or if absolutely necessary just crash)
            if (!CheckChunkIntegrity(pHeap, pChunk, false)) continue;

            pResult = pChunk;
            break;
        }
    }
    
    if (pResult == NULL)
    {
        ArchLog(LOG_ERROR, L"Couldn't find a chunk to allocate %u bytes", szLength);
        return NULL;
    }

    if (_HEAP_CHUNK_SIZE((QWORD) pResult, szLength, pHeap->dwAlignment) <
        _HEAP_CHUNK_SIZE((QWORD) pResult, pResult->qwSize, pHeap->dwAlignment))
        SplitChunk(pResult, szLength, pHeap->dwAlignment);
    
    // The chunk size must be recalculated, as SplitChunk could have changed it.
    pHeap->qwFreeMemory -= _HEAP_CHUNK_SIZE((QWORD) pResult, szLength, pHeap->dwAlignment);
    pResult->bAllocated = true;

    return (PBYTE) pResult + sizeof(sHeapHeader) + _HEAP_PADDING_SIZE((QWORD) pResult, pHeap->dwAlignment);
}

void HeapFree(sHeap *pHeap, PVOID pMemory)
{
    // Check for invalid inputs
    if (pHeap == NULL)
    {
        ArchLog(LOG_ERROR, L"Tried to free from a NULL heap");
        return;
    }
    
    if (pMemory == NULL)
    {
        ArchLog(LOG_ERROR, L"Tried to free a NULL address in heap 0x%p", pHeap);
        return;
    }

    if (((QWORD) pMemory & (pHeap->dwAlignment - 1)) != 0)
    {
        ArchLog(LOG_ERROR, L"Tried to free an address 0x%p with invalid alignment in heap 0x%p", pMemory, pHeap);
        return;
    }

    if ((QWORD) pMemory - sizeof(sHeapHeader) < pHeap->Start || (QWORD) pMemory >= pHeap->Start + pHeap->szLength)
    {
        ArchLog(LOG_ERROR, L"Tried to free memory address 0x%p outside heap 0x%p, start 0x%p, end 0x%p", pMemory, pHeap, pHeap->pFirstChunk, (QWORD) pHeap->pFirstChunk + pHeap->szLength);
        return;
    }
    
    sHeapHeader *pHeader = *((sHeapHeader **) ((PBYTE) pMemory - sizeof(PVOID)));
    if ((QWORD) pHeader < pHeap->Start || (QWORD) pHeader >= pHeap->Start + pHeap->szLength - sizeof(sHeapHeader *))
    {
        ArchLog(LOG_ERROR, L"Got invalid header pointer 0x%p from payload 0x%p in heap 0x%p", pHeader, pMemory, pHeap);
        return;
    }
    
    // Check for a double-free
    if (!pHeader->bAllocated)
    {
        ArchLog(LOG_ERROR, L"Double free detected! Address 0x%p in heap 0x%p", pMemory, pHeap);
        return;
    }
    
    if (!CheckChunkIntegrity(pHeap, pHeader, false))
        return;
    
    // Free the chunk
    memset(pMemory, HEAP_FREE_POISON_VALUE, pHeader->qwSize);
    pHeap->qwFreeMemory += _HEAP_CHUNK_SIZE((QWORD) pHeader, pHeader->qwSize, pHeap->dwAlignment);
    pHeader->bAllocated   = false;

    sHeapFooter *pPreviousFooter = (sHeapFooter *) ((PBYTE) pHeader - sizeof(sHeapFooter));

    // Check if the footer is out of bounds.
    // This could either mean the chunk is invalid/corrupted or the first chunk.
    if ((QWORD) pPreviousFooter < pHeap->Start)
        goto SkipMergePreviousChunk;
    
    // Sanity check
    if (pPreviousFooter->qwSize >= pHeap->szLength)
    {
        ArchLog(LOG_WARNING, L"Corrupt footer at 0x%p size %u", pPreviousFooter, pPreviousFooter->qwSize);
        return;
    }

    sHeapHeader *pPreviousChunk = *((sHeapHeader **) ((PBYTE) pPreviousFooter - pPreviousFooter->qwSize- sizeof(sHeapHeader *)));

    // Ensure the header is inside the heap
    if (!pPreviousChunk->bAllocated)
    {
        pHeader = pPreviousChunk;
        MergeChunks(pHeader, pHeader->pNext, pHeap->dwAlignment);
    }

SkipMergePreviousChunk:
    if (pHeader->pNext != NULL && !pHeader->pNext->bAllocated)
        MergeChunks(pHeader, pHeader->pNext, pHeap->dwAlignment);
}

PVOID HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewSize)
{
    if (pMemory == NULL)
        return HeapAlloc(pHeap, qwNewSize);
    
    
    if (((QWORD) pMemory & (pHeap->dwAlignment - 1)) != 0)
    {
        ArchLog(LOG_ERROR, L"Tried to re-allocate an address 0x%p with invalid alignment in heap 0x%p", pMemory, pHeap);
        return NULL;
    }

    
    sHeapHeader *pChunk = (sHeapHeader *) ((QWORD) pMemory - sizeof(sHeapHeader));
    QWORD qwPreviousSize = pChunk->qwSize;
    PVOID pNewMemory = HeapAlloc(pHeap, qwNewSize);

    if (pNewMemory == NULL) return NULL;

    memcpy(pNewMemory, pMemory, qwNewSize >= qwPreviousSize ? qwPreviousSize : qwNewSize);
    HeapFree(pHeap, pMemory);
    return pNewMemory;
}
