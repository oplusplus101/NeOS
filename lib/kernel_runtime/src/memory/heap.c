
#include <memory/heap.h>
#include <memory/bitmap.h>

#include <common/panic.h>
#include <common/memory.h>

// These need be imported as extern, since the memory addresses are different in the kernel and loader
extern void DumpHeap(sHeap *pHeap);

#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3
extern void (*Log)(INT, const PWCHAR, ...);

sHeap *g_pKernelHeap;

STATUS CreateHeap(QWORD qwSize, BOOL bUser, BOOL bResizeable, WORD wAlignment, PVOID pStart, sHeap *pHeap)
{
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
    
    pHeap->bResizeable             = bResizeable;
    pHeap->qwSize                  = qwSize;
    pHeap->qwFreeMemory            = qwSize - sizeof(sHeapHeader) - sizeof(sHeapFooter);

    // Initialise the header
    pHeap->pFirstChunk             = (sHeapHeader *) pHeap->qwStart;
    pHeap->pFirstChunk->bAllocated = false;
    pHeap->pFirstChunk->pNext      = NULL;
    pHeap->pFirstChunk->qwSize     = qwSize - sizeof(sHeapHeader) - sizeof(sHeapFooter);
    
    // Initialise the footer
    sHeapFooter *pFooter           = _HEAP_FOOTER(pHeap->pFirstChunk);
    pFooter->qwSize                = pHeap->pFirstChunk->qwSize;

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

void KHeapFree (PVOID pMemory)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    HeapFree(g_pKernelHeap, pMemory);
}

PVOID KHeapReAlloc(PVOID pMemory, QWORD qwSize)
{
    _ASSERT(g_pKernelHeap, L"Kernel heap not set");
    return HeapReAlloc(g_pKernelHeap, pMemory, qwSize);
}

// Returns true if intact and false otherwise
BOOL CheckChunkIntegrity(sHeap *pHeap, sHeapHeader *pHeader)
{
    // Check whether the chunk is within bounds
    if ((QWORD) pHeader < pHeap->qwStart || (QWORD) pHeader - sizeof(sHeapHeader) >= pHeap->qwStart + pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Header 0x%p is out of bounds in heap 0x%p starting at 0x%p with size %u bytes", pHeader, pHeap, pHeap->qwStart, pHeap->qwSize);
        return false;
    }

    // Check whether the chunk is bigger than the heap
    if (pHeader->qwSize >= pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Header 0x%p is bigger than the heap 0x%p, header size: %u bytes, heap size: %u bytes", pHeader, pHeap, pHeader->qwSize, pHeap->qwSize);
        return false;
    }
    
    // Check the header vs the footer
    if (pHeader->qwSize != _HEAP_FOOTER(pHeader)->qwSize)
    {
        Log(LOG_WARNING, L"Header size %u bytes and Footer size %u bytes do not match in heap 0x%p", pHeader->qwSize, _HEAP_FOOTER(pHeader)->qwSize, pHeap);
        return false;
    }
    
    // Check whether the payload's size matches the value listed in the header (and footer)
    QWORD qwPayloadSize = (QWORD) _HEAP_FOOTER(pHeader) - (QWORD) pHeader - sizeof(sHeapHeader);
    if (qwPayloadSize != pHeader->qwSize)
    {
        Log(LOG_WARNING, L"Payload size %u bytes doesn't match the size listed in the header: %u bytes in heap 0x%p", qwPayloadSize, pHeader->qwSize, pHeap);
        return false;
    }

    return true;
}

void MergeChunks(sHeapHeader *pFirstHeader, sHeapHeader *pSecondHeader)
{
    if (pFirstHeader == NULL || pSecondHeader == NULL) return;
    pFirstHeader->pNext                = pSecondHeader->pNext;
    pFirstHeader->qwSize              += sizeof(sHeapHeader) + pSecondHeader->qwSize + sizeof(sHeapFooter);
    _HEAP_FOOTER(pFirstHeader)->qwSize = pFirstHeader->qwSize;
    pSecondHeader->bAllocated          = false;
}

// Returns the second chunk
sHeapHeader *SplitChunk(sHeapHeader *pFirstHeader, QWORD qwFirstSize)
{
    _ASSERT(qwFirstSize < pFirstHeader->qwSize, L"Tried to split a chunk out of bounds, size: %d chunk size: %d", qwFirstSize, pFirstHeader->qwSize);
    sHeapHeader *pSecondHeader = (sHeapHeader *) ((PBYTE) pFirstHeader + sizeof(sHeapHeader) + qwFirstSize + sizeof(sHeapFooter));

    // Create the second header
    pSecondHeader->pNext                = pFirstHeader->pNext;
    pSecondHeader->qwSize               = pFirstHeader->qwSize - qwFirstSize - sizeof(sHeapHeader) - sizeof(sHeapFooter);
    _HEAP_FOOTER(pSecondHeader)->qwSize = pSecondHeader->qwSize;
    pSecondHeader->bAllocated           = false;
    
    // Resize the first header
    pFirstHeader->qwSize                = qwFirstSize;
    _HEAP_FOOTER(pFirstHeader)->qwSize  = qwFirstSize;
    pFirstHeader->pNext                 = pSecondHeader;
    return pSecondHeader;
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

    pHeap->qwSize = qwNewSize * PAGE_SIZE;

    return NEOS_SUCCESS;
}

PVOID HeapAlloc(sHeap *pHeap, QWORD qwSize)
{
    if (pHeap->qwFreeMemory < sizeof(sHeapHeader) + qwSize + sizeof(sHeapFooter) || qwSize == 0)
        return NULL;

    sHeapHeader *pResult = NULL;

    for (sHeapHeader *pChunk = pHeap->pFirstChunk; pChunk != 0 && pResult == 0; pChunk = pChunk->pNext)
        if (pChunk->qwSize >= qwSize && !pChunk->bAllocated)
        {
            // TODO: Actually have the heap try to fix the chunk (perhaps with by adding it to a list of corrupted chunks or directly in the HeapAlloc routine)
            CheckChunkIntegrity(pHeap, pChunk);

            pResult = pChunk;
            break;
        }
        
    if (pResult == 0)
        return NULL;
    
    if (pResult->qwSize >= sizeof(sHeapHeader) + qwSize + sizeof(sHeapFooter) + 1)
        SplitChunk(pResult, qwSize);

    pResult->bAllocated = true;
    pHeap->qwFreeMemory -= sizeof(sHeapHeader) + qwSize + sizeof(sHeapFooter);
    
    return (PVOID) (pResult + 1);
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
    
    sHeapHeader *pHeader = (sHeapHeader *) ((PBYTE) pMemory - sizeof(sHeapHeader));

    // Check for a double-free
    if (!pHeader->bAllocated)
    {
        Log(LOG_WARNING, L"Double free detected! Address 0x%p in heap 0x%p", pMemory, pHeap);
        return;
    }
    
    if (!CheckChunkIntegrity(pHeap, pHeader))
        return;
    
    // Free the chunk
    pHeap->qwFreeMemory += sizeof(sHeapHeader) + pHeader->qwSize + sizeof(sHeapFooter);
    pHeader->bAllocated   = false;

    sHeapFooter *pPreviousFooter = (sHeapFooter *) ((PBYTE) pHeader - sizeof(sHeapFooter));

    // Check if the footer is out of bounds.
    // This could either mean the chunk is invalid/corrupted or the first chunk.
    if ((QWORD) pPreviousFooter < pHeap->qwStart)
        goto SkipMergePreviousChunk;
    
    // Sanity check
    if (pPreviousFooter->qwSize >= pHeap->qwSize)
    {
        Log(LOG_WARNING, L"Corrupted footer at 0x%p size %d", pPreviousFooter, pPreviousFooter->qwSize);
        return;
    }

    sHeapHeader *pPreviousChunk = (sHeapHeader *) ((PBYTE) pPreviousFooter - pPreviousFooter->qwSize - sizeof(sHeapHeader));
    
    // Ensure the header is inside the heap
    if ((QWORD) pPreviousChunk >= pHeap->qwStart && !pPreviousChunk->bAllocated)
    {
        pHeader = pPreviousChunk;
        MergeChunks(pHeader, pHeader->pNext);
    }

SkipMergePreviousChunk:
    if (pHeader->pNext && !pHeader->pNext->bAllocated)
        MergeChunks(pHeader, pHeader->pNext);
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
