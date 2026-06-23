
#ifndef __RUNTIME__EXE_H
#define __RUNTIME__EXE_H

// #include <common/list.h>
#include <runtime/objects.h>
#include <common/exestructs.h>
#include <ramfile.h>

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
    BYTE  bPagingFlags;
    PVOID pData; // Just the data extracted from the file (not re-mapped in any way)
} sExecutableSection;

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
    QWORD qwEntryPoint;
    
    sExecutableSection *arrSections;
    QWORD nsSections;
    
    QWORD qwVirtualStart, qwVirtualEnd;
    PVOID pPageTable;
    // sList lstExportedFunctions;
    // sList lstImportedFunctions;
    // sList lstImportedLibraries;
} sExecutable;

typedef struct
{
    sExecutable sEXE;

} sLibraryObject;

void InitExecutableLoader();

/// @brief Parses all the headers of a PE32+ file including sections and data directories. Sections are allocated on the heap
/// @param pFile A handle to a PE32+ file
/// @param pHeaders A valid pointer to a sPEExecutableHeaders structure
/// @return Returns a STATUS 
STATUS ParsePEHeaders(sRAMFile *pFile, sPE32Headers *pHeaders);

/// @brief Calculates a PE32+ executable's virtual size
/// @param pHeaders A headers structure
/// @return The total size in bytes
QWORD CalculatePEVirtualSize(sPE32Headers *pHeaders);

/// @brief Tries to load a PE32+ executable to pVirtualStart
/// @note This function assumes pVirtualStart is valid memory
/// @param pFile A handle to the the file
/// @param pHeaders sPE32Headers structure containing the headers and sections
/// @param pVirtualStart The desired starting address the executable be loaded to
/// @param pEntryPoint The entry point to be called. This value will be null upon failure
/// @return Returns a STATUS
STATUS LoadDriverIntoMemory(sRAMFile *pFile, sPE32Headers *pHeaders, PVOID pVirtualStart, PVOID *pEntryPoint);

// Loads a PE32+ file into memory
// Set qwBaseAddress to 0 in order to use the base address provided by the executable.
// INT LoadExecutable(PVOID pFile, QWORD qwVirtualAddress, sPageTable *pPML4, sExecutable *pEXE);
// void FreeExecutable(sExecutable *pExe);

#endif // __RUNTIME__DRIVER_H
