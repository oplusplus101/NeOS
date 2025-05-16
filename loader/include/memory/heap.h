
#ifndef __HEAP_H
#define __HEAP_H

#include <common/types.h>


typedef struct _tagMemoryChunk
{
    struct _tagMemoryChunk *pNext, *pPrevious;
    BOOL bAllocated;
    QWORD qwLength;
} __attribute__((packed)) sMemoryChunk;

typedef struct
{
    sMemoryChunk *pFirstChunk;
    QWORD qwStart, qwSize;
    QWORD qwFreeMemory;
} sHeap;

void  SetKernelHeap(sHeap *pHeap);
sHeap CreateHeap(QWORD qwSizeInPages, BOOL bUser);
void  DestroyHeap(sHeap *pHeap);
PVOID HeapAlloc(sHeap *pHeap, QWORD qwLength);
void  HeapFree(sHeap *pHeap, PVOID pMemory);
PVOID HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewLength);
PVOID KHeapAlloc(QWORD qwLength);
void  KHeapFree(PVOID pMemory);
BOOL HeapDefragment(sHeap *pHeap);
PVOID KHeapReAlloc(PVOID pMemory, QWORD qwLength);

#endif // __HEAP_HH
