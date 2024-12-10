
#include <common/list.h>

sList CreateEmptyList(size_t nElementSize)
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

void SwapListElements(sList *pList, size_t nElement1Index, size_t nElement2Index)
{
    if (nElement1Index >= pList->nLength ||
        nElement2Index >= pList->nLength)
        __PANIC(ERR_LIST_OUT_OF_RANGE);
    void *pTemp = malloc(pList->nElementSize);
    memcpy(pTemp, GetListElement(pList, nElement1Index), pList->nElementSize); // Backup the first element.
    SetListElement(pList, nElement1Index, GetListElement(pList, nElement2Index)); // Move element 2 to element 1.
    SetListElement(pList, nElement2Index, pTemp);
}

void *GetListElement(sList *pList, size_t nIndex)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    return (void *) ((size_t) pList->pData + nIndex * pList->nElementSize);
}

void SetListElement(sList *pList, size_t nIndex, void *pData)
{
    if (nIndex >= pList->nLength) KernelPanic(ERR_LIST_OUT_OF_RANGE, __FILE__, __LINE__);
    memcpy((void *) ((size_t) pList->pData + nIndex * pList->nElementSize), pData, pList->nElementSize);
}
