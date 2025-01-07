
#ifndef __BITMAP_H
#define __BITMAP_H

#include <common/types.h>

typedef struct
{
    BYTE *pData;
    QWORD qwLength;
} sBitmap;

BOOL SetBitmap(sBitmap *pBitmap, QWORD qwIndex, BOOL bValue);
BOOL GetBitmap(sBitmap *pBitmap, QWORD qwIndex);

#endif // __BITMAP_H
