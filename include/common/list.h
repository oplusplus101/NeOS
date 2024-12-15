
#ifndef __LIST_H
#define __LIST_H

#include <common/types.h>

typedef struct
{
    PVOID pData;
    QWORD nLength;
    QWORD nElementSize;
} sList;

sList CreateEmptyList(QWORD nElementSize);
void DestroyList(sList *pList);
void SwapElements(sList *pList, QWORD nElement1Index, QWORD nElement2Index);
PVOID GetListElement(sList *pList, QWORD nIndex);
void SetListElement(sList *pList, QWORD nIndex, PVOID pData);

#endif // __LIST_H
