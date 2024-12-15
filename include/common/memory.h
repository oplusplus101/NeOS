
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

static inline void memset(PVOID pDest, BYTE nByte, SIZE_T nSize)
{
    for (int i = 0; i < nSize; i++)
        ((BYTE *) pDest)[i] = nByte;
}

static inline void memcpy(PVOID pDest, PVOID pSrc, SIZE_T nSize)
{
    for (int i = 0; i < nSize; i++)
        ((BYTE *) pDest)[i] = ((BYTE *) pSrc)[i];
}

static inline void ZeroMemory(PVOID pDest, SIZE_T nSize)
{
    memset(pDest, 0, nSize);
}

#endif // __MEMORY_H
