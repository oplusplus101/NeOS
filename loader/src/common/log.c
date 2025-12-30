
#include <common/log.h>
#include <neos.h>
#include <common/screen.h>

void InitLogger()
{

}

void Log(INT iType, const PWCHAR wszFormat, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, wszFormat);
    
    if (iType == 0)
    {
        PrintString("[");
        SetFGColor(_RGB(0, 255, 255));
        PrintString("  LOG  ");
        SetFGColor(NEOS_FOREGROUND_COLOUR);
        PrintString("] ");
    }
    else if (iType == 1)
    {
        PrintString("[");
        SetFGColor(_RGB(255, 255, 0));
        PrintString("WARNING");
        SetFGColor(NEOS_FOREGROUND_COLOUR);
        PrintString("] ");
    }
    else if (iType == 2)
    {
        PrintString("[");
        SetFGColor(_RGB(255, 0, 0));
        PrintString(" ERROR ");
        SetFGColor(NEOS_FOREGROUND_COLOUR);
        PrintString("] ");
    }
    else if (iType == 3)
    {
        PrintString("[");
        SetFGColor(_RGB(255, 0, 255));
        PrintString("GOODBYE");
        SetFGColor(NEOS_FOREGROUND_COLOUR);
        PrintString("] ");
    }

    PrintFormatVariadic(wszFormat, args);
    PrintChar(10);
    
    __builtin_va_end(args);
}
