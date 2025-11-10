
#ifndef __MEMORY__LIST_H
#define __MEMORY__LIST_H

#include <NeoTypes.h>
#include <KNeOS.h>

typedef struct
{
    PVOID pData;
    QWORD qwLength;
    QWORD qwElementSize;
} sList;

FUNC_EXPORT sList CreateEmptyList(QWORD qwElementSize);
// Creates a list with size qwLength filled with zeroes
FUNC_EXPORT sList CreateSizedList(QWORD qwLength, QWORD qwElementSize);
FUNC_EXPORT void ClearList(sList *pList);

FUNC_EXPORT void DestroyList(sList *pList);
FUNC_EXPORT void SwapElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index);
FUNC_EXPORT PVOID GetListElement(sList *pList, QWORD qwIndex);
FUNC_EXPORT PVOID SetListElement(sList *pList, QWORD qwIndex, PVOID pData);
FUNC_EXPORT PVOID AddListElement(sList *pList, PVOID pData);
FUNC_EXPORT void RemoveListElement(sList *pList, QWORD qwIndex);

#endif // __MEMORY__LIST_H
