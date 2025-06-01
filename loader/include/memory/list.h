
#ifndef __MEMORY__LIST_H
#define __MEMORY__LIST_H

#include <common/types.h>

typedef struct
{
    PVOID pData;
    QWORD qwLength;
    QWORD qwElementSize;
} sList;

sList CreateEmptyList(QWORD qwElementSize);
// Creates a list with size qwLength filled with zeroes
sList CreateSizedList(QWORD qwLength, QWORD qwElementSize);

void DestroyList(sList *pList);
void SwapElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index);
PVOID GetListElement(sList *pList, QWORD qwIndex);
PVOID SetListElement(sList *pList, QWORD qwIndex, PVOID pData);
PVOID AddListElement(sList *pList, PVOID pData);
void RemoveListElement(sList *pList, QWORD qwIndex);

#endif // __MEMORY__LIST_H
