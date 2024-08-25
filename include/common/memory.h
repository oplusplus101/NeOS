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

#ifndef __MEMORY_H
#define __MEMORY_H

#include <common/types.h>

void memset(void *pDest, uint8_t nByte, size_t nSize);
void memcpy(void *pDest, void *pSrc, size_t nSize);
int8_t memcmp(void *pA, void *pB, size_t nSize);

size_t strlen(const char *sString);
int8_t strcmp(const char *sA, const char *sB);


#endif // __MEMORY_H
