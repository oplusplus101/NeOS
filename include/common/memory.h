
#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

void memzero(void *pDest, size_t nSize);
void memset(void *pDest, uint8_t nByte, size_t nSize);
void memcpy(void *pDest, void *pSrc, size_t nSize);
int8_t memcmp(void *pA, void *pB, size_t nSize);

size_t strlen(const char *sString);
int8_t strcmp(const char *sA, const char *sB);


#endif // __MEMORY_H
