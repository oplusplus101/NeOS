
#ifndef __RUNTIME__EXE_H
#define __RUNTIME__EXE_H

#include <memory/list.h>

typedef struct
{
    PCHAR szName;
    PVOID pFunctionAddress;
} sExecutableFunction;

typedef struct
{
    CHAR  szName[9];
    DWORD dwRawSize, dwVirtualSize;
    QWORD qwVirtualAddress;
    PVOID pData; // Just the data extracted from the file (not re-mapped in any way)
} sExecutableSection;

typedef struct
{
    PVOID pEntryPoint;
    sList lstSections;
    sList lstExportedFunctions;
} sExecutable;

sExecutable ParsePE32(PVOID pEXEData);
void FreeExecutable(sExecutable *pExe);

#endif // __RUNTIME__DRIVER_H
