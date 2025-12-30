
#ifndef __MEMORY__HEAP_H
#define __MEMORY__HEAP_H

#include <common/types.h>


typedef struct _tagMemoryChunk
{
    struct _tagMemoryChunk *pNext, *pPrevious;
    BOOL bAllocated;
    QWORD qwSize;
} __attribute__((packed)) sMemoryChunk;

typedef struct
{
    sMemoryChunk *pFirstChunk;
    QWORD qwStart, qwSize;
    QWORD qwFreeMemory;
    BOOL  bResizable;
} sHeap;

void  SetKernelHeap(sHeap *pHeap);
sHeap *GetKernelHeap();
sHeap CreateHeap(QWORD qwSizeInPages, BOOL bUser, BOOL bIdentityMapped, PVOID pStart);
void  DestroyHeap(sHeap *pHeap);
PVOID HeapAlloc(sHeap *pHeap, QWORD qwSize);
void  HeapFree(sHeap *pHeap, PVOID pMemory);
PVOID HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewSize);
PVOID KHeapAlloc(QWORD qwSize);
void  KHeapFree(PVOID pMemory);
BOOL HeapDefragment(sHeap *pHeap);
PVOID KHeapReAlloc(PVOID pMemory, QWORD qwSize);

#endif // __MEMORY__HEAP_HH
