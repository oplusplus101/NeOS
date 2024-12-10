
#include <common/list.h>

sList CreateEmptyList(QWORD nElementSize)
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

void SwapListElements(sList *pList, QWORD nElement1Index, QWORD nElement2Index)
{
    if (nElement1Index >= pList->nLength ||
        nElement2Index >= pList->nLength)
        __PANIC(ERR_LIST_OUT_OF_RANGE);
    void *pTemp = malloc(pList->nElementSize);
    memcpy(pTemp, GetListElement(pList, nElement1Index), pList->nElementSize); // Backup the first element.
    SetListElement(pList, nElement1Index, GetListElement(pList, nElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, nElement2Index, pTemp);
}

void *GetListElement(sList *pList, QWORD nIndex)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    return (void *) ((QWORD) pList->pData + nIndex * pList->nElementSize);
}

void SetListElement(sList *pList, QWORD nIndex, void *pData)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    memcpy((void *) ((QWORD) pList->pData + nIndex * pList->nElementSize), pData, pList->nElementSize);
}
