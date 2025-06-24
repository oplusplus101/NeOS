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

#define FUNC_EXPORT __attribute__((visibility("default")))

#define _ASSERT(c, m) if (!(c)) KNeoPrintString(m);

typedef PVOID sDriver;

void KNeoPrintString(PCHAR sz);
void KNeoGetCursor(INT *x, INT *y);
void KNeoSetCursor(INT x, INT y);
void KNeoClearScreen();
void KNeoGetScreenSize(INT *pWidth, INT *pHeight);
INT KNeoGetVideoMode();

PVOID KNeoGetDriver(PCHAR szName);
PVOID KNeoGetDriverFunction(PVOID pDriver, PCHAR szName);

void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);
void KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, void (*pCallback));

INT KNeoGetCurrentPID();
// INT KNeoStartProcess();
void KNeoKillProcess(INT iPID);
void KNeoPauseProcess(INT iPID);

PVOID KNeoHeapAllocate(QWORD qwSize);
PVOID KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize);
void KNeoHeapFree(PVOID pMemory);


inline static void KNeoEnableInterrupts()
{
    __asm__ volatile ("sti");
}

inline static void KNeoDisableInterrupts()
{
    __asm__ volatile ("cli");
}

#endif // __KNEOS_H
