
// A generic interface for mouse, keyboard, touch, etc.
#include <KNeOS.h>
#include <NeoHID.h>

#define KB_CAPSLOCK   1
#define KB_NUMLOCK    2
#define KB_SCROLLLOCK 4


PWCHAR g_arrDefaultCharmap        = L"\0\01234567890-=\08\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230\0\0";
PWCHAR g_arrDefaultShiftedCharmap = L"\0\0!@#$%^&*()_+\08\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0\0\0\0\0\0\0\0\0\0\0\0\0789-456+1230\0\0";
BYTE g_bKeyboardState             = 0;

WCHAR NeoGetKeyChar(BYTE bKey);

void KeyPressed(BYTE bKey, BYTE bKeyboardState)
{
    PCHAR sz = " ";
    sz[0] = (CHAR) NeoGetKeyChar(bKey);
    KNeoPrintString(sz);
}

void KeyReleased(BYTE bKey, BYTE bKeyboardState)
{
    
}

void NeoGetKeyState(BYTE bKey)
{
    
}

void NeoGetAbsoluteKey(BYTE bVirtualKey)
{

}

WCHAR NeoGetKeyChar(BYTE bKey)
{
    return g_arrDefaultCharmap[bKey];
}

INT HandleRead(sDriverObject *pObject, sIORequestPacket *pIRP)
{
    return 0;
}

INT HandleWrite(sDriverObject *pObject, sIORequestPacket *pIRP)
{
    return 0;
}

INT DriverMain(sDriverObject *pObject)
{
    pObject->arrIOHandlers[IO_READ] = HandleRead;
    pObject->arrIOHandlers[IO_READ] = HandleWrite;
    KNeoPrintString("Hello, World!");
    
    return 0;
}
