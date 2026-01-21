
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


// TEMPORARY: Setup some basic stack stuff
#define KERNEL_STACK_SIZE (32 * 1024) // 32 KiB
#define DOUBLE_FAULT_STACK_SIZE (8 * 1024) // 8 KiB shall suffice

static BYTE g_arrKernelStack[KERNEL_STACK_SIZE] __attribute__((aligned(4096)));
static BYTE g_arrDoubleFaultStack[DOUBLE_FAULT_STACK_SIZE] __attribute__((aligned(4096)));

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
    PWCHAR wszPathDup = strdupW(wszPath);
    if (!GetEntryFromPath(wszPathDup, &pFile->sEntry))
    {
        KHeapFree(wszPathDup);
        return NULL;
    }
    KHeapFree(wszPathDup);
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

void LoaderMainAfterStackSwitch(sBootData sData)
{
    // Set up the Task State Segment
    sTaskStateSegment *pTSS = GetTSS();
    pTSS->qwRSP0 = (QWORD) g_arrKernelStack + KERNEL_STACK_SIZE;
    pTSS->qwIST1 = (QWORD) g_arrDoubleFaultStack + DOUBLE_FAULT_STACK_SIZE;
    
    FlushTSS();
    Log(LOG_LOG, L"Task State Segment reloaded");
    
    // Set up the heap
    sHeap sKernelHeap;
    INT iStatus = CreateHeap(NEOS_HEAP_SIZE, false, true, 16, (PVOID) MM_KERNEL_HEAP_START, &sKernelHeap);
    _ASSERT(_SUCCESSFUL(iStatus), L"Failed to create the kernel heap [%08X]", iStatus);
    SetKernelHeap(&sKernelHeap);
    Log(LOG_LOG, L"Heap initialised!");

    InitDrives();
    
    // Set up the drive
    Log(LOG_LOG, L"Scanning for PCI devices...");
    ScanPCIDevices();

    LoadGPT(DRIVE_TYPE_AHCI_SATA);
    Log(LOG_LOG, L"GPT Loaded");
    
    sGPTPartitionEntry sKernelPartition = GetKernelPartition();
    LoadFAT32(DRIVE_TYPE_AHCI_SATA, sKernelPartition);
    
    Log(LOG_LOG, L"Loading kernel...");

    sFAT32DirectoryEntry sKernelEntry;
    _ASSERT(GetEntryFromPath(L"NeOS\\NeOS.sys", &sKernelEntry), L"Kernel not found at C:\\NeOS\\NeOS.sys");

    // Read the kernel
    PBYTE pKernelData = KHeapAlloc(sKernelEntry.dwFileSize);
    QWORD qwSizeRead = ReadDirectoryEntry(&sKernelEntry, pKernelData, sKernelEntry.dwFileSize, 0);
    _ASSERT(qwSizeRead == sKernelEntry.dwFileSize, L"Failed to read kernel! expected size: %d size read: %d", sKernelEntry.dwFileSize, qwSizeRead);
    // TODO: Implement a proper PE32+ parsing library, so that there isn't duplicate code everywhere.
    
    // Parse the file (PE32+)
    Log(LOG_LOG, L"Loading PE headers...");
    sMZHeader *pMZHeader = (sMZHeader *) pKernelData;
    _ASSERT(pMZHeader->wMagic == 0x5A4D, L"Invalid DOS stub %c%c.", pMZHeader->wMagic & 0xFF, pMZHeader->wMagic >> 8);

    sPE32Header *pPEHeader = (sPE32Header *) (pKernelData + 128); // FIXME: Replace the 128 with the proper size of the MZ header
    _ASSERT(pPEHeader->dwMagic == 0x4550, L"Invalid PE32 header.");
    _ASSERT(pPEHeader->wMachine == 0x8664, L"Kernel executable must be 64-bit.");
    _ASSERT(pPEHeader->wSizeOfOptionalHeader != 0, L"Missing PE32 optional header.");
    
    sPE32OptionalHeader *pPEOHeader = (sPE32OptionalHeader *) (pPEHeader + 1);
    _ASSERT(pPEOHeader->wMagic == 0x020B, L"Invalid PE32 optional header.");
    Log(LOG_LOG, L"Loading sections...");

    DisableInterrupts(); // To ensure that everything is loaded in the right order

    QWORD qwMinAddress = 0xFFFFFFFFFFFFFFFF, qwMaxAddress = 0x0000000000000000;

    for (QWORD i = 0; i < pPEHeader->wNumberOfSections; i++)
    {
        sPE32SectionHeader *hdr = (sPE32SectionHeader *) ((PBYTE) pPEOHeader + pPEHeader->wSizeOfOptionalHeader + sizeof(sPE32SectionHeader) * i);
        QWORD qwAddress = (QWORD) hdr->dwVirtualAddress + pPEOHeader->qwImageBase + NEOS_KERNEL_PHYSICAL_ADDRESS;
        if (hdr->dwCharacteristics & PE32_SCN_MEM_DISCARDABLE) continue;
        Log(LOG_LOG, L"Loading section: %s at 0x%p", hdr->szName, qwAddress, hdr->dwCharacteristics);
        memcpy((PVOID) qwAddress, (PBYTE) pKernelData + hdr->dwOffsetOfRawData, hdr->dwVirtualSize);
        ReservePages((PVOID) qwAddress, _BYTES_TO_PAGES(hdr->dwVirtualSize));
        qwMinAddress = _MIN(qwMinAddress, qwAddress);
        qwMaxAddress = _MAX(qwMaxAddress, qwAddress + hdr->dwVirtualSize);
    }
    
    KHeapFree(pKernelData);

    // Map the kernel to high memory
    MapPageRange(NULL, (PVOID) MM_KERNEL_START, (PVOID) qwMinAddress, _BYTES_TO_PAGES(qwMaxAddress - qwMinAddress), PF_WRITEABLE);
    
    // Prepare the kernel boot header
    
    sNEOSKernelHeader sHeader;
    sHeader.sGOP        = sData.gop;
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
    sHeader.PrintFormat       = (void (*)(const PWCHAR, ...)) PrintFormat;
    sHeader.PrintString       = (void (*)(PCHAR)) PrintString;
    sHeader.PrintBytes        = (void (*)(PVOID, QWORD, WORD, BOOL)) PrintBytes;
    sHeader.PrintChar         = (void (*)(CHAR)) PrintChar;
    sHeader.Log               = (void (*)(INT, const PWCHAR, ...)) Log;
    sHeader.GetCursorX        = (INT  (*)()) GetCursorX;
    sHeader.GetCursorY        = (INT  (*)()) GetCursorY;
    sHeader.SetCursor         = (void (*)(INT, INT)) SetCursor;
    sHeader.SetFGColor        = (void (*)(sColour)) SetFGColor;
    sHeader.SetBGColor        = (void (*)(sColour)) SetBGColor;
    sHeader.ClearScreen       = (void (*)()) ClearScreen;
    sHeader.GetScreenWidth    = (INT  (*)()) GetScreenWidth;
    sHeader.GetScreenHeight   = (INT  (*)()) GetScreenHeight;

    sHeader.RegisterException = (void (*)(BYTE, ESR)) RegisterException;
    sHeader.RegisterInterrupt = (void (*)(BYTE, ISR)) RegisterInterrupt;

    Log(LOG_LOG, L"Executing C:\\NeOS\\NeOS.sys at 0x%p", pPEOHeader->qwImageBase + (QWORD) pPEOHeader->dwAddressOfEntrypoint + NEOS_KERNEL_PHYSICAL_ADDRESS);
    
    ((void (*)(sNEOSKernelHeader)) (pPEOHeader->qwImageBase + pPEOHeader->dwAddressOfEntrypoint))(sHeader);

    // Somthing went very wrong
    _KERNEL_PANIC(L"The kernel has crashed :/");
}

void LoaderMain(sBootData sData)
{
    DisableInterrupts();

    InitGDT();
    WriteGDT();

    // Screen
    InitScreen(sData.gop.nWidth, sData.gop.nHeight, sData.gop.pFramebuffer);
    ClearScreen();
    SetBGColor(NEOS_BACKGROUND_COLOUR);
    SetFGColor(NEOS_FOREGROUND_COLOUR);
    InitLogger();

    Log(LOG_LOG, L"Screen initialised!");
    
    // Interrupts
    InitIDT();
    RegisterExceptions();
    Log(LOG_LOG, L"Interrupts initialised!");

    // Paging
    InitPaging(sData.pMemoryDescriptor,
               sData.nMemoryMapSize, sData.nMemoryDescriptorSize,
               sData.nLoaderStart, sData.nLoaderEnd);

    // Lock the GOP screen memory.
    ReservePages(sData.gop.pFramebuffer, sData.gop.nBufferSize / PAGE_SIZE + 1);
    MapPageRangeToIdentity(NULL, sData.gop.pFramebuffer, sData.gop.nBufferSize / PAGE_SIZE + 1, PF_WRITEABLE);
    Log(LOG_LOG, L"Paging initialised!");

    PBYTE pNewStackTop = g_arrKernelStack + KERNEL_STACK_SIZE;
    pNewStackTop = (PBYTE) ((QWORD) pNewStackTop & ~0x0F); // Align the stack

    // Push the boot header
    pNewStackTop -= sizeof(sBootData);
    *((sBootData *) pNewStackTop) = sData;
    
    __asm__ volatile(
        "mov %0, %%rsp\nmov %%rsp, %%rbp\njmp *%1\n"
        : : "r"(pNewStackTop), "r"(LoaderMainAfterStackSwitch)
        : "memory"
    );
}
