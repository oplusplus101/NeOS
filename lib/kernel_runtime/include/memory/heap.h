
#ifndef __MEMORY__HEAP_H
#define __MEMORY__HEAP_H

#include <common/types.h>

// Calculates the header's padding size based on the aligment
// FIXME: Figure out the proper math to account for the header pointer
#define _HEAP_PADDING_SIZE(chunkStart, align) (2 * (align) - (((chunkStart) + sizeof(sHeapHeader)) & ((align) - 1)))
#define _HEAP_CHUNK_SIZE(chunkStart, payloadSize, align) (sizeof(sHeapHeader) + _HEAP_PADDING_SIZE((chunkStart), (align)) + (payloadSize) + sizeof(sHeapFooter))

// Calculates the footer of the header h, based on it's size.
#define _HEAP_FOOTER(h, align) ((sHeapFooter *) ((PBYTE) (h) + sizeof(sHeapHeader) + _HEAP_PADDING_SIZE((QWORD) (h), (align)) + (h)->qwSize))

#define HEAP_PADDING_REDZONE_SEQUENCE 0x55
#define HEAP_FOOTER_REDZONE_SEQUENCE 0xAA
#define HEAP_FOOTER_REDZONE_SIZE 16

typedef struct _tagHeapHeader
{
    struct _tagHeapHeader *pNext;
    BOOL bAllocated;
    QWORD qwSize;
} __attribute__((packed)) sHeapHeader;

typedef struct _tagHeapFooter
{
    BYTE  arrRedzone[HEAP_FOOTER_REDZONE_SIZE];
    QWORD qwSize;
} __attribute__((packed)) sHeapFooter;

typedef struct
{
    sHeapHeader *pFirstChunk;
    QWORD qwStart, qwSize;
    DWORD dwAlignment;
    QWORD qwFreeMemory;
    BOOL  bResizeable, bUser;
} sHeap;

// Internal function for the bootloader to setup kernel memory
void   SetKernelHeap(sHeap *pHeap);
// Internal function for the bootloader to setup kernel memory
sHeap *GetKernelHeap();

/// @brief Creates a heap with the specified size and attributes
/// @param qwSize The size in bytes
/// @param bUser User-mode heap
/// @param bResizeable Can the heap grow or shrink
/// @param dwAlignment Must be a power of two (at least 8 bytes)
/// @param pStart The virtual start address of the heap
/// @param pHeap The resulting heap object
/// @return NEOS_SUCCESS upon success, an error code otherwise
STATUS CreateHeap(QWORD qwSize, BOOL bUser, BOOL bResizeable, DWORD dwAlignment, PVOID pStart, sHeap *pHeap);

/// @brief Destroys the passed heap if nothing is allocated
/// @param pHeap The heap to be destroyed
/// @return NEOS_SUCCESS upon success, an error code otherwise
STATUS DestroyHeap(sHeap *pHeap);

/// @brief Resizes a heap
/// @note When shrinking a heap into allocated memory, the function will fail.
/// @param pHeap The heap to resize
/// @param qwNewSize The new size in pages
/// @return NEOS_SUCCESS upon success, an error code otherwise
STATUS ResizeHeap(sHeap *pHeap, QWORD qwNewSize);

/// @brief Allocates a chunk of aligned memory in the specified heap
/// @param pHeap The heap in which to allocate
/// @param qwSize The size in bytes
/// @return The aligned memory address upon success, NULL otherwise
PVOID HeapAlloc(sHeap *pHeap, QWORD qwSize);

/// @brief Frees memory in a heap
/// @param pHeap The heap in which to free
/// @param pMemory The start of the memory payload
void HeapFree(sHeap *pHeap, PVOID pMemory);

/// @brief Copies pMemory into a new chunk within with a different size, behaves like HeapAlloc if pMemory is NULL
/// @param pHeap The heap in which to allocate 
/// @param pMemory The chunk to be reallocated
/// @param qwNewSize 
/// @return The new memory address upon success, NULL otherwise 
PVOID  HeapReAlloc(sHeap *pHeap, PVOID pMemory, QWORD qwNewSize);
PVOID  KHeapAlloc(QWORD qwSize);
void   KHeapFree(PVOID pMemory);
PVOID  KHeapReAlloc(PVOID pMemory, QWORD qwSize);

#endif // __MEMORY__HEAP_HH
