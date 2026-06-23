
#include <io/io.h>
#include <runtime/objects.h>
#include <archimports.h>
#include <common/panic.h>

void MS KNeoPrintChar(WORD c)
{
    ArchPrintChar(c);
}

void MS KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, ISR pCallback)
{
    _KERNEL_PANIC("KNeoRegisterInterrupt not implemented");
}

void MS KNeoPrintString(const PCHAR szString)
{
    ArchPrintFormat(L"%s", szString);
}

void MS KNeoLog(INT iType, const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    ArchLogVariadic(iType, wszFormat, args);
    __builtin_va_end(args);
}

void MS KNeoPrintFormat(const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    ArchPrintFormatVariadic(wszFormat, args);
    __builtin_va_end(args);
}

STATUS MS KNeoCreateDevice(sDriverObject *pDriverObject, sString wsPath, DWORD dwDeviceType, sDeviceObject **ppDeviceObject)
{
    return CreateDevice(pDriverObject, wsPath, dwDeviceType, ppDeviceObject);
}
