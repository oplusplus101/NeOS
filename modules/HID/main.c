
// A generic interface for mouse, keyboard, touch, etc.
#include <KNeOS.h>
#include <NeoHID.h>

#define KB_CAPSLOCK   1
#define KB_NUMLOCK    2
#define KB_SCROLLLOCK 4


PWCHAR g_arrDefaultCharmap        = L"\0\01234567890-=\08\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230\0\0";
PWCHAR g_arrDefaultShiftedCharmap = L"\0\0!@#$%^&*()_+\08\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230\0\0";
BYTE g_bKeyboardState             = 0;
sDriver *g_pCurrentKeyboardDriver = NULL;
sDriver *p_pCurrentMouseDriver    = NULL;
sDriver *p_pCurrentTouchDriver    = NULL;

FUNC_EXPORT WCHAR NeoGetKeyChar(BYTE bKey);

FUNC_EXPORT void _KeyPressed(BYTE bKey, BYTE bKeyboardState)
{
    PCHAR sz = " ";
    sz[0] = (CHAR) NeoGetKeyChar(bKey);
    KNeoPrintString(sz);
}

FUNC_EXPORT void _KeyReleased(BYTE bKey, BYTE bKeyboardState)
{
    
}

FUNC_EXPORT void NeoGetKeyState(BYTE bKey)
{
    
}

FUNC_EXPORT void NeoGetAbsoluteKey(BYTE bVirtualKey)
{

}

FUNC_EXPORT WCHAR NeoGetKeyChar(BYTE bKey)
{
    return g_arrDefaultCharmap[bKey];
}

void ModuleEntry()
{
    
    KNeoPauseProcess(KNeoGetCurrentPID());
}
