
#include <common/list.h>

sList CreateEmptyList(QWORD qwElementSize)
{
    sList list;
    list.pData = malloc(nElementSize);
    list.nLength = 0;
    list.nElementSize = nElementSize;
    return list;
}

void DestroyList(sList *pList)
{
    free(pList->pData);
    pList->nLength = 0;
    pList->nElementSize = 0;
}

void SwapListElements(sList *pList, QWORD qwElement1Index, QWORD qwElement2Index)
{
    if (nElement1Index >= pList->nLength ||
        nElement2Index >= pList->nLength)
        __PANIC(ERR_LIST_OUT_OF_RANGE);
    PVOID pTemp = malloc(pList->nElementSize);
    memcpy(pTemp, GetListElement(pList, nElement1Index), pList->nElementSize); // Backup the first element.
    SetListElement(pList, nElement1Index, GetListElement(pList, nElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, nElement2Index, pTemp);
}

PVOIDGetListElement(sList *pList, QWORD qwIndex)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    return (PVOID) ((QWORD) pList->pData + nIndex * pList->nElementSize);
}

void SetListElement(sList *pList, QWORD qwIndex, PVOID pData)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    memcpy((PVOID) ((QWORD) pList->pData + nIndex * pList->nElementSize), pData, pList->nElementSize);
}
