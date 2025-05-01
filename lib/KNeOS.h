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

void KNeoPrintString(PCHAR sz);
void KNeoKernelPanic(PCHAR szMessage);
// If the condition is false, a kernel panic occur.
void KNeoAssert(BOOL bCondition, PCHAR szMessage);

PVOID KNeoGetDriver(PCHAR szName);
PVOID KNeoGetModule(PCHAR szName);
PVOID KNeoGetDriverFunction(PVOID pDriver, PCHAR szName);
PVOID KNeoGetModuleFunction(PVOID pModule, PCHAR szName);

void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags);

#endif // __KNEOS_H
