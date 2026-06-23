
#include "exe.h"
#include <uefi.h>
#include <neos.h>

#define STRING_NO_BYTE_CHARS
#include <common/string.h>

#define _PANIC(...) { printf(__VA_ARGS__); while (1); }
#define _ASSERT(c, ...) if (!(c)) { _PANIC(__VA_ARGS__); }

sRAMFile *g_pARCHFile;
sPE32Headers g_sARCHHeaders;
sExecutableFunction *g_arrFunctions;
QWORD g_narrFunctions;

STATUS InitialiseExecutableLoader(sRAMFile *pRoot)
{
    g_pARCHFile = GetFile(L"NeOS\\ARCH.sys", pRoot);
    STATUS st = ParsePEHeaders(g_pARCHFile, &g_sARCHHeaders);
    if (st != 0) return st;
    
    st = LoadExports(g_pARCHFile, &g_sARCHHeaders, &g_arrFunctions, &g_narrFunctions, MM_ARCH_START);
    if (st != 0) return st;

    return NEOS_SUCCESS;
}

DWORD RVAToOffset(DWORD dwRVA, sPE32Headers *pHeaders)
{
    for (DWORD i = 0; i < pHeaders->nsSections; i++)
    {
        QWORD qwSectionStartRVA = pHeaders->arrSections[i].dwVirtualAddress;
        QWORD qwSectionEndRVA = qwSectionStartRVA + max(pHeaders->arrSections[i].dwVirtualSize, pHeaders->arrSections[i].dwSizeOfRawData);
        
        if (dwRVA >= qwSectionStartRVA && dwRVA < qwSectionEndRVA)
            return dwRVA - qwSectionStartRVA + pHeaders->arrSections[i].dwOffsetOfRawData;
    }
    return 0; // Not found
}

STATUS LoadRelocations(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwPhysicalStart, QWORD qwVirtualStart)
{
    QWORD qwDelta = qwVirtualStart - pHeaders->sPEOptional.qwImageBase;
    
    for (QWORD qwBlockOffset = 0; qwBlockOffset < pHeaders->sDataDirectory.sBaseRelocationTable.dwSize; )
    {
        sPE32BaseRelocationEntry sHeader;
        DWORD dwOffset = RVAToOffset(pHeaders->sDataDirectory.sBaseRelocationTable.dwVirtualAddress, pHeaders);
        memcpy(&sHeader, (PBYTE) pFile->pData + dwOffset, sizeof(sPE32BaseRelocationEntry));
        QWORD qwnEntries = (sHeader.dwBlockSize - sizeof(sPE32BaseRelocationEntry)) / sizeof(WORD);

        PWORD pTypeOffsets = malloc(sHeader.dwBlockSize - sizeof(sPE32BaseRelocationEntry));
        memcpy(pTypeOffsets, (PBYTE) pFile->pData + dwOffset + sizeof(sPE32BaseRelocationEntry), sHeader.dwBlockSize - sizeof(sPE32BaseRelocationEntry));

        for (QWORD i = 0; i < qwnEntries; i++)
        {
            PQWORD pPatch = (PQWORD) (qwPhysicalStart + (pTypeOffsets[i] & 0x0FFF) + sHeader.dwVirtualAddress);

            switch (pTypeOffsets[i] >> 12)
            {
            case 0: // Ignore (doesn't relocate anything)
                break;
            case 10:
            {
                // Alignment bullcrap
                QWORD qwValue;
                memcpy(&qwValue, pPatch, sizeof(QWORD));
                qwValue += qwDelta;
                memcpy(pPatch, &qwValue, sizeof(QWORD));
                break;
            }
            default:
                printf("[  ERROR  ] Relocation %u not implemented\n", pTypeOffsets[i] >> 12);
                free(pTypeOffsets);
                return 0x000010000 | NEOS_NOT_IMPLEMENTED;
            }
        }

        free(pTypeOffsets);

        qwBlockOffset += sHeader.dwBlockSize;
    }

    return NEOS_SUCCESS;
}


STATUS LoadImports(sRAMFile *pFile, sPE32Headers *pHeaders, QWORD qwBaseAddress)
{
    QWORD qwImportDirectoryOffset = RVAToOffset(pHeaders->sDataDirectory.sImportTable.dwVirtualAddress, pHeaders);
    if (qwImportDirectoryOffset == 0)
    {
        printf("[  ERROR  ] Import directory offset is 0\n");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
    }

    for (;; qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor))
    {
        sPE32ImageImportDescriptor sDescriptor;
        memcpy(&sDescriptor, (PBYTE) pFile->pData + qwImportDirectoryOffset, sizeof(sPE32ImageImportDescriptor));

        // Check if there is a name
        if (sDescriptor.dwAddressOfName == 0)
        {
            // ArchLog(LOG_WARNING, "Import descriptor has no name");
            qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor);
            break;
        }

        // Read the name
        QWORD qwNameOffset = RVAToOffset(sDescriptor.dwAddressOfName, pHeaders);
        if (qwNameOffset == 0)
        {
            printf("[ WARNING ] Could not get name offset\n");
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

        _ASSERT(!strcmpW(wszName, L"ARCH.sys"), "Tried to import something else than ARCH.sys"); // TODO: Make this more modular
        
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
                printf("[  ERROR  ] Ordinal imports not implemented\n");
                return NEOS_NOT_IMPLEMENTED;
            }
            else // Name
            {
                QWORD qwOffset = RVAToOffset(uImportLookupTableThunk.qwAddressOfData, pHeaders);
                if (qwOffset == 0)
                {
                    printf("[  ERROR  ] Couldn't read import name entry\n");
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
                
                for (QWORD i = 0; i < g_narrFunctions; i++)
                {
                    if (strcmp(g_arrFunctions[i].szName, szName)) continue;
                    pResolvedAddress = g_arrFunctions[i].pFunctionAddress;
                    break;
                }

                if (pResolvedAddress == NULL)
                {
                    printf("[  ERROR  ] Could not get function export '%s'\n", szName);
                    return NEOS_LOAD_FAILURE | (qwThunkIndex << 16);
                }
            }
            
            PVOID pAddressInExecutable = (PVOID) (sDescriptor.dwAddressOfFirstThunk + qwBaseAddress + qwThunkIndex * sizeof(uPE32ImageThunkData));
            *((PQWORD) pAddressInExecutable) = (QWORD) pResolvedAddress;
        }
    }

    return NEOS_SUCCESS;
}

STATUS LoadExports(sRAMFile *pFile, sPE32Headers *pHeaders, sExecutableFunction **ppExecutableFunctions, PQWORD pnsExportFuncions, QWORD qwBaseAddress)
{
    if (pHeaders->sDataDirectory.sExportTable.dwVirtualAddress == 0)
    {
        printf("[  ERROR  ] Export Table doesn't exist\n");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
    }

    sPE32ExportTableHeader sExportTableHeader;
    memcpy(&sExportTableHeader, (PBYTE) pFile->pData + RVAToOffset(pHeaders->sDataDirectory.sExportTable.dwVirtualAddress, pHeaders), sizeof(sPE32ExportTableHeader));
    
    // 4 bytes an entry
    PDWORD pNames = malloc(sExportTableHeader.dwNumberOfNames * sizeof(DWORD));
    memcpy(pNames, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfNames, pHeaders), sExportTableHeader.dwNumberOfNames * sizeof(DWORD));

    // 4 bytes an entry
    PDWORD pFunctions = malloc(sExportTableHeader.dwNumberOfFunctions * sizeof(DWORD));
    memcpy(pFunctions, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfFunctions, pHeaders), sExportTableHeader.dwNumberOfFunctions * sizeof(DWORD));

    // 2 bytes an entry
    PWORD pOrdinals = malloc(sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));
    memcpy(pOrdinals, (PBYTE) pFile->pData + RVAToOffset(sExportTableHeader.dwAddressOfNameOrdinals, pHeaders), sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));

    *ppExecutableFunctions = malloc(sizeof(sExecutableFunction) * sExportTableHeader.dwNumberOfFunctions);
    *pnsExportFuncions = sExportTableHeader.dwNumberOfFunctions;
    for (DWORD i = 0; i < sExportTableHeader.dwNumberOfFunctions; i++)
    {
        sExecutableFunction *sFunc =  *ppExecutableFunctions + i;
        sFunc->wOrdinal = pOrdinals[i];
        sFunc->pFunctionAddress = (PVOID) (qwBaseAddress + pFunctions[i]);
        memcpy(sFunc, (PBYTE) pFile->pData + RVAToOffset(pNames[i], pHeaders), 256);
    }

    free(pNames);
    free(pFunctions);
    free(pOrdinals);
    
    return NEOS_SUCCESS;
}

// TODO: Replace asserts with error codes
STATUS ParsePEHeaders(sRAMFile *pFile, sPE32Headers *pHeaders)
{
    // Load the MS-DOS header
    memcpy(&pHeaders->sMZ, pFile->pData, sizeof(sMZHeader));

    // _ASSERT(pHeaders->sMZ.wMagic == 0x5A4D, "Invalid MZ header magic number: %04X", pHeaders->sMZ.wMagic);
    if (pHeaders->sMZ.wMagic != 0x5A4D)
        return (pHeaders->sMZ.wMagic << 16) | NEOS_UNEXPECTED_VALUE;
    
    // Read the PE header offset
    DWORD dwPEHeaderOffset = *((PDWORD) ((PBYTE) pFile->pData + 0x3C));
    
    // Load the PE32+ header
    memcpy(&pHeaders->sPE, (PBYTE) pFile->pData + dwPEHeaderOffset, sizeof(sPE32Header));
    _ASSERT(pHeaders->sPE.dwMagic == 0x4550, "Invalid PE header magic number: 0x%08X", pHeaders->sPE.dwMagic);

    // Load the PE32+ "optional" header
    memcpy(&pHeaders->sPEOptional, (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header), sizeof(sPE32OptionalHeader));
    _ASSERT(pHeaders->sPEOptional.wMagic == 0x020B, "Invalid PE optional header magic number: %04X", pHeaders->sPEOptional.wMagic);

    // The data directory comes directly after the "optional" header
    memcpy(&pHeaders->sDataDirectory,
           (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header) + sizeof(sPE32OptionalHeader),
           pHeaders->sPEOptional.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry));

    pHeaders->nsSections = pHeaders->sPE.wNumberOfSections;
    pHeaders->arrSections = malloc(sizeof(sPE32SectionHeader) * pHeaders->sPE.wNumberOfSections);
    memcpy(pHeaders->arrSections,
           (PBYTE) pFile->pData + dwPEHeaderOffset + sizeof(sPE32Header) + sizeof(sPE32OptionalHeader) + pHeaders->sPEOptional.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry),
           sizeof(sPE32SectionHeader) * pHeaders->sPE.wNumberOfSections);

    return NEOS_SUCCESS;
}


QWORD CalculatePESize(sPE32Headers *pHeaders)
{
    QWORD qwMax = 0;
    for (QWORD i = 0; i < pHeaders->nsSections; i++)
        qwMax = max(qwMax, pHeaders->arrSections[i].dwVirtualAddress + max(pHeaders->arrSections[i].dwSizeOfRawData, pHeaders->arrSections[i].dwVirtualSize));
    
    return (qwMax + 0x0FFF) & ~0x0FFF;
}


STATUS LoadExecutableTo(sRAMFile *pFile, QWORD qwVirtualStart, QWORD qwDesiredPhysicalAddress, sPE32Headers *pHeaders)
{
    STATUS st = ParsePEHeaders(pFile, pHeaders);
    _ASSERT(st == 0, "Failed to parse headers [%08X]", st);

    QWORD qwExecutableSize = CalculatePESize(pHeaders);
    PVOID pDataPhysical = (PVOID) qwDesiredPhysicalAddress;
    efi_status_t s = BS->AllocatePages(AllocateAddress, EfiLoaderData, qwExecutableSize / 4096, (efi_physical_address_t *) &pDataPhysical);
    _ASSERT(s == 0 && (QWORD) pDataPhysical == qwDesiredPhysicalAddress, "Couldn't allocate pages for executable. Executable size %u", qwExecutableSize);
    printf("Sections loaded!!! physaddr %p\n", pDataPhysical);
    
    for (QWORD i = 0; i < pHeaders->nsSections; i++)
    {
        sPE32SectionHeader *pHeader = pHeaders->arrSections + i;
        QWORD qwAddress = (QWORD) pHeader->dwVirtualAddress + (QWORD) pDataPhysical;
        _ASSERT(qwAddress >= (QWORD) pDataPhysical && qwAddress + pHeader->dwVirtualSize <= (QWORD) pDataPhysical + qwExecutableSize, "Section %s is out of bounds!", pHeader->szName);

        printf("Loading section %08s at 0x%p to physical 0x%p\n", pHeader->szName, qwVirtualStart + pHeader->dwVirtualAddress, qwAddress);

        if (pHeader->dwCharacteristics & PE32_SCN_CNT_UNINITIALISED_DATA)
            memset((PVOID) qwAddress, 0, pHeader->dwVirtualSize);
        else
        {
            // TODO: Possibly add a buffer if the implementation of this function changes, but it is fine now as the current drive implementation already has a buffer
            memcpy((PVOID) qwAddress, (PBYTE) pFile->pData + pHeader->dwOffsetOfRawData, pHeader->dwSizeOfRawData);

            if (pHeader->dwVirtualSize > pHeader->dwSizeOfRawData)
                memset((PVOID) (qwAddress + pHeader->dwSizeOfRawData), 0, pHeader->dwVirtualSize - pHeader->dwSizeOfRawData);
        }
    }

    if (pHeaders->sDataDirectory.sBaseRelocationTable.dwVirtualAddress != 0)
    {
        st = LoadRelocations(pFile, pHeaders, (QWORD) pDataPhysical, qwVirtualStart);
        _ASSERT(st == 0, "Failed to load relocations [%08X]", st);
    }

    if (pHeaders->sDataDirectory.sImportTable.dwVirtualAddress != 0)
    {
        st = LoadImports(pFile, pHeaders, (QWORD) pDataPhysical);
        _ASSERT(st == 0, "Failed to load imports [%08X]", st);
    }

    return NEOS_SUCCESS;
}
