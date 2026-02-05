
#include <memory/heap.h>
#include <memory/bitmap.h>

#include <common/panic.h>
#include <common/memory.h>

// This needs importing as extern, as the memory addresses are different in the kernel and loader
extern void (*Log)(INT, const PWCHAR, ...);

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

sHeap *g_pKernelHeap;
inline static void WriteChunk(PVOID pStart, QWORD qwPayloadSize, sHeapHeader *pNext, BOOL bAllocated, DWORD dwAlignment);

STATUS CreateHeap(QWORD qwSize, BOOL bUser, BOOL bResizeable, DWORD dwAlignment, PVOID pStart, sHeap *pHeap)
{
    if (((dwAlignment) & ((dwAlignment) - 1)) && dwAlignment >= 8)
        return NEOS_INVALID_ARGUMENT;

    pHeap->qwStart = (QWORD) pStart;
    pHeap->bUser = bUser;

    for (QWORD i = 0; i < _BYTES_TO_PAGES(qwSize); i++)
    {
        PVOID pPage = AllocatePage();
        if (pPage == NULL)
        {
            // Free everything allocated until now
            for (QWORD j = 0; j <= i; j++)
            {
                PVOID pAddress = GetPhysicalAddress(NULL, (PVOID) ((QWORD) pStart + j * PAGE_SIZE));
                if (pAddress == NULL) continue;
                FreePage(pAddress);
            }
            return NEOS_CREATE_HEAP_FAILURE;
        }
        MapPage(NULL, (PVOID) ((QWORD) pStart + i * PAGE_SIZE), pPage, PF_WRITEABLE | (bUser ? PF_USER : 0));
    }
    
    pHeap->dwAlignment             = dwAlignment;
    pHeap->bResizeable             = bResizeable;
    pHeap->qwSize                  = qwSize;
    pHeap->qwFreeMemory            = qwSize - sizeof(sHeapHeader) - sizeof(sHeapFooter) - _HEAP_PADDING_SIZE(pHeap->qwStart, dwAlignment);

    WriteChunk((PVOID) pHeap->qwStart, pHeap->qwFreeMemory, NULL, false, dwAlignment);
    pHeap->pFirstChunk = (sHeapHeader *) pHeap->qwStart;

    return NEOS_SUCCESS;
}


STATUS DestroyHeap(sHeap *pHeap)
{
    for (QWORD i = 0; i < pHeap->qwSize; i++)
    {
        PVOID pAddress = GetPhysicalAddress(NULL, (PVOID) (pHeap->qwStart + i * PAGE_SIZE));
        if (pAddress == NULL) continue;
        FreePage(pAddress);
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

PVOID KHeapAlloc(QWORD qwSize)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    return HeapAlloc(g_pKernelHeap, qwSize);
}

void KHeapFree(PVOID pMemory)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    HeapFree(g_pKernelHeap, pMemory);
}

PVOID KHeapReAlloc(PVOID pMemory, QWORD qwSize)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    return HeapReAlloc(g_pKernelHeap, pMemory, qwSize);
}

STATUS ResizeHeap(sHeap *pHeap, QWORD qwNewSize)
{
    if (!pHeap->bResizeable || pHeap->qwSize == qwNewSize) return NEOS_FAILURE;

    // The heap gets expanded
    if (qwNewSize > pHeap->qwSize)
        for (QWORD i = 0; i < _BYTES_TO_PAGES(qwNewSize - pHeap->qwSize); i++)
        {
            PVOID pPage = AllocatePage();
            // TODO: Free each page allocated within this loop upon failure
            if (pPage == NULL)
                return NEOS_ALLOCATION_FAILURE;
            
            MapPage(NULL, (PVOID) (pHeap->qwStart + pHeap->qwSize + i * PAGE_SIZE), pPage, PF_WRITEABLE | (pHeap->bUser ? PF_USER : 0));
        }
    // The heap gets shrunk
    else
    {
        // First check if the size is too small
        QWORD qwStart = 0;
        for (sHeapHeader *pHeader = pHeap->pFirstChunk;
             pHeader != NULL;
             pHeader = pHeader->pNext,
             qwStart += sizeof(sHeapHeader) + pHeader->qwSize + sizeof(sHeapFooter))
        {
            if (qwStart + pHeader->qwSize + sizeof(sHeapFooter) >= qwNewSize && !pHeader->bAllocated)
                return NEOS_FAILURE | NEOS_OUT_OF_BOUNDS;
        }

        for (QWORD i = 0; i < pHeap->qwSize - qwNewSize; i++)
        {
            PVOID pAddress = GetPhysicalAddress(NULL, (PVOID) (pHeap->qwStart + qwNewSize * PAGE_SIZE + i * PAGE_SIZE));
            if (pAddress == NULL) continue;
            FreePage(pAddress);
        }
    }

    pHeap->qwSize = qwNewSize;

    return NEOS_SUCCESS;
}

// Returns true if intact and false otherwise
inline static BOOL CheckChunkIntegrity(sHeap *pHeap, sHeapHeader *pHeader)
{
    // Check whether the chunk is within bounds
    if ((QWORD) pHeader < pHeap->qwStart || (QWORD) pHeader - sizeof(sHeapHeader) >= pHeap->qwStart + pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Chunk 0x%p is out of bounds in heap 0x%p starting at 0x%p with size %u bytes", pHeader, pHeap, pHeap->qwStart, pHeap->qwSize);
        return false;
    }

    // Check whether the chunk is bigger than the heap
    if (pHeader->qwSize > pHeap->qwSize - sizeof(sHeapHeader) - sizeof(sHeapFooter) - _HEAP_PADDING_SIZE((QWORD) pHeader, pHeap->dwAlignment))
    {
        Log(LOG_WARNING, L"Chunk 0x%p is bigger than the heap 0x%p, size: %u bytes, heap size: %u bytes", pHeader, pHeap, pHeader->qwSize + sizeof(sHeapHeader) + sizeof(sHeapFooter) + _HEAP_PADDING_SIZE((QWORD) pHeader, pHeap->dwAlignment), pHeap->qwSize);
        return false;
    }
    
    // Check the header vs the footer
    if (pHeader->qwSize != _HEAP_FOOTER(pHeader, pHeap->dwAlignment)->qwSize)
    {
        Log(LOG_WARNING, L"Header size %u bytes and footer size %u bytes do not match in heap 0x%p", pHeader->qwSize, _HEAP_FOOTER(pHeader, pHeap->dwAlignment)->qwSize, pHeap);
        return false;
    }
    
    // Check whether the payload's size matches the value listed in the header (and footer)
    QWORD qwPayloadSize = (QWORD) _HEAP_FOOTER(pHeader, pHeap->dwAlignment) - (QWORD) pHeader - sizeof(sHeapHeader) - _HEAP_PADDING_SIZE((QWORD) pHeader, pHeap->dwAlignment);
    if (qwPayloadSize != pHeader->qwSize)
    {
        Log(LOG_WARNING, L"Payload size %u bytes doesn't match the size listed in the header: %u bytes in heap 0x%p", qwPayloadSize, pHeader->qwSize, pHeap);
        return false;
    }

    // Check whether the padding pointer matches the header address
    sHeapHeader *pHeaderPointer = *((sHeapHeader **) ((PBYTE) pHeader + sizeof(sHeapHeader) + _HEAP_PADDING_SIZE((QWORD) pHeader, pHeap->dwAlignment) - sizeof(PVOID)));
    if ((QWORD) pHeader != (QWORD) pHeaderPointer)
    {
        Log(LOG_WARNING, L"Padding pointer 0x%p doesn't match actual header address 0x%p", pHeaderPointer, pHeader);
        return false;
    }
    return true;
}

// Writes all the necessary metadata into a chunk without overwriting the payload.
inline static void WriteChunk(PVOID pStart, QWORD qwPayloadSize, sHeapHeader *pNext, BOOL bAllocated, DWORD dwAlignment)
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

PVOID HeapAlloc(sHeap *pHeap, QWORD qwSize)
{
                                                //  Minimum padding size
    if (pHeap->qwFreeMemory < sizeof(sHeapHeader) + sizeof(PVOID) + qwSize + sizeof(sHeapFooter) || qwSize == 0)
        return NULL;

    sHeapHeader *pResult = NULL;

    for (sHeapHeader *pChunk = pHeap->pFirstChunk; pChunk != 0 && pResult == 0; pChunk = pChunk->pNext)
    {
        if (pChunk->qwSize >= qwSize && !pChunk->bAllocated)
        {
            // TODO: Actually fix the chunk (perhaps with by adding it to a list of corrupted chunks or directly in the HeapAlloc routine or if absolutely necessary just crash)
            if (!CheckChunkIntegrity(pHeap, pChunk)) continue;

            pResult = pChunk;
            break;
        }
    }

    if (pResult == NULL)
    {
        Log(LOG_ERROR, L"Couldn't find a chunk to allocate %u bytes", qwSize);
        return NULL;
    }

    QWORD qwFinalSize = _HEAP_CHUNK_SIZE((QWORD) pResult, qwSize, pHeap->dwAlignment);
    QWORD qwOriginalSize = _HEAP_CHUNK_SIZE((QWORD) pResult, pResult->qwSize, pHeap->dwAlignment);

    if (qwFinalSize < qwOriginalSize)
        SplitChunk(pResult, qwSize, pHeap->dwAlignment);

    pHeap->qwFreeMemory -= qwFinalSize;
    pResult->bAllocated = true;

    return (PBYTE) pResult + sizeof(sHeapHeader) + _HEAP_PADDING_SIZE((QWORD) pResult, pHeap->dwAlignment);
}

void HeapFree(sHeap *pHeap, PVOID pMemory)
{
    // Check for invalid inputs
    if (pHeap == NULL)
    {
        Log(LOG_WARNING, L"Tried to free from a NULL heap");
        return;
    }
    
    if (pMemory == NULL)
    {
        Log(LOG_WARNING, L"Tried to free a NULL address in heap 0x%p", pHeap);
        return;
    }
    
    if ((QWORD) pMemory - sizeof(sHeapHeader) < pHeap->qwStart || (QWORD) pMemory >= pHeap->qwStart + pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Tried to free memory address 0x%p outside heap 0x%p, start 0x%p, end 0x%p", pMemory, pHeap, pHeap->pFirstChunk, (QWORD) pHeap->pFirstChunk + pHeap->qwSize);
        return;
    }
    
    sHeapHeader *pHeader = *((sHeapHeader **) ((PBYTE) pMemory - sizeof(PVOID)));
    if ((QWORD) pHeader < pHeap->qwStart || (QWORD) pHeader >= pHeap->qwStart + pHeap->qwSize - sizeof(sHeapHeader *))
    {
        Log(LOG_WARNING, L"Got invalid header pointer 0x%p in chunk 0x%p in heap 0x%p", pHeader, pMemory, pHeap);
        return;
    }
    
    // Check for a double-free
    if (!pHeader->bAllocated)
    {
        Log(LOG_WARNING, L"Double free detected! Address 0x%p in heap 0x%p", pMemory, pHeap);
        return;
    }
    
    if (!CheckChunkIntegrity(pHeap, pHeader))
        return;
    
    // Free the chunk
    pHeap->qwFreeMemory += _HEAP_CHUNK_SIZE((QWORD) pHeader, pHeader->qwSize, pHeap->dwAlignment);
    pHeader->bAllocated   = false;

    sHeapFooter *pPreviousFooter = (sHeapFooter *) ((PBYTE) pHeader - sizeof(sHeapFooter));

    // Check if the footer is out of bounds.
    // This could either mean the chunk is invalid/corrupted or the first chunk.
    if ((QWORD) pPreviousFooter < pHeap->qwStart)
        goto SkipMergePreviousChunk;
    
    // Sanity check
    if (pPreviousFooter->qwSize >= pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Corrupt footer at 0x%p size %u", pPreviousFooter, pPreviousFooter->qwSize);
        return;
    }

    sHeapHeader *pPreviousChunk = *((sHeapHeader **) ((PBYTE) pPreviousFooter - pPreviousFooter->qwSize - sizeof(sHeapHeader *)));

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
    
    sHeapHeader *pChunk = (sHeapHeader *) ((QWORD) pMemory - sizeof(sHeapHeader));
    QWORD qwPreviousSize = pChunk->qwSize;
    PVOID pNewMemory = HeapAlloc(pHeap, qwNewSize);

    if (pNewMemory == NULL) return NULL;

    memcpy(pNewMemory, pMemory, qwNewSize >= qwPreviousSize ? qwPreviousSize : qwNewSize);
    HeapFree(pHeap, pMemory);
    return pNewMemory;
}
