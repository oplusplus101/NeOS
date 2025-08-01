
#include <KNeOS.h>
#include <NeoList.h>
#include <NeoMemory.h>

sList CreateEmptyList(QWORD qwElementSize)
{
    sList list;
    list.pData = KNeoHeapAllocate(qwElementSize);
    list.qwLength = 0;
    list.qwElementSize = qwElementSize;
    return list;
}

sList CreateSizedList(QWORD qwLength, QWORD qwElementSize)
{
    sList list;
    list.pData = KNeoHeapAllocate(qwElementSize * qwLength);
    list.qwLength = qwLength;
    list.qwElementSize = qwElementSize;
    ZeroMemory(list.pData, qwLength * qwElementSize);
    return list;
}

void DestroyList(sList *pList)
{
    KNeoHeapFree(pList->pData);
    pList->qwLength = 0;
    pList->qwElementSize = 0;
}

void ClearList(sList *pList)
{
    pList->qwLength = 1;
    pList->pData = KNeoHeapReAllocate(pList->pData, pList->qwElementSize);
}

void SwapListElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index)
{
    _ASSERT(pList != NULL, "Tried to swap elements from a NULL list");
    _ASSERT(pList->pData != NULL, "Tried to swap elements from a list without data");
    // TODO: Add a print format function so that these values below can be printed.
    _ASSERT(qwElement1Index < pList->qwLength, "Tried to move element outside the list, index: %d, list length: %d");//, qwElement1Index, pList->qwLength);
    _ASSERT(qwElement2Index < pList->qwLength, "Tried to move element outside the list, index: %d, list length: %d");//, qwElement2Index, pList->qwLength);
    PVOID pTemp = KNeoHeapAllocate(pList->qwElementSize);
    memcpy(pTemp, GetListElement(pList, qwElement1Index), pList->qwElementSize); // Backup the first element.
    SetListElement(pList, qwElement1Index, GetListElement(pList, qwElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, qwElement2Index, pTemp);
    KNeoHeapFree(pTemp);
}

PVOID GetListElement(sList *pList, QWORD qwIndex)
{
    _ASSERT(pList != NULL, "Tried to get an element from a NULL list");
    _ASSERT(pList->pData != NULL, "Tried to get an element from a list without data");
    _ASSERT(qwIndex < pList->qwLength, "Tried to access element outside list, index: %d, list length: %d");//, qwIndex, pList->qwLength);
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID SetListElement(sList *pList, QWORD qwIndex, PVOID pData)
{
    _ASSERT(pList != NULL, "Tried to set an element from a NULL list");
    _ASSERT(pList->pData != NULL, "Tried to set an element from a list without data");
    _ASSERT(qwIndex < pList->qwLength, "Tried to set element outside list, index: %d, list length: %d");//, qwIndex, pList->qwLength);
    if (pData == NULL)
        ZeroMemory((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pList->qwElementSize);
    else
        memcpy((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pData, pList->qwElementSize);
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID AddListElement(sList *pList, PVOID pData)
{
    _ASSERT(pList != NULL, "Tried to add an element to a NULL list");
    _ASSERT(pList->pData != NULL, "Tried to add an element from a list without data");
    pList->pData = KNeoHeapReAllocate(pList->pData, (++pList->qwLength) * pList->qwElementSize);
    return SetListElement(pList, pList->qwLength - 1, pData);
}

void RemoveListElement(sList *pList, QWORD qwIndex)
{
    // _ASSERT(pList != NULL, "Tried to remove an element from a NULL list");
    // _ASSERT(pList->pData != NULL, "Tried to remove an element from a list without data");
    // _ASSERT(qwIndex < pList->qwLength, "Tried to remove list element outside list, index: %d, list length %d");//, qwIndex, pList->qwLength);
    // memcpy((PBYTE) pList->pData + qwIndex * pList->qwElementSize, (PBYTE) pList->pData + (qwIndex + 1) * pList->qwElementSize, (pList->qwLength-- - qwIndex) * pList->qwElementSize);
    // pList->pData = KNeoHeapReAllocate(pList->pData, _MAX(1, pList->qwLength) * pList->qwElementSize);
}
