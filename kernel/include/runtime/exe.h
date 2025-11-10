
#ifndef __RUNTIME__EXE_H
#define __RUNTIME__EXE_H

#include <common/list.h>
#include <runtime/objects.h>
#include <memory/paging.h>

typedef struct
{
    CHAR  szName[256];
    WORD  wOrdinal;
    PVOID pFunctionAddress;
    QWORD qwLibraryIndex;
} sExecutableFunction;

typedef struct
{
    CHAR  szName[9];
    DWORD dwRawSize, dwVirtualSize;
    QWORD qwVirtualAddress;
    DWORD dwSectionOffsetWithinFile;
    PVOID pData; // Just the data extracted from the file (not re-mapped in any way)
} sExecutableSection;

typedef struct
{
    QWORD qwEntryPoint;
    sList lstSections;
    QWORD qwBaseAddress;
    sPageTable *pPageTable;
    sList lstExportedFunctions;
    sList lstImportedFunctions;
    sList lstImportedLibraries;
} sExecutable;

typedef struct
{
    sExecutable sEXE;

} sLibraryObject;

// Loads a PE32+ file into memory
// Set qwBaseAddress to 0 in order to use the base address provided by the executable.
sExecutable LoadExecutable(PVOID pFile, QWORD qwBaseAddress);
void FreeExecutable(sExecutable *pExe);

#endif // __RUNTIME__DRIVER_H
