
#include <loaderfunctions.h>
#include <io/io.h>
#include <runtime/objects.h>


void STDCALL KNeoRegisterInterrupt(BYTE bInterrupt, BYTE bRing, QWORD (*pCallback)(QWORD))
{
    RegisterInterrupt(bInterrupt, (ISR) pCallback);
}

void STDCALL KNeoPrintString(const PCHAR szString)
{
    PrintString(szString);
}

void STDCALL KNeoLog(INT iType, const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    LogVariadic(iType, wszFormat, args);
    __builtin_va_end(args);
}

void STDCALL KNeoPrintFormat(const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    PrintFormatVariadic(wszFormat, args);
    __builtin_va_end(args);
}
