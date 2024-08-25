/*
NeOS: A simple 64-bit operating system
Copyright (C) 2024 Joel Marti

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

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
