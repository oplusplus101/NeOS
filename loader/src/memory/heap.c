
#include <memory/heap.h>
#include <common/memory.h>
#include <common/panic.h>

static sMemoryChunk *g_pFirst;
static QWORD g_qwFreeMemory;

void InitHeap(QWORD qwStart, QWORD qwEnd)
{
    g_pFirst             = (sMemoryChunk *) qwStart;
    g_pFirst->bAllocated = false;
    g_pFirst->pPrevious  = NULL;
    g_pFirst->pNext      = NULL;
    g_pFirst->qwLength   = qwEnd - qwStart - sizeof(sMemoryChunk);
    g_qwFreeMemory = g_pFirst->qwLength;
}

PVOID HeapAlloc(QWORD qwLength)
{
    _ASSERT(qwLength > 0, "Tried to allocate 0 bytes");
    sMemoryChunk *pResult = NULL;
    for (sMemoryChunk *pChunk = g_pFirst; pChunk != NULL && pResult == NULL; pChunk = pChunk->pNext)
        if (pChunk->qwLength >= qwLength && !pChunk->bAllocated)
            pResult = pChunk;
    
    _ASSERT(pResult != NULL, "Out of memory! Tried to allocate %u bytes with %u bytes free", qwLength, g_qwFreeMemory);

    if (pResult->qwLength >= qwLength + sizeof(sMemoryChunk) + 1)
    {
        sMemoryChunk *pTemp = (sMemoryChunk *) (((QWORD) pResult) + sizeof(sMemoryChunk) + qwLength);
        pTemp->pNext      = pResult->pNext;
        pTemp->pPrevious  = pResult;
        pTemp->bAllocated = false;
        pTemp->qwLength   = pResult->qwLength - qwLength - sizeof(sMemoryChunk);

        if (pTemp->pNext != NULL)
            pTemp->pNext->pPrevious = pTemp;
        
        pResult->qwLength = qwLength;
        pResult->pNext    = pTemp;
    }

    pResult->bAllocated = true;
    g_qwFreeMemory -= qwLength;
    return (PVOID) (((QWORD) pResult) + sizeof(sMemoryChunk));
}

void HeapFree(PVOID pMemory)
{
    sMemoryChunk *pChunk = (sMemoryChunk *) ((QWORD) pMemory - sizeof(sMemoryChunk));
    pChunk->bAllocated   = false;
    g_qwFreeMemory += pChunk->qwLength;

    if (pChunk->pPrevious != NULL && !pChunk->pPrevious->bAllocated)
    {
        pChunk->pPrevious->pNext     = pChunk->pNext;
        pChunk->pPrevious->qwLength += pChunk->qwLength + sizeof(sMemoryChunk);

        if (pChunk->pNext != NULL)
            pChunk->pNext->pPrevious = pChunk;
        
        pChunk = pChunk->pPrevious;
    }

    if (pChunk->pNext != NULL && !pChunk->pNext->bAllocated)
    {
        pChunk->qwLength += pChunk->pNext->qwLength + sizeof(sMemoryChunk);
        pChunk->pNext     = pChunk->pNext->pNext;

        if (pChunk->pNext != NULL)
            pChunk->pNext->pPrevious = pChunk;
    }
}

PVOID HeapReAlloc(PVOID pMemory, QWORD qwNewSize)
{
    sMemoryChunk *pChunk = (sMemoryChunk *) ((QWORD) pMemory - sizeof(sMemoryChunk));
    QWORD qwPreviousSize = pChunk->qwLength;
    PVOID pNewMemory = HeapAlloc(qwNewSize);
    memcpy(pNewMemory, pMemory, qwNewSize >= qwPreviousSize ? qwPreviousSize : qwNewSize);
    HeapFree(pMemory);
    return pNewMemory;
}
