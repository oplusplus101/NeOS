#ifndef __KNEOS_H
#define __KNEOS_H

#include <NeoTypes.h>

#define FUNC_EXPORT __attribute__((visibility("default")))

inline static void KNeoPrintString(PCHAR sz)
{
    __asm__ volatile ("int $0x81" : : "a"(0), "b"(sz));
}

inline static void KNeoKernelPanic(PCHAR szMessage)
{
    __asm__ volatile ("int $0x81" : : "a"(1), "b"(szMessage));
}

// If the condition is false, a kernel panic occur.
inline static void KNeoAssert(BOOL bCondition, PCHAR szMessage)
{
    if (!bCondition)
        KNeoKernelPanic(szMessage);
}

PVOID KNeoGetModule(PCHAR szName)
{
    PVOID pModule;
    __asm__ volatile ("int $0x81" : "=c"(pModule) : "a"(0x11), "b"(szName));
    return pModule;
}

PVOID KNeoGetModuleFunction(PVOID pModule, PCHAR szName);

#endif // __KNEOS_H
