
#include <memory/heap.h>
#include <memory/bitmap.h>

#include <common/log.h>
#include <common/panic.h>
#include <common/memory.h>

sHeap *g_pKernelHeap;

sHeap CreateHeap(QWORD qwSizeInPages, BOOL bUser, BOOL bIdentityMapped, PVOID pStart)
{
    sHeap h;

    if (bIdentityMapped)
    {
        h.qwStart    = (QWORD) AllocateContinousPages(qwSizeInPages);
        h.bResizable = false;
        MapPageRangeToIdentity(NULL, (PVOID) h.qwStart, qwSizeInPages, PF_WRITEABLE | (bUser ? PF_USER : 0));
    }
    else
    {
        h.qwStart = (QWORD) pStart;
        h.bResizable = true;
        for (QWORD i = 0; i < qwSizeInPages; i++)
        {
            PVOID pPage = AllocatePage();
            _ASSERT(pPage, L"Could not allocate page for heap");
            MapPage(NULL, (PVOID) ((QWORD) pStart + i * PAGE_SIZE), pPage, PF_WRITEABLE | (bUser ? PF_USER : 0));
        }
    }
    
    h.qwSize                  = qwSizeInPages * PAGE_SIZE;
    h.pFirstChunk             = (sMemoryChunk *) h.qwStart;
    h.pFirstChunk->bAllocated = false;
    h.pFirstChunk->pPrevious  = NULL;
    h.pFirstChunk->pNext      = NULL;
    h.pFirstChunk->qwSize     = qwSizeInPages * PAGE_SIZE - sizeof(sMemoryChunk);
    h.qwFreeMemory            = h.pFirstChunk->qwSize;
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

// void MergeChunks(sMemoryChunk *pFirstChunk, sMemoryChunk *pSecondChunk)
// {
//     PrintFormat("Merging chunk %p and %p with sizes %d and %d\n ", pFirstChunk, pSecondChunk, pFirstChunk->qwSize, pSecondChunk->qwSize);
//     if (pFirstChunk == NULL || pSecondChunk == NULL || pSecondChunk->pNext == NULL) return;
//     pFirstChunk->pNext              = pSecondChunk->pNext;
//     pSecondChunk->pNext->pPrevious  = pFirstChunk;
//     pFirstChunk->qwSize            += sizeof(sMemoryChunk) + pSecondChunk->qwSize;
//     pSecondChunk->bAllocated        = false; // For saftey
// }

// // Returns the second chunk
// sMemoryChunk *SplitChunk(sMemoryChunk *pChunk, QWORD qwFirstChunkSize)
// {
//     _ASSERT(qwFirstChunkSize < pChunk->qwSize, "Tried to split a chunk out of bounds, size: %d chunk size: %d", qwFirstChunkSize, pChunk->qwSize);
//     sMemoryChunk *pSecondChunk     = (sMemoryChunk *) ((PBYTE) (pChunk + 1) + qwFirstChunkSize);
//     pSecondChunk->bAllocated       = false;
//     pSecondChunk->pNext            = pChunk->pNext;
//     pSecondChunk->pPrevious        = pChunk;
//     pSecondChunk->qwSize           = pChunk->qwSize - qwFirstChunkSize - sizeof(sMemoryChunk);
//     pSecondChunk->pNext->pPrevious = pSecondChunk;
//     pChunk->pNext                  = pSecondChunk;
//     pChunk->qwSize                 = qwFirstChunkSize;
//     return pSecondChunk;
// }

PVOID HeapAlloc(sHeap *pHeap, QWORD qwSize)
{
    if (pHeap->qwFreeMemory < qwSize + sizeof(sMemoryChunk))
        return NULL;
    
    sMemoryChunk *pResult = NULL;

    for (sMemoryChunk *pChunk = pHeap->pFirstChunk; pChunk != 0 && pResult == 0; pChunk = pChunk->pNext)
        if (pChunk->qwSize > qwSize && !pChunk->bAllocated)
        {
            pResult = pChunk;
            break;
        }
        
    if (pResult == 0)
        return NULL;
    
    if (pResult->qwSize >= qwSize + sizeof(sMemoryChunk) + 1)
    {
        sMemoryChunk *pTemp = (sMemoryChunk *) ((QWORD) pResult + sizeof(sMemoryChunk) + qwSize);
        
        pTemp->bAllocated = false;
        pTemp->qwSize = pResult->qwSize - qwSize - sizeof(sMemoryChunk);
        pTemp->pPrevious = pResult;
        pTemp->pNext = pResult->pNext;

        if(pTemp->pNext != NULL)
            pTemp->pNext->pPrevious = pTemp;

        pResult->qwSize = qwSize;
        pResult->pNext = pTemp;
    }

    pResult->bAllocated = true;
    pHeap->qwFreeMemory -= sizeof(sMemoryChunk) + qwSize;
    return (PVOID) ((QWORD) pResult + sizeof(sMemoryChunk));
}

void HeapFree(sHeap *pHeap, PVOID pMemory)
{
//    Log(LOG_WARNING, L"HeapFree has not yet been properly implemented - meaning no memory will be freed, Heap 0x%p, ADDR 0x%p", pMemory);
    return;
    sMemoryChunk *pChunk = (sMemoryChunk *) ((QWORD) pMemory - sizeof(sMemoryChunk));
    
    pHeap->qwFreeMemory += sizeof(sMemoryChunk) + pChunk->qwSize;
    pChunk->bAllocated   = false;
    
    if(pChunk->pPrevious != 0 && !pChunk->pPrevious->bAllocated)
    {
        pChunk->pPrevious->pNext   = pChunk->pNext;
        pChunk->pPrevious->qwSize += pChunk->qwSize + sizeof(sMemoryChunk);
        if(pChunk->pNext != 0)
            pChunk->pNext->pPrevious = pChunk->pPrevious;
        
        pChunk = pChunk->pPrevious;
    }
    
    if(pChunk->pNext != 0 && !pChunk->pNext->bAllocated)
    {
        pChunk->qwSize += pChunk->pNext->qwSize + sizeof(sMemoryChunk);
        pChunk->pNext = pChunk->pNext->pNext;
        if(pChunk->pNext != 0)
            pChunk->pNext->pPrevious = pChunk;
    }
}

PVOID HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewSize)
{
    if (pMemory == NULL)
    {
        PVOID pData = HeapAlloc(pHeap, qwNewSize);
        memcpy(pData, pMemory, qwNewSize);
        return pData;
    }
    sMemoryChunk *pChunk = (sMemoryChunk *) ((QWORD) pMemory - sizeof(sMemoryChunk));
    QWORD qwPreviousSize = pChunk->qwSize;
    PVOID pNewMemory = HeapAlloc(pHeap, qwNewSize);
    if (pNewMemory == NULL) return NULL;
    memcpy(pNewMemory, pMemory, qwNewSize >= qwPreviousSize ? qwPreviousSize : qwNewSize);
    HeapFree(pHeap, pMemory);
    return pNewMemory;
}
