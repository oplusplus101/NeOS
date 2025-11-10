
#include <common/math.h>
#include <common/types.h>
#include <common/panic.h>
#include <common/string.h>
#include <common/bootstructs.h>
#include <common/exceptions.h>
#include <common/exestructs.h>
#include <common/screen.h>

#include <hardware/storage/drive.h>
#include <filesystem/gpt.h>
#include <filesystem/fat32.h>
#include <hardware/gdt.h>
#include <hardware/idt.h>
#include <hardware/pci.h>
#include <memory/paging.h>
#include <memory/heap.h>
#include <common/memory.h>
#include <common/string.h>
#include <common/ini.h>
#include <common/log.h>
#include <neos.h>

QWORD __stack_chk_guard = 0x595e9fbd94fda766;

void __stack_chk_fail(void)
{
	_KERNEL_PANIC(L"Stack smashing detected");
}

// The functions below are for the kernel,
// so the it doesn't have to access the directory entry directly (for compatibility with other file systems)

typedef struct
{
    sFAT32DirectoryEntry sEntry;
    QWORD qwPosition;
} __attribute__((packed)) sFile;

QWORD GetFileSize(sFAT32DirectoryEntry *pFile)
{
    return pFile->dwFileSize;
}

PVOID OpenFile(PWCHAR wszPath)
{
    sFile *pFile = KHeapAlloc(sizeof(sFile));
    if (pFile == NULL) return NULL;
    pFile->qwPosition = 0;
    if (!GetEntryFromPath(wszPath, &pFile->sEntry)) return NULL;
    return pFile;
}

QWORD ReadFile(PVOID pFile, PVOID pBuffer, QWORD qwSize)
{
    sFile *pFileStruct = (sFile *) pFile;
    QWORD qwReadSize = ReadDirectoryEntry(&pFileStruct->sEntry, pBuffer, qwSize, pFileStruct->qwPosition);
    pFileStruct->qwPosition += qwReadSize;
    return qwReadSize;
}

QWORD TellFile(PVOID pFile)
{
    return ((sFile *) pFile)->qwPosition;
}

void SeekFile(PVOID pFile, QWORD qwPosition)
{
    if (qwPosition >= ((sFile *) pFile)->sEntry.dwFileSize) return;
    ((sFile *) pFile)->qwPosition = qwPosition;
}

void CloseFile(PVOID pFile)
{
    KHeapFree(pFile);
}

void LoaderMain(sBootData data)
{
    DisableInterrupts();

    InitGDT();
    WriteGDT();

    // Screen
    InitScreen(data.gop.nWidth, data.gop.nHeight, data.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(NEOS_BACKGROUND_COLOR);
    SetFGColor(NEOS_FOREGROUND_COLOR);
    InitLogger();

    Log(LOG_LOG, L"Screen initialised!");
    
    // Interrupts
    InitIDT();
    RegisterExceptions();
    Log(LOG_LOG, L"Interrupts initialised!");

    // Paging
    InitPaging(data.pMemoryDescriptor,
               data.nMemoryMapSize, data.nMemoryDescriptorSize,
               data.nLoaderStart, data.nLoaderEnd);

    for (;;);
    // Lock the GOP screen memory.
    ReservePages(data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1);
    MapPageRangeToIdentity(NULL, data.gop.pFramebuffer, data.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    Log(LOG_LOG, L"Paging initialised!");
    
    // Set up the heap
    sHeap sKernelHeap = CreateHeap(NEOS_HEAP_SIZE / PAGE_SIZE, false, true, NULL);
    SetKernelHeap(&sKernelHeap);
    Log(LOG_LOG, L"Heap initialised!");

    // Set up the drive
    Log(LOG_LOG, L"Scanning for PCI devices...");
    ScanPCIDevices();

    InitDrives();

    LoadGPT(DRIVE_TYPE_AHCI_SATA);
    Log(LOG_LOG, L"GPT Loaded");
    
    sGPTPartitionEntry sKernelPartition = GetKernelPartition();
    LoadFAT32(DRIVE_TYPE_AHCI_SATA, sKernelPartition);

    Log(LOG_LOG, L"Loading kernel...");

    sFAT32DirectoryEntry sKernelEntry;
    _ASSERT(GetEntryFromPath(L"NeOS\\NeOS.sys", &sKernelEntry), L"Kernel not found at C:\\NeOS\\NeOS.sys");

    // Read the kernel
    PBYTE pKernelData = KHeapAlloc(sKernelEntry.dwFileSize);
    ReadDirectoryEntry(&sKernelEntry, pKernelData, sKernelEntry.dwFileSize, 0);
    
    // Parse the file (PE32+)
    Log(LOG_LOG, L"Loading PE headers...");
    sMZHeader *pMZHeader = (sMZHeader *) pKernelData;
    _ASSERT(pMZHeader->wMagic == 0x5A4D, L"Invalid DOS stub.");

    sPE32Header *pPEHeader = (sPE32Header *) (pKernelData + 128); // FIXME: Replace the 128 with the proper size of the MZ header
    _ASSERT(pPEHeader->dwMagic == 0x4550, L"Invalid PE32 header.");
    _ASSERT(pPEHeader->wMachine == 0x8664, L"Kernel executable must be 64-bit.");
    _ASSERT(pPEHeader->wSizeOfOptionalHeader != 0, L"Missing PE32 optional header.");
    
    sPE32OptionalHeader *pPEOHeader = (sPE32OptionalHeader *) (pPEHeader + 1);
    _ASSERT(pPEOHeader->wMagic == 0x020B, L"Invalid PE32 optional header.");
    Log(LOG_LOG, L"Loading sections...");

    // TODO: Add saftey checks to ensure that the entrypoint inside the file matches the expected value (NEOS_KERNEL_LOCATION).
    DisableInterrupts(); // To ensure that everything is loaded in the right order
    for (QWORD i = 0; i < pPEHeader->wNumberOfSections; i++)
    {
        sPE32SectionHeader *hdr = (sPE32SectionHeader *) ((PBYTE) pPEOHeader + pPEHeader->wSizeOfOptionalHeader + sizeof(sPE32SectionHeader) * i);
        QWORD qwAddress = hdr->dwVirtualAddress + pPEOHeader->qwImageBase;
        if (hdr->dwCharacteristics & PE32_SCN_MEM_DISCARDABLE) continue;
        Log(LOG_LOG, L"Loading section: %s at 0x%p", hdr->szName, qwAddress, hdr->dwCharacteristics);
        memcpy((PVOID) qwAddress, (PBYTE) pKernelData + hdr->dwOffsetOfRawData, hdr->dwVirtualSize);
    }
    
    KHeapFree(pKernelData);

    // Assume the kernel will be between 0x200000 - 0x300000
    ReservePages((PVOID) NEOS_KERNEL_LOC_LOW, (NEOS_KERNEL_LOC_HIGH - NEOS_KERNEL_LOC_LOW) / PAGE_SIZE);
    MapPageRangeToIdentity(NULL, (PVOID) NEOS_KERNEL_LOC_LOW, (NEOS_KERNEL_LOC_HIGH - NEOS_KERNEL_LOC_LOW) / PAGE_SIZE, PF_WRITEABLE);
    
    // Prepare the kernel boot header
    
    sNEOSKernelHeader sHeader;
    sHeader.sGOP        = data.gop;
    sHeader.pKernelHeap = &sKernelHeap;
    sHeader.sPaging     = ExportPagingData();

    // Filesystem functions
    sHeader.LoaderOpenFile    = (PVOID (*)(PWCHAR)) OpenFile;
    sHeader.LoaderGetFileSize = (QWORD (*)(PVOID)) GetFileSize;
    sHeader.LoaderReadFile    = (QWORD (*)(PVOID, PVOID, QWORD)) ReadFile;
    sHeader.LoaderCloseFile   = (void (*)(PVOID)) CloseFile;
    sHeader.LoaderSeekFile    = (void (*)(PVOID, QWORD)) SeekFile;
    sHeader.LoaderTellFile    = (QWORD (*)(PVOID)) TellFile;

    // Screen functions
    sHeader.PrintFormat     = (void (*)(const PWCHAR, ...)) PrintFormat;
    sHeader.PrintString     = (void (*)(PCHAR)) PrintString;
    sHeader.PrintBytes      = (void (*)(PVOID, QWORD, WORD, BOOL)) PrintBytes;
    sHeader.PrintChar       = (void (*)(CHAR)) PrintChar;
    sHeader.Log             = (void (*)(INT, const PWCHAR, ...)) Log;
    sHeader.GetCursorX      = (INT  (*)()) GetCursorX;
    sHeader.GetCursorY      = (INT  (*)()) GetCursorY;
    sHeader.SetCursor       = (void (*)(INT, INT)) SetCursor;
    sHeader.SetFGColor      = (void (*)(color_t)) SetFGColor;
    sHeader.SetBGColor      = (void (*)(color_t)) SetBGColor;
    sHeader.ClearScreen     = (void (*)()) ClearScreen;
    sHeader.GetScreenWidth  = (INT  (*)()) GetScreenWidth;
    sHeader.GetScreenHeight = (INT  (*)()) GetScreenHeight;

    Log(LOG_LOG, L"Executing C:\\NeOS\\NeOS.sys at 0x%p", pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint);
    
    ((void (*)(sNEOSKernelHeader)) (pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint))(sHeader);

    // Somthing went very wrong
    _KERNEL_PANIC(L"The kernel has crashed :/");
}
