#ifndef __KNEOS_H
#define __KNEOS_H

#include <NeoTypes.h>
#include <NeoList.h>

#define PAGE_PRESENT            1
#define PAGE_WRITEABLE          2
#define PAGE_USER               4
#define PAGE_WRITETHROUGH       8
#define PAGE_CACHEDISABLE       16
#define PAGE_ACCESSED           32
#define PAGE_DIRTY              64
#define PAGE_PAGETABLEATTRIBUTE 128
#define PAGE_GLOBAL             256

#define FUNC_EXPORT __attribute__((visibility("default")))

#define _ASSERT(c, m) if (!(c)) KNeoPrintString(m);

enum
{
    IO_CREATE,
    IO_DESTROY,
    IO_READ,
    IO_WRITE,
    IO_CONTROL
};


typedef struct
{
    WCHAR        wszName[256];
    sList        lstDriverStack;
    DWORD        dwDeviceType;
} sDeviceObject;

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

typedef struct _tagDriverObject
{
    void (*pDriverEntry)(struct _tagDriverObject *);
    void (*pDriverUnload)(struct _tagDriverObject *);

    INT (*arrIOHandlers[5])(struct _tagDriverObject *, sIORequestPacket *);

    PVOID pPML4; // Drivers shouln't have to access this
} __attribute__((packed)) sDriverObject;


void KNeoPrintString(PCHAR sz);
void KNeoGetCursor(INT *x, INT *y);
void KNeoSetCursor(INT x, INT y);
void KNeoClearScreen();
void KNeoGetScreenSize(INT *pWidth, INT *pHeight);
INT KNeoGetVideoMode();

PVOID KNeoGetDriverObject(PCHAR szName);
INT KNeoCallDriver(sDriverObject *pDriverObject, sIORequestPacket *pIRP);

void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);
void KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, void (*pCallback)());

INT KNeoGetCurrentPID();
INT KNeoStartProcess(void (*pEntryPoint), QWORD qwStackSize);
void KNeoKillProcess(INT iPID);
void KNeoPauseProcess(INT iPID);

INT KNeoCreateDevice(sDriverObject *pDriverObject, PWCHAR wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject);
INT KNeoCreateFile(HANDLE *pHandle, PWCHAR wszPath, HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType);

PVOID KNeoHeapAllocate(QWORD qwSize);
PVOID KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize);
void KNeoHeapFree(PVOID pMemory);
PVOID KNeoGetPhysicalAddress(PVOID pVirtualAddress);


void KNeoPrintHex(QWORD n, BYTE nDigits, BOOL bUppercase);
void KNeoPrintDec(QWORD n);
void KNeoPrintFormat(const PWCHAR wszFormat, ...);

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
