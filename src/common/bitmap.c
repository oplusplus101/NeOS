
#include <common/bitmap.h>

BOOL SetBitmap(sBitmap *pBitmap, QWORD qwIndex, BOOL bValue)
{
    if (qwIndex > pBitmap->qwLength * 8) return false;
    QWORD  nByteIndex  = qwIndex / 8;
    BYTE nBitIndex   = qwIndex % 8;
    BYTE nBitMask    = 1 << nBitIndex;
    pBitmap->pData[nByteIndex] &= ~nBitMask;
    pBitmap->pData[nByteIndex] |= nBitMask * bValue;

    return true;
}

BOOL GetBitmap(sBitmap *pBitmap, QWORD qwIndex)
{
    if (qwIndex > pBitmap->qwLength * 8) return false;
    QWORD  nByteIndex  = qwIndex / 8;
    BYTE nBitIndex   = qwIndex % 8;
    BYTE nBitMask    = 1 << nBitIndex;

    return (pBitmap->pData[nByteIndex] & nBitMask) != 0;
}
