
#include <runtime/exe.h>
#include <common/exestructs.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/panic.h>
#include <common/math.h>
#include <memory/heap.h>
#include <archimports.h>
#include <file.h>

sObjectType *g_pLibraryObjectType;
sExecutableFunction *g_arrExportedKernelFunctions;
QWORD g_nsExportedKernelFunctions;

STATUS LoadExports(sRAMFile *pFile, sPE32Headers *pHeaders, sExecutableFunction **ppExecutableFunctions, PQWORD pnsExportFuncions, QWORD qwBaseAddress);

void DestoryLibrary(PVOID pObject)
{
    sLibraryObject *pLibrary = pObject;
    // FreeExecutable(&pLibrary->sEXE);
    KHeapFree(pLibrary);
}

void InitExecutableLoader()
{
    sPE32Headers sHeaders;
    sRAMFile *pKernel = GetRAMFile(L"NeOS\\NeOS.sys");
    STATUS status = ParsePEHeaders(pKernel, &sHeaders);
    _ASSERT(_SUCCESSFUL(status), L"Could not parse the kernel's headers");
    
    status = LoadExports(pKernel, &sHeaders, &g_arrExportedKernelFunctions, &g_nsExportedKernelFunctions, MM_KERNEL_START);
    _ASSERT(_SUCCESSFUL(status), L"Could not parse the kernel's export table");

    // LoaderCloseFile(pFile);

    g_pLibraryObjectType = CreateType(MakeString(L"Library"), sizeof(sLibraryObject), DestoryLibrary, NULL);
}

DWORD RVAToOffset(DWORD dwRVA, sPE32Headers *pHeaders)
{
    for (int i = 0; i < pHeaders->nsSections; i++)
    {
        QWORD qwSectionStartRVA = pHeaders->arrSections[i].dwVirtualAddress;
        QWORD qwSectionEndRVA = qwSectionStartRVA + _MAX(pHeaders->arrSections[i].dwVirtualSize, pHeaders->arrSections[i].dwSizeOfRawData);
        
        if (dwRVA >= qwSectionStartRVA && dwRVA < qwSectionEndRVA)
            return dwRVA - qwSectionStartRVA + pHeaders->arrSections[i].dwOffsetOfRawData;
    }
    return 0; // Not found
}

STATUS LoadRelocations(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwVirtualStart)
{
    for (QWORD i = 0; i < pHeaders->sDataDirectory.sBaseRelocationTable.dwSize; )
    {
        sPE32BaseRelocationEntry sEntry;
        DWORD dwOffset = RVAToOffset(pHeaders->sDataDirectory.sBaseRelocationTable.dwVirtualAddress, pHeaders);
        memcpy(&sEntry, (PBYTE) pFile->pData + dwOffset + i, sizeof(sPE32BaseRelocationEntry));

        DWORD dwTypeOffsetSize = sEntry.dwBlockSize - sizeof(sPE32BaseRelocationEntry);
        PWORD pTypeOffsets = KHeapAlloc(dwTypeOffsetSize);
        ArchPrintFormat(L"Relocation : %u %p\n", dwTypeOffsetSize, sEntry.dwVirtualAddress);
        memcpy(pTypeOffsets, (PBYTE) pFile->pData + RVAToOffset(sEntry.dwVirtualAddress, pHeaders), dwTypeOffsetSize);

        for (DWORD j = 0; j < dwTypeOffsetSize / sizeof(WORD); j++)
        {
            switch (pTypeOffsets[j] >> 12)
            {
            case 0: // Ignore (doesn't relocate anything)
                break;
            
            default:
                ArchLog(LOG_ERROR, L"Relocation %u not implemented", pTypeOffsets[i] >> 12);
                return 0x000010000 | NEOS_NOT_IMPLEMENTED;
                break;
            }
        }

        i += dwTypeOffsetSize + sizeof(sPE32BaseRelocationEntry);
    }

    return NEOS_SUCCESS;
}

STATUS LoadExports(sRAMFile *pFile, sPE32Headers *pHeaders, sExecutableFunction **ppExecutableFunctions, PQWORD pnsExportFuncions, QWORD qwBaseAddress)
{
    if (pHeaders->sDataDirectory.sExportTable.dwVirtualAddress == 0)
    {
        ArchLog(LOG_ERROR, L"Export Table doesn't exist");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
    }

    sPE32ExportTableHeader sExportTableHeader;
    memcpy(&sExportTableHeader, (PBYTE) pFile->pData + RVAToOffset(pHeaders->sDataDirectory.sExportTable.dwVirtualAddress, pHeaders), sizeof(sPE32ExportTableHeader));
    
    // 4 bytes an entry
    PDWORD pNames = KHeapAlloc(sExportTableHeader.dwNumberOfNames * sizeof(DWORD));
    memcpy(pNames, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfNames, pHeaders), sExportTableHeader.dwNumberOfNames * sizeof(DWORD));

    // 4 bytes an entry
    PDWORD pFunctions = KHeapAlloc(sExportTableHeader.dwNumberOfFunctions * sizeof(DWORD));
    memcpy(pFunctions, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfFunctions, pHeaders), sExportTableHeader.dwNumberOfFunctions * sizeof(DWORD));

    // 2 bytes an entry
    PWORD pOrdinals = KHeapAlloc(sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));
    memcpy(pOrdinals, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfNameOrdinals, pHeaders), sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));

    *ppExecutableFunctions = KHeapAlloc(sizeof(sExecutableFunction) * sExportTableHeader.dwNumberOfFunctions);
    *pnsExportFuncions = sExportTableHeader.dwNumberOfFunctions;
    for (DWORD i = 0; i < sExportTableHeader.dwNumberOfFunctions; i++)
    {
        sExecutableFunction *sFunc =  *ppExecutableFunctions + i;
        sFunc->wOrdinal = pOrdinals[i];
        sFunc->pFunctionAddress = (PVOID) (qwBaseAddress + pFunctions[i]);
        memcpy(sFunc, (PBYTE) pFile->pData + RVAToOffset(pNames[i], pHeaders), 256);
    }

    return NEOS_SUCCESS;
}

PVOID GetKernelExportFunction(PCHAR szName)
{
    for (QWORD i = 0; i < g_nsExportedKernelFunctions; i++)
        if (!strncmp(g_arrExportedKernelFunctions[i].szName, szName, 256))
            return g_arrExportedKernelFunctions[i].pFunctionAddress;
    
    return NULL;
}

STATUS LoadImports(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwBaseAddress)
{
    QWORD qwImportDirectoryOffset = RVAToOffset(pHeaders->sDataDirectory.sImportTable.dwVirtualAddress, pHeaders);
    if (qwImportDirectoryOffset == 0)
    {
        ArchLog(LOG_ERROR, L"Import directory offset is 0");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
    }

    for (;; qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor))
    {
        sPE32ImageImportDescriptor sDescriptor;
        memcpy(&sDescriptor, (PBYTE) pFile->pData + qwImportDirectoryOffset, sizeof(sPE32ImageImportDescriptor));

        // Check if there is a name
        if (sDescriptor.dwAddressOfName == 0)
        {
            // ArchLog(LOG_WARNING, L"Import descriptor has no name");
            qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor);
            break;
        }

        // Read the name
        QWORD qwNameOffset = RVAToOffset(sDescriptor.dwAddressOfName, pHeaders);
        if (qwNameOffset == 0)
        {
            ArchLog(LOG_ERROR, L"Could not get name offset");
            continue;
            // return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
        }

        WCHAR wszName[512] = { 0 };
        for (DWORD i = 0; i < sizeof(wszName) / sizeof(WCHAR) - 1; i++)
        {
            wszName[i] = ((PBYTE) pFile->pData + qwNameOffset)[i];

            if (wszName[i] == 0)
                break;
        }

        QWORD qwImportLookupTableOffset = RVAToOffset(sDescriptor.dwAddressOfOriginalFirstThunk != 0 ? sDescriptor.dwAddressOfOriginalFirstThunk : sDescriptor.dwAddressOfFirstThunk, pHeaders);
        QWORD qwImportAddressTableOffset = RVAToOffset(sDescriptor.dwAddressOfFirstThunk, pHeaders);
        
        uPE32ImageThunkData uImportLookupTableThunk, uImportAddressTableThunk;
        for (QWORD qwThunkIndex = 0; ; qwThunkIndex++)
        {
            memcpy(&uImportLookupTableThunk, (PBYTE) pFile->pData + qwImportLookupTableOffset + qwThunkIndex * sizeof(uPE32ImageThunkData), sizeof(uPE32ImageThunkData));
            memcpy(&uImportAddressTableThunk, (PBYTE) pFile->pData + qwImportAddressTableOffset + qwThunkIndex * sizeof(uPE32ImageThunkData), sizeof(uPE32ImageThunkData));

            if (uImportLookupTableThunk.qwAddressOfData == 0)
                break;

            PVOID pResolvedAddress = NULL;

            if (uImportLookupTableThunk.qwOrdinal & 0x8000000000000000ULL) // Ordinal
            {
                // WORD wOrdinal = sImportLookupTableThunk.qwOrdinal & 0xFFFF;
                ArchLog(LOG_ERROR, L"Ordinal imports not implemented");
                return NEOS_NOT_IMPLEMENTED;
            }
            else // Name
            {
                QWORD qwOffset = RVAToOffset(uImportLookupTableThunk.qwAddressOfData, pHeaders);
                if (qwOffset == 0)
                {
                    ArchLog(LOG_ERROR, L"Couldn't read import name entry");
                    return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
                }

                // WORD wHint = *((PWORD) ((PBYTE) pFile->pData + qwOffset));
                CHAR szName[512];
                for (DWORD i = 0; i < sizeof(szName) - 1; i++)
                {
                    szName[i] = ((PBYTE) pFile->pData + qwOffset + sizeof(WORD))[i];
                    if (szName[i] == 0)
                        break;
                }
                
                pResolvedAddress = GetKernelExportFunction(szName);
                if (pResolvedAddress == NULL)
                {
                    ArchLog(LOG_ERROR, L"Could not get function export '%s'", szName);
                    return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
                }
            }
            
            PVOID pAddressInExecutable = (PVOID) (sDescriptor.dwAddressOfFirstThunk + qwBaseAddress + qwThunkIndex * sizeof(uPE32ImageThunkData));
            *((PQWORD) pAddressInExecutable) = (QWORD) pResolvedAddress;
        }
    }

    return NEOS_SUCCESS;
}

// TODO: Replace asserts with error codes
STATUS ParsePEHeaders(sRAMFile *pFile, sPE32Headers *pHeaders)
{
    // Load the MS-DOS header
    memcpy(&pHeaders->sMZ, pFile->pData, sizeof(sMZHeader));

    // _ASSERT(pHeaders->sMZ.wMagic == 0x5A4D, L"Invalid MZ header magic number: %04X", pHeaders->sMZ.wMagic);
    if (pHeaders->sMZ.wMagic != 0x5A4D)
        return (pHeaders->sMZ.wMagic << 16) | NEOS_LOAD_FAILURE;
    
    // Read the PE header offset
    DWORD dwPEHeaderOffset = *((PDWORD) ((PBYTE) pFile->pData + 0x3C));
    
    // Load the PE32+ header
    memcpy(&pHeaders->sPE, (PBYTE) pFile->pData + dwPEHeaderOffset, sizeof(sPE32Header));
    _ASSERT(pHeaders->sPE.dwMagic == 0x4550, L"Invalid PE header magic number: 0x%08X", pHeaders->sPE.dwMagic);

    // Load the PE32+ "optional" header
    memcpy(&pHeaders->sPEOptional, (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header), sizeof(sPE32OptionalHeader));
    _ASSERT(pHeaders->sPEOptional.wMagic == 0x020B, L"Invalid PE optional header magic number: %04X", pHeaders->sPEOptional.wMagic);

    // The data directory comes directly after the "optional" header
    memcpy(&pHeaders->sDataDirectory,
           (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header) + sizeof(sPE32OptionalHeader),
           pHeaders->sPEOptional.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry));

    pHeaders->nsSections = pHeaders->sPE.wNumberOfSections;
    pHeaders->arrSections = KHeapAlloc(sizeof(sPE32SectionHeader) * pHeaders->sPE.wNumberOfSections);
    memcpy(pHeaders->arrSections,
           (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header) + sizeof(sPE32OptionalHeader) + pHeaders->sPEOptional.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry),
           sizeof(sPE32SectionHeader) * pHeaders->sPE.wNumberOfSections);

    return NEOS_SUCCESS;
}

STATUS LoadDriverIntoMemory(sRAMFile *pFile, sPE32Headers *pHeaders, PVOID pVirtualStart, PVOID *pEntryPoint)
{
    ArchLog(LOG_LOG, L"Loading driver to base address 0x%p", pVirtualStart);
    // Loads all sections that aren't marked with PE32_SCN_MEM_DISCARDABLE into memory at pVirtualStart
    for (QWORD i = 0; i < pHeaders->nsSections; i++)
    {
        sPE32SectionHeader *pSection = pHeaders->arrSections + i;
        if (pSection->dwCharacteristics & PE32_SCN_MEM_DISCARDABLE)
            continue;

        ArchLog(LOG_LOG, L"Loading driver section '%s' to RVA 0x%08X to PHYS 0x%p", pSection->szName, pSection->dwVirtualAddress, (PVOID) ((QWORD) pVirtualStart + pSection->dwVirtualAddress));
        // Check if the section is aligned. & (PAGE_SIZE - 1) != 0 is the same as a modulo but faster.
        if (pSection->dwVirtualAddress & (PAGE_SIZE - 1))
        {
            ArchLog(LOG_WARNING, L"Section '%s' contains an invalid RVA 0x%08X", pSection->szName, pSection->dwVirtualAddress);
            return NEOS_INVALID_ARGUMENT;
        }
        
        memcpy((PVOID) ((QWORD) pVirtualStart + pSection->dwVirtualAddress), (PBYTE) pFile->pData + pSection->dwOffsetOfRawData, pSection->dwSizeOfRawData);
    }

    STATUS status = LoadRelocations(pFile, pHeaders, (QWORD) pVirtualStart);
    if (!_SUCCESSFUL(status))
        return status;

    status = LoadImports(pFile, pHeaders, (QWORD) pVirtualStart);
    if (!_SUCCESSFUL(status))
        return status;

    *pEntryPoint = (PVOID) ((QWORD) pVirtualStart + (QWORD) pHeaders->sPEOptional.dwAddressOfEntrypoint);
    
    return status;
}
