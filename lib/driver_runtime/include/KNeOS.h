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

#define MS __attribute__((stdcall))
#define SYSV __attribute__((sysv_abi))

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
    void (MS *pDriverEntry)(struct _tagDriverObject *);
    void (MS *pDriverUnload)(struct _tagDriverObject *);

    INT (MS *arrIOHandlers[5])(struct _tagDriverObject *, struct _tagIORequestPacket *);
} sDriverObject;

typedef struct
{
    WCHAR wszName[256];
    DWORD dwDriverStackSize;
    sDriverObject **ppDriverStack;
    DWORD dwDeviceType;
} sDeviceObject;

typedef struct
{
    sDeviceObject *pDeviceObject;
    sString wsPath;
    PVOID pContext;
} sFileObject;


typedef struct _tagIORequestPacket
{
    BYTE           bOperation;
    PVOID          pBuffer;
    QWORD          qwLength;
    INT            iStatus;
    sDeviceObject *pDevice;
    QWORD          qwDriverStackIndex;
    void (MS *pFinishCallback)(struct _tagIORequestPacket *);

    sFileObject   *pFileObject;
} sIORequestPacket;

void MS KNeoPrintChar(CHAR c);
void MS KNeoPrintString(PCHAR sz);
void MS KNeoGetCursor(INT *x, INT *y);
void MS KNeoSetCursor(INT x, INT y);
void MS KNeoClearScreen();
void MS KNeoGetScreenSize(INT *pWidth, INT *pHeight);
INT MS KNeoGetVideoMode();

PVOID MS KNeoGetDriverObject(PCHAR szName);
INT MS KNeoCallDriver(sDriverObject *pDriverObject, sIORequestPacket *pIRP);

void MS KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);
void MS KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, QWORD (SYSV *pCallback)(QWORD));
void MS KNeoLog(INT iType, const PWCHAR wszMessage, ...);

INT MS KNeoGetCurrentPID();
INT MS KNeoStartProcess(void (*pEntryPoint), QWORD qwStackSize);
void MS KNeoKillProcess(INT iPID);
void MS KNeoPauseProcess(INT iPID);

INT MS KNeoCreateDevice(sDriverObject *pDriverObject, sString wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
INT MS KNeoCreateFile(HANDLE *pHandle, sString wsPath, HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);

PVOID MS KNeoHeapAllocate(QWORD qwSize);
PVOID MS KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize);
void MS KNeoHeapFree(PVOID pMemory);
PVOID MS KNeoGetPhysicalAddress(PVOID pVirtualAddress);

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
