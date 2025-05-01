
#include <memory/list.h>
#include <memory/heap.h>
#include <common/panic.h>
#include <common/memory.h>

sList CreateEmptyList(QWORD qwElementSize)
{
    sList list;
    list.pData = HeapAlloc(qwElementSize);
    list.qwLength = 0;
    list.qwElementSize = qwElementSize;
    return list;
}

sList CreateSizedList(QWORD qwLength, QWORD qwElementSize)
{
    sList list;
    list.pData = HeapAlloc(qwElementSize * qwLength);
    list.qwLength = qwLength;
    list.qwElementSize = qwElementSize;
    ZeroMemory(list.pData, qwLength * qwElementSize);
    return list;
}

void DestroyList(sList *pList)
{
    HeapFree(pList->pData);
    pList->qwLength = 0;
    pList->qwElementSize = 0;
}

void SwapListElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index)
{
    _ASSERT(pList != NULL, "Tried to swap elements from a NULL list");
    _ASSERT(qwElement1Index < pList->qwLength, "Tried to move element outside the list, index: %d, list length: %d", qwElement1Index, pList->qwLength);
    _ASSERT(qwElement2Index < pList->qwLength, "Tried to move element outside the list, index: %d, list length: %d", qwElement2Index, pList->qwLength);
    PVOID pTemp = HeapAlloc(pList->qwElementSize);
    memcpy(pTemp, GetListElement(pList, qwElement1Index), pList->qwElementSize); // Backup the first element.
    SetListElement(pList, qwElement1Index, GetListElement(pList, qwElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, qwElement2Index, pTemp);
}

PVOID GetListElement(sList *pList, QWORD qwIndex)
{
    _ASSERT(pList != NULL, "Tried to get an element from a NULL list");
    _ASSERT(qwIndex < pList->qwLength, "Tried to access element outside list, index: %d, list length: %d", qwIndex, pList->qwLength);
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID SetListElement(sList *pList, QWORD qwIndex, PVOID pData)
{
    _ASSERT(pList != NULL, "Tried to set an element from a NULL list");
    _ASSERT(qwIndex < pList->qwLength, "Tried to set element outside list, index: %d, list length: %d", qwIndex, pList->qwLength);
    if (pData == NULL)
        ZeroMemory((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pList->qwElementSize);
    else
        memcpy((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pData, pList->qwElementSize);
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID AddListElement(sList *pList, PVOID pData)
{
    _ASSERT(pList != NULL, "Tried to add an element to a NULL list");
    pList->pData = HeapReAlloc(pList->pData, (++pList->qwLength) * pList->qwElementSize);
    return SetListElement(pList, pList->qwLength - 1, pData);
}

void RemoveListElement(sList *pList, QWORD qwIndex)
{
    _ASSERT(pList != NULL, "Tried to remove an element from a NULL list");
    _ASSERT(qwIndex < pList->qwLength, "Tried to remove list element outside list, index: %d, list length %d", qwIndex, pList->qwLength);
    memcpy((PBYTE) pList->pData + qwIndex * pList->qwElementSize, (PBYTE) pList->pData + (qwIndex + 1) * pList->qwElementSize, pList->qwLength - qwIndex);
    pList->qwLength--;
    pList->pData = HeapReAlloc(pList->pData, pList->qwLength * pList->qwElementSize);
}
