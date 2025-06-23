
#include <KNeOS.h>

FUNC_EXPORT void KNeoPrintString(PCHAR sz)
{
    __asm__ volatile ("int $0x81" : : "a"(0x00), "b"(sz));
}

FUNC_EXPORT void KNeoGetCursor(INT *x, INT *y)
{
    __asm__ volatile ("int $0x81" : "=b"(*x), "=c"(*y) : "a"(0x01));
}

FUNC_EXPORT void KNeoGetCursor(INT *x, INT *y)
{
    __asm__ volatile ("int $0x81" : : "a"(0x02), "b"(x), "c"(y));
}

FUNC_EXPORT void KNeoClearScreen()
{
    __asm__ volatile ("int $0x81" : : "a"(0x03));
}

FUNC_EXPORT void KNeoGetScreenSize(INT *pWidth, INT *pHeight)
{
    __asm__ volatile ("int $0x81" : "b"(*pWidth), "c"(*pHeight) : "a"(0x04));
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

FUNC_EXPORT void KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, void (*pCallback))
{
    __asm__ volatile ("int $0x81" : : "a"(0x30), "b"(bInterrupt), "c"(bRing), "d"(pCallback));
}

FUNC_EXPORT INT KNeoGetCurrentPID()
{
    INT iPID;
    __asm__ volatile ("int $0x81" : "=b"(iPID) : "a"(0x40));
    return iPID;
}

FUNC_EXPORT INT KNeoStartProcess()
{
    return 0;
}

FUNC_EXPORT void KNeoKillProcess(INT iPID)
{
    __asm__ volatile ("int $0x81" : : "a"(0x42), "b"(iPID));
}

FUNC_EXPORT void KNeoPauseProcess(INT iPID)
{
    __asm__ volatile ("int $0x81" : : "a"(0x34), "b"(iPID));
}

PVOID KNeoHeapAllocate(QWORD qwSize)
{
    PVOID pAddress;
    __asm__ volatile ("int $0x81" : "=b"(pAddress) : "a"(0x24));
    return pAddress;
}

PVOID KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize)
{
    PVOID pAddress;
    __asm__ volatile ("int $0x81" : "=d"(pAddress) : "a"(0x25), "b"(pOldMemory), "c"(qwNewSize));
    return pAddress;
}

void KNeoHeapFree(PVOID pMemory)
{
    __asm__ volatile ("int $0x81" : : "a"(0x26), "b"(pMemory));
}
