
#ifndef __EXEHEADERS_H
#define __EXEHEADERS_H

// Code taken from: https://wiki.osdev.org/PE

typedef struct
{
    uint16_t nMagic;
    uint16_t nExtraBytes;
    uint16_t nPages;
    uint16_t nRelocationItems;
    uint16_t nHeaderSize;
    uint16_t nMinimumAllocation;
    uint16_t nMaximumAllocation;
    uint16_t nInitialSS;
    uint16_t nInitialSP;
    uint16_t nChecksum;
    uint16_t nInitialIP;
    uint16_t nInitialCS;
    uint16_t nRelocationTable;
    uint16_t nOverlay;
} __attribute__((packed)) sMZHeader;

typedef struct
{
    uint32_t nMagic;
    uint16_t nMachine;
    uint16_t nNumberOfSections;
    uint32_t nTimeDateStamp;
    uint32_t nPointerToSymbolTable;
    uint32_t nNumberOfSymbols;
    uint16_t nSizeOfOptionalHeader;
    uint16_t nCharacteristics;
} __attribute__((packed)) sPE32Header;

typedef struct
{
    uint16_t nMagic;
    uint8_t  nMajorLinkerVersion;
    uint8_t  nMinorLinkerVersion;
    uint32_t nSizeOfCode;
    uint32_t nSizeOfInitializedData;
    uint32_t nSizeOfUninitializedData;
    uint32_t nAddressOfEntrypoint;
    uint32_t nBaseOfCode;
    uint64_t nImageBase;
    uint32_t nSectionAlignment;
    uint32_t nFileAlignment;
    uint16_t nMajorOperatingSystemVersion;
    uint16_t nMinorOperatingSystemVersion;
    uint16_t nMajorImageVersion;
    uint16_t nMinorImageVersion;
    uint16_t nMajorSubsystemVersion;
    uint16_t nMinorSubsystemVersion;
    uint32_t nWin32VersionValue;
    uint32_t nSizeOfImage;
    uint32_t nSizeOfHeaders;
    uint32_t nCheckSum;
    uint16_t nSubsystem;
    uint16_t nDllCharacteristics;
    uint64_t nSizeOfStackReserve;
    uint64_t nSizeOfStackCommit;
    uint64_t nSizeOfHeapReserve;
    uint64_t nSizeOfHeapCommit;
    uint32_t nLoaderFlags;
    uint32_t nNumberOfRvaAndSizes;
} __attribute__((packed)) sPE32OptionalHeader;

typedef struct
{
    uint8_t  sName[8];
    uint32_t nVirtualSize;
    uint32_t nVirtualAddress;
    uint32_t nSizeOfRawData;
    uint32_t nPointerToRawData;
    uint32_t nPointerToRelocations;
    uint32_t nPointerToLinenumbers;
    uint16_t nNumberOfRelocations;
    uint16_t nNumberOfLineNumbers;
    uint32_t nCharacteristics;
} __attribute__((packed)) sPE32SectionHeader;

#endif // __EXEHEADERS_H
