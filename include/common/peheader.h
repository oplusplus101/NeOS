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

#ifndef __PEHDR_H
#define __PEHDR_H

#include <common/types.h>

typedef struct
{
	uint32_t nMagic;
	uint16_t nMachine;
	uint16_t nNumberOfSections;
	uint32_t nTimeDateStamp;
	uint32_t nPointerToSymbolTable;
	uint32_t nNumberOfSymbols;
	uint16_t nSizeOfOptionalHeader;
	uint16_t nCharacteristics;
} __attribute__((packed)) sPEHeader;

#endif // __PEHDR_H
