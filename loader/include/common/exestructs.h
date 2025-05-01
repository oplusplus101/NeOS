
#ifndef __EXEHEADERS_H
#define __EXEHEADERS_H

// Code taken from: https://wiki.osdev.org/PE

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
