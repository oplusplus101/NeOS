
#include <KNeOS.h>
#include <NeoList.h>

typedef struct
{
    INT x, y, w, h;
} sRectangle;

typedef struct
{
    sRectangle rectArea;
    sDriver *pDriver;
} sScreen;

sList g_lstScreens;

BOOL IsPointInsideRectangle(sRectangle *pRect, INT x, INT y)
{
    return x >= pRect->x &&
           y >= pRect->y &&
           x < pRect->x + pRect->w &&
           y < pRect->y + pRect->h;
}

FUNC_EXPORT BOOL IsPointInsideScreens(INT x, INT y)
{
    for (QWORD i = 0; i < g_lstScreens.qwLength; i++)
    {
        sScreen *pScreen = GetListElement(&g_lstScreens, i);
        if (IsPointInsideRectangle(&pScreen->rectArea, x, y))
            return true;
    }
    
    return false;
}

// FUNC_EXPORT void NeoGetScreenInfo(INT iScreen, sNeoScreenInfo *pData)
// {

// }

// Returns the screen ID
// INT NeoRegisterScreenDriver(sDriver *)
// {

// }

void DriverMain()
{
    // g_lstScreenDrivers = CreateEmptyList(sizeof(sDriver *));

}
