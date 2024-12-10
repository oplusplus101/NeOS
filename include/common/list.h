
#ifndef __LIST_H
#define __LIST_H

#include <common/types.h>

typedef struct
{
    void *pData;
    size_t nLength;
    size_t nElementSize;
} sList;

sList CreateEmptyList(size_t nElementSize);
void DestroyList(sList *pList);
void SwapElements(sList *pList, size_t nElement1Index, size_t nElement2Index);
void *GetListElement(sList *pList, size_t nIndex);
void SetListElement(sList *pList, size_t nIndex, void *pData);

#endif // __LIST_H
