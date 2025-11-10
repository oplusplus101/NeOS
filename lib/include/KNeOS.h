#ifndef __KNEOS_H
#define __KNEOS_H

#include <NeoTypes.h>

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
#define FUNC_EXPORT __declspec(dllexport)
#else
#define FUNC_EXPORT __declspec(dllimport)
#endif

#define _ASSERT(c, m) if (!(c)) KNeoPrintFormat(L"%w", m);

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
    void (*pDriverEntry)(struct _tagDriverObject *);
    void (*pDriverUnload)(struct _tagDriverObject *);

    INT (*arrIOHandlers[5])(struct _tagDriverObject *, struct _tagIORequestPacket *);

    PVOID pPML4; // Drivers shouln't have to access this
} __attribute__((packed)) sDriverObject;

typedef struct
{
    WCHAR wszName[256];
    DWORD dwDriverStackSize;
    sDriverObject **ppDriverStack;
    DWORD dwDeviceType;
} __attribute__((packed)) sDeviceObject;

typedef struct _tagIORequestPacket
{
    BYTE           nOperation;
    PVOID          pBuffer;
    QWORD          qwLength;
    INT            iStatus;
    sDeviceObject *pDevice;
    QWORD          qwDriverStackIndex;
    void         (*pFinishCallback)(struct _tagIORequestPacket *);
} __attribute__((packed)) sIORequestPacket;

FUNC_EXPORT void KNeoPrintString(PCHAR sz);
FUNC_EXPORT void KNeoGetCursor(INT *x, INT *y);
FUNC_EXPORT void KNeoSetCursor(INT x, INT y);
FUNC_EXPORT void KNeoClearScreen();
FUNC_EXPORT void KNeoGetScreenSize(INT *pWidth, INT *pHeight);
FUNC_EXPORT INT KNeoGetVideoMode();

FUNC_EXPORT PVOID KNeoGetDriverObject(PCHAR szName);
FUNC_EXPORT INT KNeoCallDriver(sDriverObject *pDriverObject, sIORequestPacket *pIRP);

FUNC_EXPORT void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);
FUNC_EXPORT void KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, void (*pCallback)());
FUNC_EXPORT void KNeoLog(PWCHAR wszMessage, BYTE bCode);

FUNC_EXPORT INT KNeoGetCurrentPID();
FUNC_EXPORT INT KNeoStartProcess(void (*pEntryPoint), QWORD qwStackSize);
FUNC_EXPORT void KNeoKillProcess(INT iPID);
FUNC_EXPORT void KNeoPauseProcess(INT iPID);

FUNC_EXPORT INT KNeoCreateDevice(sDriverObject *pDriverObject, PWCHAR wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
FUNC_EXPORT INT KNeoCreateFile(HANDLE *pHandle, PWCHAR wszPath, HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);

FUNC_EXPORT PVOID KNeoHeapAllocate(QWORD qwSize);
FUNC_EXPORT PVOID KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize);
FUNC_EXPORT void KNeoHeapFree(PVOID pMemory);
FUNC_EXPORT PVOID KNeoGetPhysicalAddress(PVOID pVirtualAddress);


FUNC_EXPORT void KNeoPrintHex(QWORD n, BYTE nDigits, BOOL bUppercase);
FUNC_EXPORT void KNeoPrintDec(QWORD n);
FUNC_EXPORT void KNeoPrintFormat(const PWCHAR wszFormat, ...);

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
