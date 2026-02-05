
#include <common/list.h>
#include <common/math.h>
#include <common/memory.h>
#include <common/panic.h>

// TODO: Move these into a header
#define LOG_LOG 0
#define LOG_WARNING 1
#define LOG_ERROR 2
#define LOG_GOODBYE 3

extern PVOID KHeapAlloc(QWORD qwSize);
extern void KHeapFree(PVOID pMemory);
extern PVOID KHeapReAlloc(PVOID pOldMemory, QWORD qwSize);
extern void (*Log)(INT, const PWCHAR, ...);

sList CreateEmptyList(QWORD qwElementSize)
{
    sList list;
    list.pData = KHeapAlloc(qwElementSize);
    list.qwLength = 0;
    list.qwElementSize = qwElementSize;
    return list;
}

sList CreateSizedList(QWORD qwLength, QWORD qwElementSize)
{
    sList list;
    list.pData = KHeapAlloc(qwElementSize * qwLength);
    list.qwLength = qwLength;
    list.qwElementSize = qwElementSize;
    ZeroMemory(list.pData, qwLength * qwElementSize);
    return list;
}

void DestroyList(sList *pList)
{
    KHeapFree(pList->pData);
    pList->qwLength = 0;
    pList->qwElementSize = 0;
}

void SwapListElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index)
{
    _ASSERT(pList != NULL, L"Tried to swap elements from a NULL list");
    _ASSERT(pList->pData != NULL, L"Tried to swap elements from a list without data");
    _ASSERT(qwElement1Index < pList->qwLength, L"Tried to move element outside the list, index: %u, list length: %u", qwElement1Index, pList->qwLength);
    _ASSERT(qwElement2Index < pList->qwLength, L"Tried to move element outside the list, index: %u, list length: %u", qwElement2Index, pList->qwLength);
    PVOID pTemp = KHeapAlloc(pList->qwElementSize);
    memcpy(pTemp, GetListElement(pList, qwElement1Index), pList->qwElementSize); // Backup the first element.
    SetListElement(pList, qwElement1Index, GetListElement(pList, qwElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, qwElement2Index, pTemp);
    KHeapFree(pTemp);
}

PVOID GetListElement(sList *pList, QWORD qwIndex)
{
    if (pList == NULL)
    {
        Log(LOG_ERROR, L"Tried to get an element from a NULL list");
        return NULL;
    }
    if (pList->pData == NULL)
    {
        Log(LOG_ERROR, L"Tried to get an element from a list without data");
        return NULL;
    }
    if (qwIndex >= pList->qwLength)
    {
        Log(LOG_ERROR, L"Tried to access element outside list, index: %u, list length: %u", qwIndex, pList->qwLength);
        return NULL;
    }
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID SetListElement(sList *pList, QWORD qwIndex, PVOID pData)
{
    _ASSERT(pList != NULL, L"Tried to set an element from a NULL list");
    _ASSERT(pList->pData != NULL, L"Tried to set an element from a list without data");
    _ASSERT(qwIndex < pList->qwLength, L"Tried to set element outside list, index: %u, list length: %u", qwIndex, pList->qwLength);
    if (pData == NULL)
        ZeroMemory((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pList->qwElementSize);
    else
        memcpy((PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize), pData, pList->qwElementSize);
    return (PVOID) ((QWORD) pList->pData + qwIndex * pList->qwElementSize);
}

PVOID AddListElement(sList *pList, PVOID pData)
{
    _ASSERT(pList != NULL, L"Tried to add an element to a NULL list");
    _ASSERT(pList->pData != NULL, L"Tried to add an element from a list without data");
    PVOID pNewData = KHeapReAlloc(pList->pData, (++pList->qwLength) * pList->qwElementSize);

    if (pNewData == NULL)
    {
        Log(LOG_ERROR, L"Failed to re-allocate memory to add a list element");
        return NULL;
    }
    pList->pData = pNewData;
    
    return SetListElement(pList, pList->qwLength - 1, pData);
}


void InsertListElement(sList *pList, QWORD qwIndex, PVOID pData)
{
    _ASSERT(pList != NULL, L"Tried to instert an element into a NULL list");
    _ASSERT(qwIndex >= pList->qwLength, L"Tried to instert an element outside the list, index: %u, length: %u", qwIndex, pList->qwLength);
    PVOID pNewData = KHeapReAlloc(pList->pData, (++pList->qwLength) * pList->qwElementSize);

    if (pNewData == NULL)
    {
        Log(LOG_ERROR, L"Failed to re-allocate memory to insert a list element");
        return;
    }
    pList->pData = pNewData;
    
    memcpy((PBYTE) pList->pData + (qwIndex + 1) * pList->qwElementSize, (PBYTE) pList->pData + qwIndex * pList->qwElementSize,  pList->qwLength * pList->qwElementSize - 1);
    memcpy((PBYTE) pList->pData + qwIndex * pList->qwElementSize, pData, pList->qwElementSize);
}


void RemoveListElement(sList *pList, QWORD qwIndex)
{
    _ASSERT(pList != NULL, L"Tried to remove an element from a NULL list");
    _ASSERT(pList->pData != NULL, L"Tried to remove an element from a list without data");
    _ASSERT(qwIndex < pList->qwLength, L"Tried to remove element outside list, index: %u, list length: %u", qwIndex, pList->qwLength);
    memcpy((PBYTE) pList->pData + qwIndex * pList->qwElementSize, (PBYTE) pList->pData + (qwIndex + 1) * pList->qwElementSize, (pList->qwLength-- - qwIndex) * pList->qwElementSize);
    pList->pData = KHeapReAlloc(pList->pData, _MAX(1, pList->qwLength) * pList->qwElementSize);
}
