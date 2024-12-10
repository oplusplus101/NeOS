
#include <common/bitmap.h>

bool SetBitmap(sBitmap *pBitmap, size_t nIndex, bool bValue)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    size_t  nByteIndex  = nIndex / 8;
    uint8_t nBitIndex   = nIndex % 8;
    uint8_t nBitMask    = 1 << nBitIndex;
    pBitmap->pData[nByteIndex] &= ~nBitMask;
    pBitmap->pData[nByteIndex] |= nBitMask * bValue;

    return true;
}

bool GetBitmap(sBitmap *pBitmap, size_t nIndex)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    size_t  nByteIndex  = nIndex / 8;
    uint8_t nBitIndex   = nIndex % 8;
    uint8_t nBitMask    = 1 << nBitIndex;

    return (pBitmap->pData[nByteIndex] & nBitMask) != 0;
}
