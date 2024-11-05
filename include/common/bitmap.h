
#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/types.h>

typedef struct
{
    uint8_t *pData;
    size_t nLength;
} sBitmap;

bool SetBitmap(sBitmap *pBitmap, size_t nIndex, bool bValue);
bool GetBitmap(sBitmap *pBitmap, size_t nIndex);

#endif // __BITMAP_H
