
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

static inline void memset(void *pDest, BYTE nByte, QWORD nSize)
{
    for (int i = 0; i < nSize; i++)
        ((BYTE *) pDest)[i] = nByte;
}

static inline void memcpy(void *pDest, void *pSrc, QWORD nSize)
{
    for (int i = 0; i < nSize; i++)
        ((BYTE *) pDest)[i] = ((BYTE *) pSrc)[i];
}

static inline void memzero(void *pDest, QWORD nSize)
{
    memset(pDest, 0, nSize);
}

CHAR memcmp(void *pA, void *pB, QWORD nSize);

QWORD strlen(const char *sString);
CHAR strcmp(const char *sA, const char *sB);


#endif // __MEMORY_H
