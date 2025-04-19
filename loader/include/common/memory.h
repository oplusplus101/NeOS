
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

static inline void memset(PVOID pDest, BYTE nByte, QWORD qwSize)
{
#ifndef DO_NOT_OPTIMIZE_MEMORY_ROUTINES
    __asm__ volatile ("mov %%rdx, %%rdi\nrep stosb" : : "a"(nByte), "c"(qwSize), "d"(pDest));
#else
    for (QWORD i = 0; i < qwSize; i++)
        ((BYTE *) pDest)[i] = nByte;
#endif
}

static inline void memcpy(PVOID pDest, PVOID pSource, QWORD qwSize)
{
#ifndef DO_NOT_OPTIMIZE_MEMORY_ROUTINES
    __asm__ volatile ("mov %%rax, %%rsi\nmov %%rbx, %%rdi\nrep movsb" : : "a"(pSource), "b"(pDest), "c"(qwSize));
#else
    for (QWORD i = 0; i < qwSize; i++)
        ((BYTE *) pDest)[i] = ((BYTE *) pSrc)[i];
#endif
}

static inline int memcmp(PVOID pA, PVOID pB, QWORD qwSize)
{
    for (QWORD i = 0; i < qwSize; i++)
    {
        if (((BYTE *) pA)[i] < ((BYTE *) pB)[i])
            return -1;
        else if (((BYTE *) pA)[i] > ((BYTE *) pB)[i])
            return 1;
    }

    return 0;
}

static inline void ZeroMemory(PVOID pDest, QWORD qwSize)
{
#ifndef DO_NOT_OPTIMIZE_MEMORY_ROUTINES
    __asm__ volatile ("xor %%rax, %%rax\nmov %%rdx, %%rdi\nrep stosb" : : "c"(qwSize), "d"(pDest));
#else
    for (QWORD i = 0; i < qwSize; i++)
        ((BYTE *) pDest)[i] = 0;
#endif
}

#endif // __MEMORY_H
