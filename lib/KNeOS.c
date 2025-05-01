#include <KNeOS.h>

FUNC_EXPORT void KNeoPrintString(PCHAR sz)
{
    __asm__ volatile ("int $0x81" : : "a"(0x00), "b"(sz));
}

FUNC_EXPORT void KNeoKernelPanic(PCHAR szMessage)
{
    __asm__ volatile ("int $0x81" : : "a"(0x01), "b"(szMessage));
}

// If the condition is false, a kernel panic occur.
FUNC_EXPORT void KNeoAssert(BOOL bCondition, PCHAR szMessage)
{
    if (!bCondition)
        KNeoKernelPanic(szMessage);
}

FUNC_EXPORT PVOID KNeoGetDriver(PCHAR szName)
{
    PVOID pDriver;
    __asm__ volatile ("int $0x81" : "=c"(pDriver) : "a"(0x10), "b"(szName));
    return pDriver;
}

FUNC_EXPORT PVOID KNeoGetModule(PCHAR szName)
{
    PVOID pModule;
    __asm__ volatile ("int $0x81" : "=c"(pModule) : "a"(0x11), "b"(szName));
    return pModule;
}

FUNC_EXPORT PVOID KNeoGetDriverFunction(PVOID pDriver, PCHAR szName)
{
    PVOID pFunction;
    __asm__ volatile ("int $0x81" : "=d"(pFunction) : "a"(0x12), "b"(pDriver), "c"(szName));
    return pFunction;
}

FUNC_EXPORT PVOID KNeoGetModuleFunction(PVOID pModule, PCHAR szName)
{
    PVOID pFunction;
    __asm__ volatile ("int $0x81" : "=d"(pFunction) : "a"(0x13), "b"(pModule), "c"(szName));
    return pFunction;
}


FUNC_EXPORT void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags)
{
    __asm__ volatile ("int $0x81" : : "a"(0x20), "b"(pAddress), "c"(qwPages), "d"(wFlags));
}