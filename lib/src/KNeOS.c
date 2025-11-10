
#define DLL_BUILD
#include <KNeOS.h>

FUNC_EXPORT void KNeoPrintString(PCHAR sz)
{
    __asm__ volatile ("int $0x81" : : "a"(0x00), "b"(sz));
}

FUNC_EXPORT void KNeoGetCursor(INT *x, INT *y)
{
    __asm__ volatile ("int $0x81" : "=b"(x), "=c"(y) : "a"(0x01));
}

FUNC_EXPORT void KNeoSetCursor(INT x, INT y)
{
    __asm__ volatile ("int $0x81" : : "a"(0x02), "b"(x), "c"(y));
}

FUNC_EXPORT void KNeoClearScreen()
{
    __asm__ volatile ("int $0x81" : : "a"(0x03));
}

FUNC_EXPORT void KNeoGetScreenSize(INT *pWidth, INT *pHeight)
{
    __asm__ volatile ("int $0x81" : "=b"(*pWidth), "=c"(*pHeight) : "a"(0x04));
}

FUNC_EXPORT void KNeoPrintChar(CHAR c)
{
    __asm__ volatile ("int $0x81" : : "a"(0x05), "b"(c));
}

FUNC_EXPORT PVOID KNeoGetDriverObject(PCHAR szName)
{
    PVOID pDriver;
    __asm__ volatile ("int $0x81" : "=c"(pDriver) : "a"(0x10), "b"(szName));
    return pDriver;
}

FUNC_EXPORT INT KNeoCallDriver(sDriverObject *pDriverObject, sIORequestPacket *pIRP)
{
    INT iStatus;
    __asm__ volatile ("int $0x81" : "=d"(iStatus) : "a"(0x11), "b"(pDriverObject), "c"(pIRP));
    return iStatus;
}

FUNC_EXPORT void KNeoMapPagesToIdentity(PVOID pAddress, QWORD qwPages, WORD wFlags)
{
    __asm__ volatile ("int $0x81" : : "a"(0x20), "b"(pAddress), "c"(qwPages), "d"(wFlags));
}

FUNC_EXPORT PVOID KNeoGetPhysicalAddress(PVOID pVirtualAddress)
{
    PVOID pAddr;
    __asm__ volatile ("int $0x81" : "=c"(pAddr) : "a"(0x27), "b"(pVirtualAddress));
    return pAddr;
}


FUNC_EXPORT void KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, void (*pCallback)())
{
    __asm__ volatile ("int $0x81" : : "a"(0x32), "b"(bInterrupt), "c"(bRing), "d"(pCallback));
}

FUNC_EXPORT void KNeoLog(PWCHAR wszMessage, BYTE bCode)
{
    __asm__ volatile ("int $0x81" : : "a"(0x33), "b"(wszMessage), "c"(bCode));
}


FUNC_EXPORT INT KNeoGetCurrentPID()
{
    INT iPID;
    __asm__ volatile ("int $0x81" : "=b"(iPID) : "a"(0x40));
    return iPID;
}

FUNC_EXPORT INT KNeoStartProcess(void (*pEntryPoint), QWORD qwStackSize)
{
    INT iPID;
    __asm__ volatile ("int $0x81" : "=d"(iPID) : "a"(0x41), "b"(pEntryPoint), "c"(qwStackSize));
    return iPID;
}

FUNC_EXPORT void KNeoKillProcess(INT iPID)
{
    __asm__ volatile ("int $0x81" : : "a"(0x42), "b"(iPID));
}

FUNC_EXPORT void KNeoPauseProcess(INT iPID)
{
    __asm__ volatile ("int $0x81" : : "a"(0x43), "b"(iPID));
}

FUNC_EXPORT INT KNeoCreateDevice(sDriverObject *pDriverObject, PWCHAR wszDeviceName, DWORD dwDeviceType, sDeviceObject **ppDeviceObject)
{
    INT iStatus;
    __asm__ volatile ("int $0x81" : "=a"(ppDeviceObject), "=d"(iStatus) : "a"(0x50), "b"(pDriverObject), "c"(wszDeviceName), "d"(dwDeviceType));
    return iStatus;
}

FUNC_EXPORT INT KNeoCreateFile(HANDLE *pHandle, PWCHAR wszPath, HANDLE hParentDirectory, WORD wAccessFlags, WORD wFileAttributes, WORD wShareFlags, WORD wCreateType)
{
    INT iStatus;
    __asm__ volatile ("int $0x81" : "=a"(pHandle), "=d"(iStatus) : "a"(0x52), "b"(wszPath), "c"(hParentDirectory));

    return iStatus;
}

FUNC_EXPORT PVOID KNeoHeapAllocate(QWORD qwSize)
{
    PVOID pAddress;
    __asm__ volatile ("int $0x81" : "=b"(pAddress) : "a"(0x24));
    return pAddress;
}

FUNC_EXPORT PVOID KNeoHeapReAllocate(PVOID pOldMemory, QWORD qwNewSize)
{
    PVOID pAddress;
    __asm__ volatile ("int $0x81" : "=d"(pAddress) : "a"(0x25), "b"(pOldMemory), "c"(qwNewSize));
    return pAddress;
}

FUNC_EXPORT void KNeoHeapFree(PVOID pMemory)
{
    __asm__ volatile ("int $0x81" : : "a"(0x26), "b"(pMemory));
}


// Extra functions
FUNC_EXPORT void KNeoPrintHex(QWORD n, BYTE nDigits, BOOL bUppercase)
{
    nDigits = nDigits > 16 ? 16 : nDigits;
    PCHAR sDigits = bUppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    CHAR sNumber[16];

    for (INT i = 0; i < nDigits; i++)
    {
        sNumber[i] = sDigits[n % 16];
        n /= 16;
    }
    
    for (INT i = nDigits - 1; i >= 0; i--)
        KNeoPrintChar(sNumber[i]);
}

FUNC_EXPORT void KNeoPrintDec(QWORD n)
{
    if (n == 0)
    {
        KNeoPrintChar('0');
        return;
    }

    CHAR sNumber[20];
    INT i = 0;
    while (n)
    {
        sNumber[i++] = (n % 10) + '0';
        n /= 10;
    }
    
    for (INT j = i - 1; j >= 0; j--)
        KNeoPrintChar(sNumber[j]);
}

FUNC_EXPORT void KNeoPrintFormat(const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    for (QWORD i = 0; wszFormat[i] != 0; i++)
    {
        if (wszFormat[i] == L'%')
        {
            i++; // Skip the %
            switch (wszFormat[i])
            {
            case L'd':
            case L'i':
                INT n = __builtin_va_arg(args, INT);
                if (n < 0)
                {
                    n *= -1;
                    KNeoPrintChar('-');
                }
                KNeoPrintDec(n);
                break;
            case L'u':
                KNeoPrintDec(__builtin_va_arg(args, QWORD));
                break;
            case L'p':
                KNeoPrintHex(__builtin_va_arg(args, QWORD), 16, true);
                break;
                case L's':
                const PCHAR sz = __builtin_va_arg(args, const PCHAR);
                if (sz == NULL)
                    KNeoPrintString("(null)");
                else
                    KNeoPrintString(sz);
                break;
            case L'w':
                const PWCHAR wsz = __builtin_va_arg(args, const PWCHAR);
                if (wsz == NULL)
                    KNeoPrintString("(null)");
                else
                    for (QWORD j = 0; wsz[j] != 0; j++)
                        KNeoPrintChar(wsz[j]);
                break;
            
            case L'c':
                KNeoPrintChar((char) __builtin_va_arg(args, DWORD));
                break;
            case L'%':
                KNeoPrintChar('%');
                break;
            // case L'G': // GUID
            //     BYTE *pGUID = __builtin_va_arg(args, BYTE *);
                // PrintFormat("%08X-%04X-%04X-%04X-%12X", *((DWORD *) &pGUID[0]),
                //                                         *((WORD *)  &pGUID[4]),
                //                                         *((WORD *)  &pGUID[6]),
                //                                         _BE2LEW(*((WORD *)  &pGUID[8])),
                //                                         _BE2LE48(*((QWORD *) &pGUID[10])));
                break;
            default:
    
                if (wszFormat[i] >= '0' && wszFormat[i] <= '9' && wszFormat[i + 1] >= '0' && wszFormat[i + 1] <= '9')
                {
                    BYTE qwDigits = (wszFormat[i] - '0') * 10 + (wszFormat[i + 1] - '0');
                    i += 2;
                    if (wszFormat[i] == 'X')
                        KNeoPrintHex(__builtin_va_arg(args, QWORD), qwDigits, true);
                    else if (wszFormat[i] == 'x')
                        KNeoPrintHex(__builtin_va_arg(args, QWORD), qwDigits, false);
                    else if (wszFormat[i] == 's')
                        for (BYTE j = 0; j < qwDigits; j++)
                            KNeoPrintChar(__builtin_va_arg(args, const PCHAR)[j]);
                }
            }
            
        }
        else
            KNeoPrintChar(wszFormat[i]);
    }
    __builtin_va_end(args);
}


FUNC_EXPORT void DllMainCRTStartup()
{
    
}
