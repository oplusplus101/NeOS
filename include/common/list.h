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

#ifndef __LIST_H
#define __LIST_H

#include <common/types.h>

typedef struct
{
    void *pData;
    size_t nLength;
    size_t nElementSize;
} sList;

sList CreateEmptyList(size_t nElementSize);
void DestroyList(sList *pList);
void SwapElements(sList *pList, size_t nElement1Index, size_t nElement2Index);
void *GetListElement(sList *pList, size_t nIndex);
void SetListElement(sList *pList, size_t nIndex, void *pData);

#endif // __LIST_H
