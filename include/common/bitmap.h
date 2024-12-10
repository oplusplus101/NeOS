
#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/types.h>

typedef struct
{
    BYTE *pData;
    QWORD nLength;
} sBitmap;

BOOL SetBitmap(sBitmap *pBitmap, QWORD nIndex, BOOL bValue);
BOOL GetBitmap(sBitmap *pBitmap, QWORD nIndex);

#endif // __BITMAP_H
