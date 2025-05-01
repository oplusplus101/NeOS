
#include <KNeOS.h>
#include <NeOS.h>

typedef struct
{
    sRectangle rectArea;
    
} sScreen;

sList g_lstScreenDrivers;

FUNC_EXPORT void NeoGetScreenInfo(INT iScreen, sNeoScreenInfo *pData)
{

}

// Returns the screen ID
INT NeoRegisterScreenDriver(sDriver *)
{

}


void EnableModule()
{
    g_lstScreenDrivers = CreateEmptyList(sizeof(sDriver *));
}

void DisableModule()
{
    
}
