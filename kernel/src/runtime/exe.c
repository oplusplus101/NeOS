
#include <runtime/exe.h>
#include <common/exestructs.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/panic.h>
#include <common/math.h>
#include <memory/heap.h>
#include <loaderfunctions.h>

sObjectType *g_pLibraryObjectType;

void DestoryLibrary(sObject *pObject)
{
    sLibraryObject *pLibrary = pObject->pBody;
    FreeExecutable(&pLibrary->sEXE);
    KHeapFree(pLibrary);
}

void InitExecutableLoader()
{
    g_pLibraryObjectType = CreateType(L"Library", sizeof(sLibraryObject), DestoryLibrary);
}

DWORD RVAToOffset(DWORD dwRVA, sList *pSections)
{
    for (int i = 0; i < pSections->qwLength; i++)
    {
        sExecutableSection *pSection = GetListElement(pSections, i);
        QWORD qwSectionStartRVA = pSection->qwVirtualAddress;
        QWORD qwSectionEndRVA = qwSectionStartRVA + _MAX(pSection->dwVirtualSize, pSection->dwRawSize);
        
        if (dwRVA >= qwSectionStartRVA && dwRVA < qwSectionEndRVA)
            return dwRVA - qwSectionStartRVA + pSection->dwSectionOffsetWithinFile;
    }
    return 0; // Not found
}

QWORD ResolveNameImport(sObject *pLibrary, PWCHAR wszName, WORD wHint)
{
    return 0;
}

QWORD ResolveOrdinalImport(sObject *pLibrary, WORD wOrdinal)
{
    return 0;
}


// Loads a DLL and caches it, if it isn't yet loaded,
// returns cached result otherwise
sObject *LoadKernelLibrary(PWCHAR wszName, QWORD qwBaseAddress)
{
    WCHAR wszObjectPath[256];
    strcpyW(wszObjectPath, L"Cache\\Libraries\\Kernel\\");
    strcpyW(wszObjectPath, wszName);

    // See if the library exists
    sObject *pObject;
    if ((pObject = FindObject(wszObjectPath, NULL)) != NULL)
        return pObject;

    // Create it otherwise
        
    // TODO: Add a proper PATH variable to NeOS.cfg, so that other folders can be used
    WCHAR wszFilePath[256];
    strcpyW(wszFilePath, L"NeOS\\Libraries\\");
    strcatW(wszFilePath, wszName);
    
    PVOID pFile = LoaderOpenFile(wszFilePath);
    if (pFile == NULL) return NULL;
    sLibraryObject sLibrary;
    Log(LOG_WARNING, L"Replace GetKernelPML4() with a executable-independent value in exe.c:%d", __LINE__ + 1);
    INT iStatus = LoadExecutable(pFile, qwBaseAddress, GetKernelPML4(), &sLibrary.sEXE);
    LoaderCloseFile(pFile);
    if (!_SUCCESSFUL(iStatus)) return NULL;

    return CreateObject(wszObjectPath, g_pLibraryObjectType, &sLibrary);
}

INT LoadRelocations(PVOID pFile, sPE32DataDirectoryEntry sRelocationTable, QWORD qwBaseAddress, sExecutable *pEXE)
{
    Log(LOG_ERROR, L"LoadRelocations not yet implemented");
    return NEOS_FAILURE;
}

INT LoadExports(PVOID pFile, sPE32DataDirectoryEntry sExportTable, sExecutable *pEXE)
{
    sPE32ExportTableHeader sExportTableHeader;
    LoaderSeekFile(pFile, RVAToOffset(sExportTable.dwVirtualAddress, &pEXE->lstSections));
    if (LoaderReadFile(pFile, &sExportTableHeader, sizeof(sPE32ExportTableHeader)) != sizeof(sPE32ExportTableHeader))
    {
        Log(LOG_ERROR, L"Invalid read size for sPE32ExportHeader");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_LIBRARY;
    }
    
    // 4 bytes an entry
    PDWORD pNames = KHeapAlloc(sExportTableHeader.dwNumberOfNames * sizeof(DWORD));
    LoaderSeekFile(pFile, RVAToOffset(sExportTableHeader.dwAddressOfNames, &pEXE->lstSections));
    _ASSERT(LoaderReadFile(pFile, pNames, sExportTableHeader.dwNumberOfNames * sizeof(DWORD)) == sExportTableHeader.dwNumberOfNames * sizeof(DWORD), L"Invalid read size for Names");
    // 4 bytes an entry
    // PDWORD pFunctions = (PVOID) ((PBYTE) pEXEData + RVAToOffset(sExportTableHeader.dwAddressOfFunctions, &pEXE->lstSections));
    // 2 bytes an entry
    PWORD pOrdinals = KHeapAlloc(sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));
    LoaderSeekFile(pFile, RVAToOffset(sExportTableHeader.dwAddressOfNameOrdinals, &pEXE->lstSections));
    _ASSERT(LoaderReadFile(pFile, pOrdinals, sExportTableHeader.dwNumberOfFunctions * sizeof(WORD)) == sExportTableHeader.dwNumberOfFunctions * sizeof(WORD), L"Invalid read size for Name Ordinals");

    for (DWORD i = 0; i < sExportTableHeader.dwNumberOfFunctions; i++)
    {
        sExecutableFunction sFunc;
        // sFunc.pFunctionAddress = pFunctions[i];
        sFunc.wOrdinal = pOrdinals[i];
        LoaderSeekFile(pFile, RVAToOffset(pNames[i], &pEXE->lstSections));
        LoaderReadFile(pFile, sFunc.szName, 256); // Not the best way to do it, for it ignores the actual length of function names.
        AddListElement(&pEXE->lstExportedFunctions, &sFunc);
    }

    KHeapFree(pFile);
    return NEOS_SUCCESS;
}

INT LoadImports(PVOID pFile, sPE32DataDirectoryEntry sImportTable, sExecutable *pEXE)
{
    for (QWORD qwImportDirectoryOffset = RVAToOffset(sImportTable.dwVirtualAddress, &pEXE->lstSections); ;
         qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor))
    {
        sPE32ImageImportDescriptor sDescriptor;
        LoaderSeekFile(pFile, qwImportDirectoryOffset);
        if (LoaderReadFile(pFile, &sDescriptor, sizeof(sPE32ImageImportDescriptor)) != sizeof(sPE32ImageImportDescriptor))
        {
            Log(LOG_ERROR, L"Invalid read size for sPE32ImageImportDescriptor");
            return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
        }

        // Check if there is a name
        if (sDescriptor.dwAddressOfName == 0)
        {
            // Log(LOG_WARNING, L"Import descriptor has no name");
            qwImportDirectoryOffset += sizeof(sPE32ImageImportDescriptor);
            break;
        }

        // Read the name
        QWORD qwNameOffset = RVAToOffset(sDescriptor.dwAddressOfName, &pEXE->lstSections);
        if (qwNameOffset == 0)
        {
            Log(LOG_ERROR, L"Could not get name offset");
            return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
        }

        LoaderSeekFile(pFile, qwNameOffset);
        WCHAR wszName[512] = { 0 };
        for (DWORD i = 0; i < sizeof(wszName) / sizeof(WCHAR) - 1; i++)
        {
            if (LoaderReadFile(pFile, &wszName[i], sizeof(CHAR)) != sizeof(CHAR))
            {
                Log(LOG_ERROR, L"Failed to read import name %S", wszName);
                return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
            }

            if (wszName[i] == 0)
                break;
        }

        // TODO: Handle the base address properly
        sObject *pLibrary = LoadKernelLibrary(wszName, 0);
        if (pLibrary == NULL)
        {
            Log(LOG_ERROR, L"Could not load library '%S'", wszName);
            return NEOS_LOAD_FAILURE | NEOS_ERROR_LIBRARY;
        }

        QWORD qwImportLookupTableOffset = RVAToOffset(sDescriptor.dwAddressOfOriginalFirstThunk != 0 ? sDescriptor.dwAddressOfOriginalFirstThunk : sDescriptor.dwAddressOfFirstThunk, &pEXE->lstSections);
        QWORD qwImportAddressTableOffset = RVAToOffset(sDescriptor.dwAddressOfFirstThunk, &pEXE->lstSections);
        
        uPE32ImageThunkData uImportLookupTableThunk, uImportAddressTableThunk;
        for (QWORD qwThunkIndex = 0; ; qwThunkIndex++)
        {
            LoaderSeekFile(pFile, qwImportLookupTableOffset + qwThunkIndex * sizeof(uPE32ImageThunkData));
            _ASSERT(LoaderReadFile(pFile, &uImportLookupTableThunk, sizeof(uPE32ImageThunkData)) == sizeof(uPE32ImageThunkData), L"Failed to read the import lookup table thunk");
            LoaderSeekFile(pFile, qwImportAddressTableOffset + qwThunkIndex * sizeof(uPE32ImageThunkData));
            _ASSERT(LoaderReadFile(pFile, &uImportAddressTableThunk, sizeof(uPE32ImageThunkData)) == sizeof(uPE32ImageThunkData), L"Failed to read the import address table thunk");

            if (uImportLookupTableThunk.qwAddressOfData == 0)
                break;
            
            Log(LOG_WARNING, L"Importing not fully implemented yet!");

            if (uImportLookupTableThunk.qwOrdinal & 0x8000000000000000ULL) // Ordinal
            {
                // WORD wOrdinal = sImportLookupTableThunk.qwOrdinal & 0xFFFF;
                Log(LOG_ERROR, L"Ordinal imports not implemented");
                return NEOS_FAILURE;
            }
            else // Name
            {
                QWORD qwOffset = RVAToOffset(uImportLookupTableThunk.qwAddressOfData, &pEXE->lstSections);
                if (qwOffset == 0)
                {
                    Log(LOG_ERROR, L"Couldn't read import name entry");
                    return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
                }
                LoaderSeekFile(pFile, qwOffset);

                WORD wHint;
                _ASSERT(LoaderReadFile(pFile, &wHint, sizeof(WORD)) == sizeof(WORD), L"Failed to read hint while parsing import table");
                
                CHAR szName[512];
                for (DWORD i = 0; i < sizeof(szName) - 1; i++)
                {
                    _ASSERT(LoaderReadFile(pFile, &szName[i], sizeof(CHAR)) == sizeof(CHAR), L"Failed to read import name");
                    if (szName[i] == 0)
                        break;
                }
                
                // pResolvedAddress = 
            }
            
            // PVOID pAddressInExecutable = (PVOID) (sDescriptor.dwAddressOfFirstThunk + pEXE->qwVirtualStart + qwThunkIndex * sizeof(uPE32ImageThunkData));
            // *((QWORD *) GetPhysicalAddress(pEXE->pPageTable, (PBYTE) pAddressInExecutable)) = (QWORD) ABTest;
        }
    }

    return NEOS_SUCCESS;
}

INT LoadExecutable(PVOID pFile, QWORD qwVirtualAddress, sPageTable *pPML4, sExecutable *pEXE)
{
    // Load the MS-DOS header
    sMZHeader sMZHeader;
    if (LoaderReadFile(pFile, &sMZHeader, sizeof(sMZHeader)) != sizeof(sMZHeader))
    {
        Log(LOG_ERROR, L"Invalid read size for sMZHeader");
        return NEOS_LOAD_FAILURE | NEOS_ERROR_EXECUTABLE;
    }
    _ASSERT(sMZHeader.wMagic == 0x5A4D, L"Invalid MZ header magic number: %04X", sMZHeader.wMagic);
    
    // Read the PE header offset
    DWORD dwPEHeaderOffset;
    LoaderSeekFile(pFile, 0x3C);
    LoaderReadFile(pFile, &dwPEHeaderOffset, sizeof(DWORD));
    
    // Load the PE32+ header
    sPE32Header sPEHeader;
    LoaderSeekFile(pFile, dwPEHeaderOffset);
    _ASSERT(LoaderReadFile(pFile, &sPEHeader, sizeof(sPE32Header)) == sizeof(sPE32Header), L"Invalid read size for sPE32Header");
    _ASSERT(sPEHeader.dwMagic == 0x4550, L"Invalid PE header magic number: 0x%08X", sPEHeader.dwMagic);

    // Load the PE32+ "optional" header
    sPE32OptionalHeader sOptionalHeader;
    _ASSERT(LoaderReadFile(pFile, &sOptionalHeader, sizeof(sPE32OptionalHeader)) == sizeof(sPE32OptionalHeader), L"Invalid read size for sPE32OptionalHeader");
    _ASSERT(sOptionalHeader.wMagic == 0x020B, L"Invalid PE optional header magic number: %04X", sPEHeader.dwMagic);

    // The data directory comes directly after the "optional" header
    sPE32DataDirectory sDataDirectory;
    _ASSERT(LoaderReadFile(pFile, &sDataDirectory, sOptionalHeader.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry)) == sOptionalHeader.dwNumberOfRVAAndSizes * sizeof(sPE32DataDirectoryEntry), L"Invalid read size for data directory");

    pEXE->qwEntryPoint         = (QWORD) sOptionalHeader.dwAddressOfEntrypoint;
    pEXE->lstSections          = CreateEmptyList(sizeof(sExecutableSection));
    pEXE->lstExportedFunctions = CreateEmptyList(sizeof(sExecutableFunction));
    pEXE->pPageTable           = pPML4;
    pEXE->qwVirtualStart       = qwVirtualAddress == 0 ? sOptionalHeader.qwImageBase : qwVirtualAddress;
    pEXE->qwVirtualEnd         = 0;

    sPE32SectionHeader *pSections = KHeapAlloc(sizeof(sPE32SectionHeader) * sPEHeader.wNumberOfSections);
    LoaderReadFile(pFile, pSections, sizeof(sPE32SectionHeader) * sPEHeader.wNumberOfSections);

    for (WORD i = 0; i < sPEHeader.wNumberOfSections; i++)
    {
        sPE32SectionHeader *pSection = pSections++;
        
        sExecutableSection sSection;
        strncpy(sSection.szName, (PCHAR) pSection->szName, 8);
        sSection.dwRawSize                 = pSection->dwSizeOfRawData;
        sSection.dwVirtualSize             = pSection->dwVirtualSize;
        sSection.qwVirtualAddress          = pSection->dwVirtualAddress;
        sSection.dwSectionOffsetWithinFile = pSection->dwOffsetOfRawData;

        if (pSection->dwSizeOfRawData > 0 && !(pSection->dwCharacteristics & PE32_SCN_MEM_DISCARDABLE))
        {
            sSection.pData = AllocateContinousPages(_BYTES_TO_PAGES((QWORD) pSection->dwSizeOfRawData));
            pEXE->qwVirtualEnd = _MAX(pEXE->qwVirtualEnd, sSection.qwVirtualAddress + pEXE->qwVirtualStart + pSection->dwVirtualSize);
            MapPageRange(pEXE->pPageTable, (PVOID) (sSection.qwVirtualAddress + pEXE->qwVirtualStart), sSection.pData, _BYTES_TO_PAGES((QWORD) pSection->dwSizeOfRawData),
                                                  (pSection->dwCharacteristics & PE32_SCN_MEM_WRITE ? PF_WRITEABLE : 0) |
                                                  (pSection->dwCharacteristics & PE32_SCN_MEM_NOT_CACHED ? PF_CACHEDISABLE : 0));
            LoaderSeekFile(pFile, sSection.dwSectionOffsetWithinFile);
            LoaderReadFile(pFile, (PBYTE) sSection.pData + (sSection.qwVirtualAddress % PAGE_SIZE), sSection.dwRawSize);
        }
        
        AddListElement(&pEXE->lstSections, &sSection);
    }

    KHeapFree(pSections);

    INT iStatus = NEOS_SUCCESS;

    if (sDataDirectory.sBaseRelocationTable.dwVirtualAddress != 0)
        iStatus = LoadRelocations(pFile, sDataDirectory.sBaseRelocationTable, qwVirtualAddress, pEXE);

    if (!_SUCCESSFUL(iStatus))
        return iStatus;
    
    if (sDataDirectory.sExportTable.dwVirtualAddress != 0)
        iStatus = LoadExports(pFile, sDataDirectory.sExportTable, pEXE);

    if (!_SUCCESSFUL(iStatus))
        return iStatus;
    
    if (sDataDirectory.sImportTable.dwVirtualAddress != 0)
        iStatus = LoadImports(pFile, sDataDirectory.sImportTable, pEXE);
    
    return iStatus;
}

void FreeExecutable(sExecutable *pEXE)
{
    _KERNEL_PANIC(L"FreeExecutable not yet implemented");
    for (INT i = 0; i < pEXE->lstSections.qwLength; i++)
    {
        sExecutableSection *sSection = (sExecutableSection *) GetListElement(&pEXE->lstSections, i);
        FreeContinousPages(sSection->pData, _BYTES_TO_PAGES(sSection->dwVirtualSize));
    }
    DestroyList(&pEXE->lstSections);
}
