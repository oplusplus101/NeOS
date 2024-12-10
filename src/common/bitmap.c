
#include <common/bitmap.h>

BOOL SetBitmap(sBitmap *pBitmap, QWORD nIndex, BOOL bValue)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    QWORD  nByteIndex  = nIndex / 8;
    BYTE nBitIndex   = nIndex % 8;
    BYTE nBitMask    = 1 << nBitIndex;
    pBitmap->pData[nByteIndex] &= ~nBitMask;
    pBitmap->pData[nByteIndex] |= nBitMask * bValue;

    return true;
}

BOOL GetBitmap(sBitmap *pBitmap, QWORD nIndex)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    QWORD  nByteIndex  = nIndex / 8;
    BYTE nBitIndex   = nIndex % 8;
    BYTE nBitMask    = 1 << nBitIndex;

    return (pBitmap->pData[nByteIndex] & nBitMask) != 0;
}
