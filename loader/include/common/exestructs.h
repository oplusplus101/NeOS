
#ifndef __EXEHEADERS_H
#define __EXEHEADERS_H

// Code taken from: https://wiki.osdev.org/PE and https://learn.microsoft.com/en-us/windows/win32/debug/pe-format

#define PE32_SCN_CNT_CODE               0x00000020 // The section contains executable code.
#define PE32_SCN_CNT_INITIALIZED_DATA   0x00000040 // The section contains initialized data.
#define PE32_SCN_CNT_UNINITIALIZED_DATA 0x00000080 // The section contains uninitialized data.
#define PE32_SCN_GPREL                  0x00008000 // The section contains data referenced through the global pointer (GP).
#define PE32_SCN_LNK_NRELOC_OVFL        0x01000000 // The section contains extended relocations.
#define PE32_SCN_MEM_DISCARDABLE        0x02000000 // The section can be discarded as needed.
#define PE32_SCN_MEM_NOT_CACHED         0x04000000 // The section cannot be cached.
#define PE32_SCN_MEM_NOT_PAGED          0x08000000 // The section is not pageable.
#define PE32_SCN_MEM_SHARED             0x10000000 // The section can be shared in memory.
#define PE32_SCN_MEM_EXECUTE            0x20000000 // The section can be executed as code.
#define PE32_SCN_MEM_READ               0x40000000 // The section can be read.
#define PE32_SCN_MEM_WRITE              0x80000000 // The section can be written to.

typedef struct
{
    WORD wMagic;
    WORD wExtraBytes;
    WORD wPages;
    WORD wRelocationItems;
    WORD wHeaderSize;
    WORD wMinimumAllocation;
    WORD wMaximumAllocation;
    WORD wInitialSS;
    WORD wInitialSP;
    WORD wChecksum;
    WORD wInitialIP;
    WORD wInitialCS;
    WORD wRelocationTable;
    WORD wOverlay;
} __attribute__((packed)) sMZHeader;

typedef struct
{
    DWORD dwMagic;
    WORD  wMachine;
    WORD  wNumberOfSections;
    DWORD dwTimeDateStamp;
    DWORD dwPointerToSymbolTable;
    DWORD dwNumberOfSymbols;
    WORD  wSizeOfOptionalHeader;
    WORD  wCharacteristics;
} __attribute__((packed)) sPE32Header;

typedef struct
{
    WORD  wMagic;
    BYTE  nMajorLinkerVersion;
    BYTE  nMinorLinkerVersion;
    DWORD dwSizeOfCode;
    DWORD dwSizeOfInitializedData;
    DWORD dwSizeOfUninitializedData;
    DWORD dwAddressOfEntrypoint;
    DWORD dwBaseOfCode;
    QWORD qwImageBase;
    DWORD dwSectionAlignment;
    DWORD dwFileAlignment;
    WORD  wMajorOperatingSystemVersion;
    WORD  wMinorOperatingSystemVersion;
    WORD  wMajorImageVersion;
    WORD  wMinorImageVersion;
    WORD  wMajorSubsystemVersion;
    WORD  wMinorSubsystemVersion;
    DWORD dwWin32VersionValue;
    DWORD dwSizeOfImage;
    DWORD dwSizeOfHeaders;
    DWORD dwCheckSum;
    WORD  wSubsystem;
    WORD  wDllCharacteristics;
    QWORD qwSizeOfStackReserve;
    QWORD qwSizeOfStackCommit;
    QWORD qwSizeOfHeapReserve;
    QWORD qwSizeOfHeapCommit;
    DWORD dwLoaderFlags;
    DWORD dwNumberOfRvaAndSizes;
} __attribute__((packed)) sPE32OptionalHeader;

typedef struct
{
    BYTE  szName[8];
    DWORD dwVirtualSize;
    DWORD dwVirtualAddress;
    DWORD dwSizeOfRawData;
    DWORD dwPointerToRawData;
    DWORD dwPointerToRelocations;
    DWORD dwPointerToLineNumbers;
    WORD  wNumberOfRelocations;
    WORD  wNumberOfLineNumbers;
    DWORD dwCharacteristics;
} __attribute__((packed)) sPE32SectionHeader;

#endif // __EXEHEADERS_H
