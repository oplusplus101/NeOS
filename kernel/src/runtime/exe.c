
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
        DWORD dwSectionStartRVA = pSection->qwVirtualAddress;
        DWORD dwSectionEndRVA = dwSectionStartRVA + _MAX(pSection->dwVirtualSize, pSection->dwRawSize);
        
        if (dwRVA >= dwSectionStartRVA && dwRVA < dwSectionEndRVA)
            return dwRVA - dwSectionStartRVA + pSection->dwSectionOffsetWithinFile;
    }
    return 0; // Not found
}


// Loads a DLL and caches it
sObject *LoadKernelLibrary(PWCHAR wszName, QWORD qwBaseAddress)
{
    WCHAR wszPath[256];
    strcpyW(wszPath, L"NeOS\\Libraries\\");
    strcatW(wszPath, wszName);

    PVOID pFile = LoaderOpenFile(wszPath);
    sLibraryObject sLibrary;
    sLibrary.sEXE = LoadExecutable(pFile, qwBaseAddress);
    LoaderCloseFile(pFile);

    return CreateObject(&wszPath[5], g_pLibraryObjectType, &sLibrary);
}


sExecutable LoadExecutable(PVOID pFile, QWORD qwBaseAddress)
{
    // Load the MS-DOS header
    sMZHeader sMZHeader;
    _ASSERT(LoaderReadFile(pFile, &sMZHeader, sizeof(sMZHeader)) == sizeof(sMZHeader), L"Invalid read size for sMZHeader");
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

    sExecutable sEXE =
    {
        .qwEntryPoint         = (QWORD) sOptionalHeader.dwAddressOfEntrypoint,
        .lstSections          = CreateEmptyList(sizeof(sExecutableSection)),
        .lstExportedFunctions = CreateEmptyList(sizeof(sExecutableFunction)),
        .pPageTable           = ClonePML4(GetCurrentPML4()), // Probably a bad way to do this; especially once user mode programs are implemented
        .qwBaseAddress        = qwBaseAddress == 0 ? sOptionalHeader.qwImageBase : qwBaseAddress
    };
    

    sPE32SectionHeader *pSections = KHeapAlloc(sizeof(sPE32SectionHeader) * sPEHeader.wNumberOfSections);
    LoaderReadFile(pFile, pSections, sizeof(sPE32SectionHeader) * sPEHeader.wNumberOfSections);

    // Log(LOG_LOG, L"PREV HASH: %p\n", PML4Hash(sEXE.pPageTable));
    
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
            Log(LOG_LOG, L"ADDR: %p DATA %p", sSection.qwVirtualAddress + sEXE.qwBaseAddress, sSection.pData);
            MapPageRange(sEXE.pPageTable, (PVOID) (sSection.qwVirtualAddress + sEXE.qwBaseAddress), sSection.pData, _BYTES_TO_PAGES((QWORD) pSection->dwSizeOfRawData),
                                                  (pSection->dwCharacteristics & PE32_SCN_MEM_WRITE ? PF_WRITEABLE : 0) |
                                                  (pSection->dwCharacteristics & PE32_SCN_MEM_NOT_CACHED ? PF_CACHEDISABLE : 0));
            LoaderSeekFile(pFile, dwPEHeaderOffset);
            LoaderReadFile(pFile, (PBYTE) sSection.pData + (sSection.qwVirtualAddress % PAGE_SIZE), sSection.dwRawSize);
        }
        
        AddListElement(&sEXE.lstSections, &sSection);
    }

    // Log(LOG_LOG, L"HASH: %p\n", PML4Hash(sEXE.pPageTable));

    KHeapFree(pSections);

    if (sDataDirectory.sExportTable.dwVirtualAddress != 0)
    {
        sPE32ExportTableHeader sExportTableHeader;
        LoaderSeekFile(pFile, RVAToOffset(sDataDirectory.sExportTable.dwVirtualAddress, &sEXE.lstSections));
        _ASSERT(LoaderReadFile(pFile, &sExportTableHeader, sizeof(sPE32ExportTableHeader)) == sizeof(sPE32ExportTableHeader), L"Invalid read size for sPE32ExportHeader");
        
        // 4 bytes an entry
        PDWORD pNames = KHeapAlloc(sExportTableHeader.dwNumberOfNames * sizeof(DWORD));
        LoaderSeekFile(pFile, RVAToOffset(sExportTableHeader.dwAddressOfNames, &sEXE.lstSections));
        _ASSERT(LoaderReadFile(pFile, pNames, sExportTableHeader.dwNumberOfNames * sizeof(DWORD)) == sExportTableHeader.dwNumberOfNames * sizeof(DWORD), L"Invalid read size for Names");
        // 4 bytes an entry
        // PDWORD pFunctions = (PVOID) ((PBYTE) pEXEData + RVAToOffset(sExportTableHeader.dwAddressOfFunctions, &sEXE.lstSections));
        // 2 bytes an entry
        PWORD pOrdinals = KHeapAlloc(sExportTableHeader.dwNumberOfFunctions * sizeof(WORD));
        LoaderSeekFile(pFile, RVAToOffset(sExportTableHeader.dwAddressOfNameOrdinals, &sEXE.lstSections));
        _ASSERT(LoaderReadFile(pFile, pOrdinals, sExportTableHeader.dwNumberOfFunctions * sizeof(WORD)) == sExportTableHeader.dwNumberOfFunctions * sizeof(WORD), L"Invalid read size for Name Ordinals");

        for (DWORD i = 0; i < sExportTableHeader.dwNumberOfFunctions; i++)
        {
            sExecutableFunction sFunc;
            // sFunc.pFunctionAddress = pFunctions[i];
            sFunc.wOrdinal = pOrdinals[i];
            LoaderSeekFile(pFile, RVAToOffset(pNames[i], &sEXE.lstSections));
            LoaderReadFile(pFile, sFunc.szName, 256); // Not the best way to do it, for it ignores the actual length of function names.
            AddListElement(&sEXE.lstExportedFunctions, &sFunc);
        }
    }
    
    sPE32ImageImportDescriptor sImportDescriptors;
    PrintFormat(L"dwAddressOfOriginalFirstThunk : %d\n", sImportDescriptors.dwAddressOfOriginalFirstThunk);
    PrintFormat(L"dwAddressOfName               : %d\n", sImportDescriptors.dwAddressOfName);
    PrintFormat(L"dwAddressOfFirstThunk         : %d\n", sImportDescriptors.dwAddressOfFirstThunk);

    QWORD qwImportDescriptorOffset = RVAToOffset(sDataDirectory.sImportTable.dwVirtualAddress, &sEXE.lstSections);
    if (qwImportDescriptorOffset == 0)
        goto SkipParsingImportTable;
    LoaderSeekFile(pFile, qwImportDescriptorOffset);
    _ASSERT(LoaderReadFile(pFile, &sImportDescriptors, sizeof(sPE32ImageImportDescriptor)) == sizeof(sPE32ImageImportDescriptor), L"Invalid read size for sPE32ImageImportDescriptor");

    while (sImportDescriptors.dwAddressOfOriginalFirstThunk || sImportDescriptors.dwAddressOfName || sImportDescriptors.dwAddressOfFirstThunk)
    {
        LoaderSeekFile(pFile, RVAToOffset(sImportDescriptors.dwAddressOfName, &sEXE.lstSections));
        CHAR szName[257] = { 0 };
        // No assert for now, in case it tries to read over the file size.
        LoaderReadFile(pFile, szName, 256);
        // QWORD qwLibraryIndex = sEXE.lstImportedLibraries.qwLength;

        // Load all the functions
        sPE32ImageThunkData sThunk;
        QWORD qwDescriptorSeek = LoaderTellFile(pFile);
        QWORD qwFirstThunkOffset = RVAToOffset(sImportDescriptors.dwAddressOfOriginalFirstThunk, &sEXE.lstSections);
        _ASSERT(qwFirstThunkOffset, L"Thunk '%s' maybe corrupted!", szName);
        LoaderSeekFile(pFile, qwFirstThunkOffset);
        _ASSERT(LoaderReadFile(pFile, &sThunk, sizeof(sPE32ImageThunkData)) == sizeof(sPE32ImageThunkData), L"Invalid read size for sPE32ImageThunkData");

        while (sThunk.qwFunction)
        {
            PrintFormat(L"Function: %p Name: %s Pos: %d\n", sThunk.qwFunction, szName, LoaderTellFile(pFile));
            QWORD qw = LoaderReadFile(pFile, &sThunk, sizeof(sPE32ImageThunkData));
            PrintFormat(L"Length %d Size %d\n", qw, sizeof(sPE32ImageThunkData));
            _ASSERT(qw == sizeof(sPE32ImageThunkData), L"Invalid read size for sPE32ImageThunkData");
        }
        
        // Move to the next descriptor
        LoaderSeekFile(pFile, qwDescriptorSeek + sizeof(sPE32ImageImportDescriptor));
        _ASSERT(LoaderReadFile(pFile, &sImportDescriptors, sizeof(sPE32ImageImportDescriptor)) == sizeof(sPE32ImageImportDescriptor), L"Invalid read size for sPE32ImageImportDescriptor");
    }

SkipParsingImportTable:
    return sEXE;
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
