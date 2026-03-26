#ifndef __KNEOS_H
#define __KNEOS_H

#include <NeoTypes.h>
#include <NeoCountedString.h>

#define PAGE_PRESENT            1
#define PAGE_WRITEABLE          2
#define PAGE_USER               4
#define PAGE_WRITETHROUGH       8
#define PAGE_CACHEDISABLE       16
#define PAGE_ACCESSED           32
#define PAGE_DIRTY              64
#define PAGE_PAGETABLEATTRIBUTE 128
#define PAGE_GLOBAL             256

#ifdef DLL_BUILD
#define LIB_FUNC __declspec(dllexport)
#else
#define LIB_FUNC __declspec(dllimport)
#endif
#define STDCALL __attribute__((stdcall))

#define _ASSERT(c, m) if (!(c)) KNeoPrintFormat(L"%S", m);

enum
{
    IO_CREATE,
    IO_DESTROY,
    IO_READ,
    IO_WRITE,
    IO_CONTROL
};


struct _tagIORequestPacket;

typedef struct _tagDriverObject
{
    void (STDCALL *pDriverEntry)(struct _tagDriverObject *);
    void (STDCALL *pDriverUnload)(struct _tagDriverObject *);

    INT (STDCALL *arrIOHandlers[5])(struct _tagDriverObject *, struct _tagIORequestPacket *);

    PVOID pPML4; // Drivers shouln't have to access this
} __attribute__((packed)) sDriverObject;

typedef struct
{
    WCHAR wszName[256];
    DWORD dwDriverStackSize;
    sDriverObject **ppDriverStack;
    DWORD dwDeviceType;
} __attribute__((packed)) sDeviceObject;

typedef struct
{
    DWORD dwType;
    QWORD qwSize;
    sDeviceObject *pDeviceObject;
    PWCHAR wszFileName;
    QWORD qwOffsetInBytes;
    PVOID pContext;
} __attribute__((packed)) sFileObject;

typedef struct _tagIORequestPacket
{
    BYTE           nOperation;
    PVOID          pBuffer;
    QWORD          qwLength;
    INT            iStatus;
    sDeviceObject *pDevice;
    QWORD          qwDriverStackIndex;
    void (STDCALL *pFinishCallback)(struct _tagIORequestPacket *);

    sFileObject   *pFileObject;
} __attribute__((packed)) sIORequestPacket;

LIB_FUNC void STDCALL KNeoPrintChar(CHAR c);
LIB_FUNC void STDCALL KNeoPrintString(PCHAR sz);
LIB_FUNC void STDCALL KNeoGetCursor(INT *x, INT *y);
LIB_FUNC void STDCALL KNeoSetCursor(INT x, INT y);
LIB_FUNC void STDCALL KNeoClearScreen();
LIB_FUNC void STDCALL KNeoGetScreenSize(INT *pWidth, INT *pHeight);
LIB_FUNC INT STDCALL KNeoGetVideoMode();

LIB_FUNC PVOID STDCALL KNeoGetDriverObject(PCHAR szName);
LIB_FUNC INT STDCALL KNeoCallDriver(sDriverObject *pDriverObject, sIORequestPacket *pIRP);

LIB_FUNC void STDCALL KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);
LIB_FUNC void STDCALL KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, QWORD (*pCallback)(QWORD));
LIB_FUNC void STDCALL KNeoLog(INT iType, const PWCHAR wszMessage, ...);

LIB_FUNC INT STDCALL KNeoGetCurrentPID();
LIB_FUNC INT STDCALL KNeoStartProcess(void (*pEntryPoint), QWORD qwStackSize);
LIB_FUNC void STDCALL KNeoKillProcess(INT iPID);
LIB_FUNC void STDCALL KNeoPauseProcess(INT iPID);

LIB_FUNC INT STDCALL KNeoCreateDevice(sDriverObject *pDriverObject, sString wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
LIB_FUNC INT STDCALL KNeoCreateFile(HANDLE *pHandle, sString wszPath, HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);

LIB_FUNC PVOID STDCALL KNeoHeapAllocate(QWORD qwSize);
LIB_FUNC PVOID STDCALL KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize);
LIB_FUNC void STDCALL KNeoHeapFree(PVOID pMemory);
LIB_FUNC PVOID STDCALL KNeoGetPhysicalAddress(PVOID pVirtualAddress);


LIB_FUNC void STDCALL KNeoPrintHex(QWORD n, BYTE nDigits, BOOL bUppercase);
LIB_FUNC void STDCALL KNeoPrintDec(QWORD n);
LIB_FUNC void STDCALL KNeoPrintFormat(const PWCHAR wszFormat, ...);

// TODO: Replace these with ARCH calls
inline static void KNeoEnableInterrupts()
{
    __asm__ volatile ("sti");
}

inline static void KNeoDisableInterrupts()
{
    __asm__ volatile ("cli");
}

inline static BOOL KNeoAreInterruptsEnabled()
{
    QWORD qwFLAGS;
    __asm__ volatile ("pushf\npop %%rax" : "=a"(qwFLAGS));
    return (qwFLAGS >> 9) & 1;
}

#endif // __KNEOS_H
