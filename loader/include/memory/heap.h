
#ifndef __HEAP_H
#define __HEAP_H

#include <common/types.h>

typedef struct _tagMemoryChunk
{
    struct _tagMemoryChunk *pNext, *pPrevious;
    BOOL bAllocated;
    QWORD qwLength;
} __attribute__((packed)) sMemoryChunk;

void InitHeap(QWORD qwStart, QWORD qwEnd);
PVOID HeapAlloc(QWORD qwLength);
void HeapFree(PVOID pMemory);
PVOID HeapReAlloc(PVOID pMemory, QWORD qwNewSize);

#endif // __HEAP_HH
