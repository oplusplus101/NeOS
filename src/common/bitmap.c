
#include <common/bitmap.h>

bool SetBitmap(sBitmap *pBitmap, size_t nIndex, bool bValue)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    size_t nByteIndex = nIndex / 8;
    uint8_t nBitIndex = nIndex % 8;
    uint8_t nBitIndexer = 0x80 >> nBitIndex;
    pBitmap->pData[nByteIndex] &= ~nBitIndexer;
    pBitmap->pData[nByteIndex] |= nBitIndex * bValue;

    return true;
}

bool GetBitmap(sBitmap *pBitmap, size_t nIndex)
{
    if (nIndex > pBitmap->nLength * 8) return false;
    size_t nByteIndex = nIndex / 8;
    uint8_t nBitIndex = nIndex % 8;
    uint8_t nBitIndexer = 0x80 >> nBitIndex;

    return pBitmap->pData[nByteIndex] & nBitIndexer != 0;
}
