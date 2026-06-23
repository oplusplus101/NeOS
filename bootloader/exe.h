
#ifndef __EXE_H
#define __EXE_H

#include <common/types.h>
#include <common/exestructs.h>
#include "fs.h"

typedef struct
{
    sMZHeader sMZ;
    sPE32Header sPE;
    sPE32OptionalHeader sPEOptional;
    sPE32DataDirectory sDataDirectory;
    DWORD nsSections;
    sPE32SectionHeader *arrSections;
} sPE32Headers;

typedef struct
{
    CHAR  szName[256];
    WORD  wOrdinal;
    PVOID pFunctionAddress;
    QWORD qwLibraryIndex;
} sExecutableFunction;

DWORD RVAToOffset(DWORD dwRVA, sPE32Headers *pHeaders);
STATUS LoadRelocations(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwPhysicalStart, QWORD qwVirtualStart);
STATUS LoadImports(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwBaseAddress);
STATUS LoadExports(sRAMFile *pFile, sPE32Headers *pHeaders, sExecutableFunction **ppExecutableFunctions, PQWORD pnsExportFuncions, QWORD qwBaseAddress);
PVOID GetExportFunction(PCHAR szName);
STATUS ParsePEHeaders(sRAMFile *pFile, sPE32Headers *pHeaders);
QWORD CalculatePESize(sPE32Headers *pHeaders);
STATUS LoadExecutableTo(sRAMFile *pFile, QWORD qwVirtualStart, QWORD qwDesiredPhysicalAddress, sPE32Headers *pHeaders);
STATUS InitialiseExecutableLoader(sRAMFile *pRoot);


#endif // __EXE_H
