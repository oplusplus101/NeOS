
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

static inline void memset(void *pDest, uint8_t nByte, size_t nSize)
{
    for (int i = 0; i < nSize; i++)
        ((uint8_t *) pDest)[i] = nByte;
}

static inline void memcpy(void *pDest, void *pSrc, size_t nSize)
{
    for (int i = 0; i < nSize; i++)
        ((uint8_t *) pDest)[i] = ((uint8_t *) pSrc)[i];
}

static inline void memzero(void *pDest, size_t nSize)
{
    memset(pDest, 0, nSize);
}

int8_t memcmp(void *pA, void *pB, size_t nSize);

size_t strlen(const char *sString);
int8_t strcmp(const char *sA, const char *sB);


#endif // __MEMORY_H
