
#include <KNeOS.h>
#include <NeOS.h>

sList g_lstScreenDrivers;

FUNC_EXPORT void NeoGetScreenData(INT iScreen, sNeoScreenData *pData)
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
