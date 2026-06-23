
#include <io/log.h>
#include <io/screen.h>
#include <neos.h>

void SYSV ArchLogVariadic(INT iType, const PWCHAR wszFormat, __builtin_va_list lst)
{
    if (iType == 0)
    {
        ArchPrintFormat(L"[");
        ArchSetFGColour(_RGB(0, 255, 255));
        ArchPrintFormat(L"  LOG  ");
        ArchSetFGColour(NEOS_FOREGROUND_COLOUR);
        ArchPrintFormat(L"] ");
    }
    else if (iType == 1)
    {
        ArchPrintFormat(L"[");
        ArchSetFGColour(_RGB(255, 255, 0));
        ArchPrintFormat(L"WARNING");
        ArchSetFGColour(NEOS_FOREGROUND_COLOUR);
        ArchPrintFormat(L"] ");
    }
    else if (iType == 2)
    {
        ArchPrintFormat(L"[");
        ArchSetFGColour(_RGB(255, 0, 0));
        ArchPrintFormat(L" ERROR ");
        ArchSetFGColour(NEOS_FOREGROUND_COLOUR);
        ArchPrintFormat(L"] ");
    }
    else if (iType == 3)
    {
        ArchPrintFormat(L"[");
        ArchSetFGColour(_RGB(255, 0, 255));
        ArchPrintFormat(L"GOODBYE");
        ArchSetFGColour(NEOS_FOREGROUND_COLOUR);
        ArchPrintFormat(L"] ");
    }

    ArchPrintFormatVariadic(wszFormat, lst);
    ArchPrintChar(10);
}

void MS ArchLog(INT iType, const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    ArchLogVariadic(iType, wszFormat, args);
    __builtin_va_end(args);
}
