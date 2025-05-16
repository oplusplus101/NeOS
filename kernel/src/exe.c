
#include <exe.h>
#include <common/exestructs.h>
#include <common/string.h>
#include <common/memory.h>
#include <common/screen.h>
#include <common/panic.h>
#include <memory/heap.h>

sExecutable ParsePE32(PVOID pEXEData)
{
    sMZHeader *pMZHeader = (sMZHeader *) pEXEData;
    _ASSERT(pMZHeader->wMagic == 0x5A4D, "Invalid MZ header magic number: %04X", pMZHeader->wMagic);
    sPE32Header *pHeader = (sPE32Header *) ((PBYTE) pEXEData + 128);
    _ASSERT(pHeader->dwMagic == 0x4550, "Invalid PE header magic number: %08X", pHeader->dwMagic);
    sPE32OptionalHeader *pOptionalHeader = (sPE32OptionalHeader *) ((PBYTE) pEXEData + sizeof(sPE32Header) + 128);
    _ASSERT(pOptionalHeader->wMagic == 0x020B, "Invalid PE optional header magic number: %04X", pHeader->dwMagic);
    sExecutable sEXE =
    {
        .pEntryPoint = (PVOID) (pOptionalHeader->qwImageBase + pOptionalHeader->dwAddressOfEntrypoint),
        .lstExportedFunctions = CreateEmptyList(sizeof(sExecutableFunction)),
        .lstSections = CreateEmptyList(sizeof(sExecutableSection))
    };

    PrintFormat("Image Base  : %p\n", pOptionalHeader->qwImageBase);
    PrintFormat("Entry Point : %p\n", pOptionalHeader->dwAddressOfEntrypoint);
    
    for (WORD i = 0; i < pHeader->wNumberOfSections; i++)
    {
        sPE32SectionHeader *pSection = (sPE32SectionHeader *) ((PBYTE) pOptionalHeader + pHeader->wSizeOfOptionalHeader) + i;
        
        sExecutableSection sSection;
        strncpy(sSection.szName, (PCHAR) pSection->szName, 8);
        sSection.dwRawSize        = pSection->dwSizeOfRawData;
        sSection.dwVirtualSize    = pSection->dwVirtualSize;
        sSection.qwVirtualAddress = pOptionalHeader->qwImageBase + pSection->dwVirtualAddress;
        PrintFormat("Name            : %s\n", sSection.szName);
        PrintFormat("Virtual Size    : %u\n", sSection.dwVirtualSize);
        PrintFormat("Raw Size        : %u\n", sSection.dwRawSize);
        PrintFormat("Virtual Address : %p\n", sSection.qwVirtualAddress);

        if (pSection->dwSizeOfRawData > 0)
        {
            sSection.pData = AllocateContinousPages(_BYTES_TO_PAGES(pSection->dwVirtualSize));
            memcpy((PBYTE) sSection.pData + (pSection->dwVirtualAddress % PAGE_SIZE), (PBYTE) pEXEData + pSection->dwPointerToRawData, sizeof(pSection->dwSizeOfRawData));
        }
        else
            sSection.pData = NULL;
        
        AddListElement(&sEXE.lstSections, &sSection);
    }
    
    return sEXE;
}

void FreeExecutable(sExecutable *pEXE)
{
}
