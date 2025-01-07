
#ifndef __LIST_H
#define __LIST_H

#include <common/types.h>

typedef struct
{
    PVOID pData;
    QWORD qwLength;
    QWORD qwElementSize;
} sList;

sList CreateEmptyList(QWORD qwElementSize);
void DestroyList(sList *pList);
void SwapElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index);
PVOID GetListElement(sList *pList, QWORD qwIndex);
void SetListElement(sList *pList, QWORD qwIndex, PVOID pData);

#endif // __LIST_H
