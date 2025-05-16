
#include <memory/heap.h>
#include <memory/bitmap.h>
#include <common/panic.h>
#include <common/memory.h>

sHeap *g_pKernelHeap;

sHeap CreateHeap(QWORD qwSizeInPages, BOOL bUser)
{
    sHeap h;
    h.qwStart                 = (QWORD) AllocateContinousPages(qwSizeInPages);
    h.qwSize                  = qwSizeInPages * PAGE_SIZE;
    h.pFirstChunk             = (sMemoryChunk *) h.qwStart;
    h.pFirstChunk->bAllocated = false;
    h.pFirstChunk->pPrevious  = NULL;
    h.pFirstChunk->pNext      = NULL;
    h.pFirstChunk->qwLength   = qwSizeInPages * PAGE_SIZE - sizeof(sMemoryChunk);
    h.qwFreeMemory            = h.pFirstChunk->qwLength;
    MapPageRangeToIdentity(NULL, (PVOID) h.qwStart, qwSizeInPages, PF_WRITEABLE | (bUser ? PF_USER : 0));
    return h;
}

void DestroyHeap(sHeap *pHeap)
{
    FreeContinousPages(pHeap->pFirstChunk, pHeap->qwSize / PAGE_SIZE);
}

void SetKernelHeap(sHeap *pHeap)
{
    g_pKernelHeap = pHeap;
}

PVOID KHeapAlloc(QWORD qwLength)
{
    _ASSERT(g_pKernelHeap, "Kernel heap not set");
    return HeapAlloc(g_pKernelHeap, qwLength);
}

void KHeapFree(PVOID pMemory)
{
    _ASSERT(g_pKernelHeap, "Kernel heap not set");
    HeapFree(g_pKernelHeap, pMemory);
}

PVOID KHeapReAlloc(PVOID pMemory, QWORD qwLength)
{
    _ASSERT(g_pKernelHeap, "Kernel heap not set");
    return HeapReAlloc(g_pKernelHeap, pMemory, qwLength);
}

void MergeChunks(sMemoryChunk *pFirstChunk, sMemoryChunk *pSecondChunk)
{
    if (pFirstChunk == NULL || pSecondChunk == NULL || pSecondChunk->pNext == NULL) return;

}

// Returns the second chunk
sMemoryChunk *SplitChunk(sMemoryChunk *pChunk, QWORD qwLength)
{
    sMemoryChunk *pSecondChunk = (sMemoryChunk *) ((QWORD) pChunk + qwLength + sizeof(sMemoryChunk));
    pSecondChunk->bAllocated   = false;
    pSecondChunk->pNext        = pChunk->pNext;
    pSecondChunk->pPrevious    = pChunk;
    pSecondChunk->qwLength     = pChunk->qwLength - qwLength;
    pChunk->pNext              = pSecondChunk;
    pChunk->qwLength           = qwLength;
    return pSecondChunk;
}

PVOID HeapAlloc(sHeap *pHeap, QWORD qwLength)
{
    if (qwLength == 0) return NULL;
    sMemoryChunk *pResult = NULL;
    for (sMemoryChunk *pChunk = pHeap->pFirstChunk; pChunk != NULL && pResult == NULL; pChunk = pChunk->pNext)
    {
        if (pChunk->qwLength >= qwLength && !pChunk->bAllocated)
            pResult = pChunk;
    }

    if (pResult == NULL) return NULL;

    if (pResult->qwLength >= qwLength + sizeof(sMemoryChunk) + 1)
        SplitChunk(pResult, qwLength);

    pResult->bAllocated  = true;
    pHeap->qwFreeMemory -= qwLength + sizeof(sMemoryChunk);

    return (sMemoryChunk *) pResult + 1;
}

void HeapFree(sHeap *pHeap, PVOID pMemory)
{
    _ASSERT(pMemory != NULL, "Tried to free a NULL address");
    sMemoryChunk *pChunk = (sMemoryChunk *) pMemory - 1;
    _ASSERT(pChunk->bAllocated, "Tried to free a non allocated chunk");
    
    return;
    pChunk->bAllocated   = false;
    pHeap->qwFreeMemory += pChunk->qwLength + sizeof(sMemoryChunk);

    if (!pChunk->pPrevious->bAllocated) MergeChunks(pChunk->pPrevious, pChunk);
    if (!pChunk->pNext->bAllocated)     MergeChunks(pChunk, pChunk->pNext);
}

PVOID HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewLength)
{
    if (pMemory == NULL) return HeapAlloc(pHeap, qwNewLength);
    sMemoryChunk *pChunk = (sMemoryChunk *) ((QWORD) pMemory - sizeof(sMemoryChunk));
    QWORD qwPreviousSize = pChunk->qwLength;
    PVOID pNewMemory = HeapAlloc(pHeap, qwNewLength);
    memcpy(pNewMemory, pMemory, qwNewLength >= qwPreviousSize ? qwPreviousSize : qwNewLength);
    HeapFree(pHeap, pMemory);
    return pNewMemory;
}
