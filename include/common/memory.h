
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

static inline void memset(PVOID pDest, BYTE nByte, QWORD qwSize)
{
    for (int i = 0; i < qwSize; i++)
        ((BYTE *) pDest)[i] = nByte;
}

static inline void memcpy(PVOID pDest, PVOID pSrc, QWORD qwSize)
{
    for (int i = 0; i < qwSize; i++)
        ((BYTE *) pDest)[i] = ((BYTE *) pSrc)[i];
}

static inline void ZeroMemory(PVOID pDest, QWORD qwSize)
{
    memset(pDest, 0, qwSize);
}

#endif // __MEMORY_H
